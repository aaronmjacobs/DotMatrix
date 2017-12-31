#include "Constants.h"

#if !defined(GBC_RUN_TESTS)

#include "GLIncludes.h"
#include "Log.h"

#include "GBC/Cartridge.h"
#include "GBC/Device.h"

#include "Wrapper/Audio/AudioManager.h"
#include "Wrapper/Platform/IOUtils.h"
#include "Wrapper/Platform/OSUtils.h"
#include "Wrapper/Video/Renderer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <vector>

namespace {

std::function<void(int, int)> framebufferCallback;
std::function<void(int, bool)> keyCallback;

void onFramebufferSizeChange(GLFWwindow *window, int width, int height) {
   if (framebufferCallback) {
      framebufferCallback(width, height);
   }
}

void onKeyChanged(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (keyCallback) {
      keyCallback(key, action != GLFW_RELEASE);
   }
}

void updateJoypadState(GBC::Joypad& joypadState, int key, bool enabled) {
   switch (key) {
      case GLFW_KEY_LEFT:
         joypadState.left = enabled;
         break;
      case GLFW_KEY_RIGHT:
         joypadState.right = enabled;
         break;
      case GLFW_KEY_UP:
         joypadState.up = enabled;
         break;
      case GLFW_KEY_DOWN:
         joypadState.down = enabled;
         break;
      case GLFW_KEY_S:
         joypadState.a = enabled;
         break;
      case GLFW_KEY_A:
         joypadState.b = enabled;
         break;
      case GLFW_KEY_Z:
         joypadState.select = enabled;
         break;
      case GLFW_KEY_X:
         joypadState.start = enabled;
         break;

      // Needed since many keyboards do not support 4 nearby buttons being pressed at the same time
      // Used for certain game functions, e.g. reset, save, etc.
      case GLFW_KEY_D:
         joypadState.a = enabled;
         joypadState.b = enabled;
         joypadState.select = enabled;
         joypadState.start = enabled;
         break;
   }
}

std::string getSaveName(const char* title) {
   // Start with the cartridge title
   std::string fileName = title;

   // Remove all non-letters
   fileName.erase(std::remove_if(fileName.begin(), fileName.end(), [](char c){ return !isalpha(c); }));

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

void saveGame(const GBC::Device& device) {
   IOUtils::Archive cartRam = device.saveCartRAM();
   const std::vector<uint8_t>& cartRamData = cartRam.getData();

   if (cartRamData.size() > 0) {
      std::string saveFileName = getSaveName(device.title());

      if (saveFileName.size() > 0 && IOUtils::writeBinaryFile(saveFileName, cartRamData)) {
         LOG_INFO("Saved game to: " << saveFileName);
      }
   }
}

void loadGame(GBC::Device& device) {
   std::string saveFileName = getSaveName(device.title());

   if (saveFileName.size() > 0 && IOUtils::canRead(saveFileName)) {
      std::vector<uint8_t> cartRamData = IOUtils::readBinaryFile(saveFileName);
      IOUtils::Archive cartRam(cartRamData);

      if (device.loadCartRAM(cartRam)) {
         LOG_INFO("Loaded game from: " << saveFileName);
      }
   }
}

UPtr<GBC::Device> createDevice(GLFWwindow* window, const char* cartPath) {
   UPtr<GBC::Device> device(std::make_unique<GBC::Device>());

   // Try to load a cartridge
   if (cartPath) {
      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(cartPath);

      LOG_INFO("Loading cartridge: " << cartPath);
      UPtr<GBC::Cartridge> cartridge(GBC::Cartridge::fromData(std::move(cartData)));

      if (cartridge) {
         device->setCartridge(std::move(cartridge));
         glfwSetWindowTitle(window, device->title());

         // Try to load a save file
         loadGame(*device);
      }
   }

   return device;
}

void onFilesDropped(GLFWwindow* window, int count, const char* paths[]) {
   void* userPointer = glfwGetWindowUserPointer(window);

   if (userPointer && count > 0) {
      UPtr<GBC::Device>* device = reinterpret_cast<UPtr<GBC::Device>*>(userPointer);
      *device = createDevice(window, paths[0]);
   }
}

GLFWwindow* init() {
   int glfwInitRes = glfwInit();
   if (!glfwInitRes) {
      LOG_ERROR_MSG_BOX("Unable to initialize GLFW");
      return nullptr;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

   GLFWwindow *window = glfwCreateWindow(GBC::kScreenWidth, GBC::kScreenHeight, kProjectDisplayName, nullptr, nullptr);
   if (!window) {
      glfwTerminate();
      LOG_ERROR_MSG_BOX("Unable to create GLFW window");
      return nullptr;
   }

   glfwSetFramebufferSizeCallback(window, onFramebufferSizeChange);
   glfwSetKeyCallback(window, onKeyChanged);
   glfwSetDropCallback(window, onFilesDropped);
   glfwMakeContextCurrent(window);
   glfwSwapInterval(1); // VSYNC

   int gladInitRes = gladLoadGL();
   if (!gladInitRes) {
      glfwDestroyWindow(window);
      glfwTerminate();
      LOG_ERROR_MSG_BOX("Unable to initialize glad");
      return nullptr;
   }

   return window;
}

} // namespace

uint8_t onSerial(uint8_t receivedVal) {
   static std::vector<char> vals;

   vals.push_back((char)receivedVal);

   vals.push_back('\0');
   LOG_DEBUG("serial:\n" << vals.data());
   vals.pop_back();

   return 0xFF;
}

int main(int argc, char* argv[]) {
   LOG_INFO(kProjectName << " " << kVersionMajor << "." << kVersionMinor << "."
      << kVersionMicro << " (" << kVersionBuild << ")");

   if (!OSUtils::fixWorkingDirectory()) {
      LOG_ERROR_MSG_BOX("Unable to set working directory to executable directory");
   }

   GLFWwindow *window = init();
   if (!window) {
      return EXIT_FAILURE;
   }

   int framebufferWidth = 0, framebufferHeight = 0;
   glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

   Renderer renderer(framebufferWidth, framebufferHeight);
   framebufferCallback = [&renderer](int width, int height) {
      renderer.onFramebufferSizeChange(width, height);
   };

   AudioManager audioManager;

   GBC::Joypad joypadState{};
   float timeModifier = 1.0f;
   bool doSave = false;
   keyCallback = [&joypadState, &timeModifier, &doSave](int key, bool enabled) {
   #if !NDEBUG
      if (key == GLFW_KEY_1 && enabled) {
         timeModifier = 1.0f;
      } else if (key == GLFW_KEY_2 && enabled) {
         timeModifier = 2.0f;
      } else if (key == GLFW_KEY_3 && enabled) {
         timeModifier = 5.0f;
      } else if (key == GLFW_KEY_4 && enabled) {
         timeModifier = 1.0f / 60.0f;
      } else if (key == GLFW_KEY_5 && enabled) {
         timeModifier = 0.0f;
      }
   #endif

      if (key == GLFW_KEY_SPACE && enabled) {
         doSave = true;
      }

      updateJoypadState(joypadState, key, enabled);
   };

   UPtr<GBC::Device> device = createDevice(window, argc > 1 ? argv[1] : nullptr);
   glfwSetWindowUserPointer(window, &device);

   static const double kMaxFrameTime = 0.25;
   static const double kDt = 1.0 / 60.0;
   double lastTime = glfwGetTime();
   double accumulator = 0.0;

   while (!glfwWindowShouldClose(window)) {
      double now = glfwGetTime();
      double frameTime = std::min(now - lastTime, kMaxFrameTime); // Cap the frame time to prevent spiraling
      lastTime = now;

      accumulator += frameTime * timeModifier;
      while (accumulator >= kDt) {
         device->setJoypadState(joypadState);
         device->tick(kDt);
         accumulator -= kDt;
      }

      renderer.draw(device->getLCDController().getFramebuffer());

      std::vector<uint8_t> audioData = device->getSoundController().getAudioData();
      audioManager.queue(audioData);

      // Try to save the game
      if (doSave) {
         doSave = false;
         saveGame(*device);
      }

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glfwDestroyWindow(window);
   glfwTerminate();

   return EXIT_SUCCESS;
}

#if defined(_WIN32)
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
   return main(__argc, __argv);
}
#endif // defined(_WIN32)

#endif // !defined(GBC_RUN_TESTS)
