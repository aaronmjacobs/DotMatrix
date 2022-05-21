#include "Core/Archive.h"
#include "Core/Assert.h"
#include "Core/Log.h"

#include "Emulator/Emulator.h"

#include "GameBoy/Cartridge.h"
#include "GameBoy/GameBoy.h"
#include "GameBoy/LCDController.h"

#include "Platform/Video/Renderer.h"

#if DM_WITH_UI
#include "UI/UI.h"
#endif // DM_WITH_UI

#include <boxer/boxer.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <PlatformUtils/IOUtils.h>

#include <algorithm>
#include <future>
#include <vector>

namespace DotMatrix
{

// Green / blue (trying to approximate original Game Boy screen colors)
const std::array<Pixel, 4> kFramebufferColors =
{
   Pixel(0xAC, 0xCD, 0x4A),
   Pixel(0x7B, 0xAC, 0x6A),
   Pixel(0x20, 0x6A, 0x62),
   Pixel(0x08, 0x29, 0x52)
};

namespace
{
   #include "Logo.inl"

   const DotMatrix::Framebuffer& getLogoFramebuffer()
   {
      static DotMatrix::Framebuffer logoFramebuffer;
      static bool initialized = false;

      if (!initialized)
      {
         for (std::size_t i = 0; i < kLogo.size(); ++i)
         {
            for (std::size_t j = 0; j < 4; ++j)
            {
               uint8_t index = (kLogo[i] & (0x03 << (j * 2))) >> (j * 2);
               logoFramebuffer[4 * i + j] = index;
            }
         }

         initialized = true;
      }

      return logoFramebuffer;
   }

   void framebufferToPixels(const DotMatrix::Framebuffer& framebuffer, PixelArray& pixels)
   {
      DM_ASSERT(pixels.size() == framebuffer.size());

      for (std::size_t i = 0; i < pixels.size(); ++i)
      {
         DM_ASSERT(framebuffer[i] < kFramebufferColors.size());
         pixels[i] = kFramebufferColors[framebuffer[i]];
      }
   }

#if DM_DEBUG
   const char* getGlErrorName(GLenum error)
   {
      switch (error)
      {
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

   void gladPostCallback(const char* name, void* funcptr, int numArgs, ...)
   {
      GLenum errorCode = glad_glGetError();
      DM_ASSERT(errorCode == GL_NO_ERROR, "OpenGL error %d (%s) in %s", errorCode, getGlErrorName(errorCode), name);
   }
#endif // DM_DEBUG

   bool loadGl()
   {
      static bool loaded = false;

      if (!loaded)
      {
         loaded = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) != 0;

#if DM_DEBUG
         if (loaded)
         {
            glad_set_post_callback(gladPostCallback);
         }
#endif // DM_DEBUG
      }

      return loaded;
   }

   void framebufferSizeCallback(GLFWwindow* window, int width, int height)
   {
      if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window)))
      {
         emulator->onFramebufferSizeChanged(width, height);
      }
   }

   void dropCallback(GLFWwindow* window, int count, const char* paths[])
   {
      if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window)))
      {
         emulator->onFilesDropped(count, paths);
      }
   }

   void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
   {
      if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window)))
      {
         emulator->onKeyChanged(key, scancode, action, mods);
      }
   }

   void windowRefreshCallback(GLFWwindow* window)
   {
      if (Emulator* emulator = static_cast<Emulator*>(glfwGetWindowUserPointer(window)))
      {
         emulator->onWindowRefreshRequested();
      }
   }

   GLFWmonitor* selectFullScreenMonitor(const Bounds& windowBounds)
   {
      GLFWmonitor* fullScreenMonitor = glfwGetPrimaryMonitor();

      int windowCenterX = windowBounds.x + (windowBounds.width / 2);
      int windowCenterY = windowBounds.y + (windowBounds.height / 2);

      int monitorCount = 0;
      GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

      for (int i = 0; i < monitorCount; ++i)
      {
         GLFWmonitor* candidateMonitor = monitors[i];

         if (const GLFWvidmode* vidMode = glfwGetVideoMode(candidateMonitor))
         {
            Bounds monitorBounds;
            glfwGetMonitorPos(candidateMonitor, &monitorBounds.x, &monitorBounds.y);
            monitorBounds.width = vidMode->width;
            monitorBounds.height = vidMode->height;

            if (windowCenterX >= monitorBounds.x && windowCenterX < monitorBounds.x + monitorBounds.width &&
                windowCenterY >= monitorBounds.y && windowCenterY < monitorBounds.y + monitorBounds.height)
            {
               fullScreenMonitor = candidateMonitor;
               break;
            }
         }
      }

      return fullScreenMonitor;
   }

   std::string getWindowTitle(const DotMatrix::GameBoy* gameBoy)
   {
      static const std::string kBaseTitle = DM_PROJECT_DISPLAY_NAME;

      const char* gameTitle = gameBoy ? gameBoy->title() : nullptr;
      if (gameTitle)
      {
         return kBaseTitle + " - " + gameTitle;
      }

      return kBaseTitle;
   }

   std::optional<std::filesystem::path> getSavePath(const char* title)
   {
      std::optional<std::filesystem::path> savePath;

      if (title)
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

            // Relative to the app data directory
            savePath = IOUtils::getAbsoluteAppDataPath(DM_PROJECT_NAME, fileName);
         }
      }

      return savePath;
   }
}

