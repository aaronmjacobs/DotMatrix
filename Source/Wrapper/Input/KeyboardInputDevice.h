#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H

#include "InputDevice.h"

struct GLFWwindow;

class KeyboardInputDevice : public InputDevice {
public:
   KeyboardInputDevice(GLFWwindow* initialWindow = nullptr);

   virtual GBC::Joypad poll() override;

   void setWindow(GLFWwindow* newWindow) {
      window = newWindow;
   }

private:
   bool pressed(int key);

   GLFWwindow* window;
};

#endif
