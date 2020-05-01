#pragma once

#include "Core/Archive.h"
#include "Core/Pointers.h"

#include "GBC/LCDController.h"

#if GBC_WITH_AUDIO
#include "Platform/Audio/AudioManager.h"
#endif // GBC_WITH_AUDIO
#include "Platform/Input/ControllerInputDevice.h"
#include "Platform/Input/KeyboardInputDevice.h"

#include "UI/UIFriend.h"

#include <readerwriterqueue.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#if GBC_WITH_BOOTSTRAP
#include <vector>
#endif // GBC_WITH_BOOTSTRAP

class Renderer;

namespace GBC
{
class Cartridge;
class GameBoy;
} // namespace GBC

struct GLFWwindow;

// 15 bits per pixel
struct Pixel
{
   uint8_t r;
   uint8_t g;
   uint8_t b;

   Pixel(uint8_t red = 0x00, uint8_t green = 0x00, uint8_t blue = 0x00)
      : r(red)
      , g(green)
      , b(blue)
   {
   }
};

using PixelArray = std::array<Pixel, GBC::kScreenWidth* GBC::kScreenHeight>;

struct SaveData
{
   Archive archive;
   std::string gameTitle;
};

struct Bounds
{
   int x = 0;
   int y = 0;
   int width = 0;
   int height = 0;
};

class Emulator
{
public:
   Emulator();
   ~Emulator();

   bool init();
   void tick(double dt);
   void render();
   bool shouldExit() const;

   void setRom(const char* romPath);

   void onFramebufferSizeChanged(int width, int height);
   void onFilesDropped(int count, const char* paths[]);
   void onKeyChanged(int key, int scancode, int action, int mods);
   void onWindowRefreshRequested();

private:
   DECLARE_UI_FRIEND

   void resetGameBoy(UPtr<GBC::Cartridge> cartridge);
   void toggleFullScreen();
   void loadGame();
   void saveGameAsync();
   void saveThreadMain();

   GLFWwindow* window = nullptr;
   UPtr<GBC::GameBoy> gameBoy;
   UPtr<Renderer> renderer;
#if GBC_WITH_AUDIO
   AudioManager audioManager;
#endif // GBC_WITH_AUDIO

   UPtr<PixelArray> pixels;

#if GBC_WITH_BOOTSTRAP
   std::vector<uint8_t> bootstrap;
#endif // GBC_WITH_BOOTSTRAP

#if GBC_WITH_UI
   UPtr<UI> ui;
   double timeScale = 1.0;
   bool renderUi = true;
   bool skipNextTick = false;
#endif // GBC_WITH_UI

   KeyboardInputDevice keyboardInputDevice;
   ControllerInputDevice controllerInputDevice;

   bool cartWroteToRamLastFrame = false;

   std::thread saveThread;
   std::mutex saveThreadMutex;
   std::condition_variable saveThreadConditionVariable;
   std::atomic_bool exiting = false;
   moodycamel::ReaderWriterQueue<SaveData> saveQueue;

   Bounds savedWindowBounds;
};
