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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window))) {
      emulator->onKeyChanged(key, scancode, action, mods);
   }
}

GLFWmonitor* selectFullScreenMonitor(const Bounds& windowBounds) {
   GLFWmonitor* fullScreenMonitor = glfwGetPrimaryMonitor();

   int windowCenterX = windowBounds.x + (windowBounds.width / 2);
   int windowCenterY = windowBounds.y + (windowBounds.height / 2);

   int monitorCount = 0;
   GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

   for (int i = 0; i < monitorCount; ++i) {
      GLFWmonitor* candidateMonitor = monitors[i];

      if (const GLFWvidmode* vidMode = glfwGetVideoMode(candidateMonitor)) {
         Bounds monitorBounds;
         glfwGetMonitorPos(candidateMonitor, &monitorBounds.x, &monitorBounds.y);
         monitorBounds.width = vidMode->width;
         monitorBounds.height = vidMode->height;

         if (windowCenterX >= monitorBounds.x && windowCenterX < monitorBounds.x + monitorBounds.width &&
             windowCenterY >= monitorBounds.y && windowCenterY < monitorBounds.y + monitorBounds.height) {
            fullScreenMonitor = candidateMonitor;
            break;
         }
      }
   }

   return fullScreenMonitor;
}

std::string getWindowTitle(const GBC::Device* device) {
   static const std::string kBaseTitle = kProjectDisplayName;

   const char* gameTitle = device ? device->title() : nullptr;
   if (gameTitle) {
      return kBaseTitle + " - " + gameTitle;
   }

   return kBaseTitle;
}

std::string getSaveName(const char* title) {
   if (!title) {
      return "";
   }

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

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

   std::string windowTitle = getWindowTitle(nullptr);
   window = glfwCreateWindow(GBC::kScreenWidth * 2, GBC::kScreenHeight * 2, windowTitle.c_str() , nullptr, nullptr);
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
   glfwSetWindowSizeLimits(window, GBC::kScreenWidth, GBC::kScreenHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
   glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
   glfwSetDropCallback(window, dropCallback);
   glfwSetKeyCallback(window, keyCallback);

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

         std::string windowTitle = getWindowTitle(device.get());
         glfwSetWindowTitle(window, windowTitle.c_str());

         // Try to load a save file
         loadGame();
      } else {
         LOG_WARNING_MSG_BOX("Failed to load rom: " << romPath);
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

void Emulator::onKeyChanged(int key, int scancode, int action, int mods) {
   bool enabled = action == GLFW_PRESS;

   if (enabled) {
      if (key == GLFW_KEY_F11 || (key == GLFW_KEY_ENTER && ((mods & GLFW_MOD_ALT) != 0))) {
         toggleFullScreen();
      }

#if GBC_DEBUG
      if (key == GLFW_KEY_1) {
         timeModifier = 1.0;
      } else if (key == GLFW_KEY_2) {
         timeModifier = 2.0;
      } else if (key == GLFW_KEY_3) {
         timeModifier = 5.0;
      } else if (key == GLFW_KEY_4) {
         timeModifier = 1.0 / 60.0;
      } else if (key == GLFW_KEY_5) {
         timeModifier = 0.0;
      }
#endif // GBC_DEBUG
   }
}

void Emulator::toggleFullScreen() {
   ASSERT(window);

   if (GLFWmonitor* currentMonitor = glfwGetWindowMonitor(window)) {
      // Currently in full screen mode, swap back to windowed (with last saved window location)
      glfwSetWindowMonitor(window, nullptr, savedWindowBounds.x, savedWindowBounds.y, savedWindowBounds.width, savedWindowBounds.height, 0);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
   } else {
      // Currently in windowed mode, save the window location and swap to full screen
      glfwGetWindowPos(window, &savedWindowBounds.x, &savedWindowBounds.y);
      glfwGetWindowSize(window, &savedWindowBounds.width, &savedWindowBounds.height);

      if (GLFWmonitor* newMonitor = selectFullScreenMonitor(savedWindowBounds)) {
         if (const GLFWvidmode* vidMode = glfwGetVideoMode(newMonitor)) {
            glfwSetWindowMonitor(window, newMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            // Due to a bug, the previously set swap interval is ignored on Windows 10 when transitioning to full screen, so we set it again here
            // See: https://github.com/glfw/glfw/issues/1072
            glfwSwapInterval(1);
         }
      }
   }
}

void Emulator::loadGame() {
   ASSERT(device);

   std::string saveFileName = getSaveName(device->title());

   if (!saveFileName.empty() && IOUtils::canRead(saveFileName)) {
      std::vector<uint8_t> cartRamData = IOUtils::readBinaryFile(saveFileName);
      IOUtils::Archive cartRam(cartRamData);

      if (device->loadCartRAM(cartRam)) {
         LOG_INFO("Loaded game from: " << saveFileName);
      }
   }
}

void Emulator::saveGameAsync() {
   ASSERT(device);

   if (const char* title = device->title()) {
      SaveData saveData;
      saveData.archive = device->saveCartRAM();
      saveData.gameTitle = title;

      if (saveData.archive.getData().size() > 0) {
         {
            std::lock_guard<std::mutex> lock(saveThreadMutex);
            saveQueue.enqueue(std::move(saveData));
         }

         saveThreadConditionVariable.notify_all();
      }
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
