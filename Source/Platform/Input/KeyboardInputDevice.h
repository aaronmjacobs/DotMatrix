#pragma once

#include "Platform/Input/InputDevice.h"

struct GLFWwindow;

class KeyboardInputDevice : public InputDevice
{
public:
   KeyboardInputDevice(GLFWwindow* initialWindow = nullptr);

   DotMatrix::Joypad poll() override;

   void setWindow(GLFWwindow* newWindow)
   {
      window = newWindow;
   }

private:
   bool pressed(int key);

   GLFWwindow* window;
};