Emulator::Emulator()
   : pixels(std::make_unique<PixelArray>(PixelArray{}))
{
}

Emulator::~Emulator()
{
   {
      std::lock_guard<std::mutex> lock(saveThreadMutex);
      exiting = true;
   }

   if (saveThread.joinable())
   {
      saveThreadConditionVariable.notify_all();
      saveThread.join();
   }

#if DM_WITH_UI
   ui = nullptr;
#endif // DM_WITH_UI

   renderer = nullptr;
   gameBoy = nullptr;

   if (window)
   {
      glfwDestroyWindow(window);
      window = nullptr;
   }
}

bool Emulator::init()
{
   if (window)
   {
      return true;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

   int windowWidth = DotMatrix::kScreenWidth * 2;
   int windowHeight = DotMatrix::kScreenHeight * 2;
#if DM_WITH_UI
   windowWidth = UI::getDesiredWindowWidth();
   windowHeight = UI::getDesiredWindowHeight();
#endif // DM_WITH_UI

   std::string windowTitle = getWindowTitle(nullptr);
   window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str() , nullptr, nullptr);
   if (!window)
   {
      boxer::show("Unable to create GLFW window", "Error", boxer::Style::Error);
      return false;
   }

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   if (!loadGl())
   {
      glfwDestroyWindow(window);
      window = nullptr;

      boxer::show("Unable to initialize OpenGL", "Error", boxer::Style::Error);
      return false;
   }

   int framebufferWidth = 0, framebufferHeight = 0;
   glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
   renderer = std::make_unique<Renderer>(framebufferWidth, framebufferHeight);

   keyboardInputDevice.setWindow(window);

   glfwSetWindowUserPointer(window, this);
   glfwSetWindowSizeLimits(window, DotMatrix::kScreenWidth, DotMatrix::kScreenHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
   glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
   glfwSetDropCallback(window, dropCallback);
   glfwSetKeyCallback(window, keyCallback);
   glfwSetWindowRefreshCallback(window, windowRefreshCallback);

#if DM_WITH_BOOTSTRAP
   if (std::optional<std::filesystem::path> bootstrapPath = IOUtils::getAboluteProjectPath("boot.bin"))
   {
      if (std::optional<std::vector<uint8_t>> bootstrapData = IOUtils::readBinaryFile(*bootstrapPath))
      {
         bootstrap = std::move(*bootstrapData);
      }
   }
#endif // DM_WITH_BOOTSTRAP

   resetGameBoy(nullptr);

   saveThread = std::thread([this]
   {
      saveThreadMain();
   });

#if DM_WITH_UI
   ui = std::make_unique<UI>(window);
#endif // DM_WITH_UI

   return true;
}

void Emulator::tick(double dt)
{
#if DM_WITH_UI
   if (skipNextTick)
   {
      skipNextTick = false;
      return;
   }

   dt *= timeScale;
#endif // DM_WITH_UI

   if (gameBoy)
   {
      DotMatrix::Joypad joypad = DotMatrix::Joypad::unionOf(keyboardInputDevice.poll(), controllerInputDevice.poll());
      gameBoy->setJoypadState(joypad);

      gameBoy->tick(dt);

      bool cartWroteToRamThisFrame = gameBoy->cartWroteToRamThisFrame();
      if (!cartWroteToRamThisFrame && cartWroteToRamLastFrame)
      {
         saveGameAsync();
      }
      cartWroteToRamLastFrame = cartWroteToRamThisFrame;
   }
   else
   {
      cartWroteToRamLastFrame = false;
   }
}

void Emulator::render()
{
   if (renderer)
   {
      if (gameBoy && gameBoy->hasProgram())
      {
         framebufferToPixels(gameBoy->getLCDController().getFramebuffer(), *pixels);
      }
      else
      {
         framebufferToPixels(getLogoFramebuffer(), *pixels);
      }

      renderer->draw(*pixels);

#if DM_WITH_UI
      if (renderUi)
      {
         ui->render(*this);
      }
#endif // DM_WITH_UI

#if DM_WITH_AUDIO
      if (gameBoy && audioManager.canQueue())
      {
         const std::vector<DotMatrix::AudioSample>& audioData = gameBoy->getSoundController().swapAudioBuffers();

         if (!audioData.empty())
         {
            audioManager.queue(audioData);
         }
      }
#endif // DM_WITH_AUDIO
   }

   glfwSwapBuffers(window);
}

bool Emulator::shouldExit() const
{
   return glfwWindowShouldClose(window) != GLFW_FALSE;
}

void Emulator::setRom(const char* romPath)
{
   // Try to load a cartridge
   if (romPath)
   {
      DM_LOG_INFO("Loading rom: " << romPath);

      if (std::optional<std::vector<uint8_t>> cartData = IOUtils::readBinaryFile(romPath))
      {
         std::string error;
         std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(*cartData), error);

         if (cartridge)
         {
            resetGameBoy(std::move(cartridge));

            std::string windowTitle = getWindowTitle(gameBoy.get());
            glfwSetWindowTitle(window, windowTitle.c_str());

            // Try to load a save file
            loadGame();

#if DM_WITH_UI
            ui->onRomLoaded(*gameBoy, romPath);
#endif // DM_WITH_UI
         }
         else
         {
            boxer::show(error.c_str(), "Error", boxer::Style::Error);
         }
      }
   }
}

