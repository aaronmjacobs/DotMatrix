#pragma once

#include "Core/Archive.h"
#include "Core/Pointers.h"

#include "GBC/CPU.h"
#include "GBC/LCDController.h"
#include "GBC/Memory.h"
#include "GBC/SoundController.h"

#include "UI/UIFriend.h"

namespace GBC
{

class Cartridge;

struct Joypad
{
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

namespace TAC
{

enum Enum : uint8_t
{
   TimerStartStop = 1 << 2,
   InputClockSelect = (1 << 1) | (1 << 0)
};

const std::array<uint16_t, 4> kCounterMasks =
{
   0x0200, // 4096 Hz, increase every 1024 clocks
   0x0008, // 262144 Hz, increase every 16 clocks
   0x0020, // 65536 Hz, increase every 64 clocks
   0x0080  // 16384 Hz, increase every 256 clocks
};

} // namespace TAC

class GameBoy
{
public:
   using SerialCallback = uint8_t (*)(uint8_t);

   GameBoy();
   ~GameBoy();

   void tick(double dt);
   void machineCycle();

   void setCartridge(UPtr<Cartridge> cartridge);
   Archive saveCartRAM() const;
   bool loadCartRAM(Archive& ram);

   const char* title() const;

   Memory& getMemory()
   {
      return memory;
   }

   CPU& getCPU()
   {
      return cpu;
   }

   LCDController& getLCDController()
   {
      return lcdController;
   }

   SoundController& getSoundController()
   {
      return soundController;
   }

   void setSerialCallback(SerialCallback callback)
   {
      serialCallback = callback;
   }

   void setJoypadState(Joypad joypadState)
   {
      joypad = joypadState;
   }

   void onDivWrite()
   {
      // Counter is reset when anything is written to DIV
      counter = 0;
   }

   void onTimaWrite()
   {
      // Writing to TIMA during the delay will prevent the TMA copy and the interrupt
      timaOverloaded = false;
   }

   void onIfWrite()
   {
      // Writing to IF during the delay between TIMA overflow and interrupt request overrides the IF change
      ifWritten = true;
   }

   bool wasTimaReloadedWithTma() const
   {
      return timaReloadedWithTma;
   }

   bool cartWroteToRamThisFrame() const
   {
      return cartWroteToRam;
   }

private:
   DECLARE_UI_FRIEND

   void tickJoypad();
   void tickDiv();
   void tickTima();
   void tickSerial();

   Memory memory;
   CPU cpu;
   LCDController lcdController;
   SoundController soundController;
   UPtr<Cartridge> cart;

   uint64_t targetCycles;
   uint64_t totalCycles;

   bool cartWroteToRam;

   Joypad joypad;
   uint8_t lastInputVals;

   uint16_t counter;
   bool timaOverloaded;
   bool ifWritten;
   bool timaReloadedWithTma;
   bool lastTimerBit;

   uint64_t serialCycles;
   SerialCallback serialCallback;
};

} // namespace GBC
