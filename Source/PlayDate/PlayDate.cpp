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

PlaydateAPI* g_pd = nullptr;

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

      SDFile* file = pd->file->open(romPath.c_str(), static_cast<FileOptions>(kFileRead | kFileReadData));
      if (!file)
      {
         pd->system->error("Failed to open ROM file: %s", pd->file->geterr());
         return nullptr;
      }

      pd->file->seek(file, 0, SEEK_END);
      int len = pd->file->tell(file);
      pd->file->seek(file, 0, SEEK_SET);
      if (len < 0)
      {
         pd->system->error("Invalid ROM file length");
         return nullptr;
      }

      std::vector<uint8_t> data(len);
      int read = 0;
      while (read < len)
      {
         int remaining = len - read;
         read += pd->file->read(file, &data[read], static_cast<int>(remaining));
      }

      pd->file->close(file);
      file = nullptr;

      std::string error;
      if (std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(data), error))
      {
         return cartridge;
      }

      pd->system->error("Failed to create cartridge: %s", error.c_str());
      return nullptr;
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

      if (!data || rowbytes != 20)
      {
         return;
      }

      int byteOffset = 0;
      int bitOffset = 7;
      for (int i = 0; i < width * height; ++i)
      {
         uint8_t& byte = data[byteOffset];
         if (bitOffset == 7)
         {
            byte = 0;
         }

         byte |= (framebuffer[i] > 1 ? 1 : 0) << bitOffset;

         --bitOffset;
         if (bitOffset < 0)
         {
            ++byteOffset;
            bitOffset = 7;
         }
      }
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

      PDButtons buttons{};
      pd->system->getButtonState(&buttons, nullptr, nullptr);
      float crankChange = pd->system->getCrankChange();

      DotMatrix::Joypad joypad;
      joypad.right = buttons & kButtonRight;
      joypad.left = buttons & kButtonLeft;
      joypad.up = buttons & kButtonUp;
      joypad.down = buttons & kButtonDown;
      joypad.a = buttons & kButtonA;
      joypad.b = buttons & kButtonB;
      joypad.select = crankChange < 0.0f;
      joypad.start = crankChange > 0.0f;
      State::gameBoy->setJoypadState(joypad);

      State::gameBoy->tick(static_cast<double>(dt));

      constexpr int kX = (LCD_COLUMNS - DotMatrix::kScreenWidth) / 2;
      constexpr int kY = (LCD_ROWS - DotMatrix::kScreenHeight) / 2;

      framebufferToBitmap(pd, State::gameBoy->getLCDController().getFramebuffer(), State::bitmap);
      pd->graphics->drawBitmap(State::bitmap, kX, kY, kBitmapUnflipped);

      pd->system->drawFPS(0, 0);

      return 1;
   }

   void initialize(PlaydateAPI* pd)
   {
      g_pd = pd;

      pd->system->setUpdateCallback(tick, pd);

      {
#if DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE
         std::lock_guard<std::mutex> lock(State::soundMutex);
#endif // DM_WITH_AUDIO && !DM_PLATFORM_PLAYDATE

         State::gameBoy = std::make_unique<DotMatrix::GameBoy>();
      }
      State::gameBoy->setCartridge(loadCartridge(pd));

      State::bitmap = pd->graphics->newBitmap(static_cast<int>(DotMatrix::kScreenWidth), static_cast<int>(DotMatrix::kScreenHeight), kColorBlack);

      pd->system->resetElapsedTime();

#if DM_WITH_AUDIO
      State::soundSource = pd->sound->addSource(audioSourceFunction, nullptr, 1);
#endif // DM_WITH_AUDIO
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
