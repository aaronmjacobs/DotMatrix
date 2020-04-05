#include "Core/Assert.h"

#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"

#include <array>

namespace GBC
{

namespace
{

namespace P1
{

enum Enum : uint8_t
{
   P10InPort = 0x01,
   P11InPort = 0x02,
   P12InPort = 0x04,
   P13InPort = 0x08,
   P14OutPort = 0x10,
   P15OutPort = 0x20,

   InMask = 0x0F,
   OutMask = 0x30
};

} // namespace P1

namespace SC
{

enum Enum : uint8_t
{
   TransferStartFlag = 1 << 7,
   ClockSpeed = 1 << 1, // CGB only
   ShiftClock = 1 << 0
};

} // namespace SC

} // namespace

GameBoy::GameBoy()
   : memory(*this)
   , cpu(*this)
   , lcdController(*this)
   , soundController()
   , cart(nullptr)
   , targetCycles(0)
   , totalCycles(0)
   , cartWroteToRam(false)
   , joypad({})
   , lastInputVals(P1::InMask)
   , counter(0)
   , timaOverloaded(false)
   , ifWritten(false)
   , timaReloadedWithTma(false)
   , lastTimerBit(false)
   , serialCycles(0)
   , serialCallback(nullptr)
{
}

// Need to define destructor in a location where the Cartridge class is defined, so a default deleter can be generated for it
GameBoy::~GameBoy()
{
}

void GameBoy::tick(double dt)
{
   targetCycles += static_cast<uint64_t>(CPU::kClockSpeed * dt + 0.5);

   while (totalCycles < targetCycles)
   {
#if GBC_WITH_UI
      if (cpu.isInBreakMode() && !cpu.isStepping())
      {
         targetCycles = totalCycles;
         break;
      }
#endif // GBC_WITH_UI

      if (!cpu.isStopped())
      {
         cpu.tick();
      }
      else
      {
         tickJoypad();

         if (cpu.isStopped())
         {
            // Not going to tick any cycles, so break to avoid an infinite loop
            targetCycles = totalCycles;
            break;
         }
      }
   }

   if (cart)
   {
      cartWroteToRam = cart->wroteToRamThisFrame();
      cart->tick(dt);
   }
   else
   {
      cartWroteToRam = false;
   }
}

void GameBoy::machineCycle()
{
   totalCycles += CPU::kClockCyclesPerMachineCycle;

   tickJoypad();
   tickDiv();
   tickTima();
   tickSerial();
   lcdController.machineCycle();
   soundController.machineCycle();
}

void GameBoy::setCartridge(UPtr<Cartridge> cartridge)
{
   cart = std::move(cartridge);
   memory.setCartridge(cart.get());
}

Archive GameBoy::saveCartRAM() const
{
   if (!cart)
   {
      return {};
   }

   return cart->saveRAM();
}

bool GameBoy::loadCartRAM(Archive& ram)
{
   if (!cart)
   {
      return false;
   }

   return cart->loadRAM(ram);
}

const char* GameBoy::title() const
{
   if (!cart)
   {
      return nullptr;
   }

   return cart->title();
}

void GameBoy::tickJoypad()
{
   // The STOP state is exited when any button is pressed
   if (cpu.isStopped()
      && (joypad.right || joypad.left || joypad.up || joypad.down
      || joypad.a || joypad.b || joypad.select || joypad.start))
   {
      cpu.resume();
   }

   uint8_t dpadVals = P1::InMask;
   if ((memory.p1 & P1::P14OutPort) == 0x00)
   {
      uint8_t right = joypad.right ? 0x00 : P1::P10InPort;
      uint8_t left = joypad.left ? 0x00 : P1::P11InPort;
      uint8_t up = joypad.up ? 0x00 : P1::P12InPort;
      uint8_t down = joypad.down ? 0x00 : P1::P13InPort;

      dpadVals = (right | left | up | down);
   }

   uint8_t faceVals = P1::InMask;
   if ((memory.p1 & P1::P15OutPort) == 0x00)
   {
      uint8_t a = joypad.a ? 0x00 : P1::P10InPort;
      uint8_t b = joypad.b ? 0x00 : P1::P11InPort;
      uint8_t select = joypad.select ? 0x00 : P1::P12InPort;
      uint8_t start = joypad.start ? 0x00 : P1::P13InPort;

      faceVals = (a | b | select | start);
   }

   uint8_t inputVals = dpadVals & faceVals;
   uint8_t inputChange = inputVals ^ lastInputVals;
   if ((inputVals & inputChange) != inputChange)
   {
      // At least one bit went from high to low
      memory.ifr |= Interrupt::Joypad;
   }
   lastInputVals = inputVals;

   memory.p1 = (memory.p1 & P1::OutMask) | (inputVals & P1::InMask);
}

void GameBoy::tickDiv()
{
   counter += CPU::kClockCyclesPerMachineCycle;

   // DIV is just the upper 8 bits of the internal counter
   memory.div = (counter & 0xFF00) >> 8;
}

void GameBoy::tickTima()
{
   timaReloadedWithTma = false;

   // Handle interrupt / TMA copy delay
   if (timaOverloaded)
   {
      timaOverloaded = false;
      memory.tima = memory.tma;
      timaReloadedWithTma = true;

      // If the IF register was written to during the last cycle, it overrides the value set here
      if (!ifWritten)
      {
         memory.ifr |= Interrupt::Timer;
      }
   }

   // Handle IF override
   ifWritten = false;

   // Increase TIMA on falling edge
   // This can be caused by a counter increase, counter reset, TAC change, TIMA disable, etc.
   bool enabled = (memory.tac & TAC::TimerStartStop) != 0;
   uint16_t mask = TAC::kCounterMasks[memory.tac & TAC::InputClockSelect];
   bool timerBit = (counter & mask) != 0 && enabled;

   if (lastTimerBit && !timerBit)
   {
      ++memory.tima;

      if (memory.tima == 0)
      {
         // Load from TMA and interrupt is delayed by 4 clocks
         timaOverloaded = true;
      }
   }
   lastTimerBit = timerBit;
}

void GameBoy::tickSerial()
{
   static const uint64_t kSerialFrequency = 8192; // 8192Hz
   static const uint64_t kCyclesPerSerialBit = CPU::kClockSpeed / kSerialFrequency; // 512
   static const uint64_t kCyclesPerSerialByte = kCyclesPerSerialBit * 8; // 4096
   STATIC_ASSERT(CPU::kClockSpeed % kSerialFrequency == 0); // Should divide evenly

   if ((memory.sc & SC::TransferStartFlag) && (memory.sc & SC::ShiftClock))
   {
      serialCycles += CPU::kClockCyclesPerMachineCycle;

      if (serialCycles >= kCyclesPerSerialByte)
      {
         // Transfer done
         serialCycles = 0;

         uint8_t sentVal = memory.sb;
         uint8_t receivedVal = 0xFF;
         if (serialCallback)
         {
            receivedVal = serialCallback(sentVal);
         }

         memory.sb = receivedVal;
         memory.sc &= ~SC::TransferStartFlag;
         memory.ifr |= Interrupt::Serial;
      }
   }
}

} // namespace GBC
