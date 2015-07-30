#include "Constants.h"
#include "GLIncludes.h"
#include "LogHelper.h"
#include "wrapper/Renderer.h"

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
      LOG_ERROR("Unable to initialize GLFW");
      return nullptr;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

   GLFWwindow *window = glfwCreateWindow(GBC::SCREEN_WIDTH, GBC::SCREEN_HEIGHT, PROJECT_DISPLAY_NAME, nullptr, nullptr);
   if (!window) {
      glfwTerminate();
      LOG_ERROR("Unable to create GLFW window");
      return nullptr;
   }

   glfwSetFramebufferSizeCallback(window, onFramebufferSizeChange);
   glfwMakeContextCurrent(window);
   glfwSwapInterval(1); // VSYNC

   int gladInitRes = gladLoadGL();
   if (!gladInitRes) {
      glfwDestroyWindow(window);
      glfwTerminate();
      LOG_ERROR("Unable to initialize glad");
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
   LOG_INFO(PROJECT_NAME << " " << VERSION_TYPE << " " << VERSION_MAJOR << "." << VERSION_MINOR << "."
      << VERSION_MICRO << "." << VERSION_BUILD);

   GLFWwindow *window = init();
   if (!window) {
      return EXIT_FAILURE;
   }

   Renderer renderer;
   std::array<GBC::Pixel, GBC::SCREEN_WIDTH * GBC::SCREEN_HEIGHT> pixels;
   for (int i = 0; i < pixels.size(); ++i) {
      int x = i % GBC::SCREEN_WIDTH;
      int y = i / GBC::SCREEN_WIDTH;
      pixels[i] = { dist(x, y, GBC::SCREEN_WIDTH * 0.25f, GBC::SCREEN_HEIGHT * 0.35f),
                    dist(x, y, GBC::SCREEN_WIDTH * 0.50f, GBC::SCREEN_HEIGHT * 0.75f),
                    dist(x, y, GBC::SCREEN_WIDTH * 0.75f, GBC::SCREEN_HEIGHT * 0.35f) };
   }

   framebufferCallback = [&renderer](int width, int height) {
      renderer.onFramebufferSizeChange(width, height);
   };

   while (!glfwWindowShouldClose(window)) {
      renderer.draw(pixels);

      glfwSwapBuffers(window);

      glfwPollEvents();
   }

   glfwDestroyWindow(window);
   glfwTerminate();

   return EXIT_SUCCESS;
}
