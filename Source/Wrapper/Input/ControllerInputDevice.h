#ifndef CONTROLLER_INPUT_DEVICE_H
#define CONTROLLER_INPUT_DEVICE_H

#include "InputDevice.h"

struct GLFWwindow;

class ControllerInputDevice : public InputDevice {
public:
   ControllerInputDevice(GLFWwindow* const window);

   virtual GBC::Joypad poll() override;

private:
   GLFWwindow* const window;
   int controllerId;
};

#endif
