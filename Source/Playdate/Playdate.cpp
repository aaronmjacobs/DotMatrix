#include "Core/Archive.h"
#include "Core/Log.h"

#include "GameBoy/Cartridge.h"
#include "GameBoy/CPU.h"
#include "GameBoy/GameBoy.h"
#include "GameBoy/LCDController.h"
#include "GameBoy/SoundController.h"

#include <pdcpp/pdnewlib.h>

#if !DM_PLATFORM_PLAYDATE
#include <mutex>
#endif // !DM_PLATFORM_PLAYDATE
#include <string>
#include <utility>
#include <vector>

#define DM_PLAYDATE_SCALE 1

PlaydateAPI* g_pd = nullptr;

#if DM_PLATFORM_PLAYDATE && PPK_ASSERT_ENABLED
namespace ppk
{
namespace assert
{
namespace implementation
{
   void throwException(const ppk::assert::AssertionException& e)
   {
      abort();
   }
}
}
}
#endif // DM_PLATFORM_PLAYDATE && PPK_ASSERT_ENABLED

#if DM_PLATFORM_PLAYDATE
#include <sys/time.h>

extern "C"
{
   int _gettimeofday(struct timeval* tv, void* tzvp)
   {
      if (!g_pd)
      {
         return 1;
      }

      unsigned int ms = 0;
      unsigned int seconds = g_pd->system->getSecondsSinceEpoch(&ms);

      tv->tv_sec = seconds;
      tv->tv_usec = ms * 1000;
      return 0;
   }
}
#endif // DM_PLATFORM_PLAYDATE

namespace
{
   namespace State
   {
      std::unique_ptr<DotMatrix::GameBoy> gameBoy;
      LCDBitmap* bitmap = nullptr;
      float lastTime = 0.0f;
      bool wroteToRamLastFrame = false;

      int yOffset = 0;

      int startFrames = 0;
      int selectFrames = 0;
      int abssFrames = 0;

#if DM_WITH_AUDIO
      SoundSource* soundSource = nullptr;

#if !DM_PLATFORM_PLAYDATE
      std::mutex soundMutex;
#endif // !DM_PLATFORM_PLAYDATE
#endif // DM_WITH_AUDIO
   }

#if DM_WITH_AUDIO
   int audioSourceFunction(void* context, int16_t* left, int16_t* right, int len)
   {
#if !DM_PLATFORM_PLAYDATE
      std::lock_guard<std::mutex> lock(State::soundMutex);
#endif // !DM_PLATFORM_PLAYDATE

      if (State::gameBoy && len >= 0)
      {
         return State::gameBoy->getSoundController().consumeAudio(std::span<int16_t>(left, len), std::span<int16_t>(right, len)) > 0;
      }

      return 0;
   }
#endif // DM_WITH_AUDIO

   std::vector<uint8_t> readBinaryFile(PlaydateAPI* pd, const std::string& path)
   {
      std::vector<uint8_t> data;

      if (SDFile* file = pd->file->open(path.c_str(), static_cast<FileOptions>(kFileRead | kFileReadData)))
      {
         pd->file->seek(file, 0, SEEK_END);
         int len = pd->file->tell(file);
         pd->file->seek(file, 0, SEEK_SET);

         if (len > 0)
         {
            data = std::vector<uint8_t>(len);

            int read = 0;
            while (read < len)
            {
               int remaining = len - read;
               int readThisLoop = pd->file->read(file, &data[read], remaining);

               if (readThisLoop < 0)
               {
                  data.clear();
                  break;
               }

               read += readThisLoop;
            }
         }

         pd->file->close(file);
      }

      return data;
   }

   bool writeBinaryFile(PlaydateAPI* pd, const std::string& path, const std::vector<uint8_t>& data)
   {
      bool success = false;
      if (SDFile* file = pd->file->open(path.c_str(), static_cast<FileOptions>(kFileWrite)))
      {
         int len = static_cast<int>(data.size());

         int written = 0;
         while (written < len)
         {
            int remaining = len - written;
            int writtenThisLoop = pd->file->write(file, &data[written], remaining);

            if (writtenThisLoop < 0)
            {
               break;
            }

            written += writtenThisLoop;
         }
         success = written == len;

         pd->file->close(file);
      }

      return success;
   }

   std::string findRom(PlaydateAPI* pd)
   {
      std::string romPath;
      pd->file->listfiles(".", [](const char* filename, void* userdata)
      {
         std::string* outRomPath = static_cast<std::string*>(userdata);

         if (outRomPath->empty())
         {
            std::string fileNameString(filename);
            if (fileNameString.ends_with(".gb") || fileNameString.ends_with(".gbc"))
            {
               *outRomPath = std::move(fileNameString);
            }
         }
      }, &romPath, 0);

      return romPath;
   }

