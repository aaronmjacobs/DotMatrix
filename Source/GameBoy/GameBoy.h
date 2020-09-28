#pragma once

#include "Core/Archive.h"
#include "Core/Assert.h"
#include "Core/Enum.h"

#include "GameBoy/CPU.h"
#include "GameBoy/LCDController.h"
#include "GameBoy/SoundController.h"

#include <array>
#include <functional>
#include <memory>
#if DM_WITH_DEBUGGER
#include <vector>
#endif // DM_WITH_DEBUGGER

namespace DotMatrix
{

class Cartridge;

enum class Interrupt : uint8_t
{
   VBlank = 1 << 0,
   LCDState = 1 << 1,
   Timer = 1 << 2,
   Serial = 1 << 3,
   Joypad = 1 << 4,
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
}

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

   bool anyPressed() const
   {
      return right || left || up || down || a || b || select || start;
   }
};

class GameBoy
{
public:
   static const inline uint8_t kInvalidAddressByte = 0xFF;

   using SerialCallback = std::function<uint8_t(uint8_t)>;

   GameBoy();
   ~GameBoy();

   void tick(double dt);
   void machineCycle();

#if DM_WITH_BOOTSTRAP
   void setBootstrap(std::vector<uint8_t> data);
#endif // DM_WITH_BOOTSTRAP

   void setCartridge(std::unique_ptr<Cartridge> cartridge);
   Archive saveCartRAM() const;
   bool loadCartRAM(Archive& ram);

   const char* title() const;

   void onCPUStopped();

#if DM_WITH_DEBUGGER
   void debugBreak();
   void debugContinue();
   void debugStep();

   void setBreakpoint(uint16_t address);
   void clearBreakpoint(uint16_t address);
   bool shouldBreak() const;

   bool isInBreakMode() const
   {
      return inBreakMode;
   }
#endif // DM_WITH_DEBUGGER

   LCDController& getLCDController()
   {
      return lcdController;
   }

   SoundController& getSoundController()
   {
      return soundController;
   }

   bool hasProgram() const
   {
      return cart != nullptr
#if DM_WITH_BOOTSTRAP
         || !bootstrap.empty()
#endif // DM_WITH_BOOTSTRAP
         ;
   }

   void setSerialCallback(SerialCallback callback)
   {
      serialCallback = callback;
   }

   void setJoypadState(Joypad joypadState)
   {
      joypad = joypadState;
   }

   uint8_t read(uint16_t address)
   {
      machineCycle();
      return readDirect(address);
   }

   void write(uint16_t address, uint8_t value)
   {
      machineCycle();
      writeDirect(address, value);
   }

   bool cartWroteToRamThisFrame() const
   {
      return cartWroteToRam;
   }

   bool isAnyInterruptActive() const
   {
      DM_ASSERT((ifr & 0xE0) == 0);
      return (ifr & ie) != 0;
   }

   bool isInterruptActive(Interrupt interrupt) const
   {
      return (ifr & Enum::cast(interrupt)) && (ie & Enum::cast(interrupt));
   }

   void requestInterrupt(Interrupt interrupt)
   {
      ifr |= Enum::cast(interrupt);
   }

   void clearInterruptRequest(Interrupt interrupt)
   {
      ifr &= ~Enum::cast(interrupt);
   }

private:
   bool shouldStepCPU() const;

   void machineCycleJoypad();
   void machineCycleTima();
   void machineCycleSerial();

   uint8_t readIO(uint16_t address) const;
   void writeIO(uint16_t address, uint8_t value);

public:
   uint8_t readDirect(uint16_t address) const;
   void writeDirect(uint16_t address, uint8_t value);

private:
   struct SerialControlRegister
   {
      bool startTransfer = false;
      // bool fastSpeed = false; // CGB only
      bool useInternalClock = false;

      uint8_t read() const;
      void write(uint8_t value);
   };

   CPU cpu;
   LCDController lcdController;
   SoundController soundController;
   std::unique_ptr<Cartridge> cart;

   uint64_t targetCycles = 0;
   uint64_t totalCycles = 0;

   bool cartWroteToRam = false;

   Joypad joypad;
   uint8_t lastInputVals;

   uint16_t counter = 0;
   bool timaOverloaded = false;
   bool ifWritten = false;
   bool timaReloadedWithTma = false;
   bool lastTimerBit = false;

   SerialControlRegister serialControlRegister;
   uint16_t serialCycles = 0;
   SerialCallback serialCallback = nullptr;

#if DM_WITH_BOOTSTRAP
   std::vector<uint8_t> bootstrap;
   bool booting = true;
#endif // DM_WITH_BOOTSTRAP

#if DM_WITH_DEBUGGER
   bool inBreakMode = false;
   std::vector<uint16_t> breakpoints;
#endif // DM_WITH_DEBUGGER

   std::array<uint8_t, 0x1000> ram0 = {}; // Working RAM bank 0 (0xC000-0xCFFF)
   std::array<uint8_t, 0x1000> ram1 = {}; // Working RAM bank 1 (0xD000-0xDFFF)
   std::array<uint8_t, 0x007F> ramh = {}; // High RAM area (0xFF80-0xFFFE)

   uint8_t p1 = 0x00; // Joy pad / system info (0xFF00)
   uint8_t sb = 0x00; // Serial transfer data (0xFF01)
   uint8_t tima = 0x00; // Timer counter (0xFF05)
   uint8_t tma = 0x00; // Timer modulo (0xFF06)
   uint8_t tac = 0x00; // Timer control (0xFF07)

   uint8_t ifr = 0x00; // Interrupt flag (0xFF0F)
   uint8_t ie = 0x00; // Interrupt enable register (0xFFFF)
};

} // namespace DotMatrix
