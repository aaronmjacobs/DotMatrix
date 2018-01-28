#include "Constants.h"
#include "Emulator.h"
#include "FancyAssert.h"
#include "GLIncludes.h"
#include "Log.h"

#include "GBC/Cartridge.h"
#include "GBC/Device.h"
#include "GBC/LCDController.h"

#include "Wrapper/Platform/IOUtils.h"

#include <algorithm>
#include <future>
#include <vector>

namespace {

#if GBC_DEBUG
const char* getGlErrorName(GLenum error) {
   switch (error) {
   case GL_NO_ERROR:
      return "GL_NO_ERROR";
   case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
   case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
   case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
   case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
   case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
   default:
      return "Unknown";
   }
}

void gladPostCallback(const char* name, void* funcptr, int numArgs, ...) {
   GLenum errorCode = glad_glGetError();
   ASSERT(errorCode == GL_NO_ERROR, "OpenGL error %d (%s) in %s", errorCode, getGlErrorName(errorCode), name);
}
#endif // GBC_DEBUG

bool loadGl() {
   static bool loaded = false;

   if (!loaded) {
      loaded = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) != 0;

#if GBC_DEBUG
      if (loaded) {
         glad_set_post_callback(gladPostCallback);
      }
#endif // GBC_DEBUG
   }

   return loaded;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
   if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window))) {
      emulator->onFramebufferSizeChanged(width, height);
   }
}

void dropCallback(GLFWwindow* window, int count, const char* paths[]) {
   if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window))) {
      emulator->onFilesDropped(count, paths);
   }
}

#if GBC_DEBUG
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window))) {
      emulator->onKeyChanged(key, scancode, action, mods);
   }
}
#endif // GBC_DEBUG

std::string getSaveName(const char* title) {
   // Start with the cartridge title
   std::string fileName = title;

   // Remove all non-letters
   fileName.erase(std::remove_if(fileName.begin(), fileName.end(), [](char c) { return !isalpha(c); }));

   // Lower case
   std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

   // Extension
   fileName += ".sav";

   // Relative to the app data directory
   std::string saveName;
   if (!IOUtils::appDataPath(kProjectName, fileName, saveName)) {
      return "";
   }

   return saveName;
}

// TODO
uint8_t onSerial(uint8_t receivedVal) {
   static std::vector<char> vals;

   vals.push_back((char)receivedVal);

   vals.push_back('\0');
   LOG_DEBUG("serial:\n" << vals.data());
   vals.pop_back();

   return 0xFF;
}

} // namespace

Emulator::~Emulator() {
   {
      std::lock_guard<std::mutex> lock(saveThreadMutex);
      exiting = true;
   }

   if (saveThread.joinable()) {
      saveThreadConditionVariable.notify_all();
      saveThread.join();
   }

   renderer = nullptr;
   device = nullptr;

   if (window) {
      glfwDestroyWindow(window);
      window = nullptr;
   }
}

bool Emulator::init() {
   if (window) {
      return true;
   }

   window = glfwCreateWindow(GBC::kScreenWidth, GBC::kScreenHeight, kProjectDisplayName, nullptr, nullptr);
   if (!window) {
      LOG_ERROR_MSG_BOX("Unable to create GLFW window");
      return false;
   }

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   if (!loadGl()) {
      glfwDestroyWindow(window);
      window = nullptr;

      LOG_ERROR_MSG_BOX("Unable to initialize OpenGL");
      return false;
   }

   int framebufferWidth = 0, framebufferHeight = 0;
   glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
   renderer = std::make_unique<Renderer>(framebufferWidth, framebufferHeight);

   keyboardInputDevice.setWindow(window);

   glfwSetWindowUserPointer(window, this);
   glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
   glfwSetDropCallback(window, dropCallback);
#if GBC_DEBUG
   glfwSetKeyCallback(window, keyCallback);
#endif // GBC_DEBUG

   device = std::make_unique<GBC::Device>();

   saveThread = std::thread([this] {
      saveThreadMain();
   });

   return true;
}

