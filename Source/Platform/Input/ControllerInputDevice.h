#pragma once

#include "Platform/Input/InputDevice.h"

class ControllerInputDevice : public InputDevice
{
public:
   ControllerInputDevice();

   GBC::Joypad poll() override;

private:
   int controllerId;
};
