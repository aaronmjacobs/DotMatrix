#pragma once

#include "GBC/GameBoy.h"

class InputDevice
{
public:
   virtual ~InputDevice() = default;

   virtual GBC::Joypad poll() = 0;
};
