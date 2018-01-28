#include "Constants.h"

#if !defined(GBC_RUN_TESTS)

#include "Emulator.h"
#include "GLIncludes.h"
#include "Log.h"

int main(int argc, char* argv[]) {
   LOG_INFO(kProjectName << " " << kVersionMajor << "." << kVersionMinor << "." << kVersionMicro << " (" << kVersionBuild << ")");

   if (!glfwInit()) {
      LOG_ERROR_MSG_BOX("Unable to initialize GLFW");
      return 1;
   }

   {
      Emulator emulator;
      if (!emulator.init()) {
         glfwTerminate();
         return 1;
      }

      if (argc > 1) {
         emulator.setRom(argv[1]);
      }

      static const double kMaxFrameTime = 0.25;
      static const double kDt = 1.0 / 60.0;
      double lastTime = glfwGetTime();
      double accumulator = 0.0;

      while (!emulator.shouldExit()) {
         double now = glfwGetTime();
         double frameTime = std::min(now - lastTime, kMaxFrameTime); // Cap the frame time to prevent spiraling
         lastTime = now;

#if GBC_DEBUG
         frameTime *= emulator.getTimeModifier();
#endif // GBC_DEBUG

         accumulator += frameTime;
         while (accumulator >= kDt) {
            glfwPollEvents();
            emulator.tick(kDt);
            accumulator -= kDt;
         }

         emulator.render();
         glfwPollEvents();
      }
   }

   glfwTerminate();
   return 0;
}

#if defined(_WIN32)
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
   return main(__argc, __argv);
}
#endif // defined(_WIN32)

#endif // !defined(GBC_RUN_TESTS)
