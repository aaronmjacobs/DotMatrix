#if defined(_WIN32) && !DM_DEBUG
#  pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif // defined(_WIN32) && !DM_DEBUG

#include "Core/Log.h"

#include "Emulator/Emulator.h"

#include <boxer/boxer.h>
#include <GLFW/glfw3.h>

int main(int argc, char* argv[])
{
   DM_LOG_INFO(DM_PROJECT_DISPLAY_NAME << " version " << DM_VERSION_STRING);

   if (!glfwInit())
   {
      boxer::show("Unable to initialize GLFW", "Error", boxer::Style::Error);
      return 1;
   }

   {
      DotMatrix::Emulator emulator;
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

         glfwPollEvents();

         emulator.tick(frameTime);
         emulator.render();
      }
   }

   glfwTerminate();
   return 0;
}
