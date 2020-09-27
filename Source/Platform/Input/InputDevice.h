#pragma once

#include <DotMatrixCore/GameBoy/GameBoy.h>

class InputDevice
{
public:
   virtual ~InputDevice() = default;

   virtual DotMatrix::Joypad poll() = 0;
};
