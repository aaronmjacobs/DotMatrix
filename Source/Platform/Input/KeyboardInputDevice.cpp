#include "Platform/Input/KeyboardInputDevice.h"

#include <GLFW/glfw3.h>

KeyboardInputDevice::KeyboardInputDevice(GLFWwindow* initialWindow)
   : window(initialWindow)
{
}

DotMatrix::Joypad KeyboardInputDevice::poll()
{
   DotMatrix::Joypad values;

   values.right = pressed(GLFW_KEY_RIGHT);
   values.left = pressed(GLFW_KEY_LEFT);
   values.up = pressed(GLFW_KEY_UP);
   values.down = pressed(GLFW_KEY_DOWN);

   values.a = pressed(GLFW_KEY_S);
   values.b = pressed(GLFW_KEY_A);

   values.select = pressed(GLFW_KEY_Z);
   values.start = pressed(GLFW_KEY_X);

   // Needed since many keyboards do not support 4 nearby buttons being pressed at the same time
   // Used for certain game functions, e.g. reset, save, etc.
   if (pressed(GLFW_KEY_D))
   {
      values.a = true;
      values.b = true;
      values.select = true;
      values.start = true;
   }

   return values;
}

bool KeyboardInputDevice::pressed(int key)
{
   return window && glfwGetKey(window, key) == GLFW_PRESS;
}
