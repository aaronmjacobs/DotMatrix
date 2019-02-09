#include "Platform/Input/ControllerInputDevice.h"

#include <GLFW/glfw3.h>

namespace
{

int findFirstPresentGamepad()
{
   for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; ++i)
   {
      if (glfwJoystickIsGamepad(i) == GLFW_TRUE)
      {
         return i;
      }
   }

   return -1;
}

} // namespace

ControllerInputDevice::ControllerInputDevice()
   : controllerId(findFirstPresentGamepad())
{
}

GBC::Joypad ControllerInputDevice::poll()
{
   GBC::Joypad values;

   if (controllerId < 0 || glfwJoystickIsGamepad(controllerId) == GLFW_FALSE)
   {
      controllerId = findFirstPresentGamepad();
   }

   GLFWgamepadstate gamepadState = {};
   if (controllerId >= 0 && glfwGetGamepadState(controllerId, &gamepadState) == GLFW_TRUE)
   {
      static const float kAxisDeadzone = 0.5f;

      values.right = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS || gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > kAxisDeadzone;
      values.left = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS || gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -kAxisDeadzone;
      values.up = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS || gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -kAxisDeadzone;
      values.down = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS || gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > kAxisDeadzone;

      values.a = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
      values.b = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;

      values.select = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS;
      values.start = gamepadState.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
   }

   return values;
}
