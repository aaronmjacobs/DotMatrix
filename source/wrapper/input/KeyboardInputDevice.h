#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H

#include "InputDevice.h"

struct GLFWwindow;

class KeyboardInputDevice : public InputDevice {
private:
   GLFWwindow* const window;

   bool pressed(int key);

public:
   KeyboardInputDevice(GLFWwindow* const window);

   virtual ~KeyboardInputDevice();

   virtual InputValues poll();
};

#endif
