#include "DotMatrixCore/Core/Assert.h"

#include "DotMatrixCore/GameBoy/Cartridge.h"
#include "DotMatrixCore/GameBoy/CPU.h"
#include "DotMatrixCore/GameBoy/GameBoy.h"
#include "DotMatrixCore/GameBoy/LCDController.h"
#include "DotMatrixCore/GameBoy/SoundController.h"

#include <libretro.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
   static const double kClockCyclesPerFrame = 70224.0;
   static const double kFrameRate = DotMatrix::CPU::kClockSpeed / kClockCyclesPerFrame;

   struct Pixel
   {
      uint8_t b;
      uint8_t g;
      uint8_t r;
      uint8_t x;

      Pixel(uint8_t red = 0x00, uint8_t green = 0x00, uint8_t blue = 0x00)
         : x(0x00)
         , r(red)
         , g(green)
         , b(blue)
      {
      }
   };

   using PixelArray = std::array<Pixel, DotMatrix::kScreenWidth * DotMatrix::kScreenHeight>;

   namespace Callbacks
   {
      retro_environment_t environment = nullptr;
      retro_video_refresh_t videoRefresh = nullptr;
      retro_audio_sample_batch_t audioSampleBatch = nullptr;
      retro_input_poll_t inputPoll = nullptr;
      retro_input_state_t inputState = nullptr;
   }

   namespace State
   {
      std::unique_ptr<DotMatrix::GameBoy> gameBoy;
      std::unique_ptr<PixelArray> pixels;

      uint32_t frameCounter = 0;
      double frameTime = 0.0;
   }

   void frameTimeCallback(retro_usec_t usec)
   {
      State::frameTime = usec / 1000000.0;
   }

   void framebufferToPixels(const DotMatrix::Framebuffer& framebuffer, PixelArray& pixels)
   {
      // Green / blue (trying to approximate original Game Boy screen colors)
      static const std::array<Pixel, 4> kColors =
      {
         Pixel(0xAC, 0xCD, 0x4A),
         Pixel(0x7B, 0xAC, 0x6A),
         Pixel(0x20, 0x6A, 0x62),
         Pixel(0x08, 0x29, 0x52)
      };

      DM_ASSERT(pixels.size() == framebuffer.size());

      for (std::size_t i = 0; i < pixels.size(); ++i)
      {
         DM_ASSERT(framebuffer[i] < kColors.size());
         pixels[i] = kColors[framebuffer[i]];
      }
   }

   void updatePixelsAndRefreshVideo()
   {
      if (State::gameBoy && State::pixels)
      {
         framebufferToPixels(State::gameBoy->getLCDController().getFramebuffer(), *State::pixels);

         if (Callbacks::videoRefresh)
         {
            Callbacks::videoRefresh(State::pixels->data(), DotMatrix::kScreenWidth, DotMatrix::kScreenHeight, DotMatrix::kScreenWidth * sizeof(Pixel));
         }
      }
   }
}

void retro_set_environment(retro_environment_t callback)
{
   Callbacks::environment = callback;
}

void retro_set_video_refresh(retro_video_refresh_t callback)
{
   Callbacks::videoRefresh = callback;
}

void retro_set_audio_sample(retro_audio_sample_t callback)
{
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t callback)
{
   Callbacks::audioSampleBatch = callback;
}

void retro_set_input_poll(retro_input_poll_t callback)
{
   Callbacks::inputPoll = callback;
}

void retro_set_input_state(retro_input_state_t callback)
{
   Callbacks::inputState = callback;
}

void retro_init(void)
{
   State::pixels = std::make_unique<PixelArray>();

   State::frameCounter = 0;
   State::frameTime = 0.0;
}

void retro_deinit(void)
{
   State::pixels = nullptr;

   State::frameCounter = 0;
   State::frameTime = 0.0;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info* info)
{
   if (info)
   {
      info->library_name = DM_PROJECT_DISPLAY_NAME;
      info->library_version = DM_CORE_VERSION_STRING;
      info->valid_extensions = "";
      info->need_fullpath = false;
      info->block_extract = false;
   }
}

void retro_get_system_av_info(struct retro_system_av_info* info)
{
   if (info)
   {
      info->geometry.base_width = DotMatrix::kScreenWidth;
      info->geometry.base_height = DotMatrix::kScreenHeight;
      info->geometry.max_width = DotMatrix::kScreenWidth;
      info->geometry.max_height = DotMatrix::kScreenHeight;
      info->geometry.aspect_ratio = static_cast<float>(DotMatrix::kScreenWidth) / DotMatrix::kScreenHeight;

      info->timing.fps = kFrameRate;
      info->timing.sample_rate = DotMatrix::SoundController::kSampleRate;
   }
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   // TODO
}

void retro_reset(void)
{
   // TODO
}

void retro_run(void)
{
   if (State::gameBoy)
   {
      if (Callbacks::inputPoll)
      {
         Callbacks::inputPoll();
      }

      DotMatrix::Joypad joypad;
      if (Callbacks::inputState)
      {
         joypad.right = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) != 0;
         joypad.left = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) != 0;
         joypad.up = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0;
         joypad.down = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0;
         joypad.a = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) != 0;
         joypad.b = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) != 0;
         joypad.select = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT) != 0;
         joypad.start = Callbacks::inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) != 0;
      }
      State::gameBoy->setJoypadState(joypad);

      State::gameBoy->getSoundController().setGenerateAudioData(Callbacks::audioSampleBatch != nullptr);

      State::gameBoy->tick(State::frameTime);

      const std::vector<DotMatrix::AudioSample>& audioData = State::gameBoy->getSoundController().swapAudioBuffers();
      if (Callbacks::audioSampleBatch && !audioData.empty())
      {
         Callbacks::audioSampleBatch(&audioData[0].left, audioData.size());
      }

      uint32_t newFrameCounter = State::gameBoy->getLCDController().getFrameCounter();
      if (State::frameCounter != newFrameCounter)
      {
         updatePixelsAndRefreshVideo();
      }
      State::frameCounter = newFrameCounter;
   }
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void* data, size_t size)
{
   return false;
}

bool retro_unserialize(const void* data, size_t size)
{
   return false;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
}

bool retro_load_game(const struct retro_game_info* game)
{
   bool pixelFormatSupported = false;
   bool timeCallbackRegistered = false;
   if (Callbacks::environment)
   {
      retro_pixel_format pixelFormat = RETRO_PIXEL_FORMAT_XRGB8888;
      pixelFormatSupported = Callbacks::environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelFormat);

      struct retro_frame_time_callback callbackInfo;
      callbackInfo.callback = &frameTimeCallback;
      callbackInfo.reference = 1000000.0 / kFrameRate;
      timeCallbackRegistered = Callbacks::environment(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &callbackInfo);
   }

   if (!pixelFormatSupported || !timeCallbackRegistered)
   {
      return false;
   }

   if (game && game->data && game->size > 0)
   {
      std::vector<uint8_t> data(game->size);
      std::memcpy(data.data(), game->data, game->size);

      std::string error;
      if (std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(data), error))
      {
         State::gameBoy = std::make_unique<DotMatrix::GameBoy>();
         State::gameBoy->setCartridge(std::move(cartridge));

         updatePixelsAndRefreshVideo();

         return true;
      }
   }

   return false;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info)
{
   return false;
}

void retro_unload_game(void)
{
   State::gameBoy = nullptr;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

void* retro_get_memory_data(unsigned id)
{
   return nullptr; // TODO
}

size_t retro_get_memory_size(unsigned id)
{
   return 0; // TODO
}
