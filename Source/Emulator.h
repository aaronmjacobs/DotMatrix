#pragma once

#include "Core/Archive.h"
#include "Core/Pointers.h"

#include "Platform/Audio/AudioManager.h"
#include "Platform/Input/ControllerInputDevice.h"
#include "Platform/Input/KeyboardInputDevice.h"
#include "Platform/Video/Renderer.h"

#include "readerwriterqueue.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace GBC
{
class GameBoy;
} // namespace GBC

struct GLFWwindow;

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
   Emulator()
      : window(nullptr)
      , cartWroteToRamLastFrame(false)
      , exiting(false)
#if GBC_DEBUG
      , timeModifier(1.0)
#endif // GBC_DEBUG
   {
   }

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

#if GBC_DEBUG
   double getTimeModifier() const
   {
      return timeModifier;
   }
#endif // GBC_DEBUG

private:
   void toggleFullScreen();
   void loadGame();
   void saveGameAsync();
   void saveThreadMain();

   GLFWwindow* window;
   UPtr<GBC::GameBoy> gameBoy;
   UPtr<Renderer> renderer;
   AudioManager audioManager;

   KeyboardInputDevice keyboardInputDevice;
   ControllerInputDevice controllerInputDevice;

   bool cartWroteToRamLastFrame;

   std::thread saveThread;
   std::mutex saveThreadMutex;
   std::condition_variable saveThreadConditionVariable;
   std::atomic_bool exiting;
   moodycamel::ReaderWriterQueue<SaveData> saveQueue;

   Bounds savedWindowBounds;

#if GBC_DEBUG
   double timeModifier;
#endif // GBC_DEBUG
};