   std::unique_ptr<DotMatrix::Cartridge> loadCartridge(PlaydateAPI* pd)
   {
      std::string romPath = findRom(pd);
      std::vector<uint8_t> data = readBinaryFile(pd, romPath);
      if (data.empty())
      {
         pd->system->error("Failed to read ROM file: %s", pd->file->geterr());
      }

      std::string error;
      if (std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(data), error))
      {
         return cartridge;
      }

      pd->system->error("Failed to create cartridge: %s", error.c_str());
      return nullptr;
   }

   std::string getSaveName(const char* title)
   {
      // Start with the cartridge title
      std::string fileName = title;

      // Remove all non-letters
      fileName.erase(std::remove_if(fileName.begin(), fileName.end(), [](char c) { return !isalpha(c); }), fileName.end());

      // Lower case
      std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

      if (!fileName.empty())
      {
         // Extension
         fileName += ".sav";
      }

      return fileName;
   }

   void loadGame(PlaydateAPI* pd)
   {
      if (State::gameBoy)
      {
         std::string saveName = getSaveName(State::gameBoy->title());
         if (!saveName.empty())
         {
            std::vector<uint8_t> data = readBinaryFile(pd, saveName);
            if (!data.empty())
            {
               DotMatrix::Archive cartRam(std::move(data));

               if (State::gameBoy->loadCartRAM(cartRam))
               {
                  DM_LOG_INFO("Loaded game from: " << saveName);
               }
            }
         }
      }
   }

   void saveGame(PlaydateAPI* pd)
   {
      if (State::gameBoy)
      {
         std::string saveName = getSaveName(State::gameBoy->title());
         if (!saveName.empty())
         {
            DotMatrix::Archive cartRam = State::gameBoy->saveCartRAM();

            if (writeBinaryFile(pd, saveName, cartRam.getData()))
            {
               DM_LOG_INFO("Saved game to: " << saveName);
            }
         }
      }
   }

   DotMatrix::Joypad pollInput(PlaydateAPI* pd)
   {
      PDButtons currentButtons{}; // Current button state
      PDButtons pushedButtons{}; // Buttons pushed over the last frame
      pd->system->getButtonState(&currentButtons, &pushedButtons, nullptr);
      PDButtons buttons = static_cast<PDButtons>(currentButtons | pushedButtons);

      DotMatrix::Joypad joypad;
      joypad.right = buttons & kButtonRight;
      joypad.left = buttons & kButtonLeft;
      joypad.up = buttons & kButtonUp;
      joypad.down = buttons & kButtonDown;
      joypad.a = buttons & kButtonA;
      joypad.b = buttons & kButtonB;

      if (State::startFrames > 0)
      {
         --State::startFrames;

         joypad.start = true;
      }

      if (State::selectFrames > 0)
      {
         --State::selectFrames;

         joypad.select = true;
      }

      if (State::abssFrames > 0)
      {
         --State::abssFrames;

         joypad.a = true;
         joypad.b = true;
         joypad.start = true;
         joypad.select = true;
      }

      return joypad;
   }

   void framebufferToBitmap(PlaydateAPI* pd, const DotMatrix::Framebuffer& framebuffer, LCDBitmap* bitmap)
   {
      static int width = 0;
      static int height = 0;
      static int rowbytes = 0;
      static uint8_t* mask = nullptr;
      static uint8_t* data = nullptr;
      if (!data)
      {
         pd->graphics->getBitmapData(bitmap, &width, &height, &rowbytes, &mask, &data);
      }

#if DM_PLAYDATE_SCALE
      constexpr int kExpectedRowbytes = 40;
#else
      constexpr int kExpectedRowbytes = 20;
#endif

      if (!data || rowbytes != kExpectedRowbytes)
      {
         return;
      }

#if DM_PLAYDATE_SCALE
      for (int y = 0; y < static_cast<int>(DotMatrix::kScreenHeight); ++y)
      {
         for (int x = 0; x < static_cast<int>(DotMatrix::kScreenWidth); x += 4)
         {
            int topByteOffset = (x / 4) + 2 * y * rowbytes;
            int bottomByteOffset = topByteOffset + rowbytes;

            uint8_t& topByte = data[topByteOffset];
            uint8_t& bottomByte = data[bottomByteOffset];

            topByte = 0;
            bottomByte = 0;

            for (int xOffset = 0; xOffset < 4; ++xOffset)
            {
               int bitOffset = 6 - 2 * xOffset;

               uint8_t pixel = ~framebuffer[x + xOffset + y * DotMatrix::kScreenWidth] & 0x03;
               topByte |= pixel << bitOffset;
               bottomByte |= (pixel == 1 || pixel == 2 ? pixel - 1 : pixel) << bitOffset;
            }
         }
      }
#else
      int byteOffset = 0;
      int bitOffset = 7;
      for (int i = 0; i < width * height; ++i)
      {
         uint8_t& byte = data[byteOffset];
         if (bitOffset == 7)
         {
            byte = 0;
         }

         byte |= ((framebuffer[i] > 1 ? 0 : 1) << bitOffset);

         --bitOffset;
         if (bitOffset < 0)
         {
            ++byteOffset;
            bitOffset = 7;
         }
      }
#endif
   }

   void render(PlaydateAPI* pd)
   {
#if DM_PLAYDATE_SCALE
      constexpr float kScale = 1.0f; // static_cast<float>(LCD_ROWS) / (DotMatrix::kScreenHeight * 2);
      constexpr int kX = static_cast<int>((LCD_COLUMNS - (DotMatrix::kScreenWidth * 2 * kScale)) / 2);
      constexpr int kY = static_cast<int>((LCD_ROWS - (DotMatrix::kScreenHeight * 2 * kScale)) / 2);
#else
      constexpr float kScale = 1.0f;
      constexpr int kX = (LCD_COLUMNS - DotMatrix::kScreenWidth) / 2;
      constexpr int kY = (LCD_ROWS - DotMatrix::kScreenHeight) / 2;
#endif

      framebufferToBitmap(pd, State::gameBoy->getLCDController().getFramebuffer(), State::bitmap);

      if (pd->system->isCrankDocked())
      {
         State::yOffset = 0;
      }
      else
      {
         float crankChange = pd->system->getCrankChange();
         State::yOffset -= static_cast<int>(crankChange * 0.5f);

         if (State::yOffset > -kY)
         {
            State::yOffset = -kY;
         }

         if (State::yOffset < kY)
         {
            State::yOffset = kY;
         }
      }

#if DM_PLAYDATE_SCALE && 0
      pd->graphics->drawScaledBitmap(State::bitmap, kX, kY, kScale, kScale);
#else
      int y = kY + State::yOffset;
      pd->graphics->drawBitmap(State::bitmap, kX, y, kBitmapUnflipped);
#endif

      pd->system->drawFPS(0, 0);
   }

   int tick(void* userdata)
   {
      PlaydateAPI* pd = static_cast<PlaydateAPI*>(userdata);

      float now = pd->system->getElapsedTime();
      float dt = now - State::lastTime;
      State::lastTime = now;

#if DM_PLATFORM_PLAYDATE
      dt = std::min(dt, 0.017f); // Prevent framerate spiraling
#endif // DM_PLATFORM_PLAYDATE

      State::gameBoy->setJoypadState(pollInput(pd));

      State::gameBoy->tick(static_cast<double>(dt));

      bool wroteToRamThisFrame = State::gameBoy->cartWroteToRamThisFrame();
      if (!wroteToRamThisFrame && State::wroteToRamLastFrame)
      {
         saveGame(pd);
      }
      State::wroteToRamLastFrame = wroteToRamThisFrame;

      render(pd);

      return 1;
   }

   void initialize(PlaydateAPI* pd)
   {
      g_pd = pd;

      pd->display->setRefreshRate(0.0f);
      pd->system->setUpdateCallback(tick, pd);

      {
#if DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE
         std::lock_guard<std::mutex> lock(State::soundMutex);
#endif // DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE

         State::gameBoy = std::make_unique<DotMatrix::GameBoy>();
      }
      State::gameBoy->setCartridge(loadCartridge(pd));
      loadGame(pd);

#if DM_PLAYDATE_SCALE
      constexpr int kBitmapScale = 2;
#else
      constexpr int kBitmapScale = 1;
#endif

      State::bitmap = pd->graphics->newBitmap(static_cast<int>(DotMatrix::kScreenWidth * kBitmapScale), static_cast<int>(DotMatrix::kScreenHeight * kBitmapScale), kColorBlack);

      pd->system->resetElapsedTime();

#if DM_WITH_AUDIO
      State::soundSource = pd->sound->addSource(audioSourceFunction, nullptr, 1);
#endif // DM_WITH_AUDIO

      static constexpr int kMenuInputFrames = 3;

      pd->system->addMenuItem("start", [](void* userdata) { State::startFrames = kMenuInputFrames; }, nullptr);
      pd->system->addMenuItem("select", [](void* userdata) { State::selectFrames = kMenuInputFrames; }, nullptr);
      pd->system->addMenuItem("abss", [](void* userdata) { State::abssFrames = kMenuInputFrames; }, nullptr);
   }

   void terminate(PlaydateAPI* pd)
   {
      pd->graphics->freeBitmap(State::bitmap);
      State::bitmap = nullptr;

#if DM_WITH_AUDIO
      pd->sound->removeSource(State::soundSource);
      State::soundSource = nullptr;
#endif // DM_WITH_AUDIO

      {
#if DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE
         std::lock_guard<std::mutex> lock(State::soundMutex);
#endif // DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE

         State::gameBoy = nullptr;
      }

      g_pd = nullptr;
   }
}

extern "C"
{
#ifdef _WINDLL
   __declspec(dllexport)
#endif
   int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
   {
      eventHandler_pdnewlib(pd, event, arg);

      if (event == kEventInit)
      {
         initialize(pd);
      }
      else if (event == kEventTerminate)
      {
         terminate(pd);
      }

      return 0;
   }
}