void Emulator::onFramebufferSizeChanged(int width, int height)
{
   if (renderer)
   {
      renderer->onFramebufferSizeChanged(width, height);
   }
}

void Emulator::onFilesDropped(int count, const char* paths[])
{
   if (count > 0)
   {
      setRom(paths[0]);
   }
}

void Emulator::onKeyChanged(int key, int scancode, int action, int mods)
{
   bool enabled = action == GLFW_PRESS;

   if (enabled)
   {
      if (key == GLFW_KEY_F11 || (key == GLFW_KEY_ENTER && ((mods & GLFW_MOD_ALT) != 0)))
      {
         toggleFullScreen();
      }
#if DM_WITH_UI
      else if (key == GLFW_KEY_SPACE)
      {
         renderUi = !renderUi;

         // Hide the cursor if we're in full screen with no UI
         if (!renderUi && glfwGetWindowMonitor(window) != nullptr)
         {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
         }
      }
#endif // DM_WITH_UI
   }
}

void Emulator::onWindowRefreshRequested()
{
   // This will get called whenever the window has been dirtied and needs to be refreshed (e.g. when it is being resized)
   // We just re-render the last frame
   render();
}

void Emulator::resetGameBoy(std::unique_ptr<DotMatrix::Cartridge> cartridge)
{
   gameBoy = std::make_unique<DotMatrix::GameBoy>();

#if DM_WITH_BOOTSTRAP
   if (bootstrap.size() == 256)
   {
      gameBoy->setBootstrap(bootstrap);
   }
#endif // DM_WITH_BOOTSTRAP

   gameBoy->setCartridge(std::move(cartridge));

#if DM_WITH_AUDIO
   // Don't generate audio data if the audio manager isn't valid
   const bool generateAudioData = audioManager.isValid();
#else
   const bool generateAudioData = false;
#endif
   gameBoy->getSoundController().setGenerateAudioData(generateAudioData);

#if DM_WITH_UI
   // Render once before ticking (to make sure we hit any initial breakpoints)
   skipNextTick = true;
#endif // DM_WITH_UI
}

