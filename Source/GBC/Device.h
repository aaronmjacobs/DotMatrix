#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "Pointers.h"

#include "GBC/CPU.h"
#include "GBC/LCDController.h"
#include "GBC/Memory.h"
#include "GBC/SoundController.h"

#include "Wrapper/Platform/IOUtils.h"

namespace GBC {

struct Joypad {
   bool right = false;
   bool left = false;
   bool up = false;
   bool down = false;
   bool a = false;
   bool b = false;
   bool select = false;
   bool start = false;

   static inline Joypad unionOf(const Joypad& first, const Joypad& second)
   {
      Joypad result;

      result.right = first.right || second.right;
      result.left = first.left || second.left;
      result.up = first.up || second.up;
      result.down = first.down || second.down;
      result.a = first.a || second.a;
      result.b = first.b || second.b;
      result.select = first.select || second.select;
      result.start = first.start || second.start;

      return result;
   }
};

class Device {
public:
   using SerialCallback = uint8_t (*)(uint8_t);

   Device();

   ~Device();

   void tick(double dt);

   void machineCycle();

   void setCartridge(UPtr<class Cartridge>&& cartridge);

   IOUtils::Archive saveCartRAM() const;

   bool loadCartRAM(IOUtils::Archive& ram);

   const char* title() const;

   Memory& getMemory() {
      return memory;
   }

   const LCDController& getLCDController() const {
      return lcdController;
   }

   SoundController& getSoundController() {
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

   bool cartWroteToRamThisFrame() const {
      return cartWroteToRam;
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

   uint64_t totalCycles;

   bool cartWroteToRam;

   Joypad joypad;
   uint8_t lastInputVals;

   uint16_t counter;
   uint16_t lastCounter;
   uint8_t clocksUntilInterrupt;
   uint8_t ifOverrideClocks;
   bool lastTimerBit;

   uint64_t serialCycles;
   SerialCallback serialCallback;

#if GBC_RUN_TESTS
   bool ignoreMachineCycles = false;
#endif // GBC_RUN_TESTS
};

} // namespace GBC

#endif
