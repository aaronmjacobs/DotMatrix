#pragma once

#include "Core/Archive.h"

#include "GameBoy/LCDController.h"

#include "Platform/Input/ControllerInputDevice.h"
#include "Platform/Input/KeyboardInputDevice.h"

#include <readerwriterqueue.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#if DM_WITH_BOOTSTRAP
#include <vector>
#endif // DM_WITH_BOOTSTRAP

class Renderer;
struct GLFWwindow;

namespace DotMatrix
{

class Cartridge;
class GameBoy;
#if DM_WITH_UI
class UI;
#endif // DM_WITH_UI

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

using PixelArray = std::array<Pixel, DotMatrix::kScreenWidth* DotMatrix::kScreenHeight>;

extern const std::array<Pixel, 4> kFramebufferColors;

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
   void resetGameBoy(std::unique_ptr<DotMatrix::Cartridge> cartridge);
   void toggleFullScreen();

   void loadGame();
   void saveGameAsync();
   void saveThreadMain();

#if DM_WITH_AUDIO
   void audioThreadMain();
#endif // DM_WITH_AUDIO

   GLFWwindow* window = nullptr;
   std::unique_ptr<DotMatrix::GameBoy> gameBoy;
   std::unique_ptr<Renderer> renderer;

   std::unique_ptr<PixelArray> pixels;

#if DM_WITH_BOOTSTRAP
   std::vector<uint8_t> bootstrap;
#endif // DM_WITH_BOOTSTRAP

#if DM_WITH_UI
   std::unique_ptr<UI> ui;
   double timeScale = 1.0;
   bool renderUi = true;
   bool skipNextTick = false;
#endif // DM_WITH_UI

   KeyboardInputDevice keyboardInputDevice;
   ControllerInputDevice controllerInputDevice;

   bool cartWroteToRamLastFrame = false;
   std::atomic<double> lastLoadTime = { 0.0 };

   std::atomic_bool exiting = { false };

#if DM_WITH_AUDIO
   std::thread audioThread;
   std::mutex audioThreadMutex;
   std::condition_variable audioThreadConditionVariable;
#endif // DM_WITH_AUDIO

   std::thread saveThread;
   std::mutex saveThreadMutex;
   std::condition_variable saveThreadConditionVariable;
   moodycamel::ReaderWriterQueue<SaveData> saveQueue;

   Bounds savedWindowBounds;
};

} // namespace DotMatrix
