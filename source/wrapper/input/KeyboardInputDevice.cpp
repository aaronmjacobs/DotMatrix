#include "KeyboardInputDevice.h"

#include "GLIncludes.h"

KeyboardInputDevice::KeyboardInputDevice(GLFWwindow* const window)
   : window(window) {
}

KeyboardInputDevice::~KeyboardInputDevice() {
}

bool KeyboardInputDevice::pressed(int key) {
   return glfwGetKey(window, key) == GLFW_PRESS;
}

InputValues KeyboardInputDevice::poll() {
   InputValues values;

   values.up = pressed(GLFW_KEY_UP);
   values.down = pressed(GLFW_KEY_DOWN);
   values.left = pressed(GLFW_KEY_LEFT);
   values.right = pressed(GLFW_KEY_RIGHT);

   values.a = pressed(GLFW_KEY_A);
   values.b = pressed(GLFW_KEY_S);

   values.start = pressed(GLFW_KEY_ENTER);
   values.select = pressed(GLFW_KEY_RIGHT_SHIFT);

   return values;
}
