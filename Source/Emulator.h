#ifndef EMULATOR_H
#define EMULATOR_H

#include "Pointers.h"

#include "Wrapper/Audio/AudioManager.h"
#include "Wrapper/Input/ControllerInputDevice.h"
#include "Wrapper/Input/KeyboardInputDevice.h"
#include "Wrapper/Video/Renderer.h"

#include "readerwriterqueue.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace GBC {
class Device;
} // namespace GBC

struct GLFWwindow;

struct SaveData {
   IOUtils::Archive archive;
   std::string gameTitle;
};

class Emulator {
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
#if GBC_DEBUG
   void onKeyChanged(int key, int scancode, int action, int mods);

   double getTimeModifier() const {
      return timeModifier;
   }
#endif // GBC_DEBUG

private:
   void loadGame();
   void saveGameAsync();
   void saveThreadMain();

   GLFWwindow* window;
   UPtr<GBC::Device> device;
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

#if GBC_DEBUG
   double timeModifier;
#endif // GBC_DEBUG
};

#endif
