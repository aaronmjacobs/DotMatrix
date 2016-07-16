#include "Constants.h"
#include "GLIncludes.h"
#include "Log.h"
#include "wrapper/video/Renderer.h"

#include "gbc/Device.h"
#if defined(RUN_TESTS)
#  include "test/CPUTester.h"
#endif // defined(RUN_TESTS)

#include <cstdlib>
#include <functional>

namespace {

std::function<void(int, int)> framebufferCallback;

void onFramebufferSizeChange(GLFWwindow *window, int width, int height) {
   if (framebufferCallback) {
      framebufferCallback(width, height);
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

uint8_t dist(float x1, float y1, float x2, float y2) {
   const float MAX_DIST = 80.0f;

   float xDiff = x2 - x1;
   float yDiff = y2 - y1;
   float dist = sqrt(yDiff * yDiff + xDiff * xDiff);
   float relativeDist = fmin(dist / MAX_DIST, 1.0f);
   return 0x1F - (uint8_t)(relativeDist * 0x1F);
}

} // namespace

int main(int argc, char *argv[]) {
   LOG_INFO(kProjectName << " " << kVersionType << " " << kVersionMajor << "." << kVersionMinor << "."
      << kVersionMicro << " (" << kVersionBuild << ")");

   GLFWwindow *window = init();
   if (!window) {
      return EXIT_FAILURE;
   }

   Renderer renderer;
   std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight> pixels;
   for (int i = 0; i < pixels.size(); ++i) {
      int x = i % GBC::kScreenWidth;
      int y = i / GBC::kScreenWidth;
      pixels[i] = { dist(x, y, GBC::kScreenWidth * 0.25f, GBC::kScreenHeight * 0.35f),
                    dist(x, y, GBC::kScreenWidth * 0.50f, GBC::kScreenHeight * 0.75f),
                    dist(x, y, GBC::kScreenWidth * 0.75f, GBC::kScreenHeight * 0.35f) };
   }

   framebufferCallback = [&renderer](int width, int height) {
      renderer.onFramebufferSizeChange(width, height);
   };

#if defined(RUN_TESTS)
   GBC::CPUTester cpuTester;
   for (int i = 0; i < 10; ++i) {
      cpuTester.runTests(true);
   }
#endif // defined(RUN_TESTS)

   GBC::Device device;

   static const double kMaxFrameTime = 0.25;
   static const double kDt = 1.0 / 60.0;
   double lastTime = glfwGetTime();
   double accumulator = 0.0;

   while (!glfwWindowShouldClose(window)) {
      double now = glfwGetTime();
      double frameTime = std::min(now - lastTime, kMaxFrameTime); // Cap the frame time to prevent spiraling
      lastTime = now;

      accumulator += frameTime;
      while (accumulator >= kDt) {
         device.tick(static_cast<float>(kDt));
         accumulator -= kDt;
      }

      renderer.draw(pixels);

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glfwDestroyWindow(window);
   glfwTerminate();

   return EXIT_SUCCESS;
}
