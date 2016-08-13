#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "Pointers.h"

#include "gbc/CPU.h"
#include "gbc/LCDController.h"
#include "gbc/Memory.h"

namespace GBC {

struct Joypad {
   bool right;
   bool left;
   bool up;
   bool down;
   bool a;
   bool b;
   bool select;
   bool start;
};

class Device {
public:
   using SerialCallback = uint8_t (*)(uint8_t);

   Device();

   void tick(float dt);

   void setCartridge(UPtr<class Cartridge>&& cartridge);

   const LCDController& getLCDController() const {
      return lcdController;
   }

   void setSerialCallback(SerialCallback callback) {
      serialCallback = callback;
   }

   void setJoypadState(Joypad joypadState) {
      joypad = joypadState;
   }

private:
   void tickJoypad();

   void tickDiv(uint64_t cycles);

   void tickTima(uint64_t cycles);

   void tickSerial(uint64_t cycles);

   Memory memory;
   CPU cpu;
   LCDController lcdController;

   UPtr<class Cartridge> cart;
   Joypad joypad;
   uint8_t lastInputVals;

   uint64_t divCycles;
   uint64_t timaCycles;
   uint64_t serialCycles;

   SerialCallback serialCallback;
};

} // namespace GBC

#endif
