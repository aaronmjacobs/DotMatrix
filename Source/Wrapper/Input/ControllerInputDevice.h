#ifndef CONTROLLER_INPUT_DEVICE_H
#define CONTROLLER_INPUT_DEVICE_H

#include "InputDevice.h"

class ControllerInputDevice : public InputDevice {
public:
   ControllerInputDevice();

   virtual GBC::Joypad poll() override;

private:
   int controllerId;
};

#endif
