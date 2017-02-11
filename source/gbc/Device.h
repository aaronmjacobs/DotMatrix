#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "Pointers.h"

#include "gbc/CPU.h"
#include "gbc/LCDController.h"
#include "gbc/Memory.h"
#include "gbc/SoundController.h"

#include "wrapper/platform/IOUtils.h"

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

   void tick(double dt);

   void setCartridge(UPtr<class Cartridge>&& cartridge);

   IOUtils::Archive saveCartRAM() const;

   bool loadCartRAM(IOUtils::Archive& ram);

   const char* title() const;

   const LCDController& getLCDController() const {
      return lcdController;
   }

   const SoundController& getSoundController() const {
      return soundController;
   }

   void setSerialCallback(SerialCallback callback) {
      serialCallback = callback;
   }

   void setJoypadState(Joypad joypadState) {
      joypad = joypadState;
   }

   void onDivWrite() {
      // Counter is reset when anything is written to DIV
      counter = 0;
   }

   void onTimaWrite() {
      // Writing to TIMA during the delay will prevent the TMA copy and the interrupt
      clocksUntilInterrupt = 0;
   }

   void onIfWrite() {
      // Writing to IF during the delay between TIMA overflow and interrupt request overrides the IF change
      ifOverrideClocks = 4;
   }

private:
   void tickJoypad();

   void tickDiv(uint64_t cycles);

   void tickTima(uint64_t cycles);

   void tickSerial(uint64_t cycles);

   Memory memory;
   CPU cpu;
   LCDController lcdController;
   SoundController soundController;
   UPtr<class Cartridge> cart;

   Joypad joypad;
   uint8_t lastInputVals;

   uint16_t counter;
   uint16_t lastCounter;
   uint8_t clocksUntilInterrupt;
   uint8_t ifOverrideClocks;
   bool lastTimerBit;

   uint64_t serialCycles;
   SerialCallback serialCallback;
};

} // namespace GBC

#endif
