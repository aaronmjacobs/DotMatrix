#include "Emulator.h"

#include "Core/Constants.h"
#include "Core/Log.h"

#include <GLFW/glfw3.h>

int main(int argc, char* argv[])
{
   LOG_INFO(kProjectName << " " << kVersionMajor << "." << kVersionMinor << "." << kVersionMicro << " (" << kVersionBuild << ")");

   if (!glfwInit())
   {
      LOG_ERROR_MSG_BOX("Unable to initialize GLFW");
      return 1;
   }

   {
      Emulator emulator;
      if (!emulator.init())
      {
         glfwTerminate();
         return 1;
      }

      if (argc > 1)
      {
         emulator.setRom(argv[1]);
      }

      static const double kMaxFrameTime = 0.05;
      double lastTime = glfwGetTime();

      while (!emulator.shouldExit())
      {
         double now = glfwGetTime();
         double frameTime = now - lastTime;
         lastTime = now;

         // If the frame time is larger than the allowed max, we assume it was caused by an external event (e.g. the window was dragged)
         // In this case, it's best to not tick any time to prevent audio from getting behind
         if (frameTime > kMaxFrameTime)
         {
            frameTime = 0.0;
         }

#if GBC_DEBUG
         frameTime *= emulator.getTimeModifier();
#endif // GBC_DEBUG

         glfwPollEvents();

         emulator.tick(frameTime);
         emulator.render();
      }
   }

   glfwTerminate();
   return 0;
}

// Don't create a console in Windows release builds
#if defined(_WIN32) && !GBC_DEBUG
#  pragma comment(linker, "/SUBSYSTEM:WINDOWS")
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
   return main(__argc, __argv);
}
#endif // defined(_WIN32) && !GBC_DEBUG
