#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H

#include "InputDevice.h"

struct GLFWwindow;

class KeyboardInputDevice : public InputDevice {
public:
   KeyboardInputDevice(GLFWwindow* const window);

   virtual GBC::Joypad poll() override;

private:
   bool pressed(int key);

   GLFWwindow* const window;
};

#endif