void Emulator::toggleFullScreen()
{
   DM_ASSERT(window);

   if (GLFWmonitor* currentMonitor = glfwGetWindowMonitor(window))
   {
      // Currently in full screen mode, swap back to windowed (with last saved window location)
      glfwSetWindowMonitor(window, nullptr, savedWindowBounds.x, savedWindowBounds.y, savedWindowBounds.width, savedWindowBounds.height, 0);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
   }
   else
   {
      // Currently in windowed mode, save the window location and swap to full screen
      glfwGetWindowPos(window, &savedWindowBounds.x, &savedWindowBounds.y);
      glfwGetWindowSize(window, &savedWindowBounds.width, &savedWindowBounds.height);

      if (GLFWmonitor* newMonitor = selectFullScreenMonitor(savedWindowBounds))
      {
         if (const GLFWvidmode* vidMode = glfwGetVideoMode(newMonitor))
         {
            glfwSetWindowMonitor(window, newMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);

#if DM_WITH_UI
            // Don't want to hide the cursor when showing the UI (used for input)
            if (!renderUi)
#endif // DM_WITH_UI
            {
               glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            }

            // Due to a bug, the previously set swap interval is ignored on Windows 10 when transitioning to full screen, so we set it again here
            // See: https://github.com/glfw/glfw/issues/1072
            glfwSwapInterval(1);
         }
      }
   }
}

void Emulator::loadGame()
{
   DM_ASSERT(gameBoy);

   if (std::optional<std::filesystem::path> saveFilePath = getSavePath(gameBoy->title()))
   {
      if (std::optional<std::vector<uint8_t>> cartRamData = IOUtils::readBinaryFile(*saveFilePath))
      {
         Archive cartRam(std::move(*cartRamData));

         if (gameBoy->loadCartRAM(cartRam))
         {
            DM_LOG_INFO("Loaded game from: " << *saveFilePath);

            {
               std::lock_guard<std::mutex> lock(saveThreadMutex);
               lastLoadTime.store(glfwGetTime());
            }
         }
      }
   }
}

void Emulator::saveGameAsync()
{
   DM_ASSERT(gameBoy);

   if (const char* title = gameBoy->title())
   {
      SaveData saveData;
      saveData.archive = gameBoy->saveCartRAM();
      saveData.gameTitle = title;

      if (!saveData.archive.getData().empty())
      {
         {
            std::lock_guard<std::mutex> lock(saveThreadMutex);
            saveQueue.enqueue(std::move(saveData));
         }

         saveThreadConditionVariable.notify_all();
      }
   }
}

void Emulator::saveThreadMain()
{
   std::unique_lock<std::mutex> lock(saveThreadMutex);

   while (!exiting)
   {
      saveThreadConditionVariable.wait(lock, [this]() -> bool
      {
         return exiting || saveQueue.peek() != nullptr;
      });

      lock.unlock();

      SaveData saveData;
      while (saveQueue.try_dequeue(saveData))
      {
         if (std::optional<std::filesystem::path> saveFilePath = getSavePath(saveData.gameTitle.c_str()))
         {
            if (std::filesystem::is_regular_file(*saveFilePath))
            {
               std::filesystem::path backupSaveFilePath = *saveFilePath;
               backupSaveFilePath += ".bak";

               bool timeSinceLastLoadSufficient = true;
               bool backupTimeDiffSufficient = true;
               if (std::filesystem::is_regular_file(backupSaveFilePath))
               {
                  double loadTimeDiff = glfwGetTime() - lastLoadTime.load();
                  timeSinceLastLoadSufficient = loadTimeDiff > 3.0;

                  std::filesystem::file_time_type saveModTime = std::filesystem::last_write_time(*saveFilePath);
                  std::filesystem::file_time_type backupModTime = std::filesystem::last_write_time(backupSaveFilePath);
                  auto modTimeDiff = saveModTime - backupModTime;

                  backupTimeDiffSufficient = modTimeDiff > std::chrono::seconds(10);
               }

               if (timeSinceLastLoadSufficient && backupTimeDiffSufficient)
               {
                  std::filesystem::rename(*saveFilePath, backupSaveFilePath);
               }
            }

            bool saved = IOUtils::writeBinaryFile(*saveFilePath, saveData.archive.getData());
            if (saved)
            {
               DM_LOG_INFO("Saved game to: " << *saveFilePath);
            }
            else
            {
               DM_LOG_WARNING("Failed to save game to: " << *saveFilePath);
            }
         }
      }

      lock.lock();
   }
}

} // namespace DotMatrix
