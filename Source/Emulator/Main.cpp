#if defined(_WIN32) && !DM_WITH_DEBUG_UTILS
#  pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif // defined(_WIN32) && !DM_WITH_DEBUG_UTILS

#include "Core/Assert.h"
#include "Core/Log.h"

#include "Emulator/Emulator.h"

#include <boxer/boxer.h>
#include <GLFW/glfw3.h>

#if DM_WITH_DEBUG_UTILS
namespace
{
   void errorCallback(int errorCode, const char* description)
   {
      DM_ASSERT(false, "GLFW error %d: %s", errorCode, description);
   }
}
#endif // DM_WITH_DEBUG_UTILS

int main(int argc, char* argv[])
{
   DM_LOG_INFO(DM_PROJECT_DISPLAY_NAME << " version " << DM_VERSION_STRING);

#if DM_WITH_DEBUG_UTILS
   glfwSetErrorCallback(errorCallback);
#endif // DM_WITH_DEBUG_UTILS

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
