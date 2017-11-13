#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include "GBC/Device.h"

class InputDevice {
public:
   virtual ~InputDevice() = default;

   virtual GBC::Joypad poll() = 0;
};

#endif