void Emulator::tick(double dt) {
   if (device) {
      GBC::Joypad joypad = GBC::Joypad::unionOf(keyboardInputDevice.poll(), controllerInputDevice.poll());
      device->setJoypadState(joypad);

      device->tick(dt);

      bool cartWroteToRamThisFrame = device->cartWroteToRamThisFrame();
      if (!cartWroteToRamThisFrame && cartWroteToRamLastFrame) {
         saveGameAsync();
      }
      cartWroteToRamLastFrame = cartWroteToRamThisFrame;
   } else {
      cartWroteToRamLastFrame = false;
   }
}

void Emulator::render() {
   if (device && renderer) {
      renderer->draw(device->getLCDController().getFramebuffer());

      std::vector<uint8_t> audioData = device->getSoundController().getAudioData();
      audioManager.queue(audioData);
   }

   glfwSwapBuffers(window);
}

bool Emulator::shouldExit() const {
   return glfwWindowShouldClose(window) != GLFW_FALSE;
}

void Emulator::setRom(const char* romPath) {
   // Try to load a cartridge
   if (romPath) {
      LOG_INFO("Loading rom: " << romPath);

      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(romPath);
      UPtr<GBC::Cartridge> cartridge(GBC::Cartridge::fromData(std::move(cartData)));

      if (cartridge) {
         device = std::make_unique<GBC::Device>();
         device->setCartridge(std::move(cartridge));
         glfwSetWindowTitle(window, device->title());

         // Try to load a save file
         loadGame();
      } else {
         LOG_WARNING("Failed to load rom: " << romPath);
      }
   }
}

void Emulator::onFramebufferSizeChanged(int width, int height) {
   if (renderer) {
      renderer->onFramebufferSizeChanged(width, height);
   }
}

void Emulator::onFilesDropped(int count, const char* paths[]) {
   if (count > 0) {
      setRom(paths[0]);
   }
}

#if GBC_DEBUG
void Emulator::onKeyChanged(int key, int scancode, int action, int mods) {
   bool enabled = action != GLFW_RELEASE;

   if (key == GLFW_KEY_1 && enabled) {
      timeModifier = 1.0;
   } else if (key == GLFW_KEY_2 && enabled) {
      timeModifier = 2.0;
   } else if (key == GLFW_KEY_3 && enabled) {
      timeModifier = 5.0;
   } else if (key == GLFW_KEY_4 && enabled) {
      timeModifier = 1.0 / 60.0;
   } else if (key == GLFW_KEY_5 && enabled) {
      timeModifier = 0.0;
   }
}
#endif // GBC_DEBUG

void Emulator::loadGame() {
   ASSERT(device);

   std::string saveFileName = getSaveName(device->title());

   if (saveFileName.size() > 0 && IOUtils::canRead(saveFileName)) {
      std::vector<uint8_t> cartRamData = IOUtils::readBinaryFile(saveFileName);
      IOUtils::Archive cartRam(cartRamData);

      if (device->loadCartRAM(cartRam)) {
         LOG_INFO("Loaded game from: " << saveFileName);
      }
   }
}

void Emulator::saveGameAsync() {
   ASSERT(device);

   SaveData saveData;
   saveData.archive = device->saveCartRAM();

   if (saveData.archive.getData().size() > 0) {
      saveData.gameTitle = device->title();

      {
         std::lock_guard<std::mutex> lock(saveThreadMutex);
         saveQueue.enqueue(std::move(saveData));
      }

      saveThreadConditionVariable.notify_all();
   }
}

void Emulator::saveThreadMain() {
   std::unique_lock<std::mutex> lock(saveThreadMutex);

   while (!exiting) {
      saveThreadConditionVariable.wait(lock, [this]() -> bool {
         return exiting || saveQueue.peek() != nullptr;
      });

      lock.unlock();

      SaveData saveData;
      while (saveQueue.try_dequeue(saveData)) {
         std::string saveFileName = getSaveName(saveData.gameTitle.c_str());

         if (!saveFileName.empty()) {
            bool saved = IOUtils::writeBinaryFileLocked(saveFileName, saveData.archive.getData());

            if (saved) {
               LOG_INFO("Saved game to: " << saveFileName);
            } else {
               LOG_WARNING("Failed to save game to: " << saveFileName);
            }
         }
      }

      lock.lock();
   }
}
