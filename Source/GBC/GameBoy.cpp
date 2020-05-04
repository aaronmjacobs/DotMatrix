#include "Core/Assert.h"

#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"

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
   }

   namespace SC
   {
      enum Enum : uint8_t
      {
         TransferStartFlag = 1 << 7,
         ClockSpeed = 1 << 1, // CGB only
         ShiftClock = 1 << 0
      };
   }
}

uint8_t GameBoy::SerialControlRegister::read() const
{
   return startTransfer * SC::TransferStartFlag
      | 0x7E
      // | fastSpeed * SC::ClockSpeed // CGB only
      | useInternalClock * SC::ShiftClock;
}

void GameBoy::SerialControlRegister::write(uint8_t value)
{
   startTransfer = value & SC::TransferStartFlag;
   // fastSpeed = value & SC::ClockSpeed; // CGB only
   useInternalClock = value & SC::ShiftClock;
}

GameBoy::GameBoy()
   : cpu(*this)
   , lcdController(*this)
   , lastInputVals(P1::InMask)
{
}

// Need to define destructor in a location where the Cartridge class is defined, so a default deleter can be generated for it
GameBoy::~GameBoy()
{
}

void GameBoy::tick(double dt)
{
   if (cpu.isStopped() && joypad.anyPressed())
   {
      // The STOP state is exited when any button is pressed
      cpu.resume();
   }

   bool stepCPU = shouldStepCPU();
#if GBC_WITH_DEBUGGER
   if (inBreakMode)
   {
      stepCPU = false;
   }
#endif // GBC_WITH_DEBUGGER

   if (stepCPU)
   {
      targetCycles += static_cast<uint64_t>(CPU::kClockSpeed * dt + 0.5);

      while (totalCycles < targetCycles)
      {
         cpu.step();

#if GBC_WITH_DEBUGGER
         if (shouldBreak())
         {
            debugBreak();
         }
#endif // GBC_WITH_DEBUGGER
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
   counter += CPU::kClockCyclesPerMachineCycle;

   machineCycleJoypad();
   machineCycleTima();
   machineCycleSerial();
   lcdController.machineCycle();
   soundController.machineCycle();
}

#if GBC_WITH_BOOTSTRAP
void GameBoy::setBootstrap(std::vector<uint8_t> data)
{
   ASSERT(cpu.getPC() == 0x0100 && data.size() == 256);

   bootstrap = std::move(data);
   cpu.setPC(0x0000);
}
#endif // GBC_WITH_BOOTSTRAP

void GameBoy::setCartridge(UPtr<Cartridge> cartridge)
{
   cart = std::move(cartridge);
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

void GameBoy::onCPUStopped()
{
   targetCycles = totalCycles;
   lcdController.onCPUStopped();
}

#if GBC_WITH_DEBUGGER
void GameBoy::debugBreak()
{
   inBreakMode = true;
   targetCycles = totalCycles;
}

void GameBoy::debugContinue()
{
   inBreakMode = false;
}

void GameBoy::debugStep()
{
   if (inBreakMode && shouldStepCPU())
   {
      cpu.step();
   }
}

void GameBoy::setBreakpoint(uint16_t address)
{
   for (uint16_t breakpoint : breakpoints)
   {
      if (breakpoint == address)
      {
         return;
      }
   }

   breakpoints.push_back(address);
}

void GameBoy::clearBreakpoint(uint16_t address)
{
   breakpoints.erase(std::remove_if(breakpoints.begin(), breakpoints.end(), [address](uint16_t breakpoint) { return breakpoint == address; }), breakpoints.end());
}

bool GameBoy::shouldBreak() const
{
   for (uint16_t breakpoint : breakpoints)
   {
      if (cpu.getPC() == breakpoint)
      {
         return true;
      }
   }

   return false;
}
#endif // GBC_WITH_DEBUGGER

bool GameBoy::shouldStepCPU() const
{
   return hasProgram() & !cpu.isStopped();
}

void GameBoy::machineCycleJoypad()
{
   uint8_t dpadVals = P1::InMask;
   if ((p1 & P1::P14OutPort) == 0x00)
   {
      uint8_t right = joypad.right ? 0x00 : P1::P10InPort;
      uint8_t left = joypad.left ? 0x00 : P1::P11InPort;
      uint8_t up = joypad.up ? 0x00 : P1::P12InPort;
      uint8_t down = joypad.down ? 0x00 : P1::P13InPort;

      dpadVals = (right | left | up | down);
   }

   uint8_t faceVals = P1::InMask;
   if ((p1 & P1::P15OutPort) == 0x00)
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
      requestInterrupt(Interrupt::Joypad);
   }
   lastInputVals = inputVals;

   p1 = (p1 & P1::OutMask) | (inputVals & P1::InMask);
}

void GameBoy::machineCycleTima()
{
   timaReloadedWithTma = false;

   // Handle interrupt / TMA copy delay
   if (timaOverloaded)
   {
      timaOverloaded = false;
      tima = tma;
      timaReloadedWithTma = true;

      // If the IF register was written to during the last cycle, it overrides the value set here
      if (!ifWritten)
      {
         requestInterrupt(Interrupt::Timer);
      }
   }

   // Handle IF override
   ifWritten = false;

   // Increase TIMA on falling edge
   // This can be caused by a counter increase, counter reset, TAC change, TIMA disable, etc.
   bool enabled = (tac & TAC::TimerStartStop) != 0;
   uint16_t mask = TAC::kCounterMasks[tac & TAC::InputClockSelect];
   bool timerBit = (counter & mask) != 0 && enabled;

   if (lastTimerBit && !timerBit)
   {
      ++tima;

      if (tima == 0)
      {
         // Load from TMA and interrupt is delayed by 4 clocks
         timaOverloaded = true;
      }
   }
   lastTimerBit = timerBit;
}

void GameBoy::machineCycleSerial()
{
   static const uint16_t kSerialFrequency = 8192; // 8192Hz
   static const uint16_t kCyclesPerSerialBit = CPU::kClockSpeed / kSerialFrequency; // 512
   static const uint16_t kCyclesPerSerialByte = kCyclesPerSerialBit * 8; // 4096
   STATIC_ASSERT(CPU::kClockSpeed % kSerialFrequency == 0); // Should divide evenly

   if (serialControlRegister.startTransfer && serialControlRegister.useInternalClock)
   {
      serialCycles += CPU::kClockCyclesPerMachineCycle;

      if (serialCycles >= kCyclesPerSerialByte)
      {
         // Transfer done
         serialCycles = 0;

         uint8_t sentVal = sb;
         uint8_t receivedVal = 0xFF;
         if (serialCallback)
         {
            receivedVal = serialCallback(sentVal);
         }

         sb = receivedVal;
         serialControlRegister.startTransfer = false;
         requestInterrupt(Interrupt::Serial);
      }
   }
}

uint8_t GameBoy::readIO(uint16_t address) const
{
   ASSERT(address >= 0xFF00);

   uint8_t value = kInvalidAddressByte;

   switch (address & 0x00F0)
   {
   // General
   case 0x0000:
      switch (address)
      {
      case 0xFF00:
         value = p1 | 0xC0;
         break;
      case 0xFF01:
         value = sb;
         break;
      case 0xFF02:
         value = serialControlRegister.read();
         break;
      case 0xFF04:
         // DIV is just the upper 8 bits of the internal counter
         value = static_cast<uint8_t>((counter & 0xFF00) >> 8);
         break;
      case 0xFF05:
         value = tima;
         break;
      case 0xFF06:
         value = tma;
         break;
      case 0xFF07:
         value = tac | 0xF8;
         break;
      case 0xFF0F:
         value = ifr | 0xE0;
         break;
      default:
         break;
      }
      break;
   // Sound
   case 0x0010:
   case 0x0020:
   case 0x0030:
      value = soundController.read(address);
      break;
   // LCD
   case 0x0040:
      value = lcdController.read(address);
      break;
   // Bootstrap flag
   case 0x0050:
   case 0x0060:
   case 0x0070:
      break;
   // High RAM area
   case 0x0080:
   case 0x0090:
   case 0x00A0:
   case 0x00B0:
   case 0x00C0:
   case 0x00D0:
   case 0x00E0:
      value = ramh[address - 0xFF80];
      break;
   case 0x00F0:
      switch (address & 0x000F)
      {
      default:
         value = ramh[address - 0xFF80];
         break;
      // Interrupt enable register
      case 0x000F:
         value = ie;
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }

   return value;
}

void GameBoy::writeIO(uint16_t address, uint8_t value)
{
   ASSERT(address >= 0xFF00);

   switch (address & 0x00F0)
   {
   // General
   case 0x0000:
      switch (address)
      {
      case 0xFF00:
         p1 = value & 0x3F;
         break;
      case 0xFF01:
         sb = value;
         break;
      case 0xFF02:
         serialControlRegister.write(value);
         break;
      case 0xFF04: // DIV
         // Counter is reset when anything is written to DIV
         counter = 0;
         break;
      case 0xFF05: // TIMA
         // If TIMA was reloaded with TMA this machine cycle, the write is ignored
         if (!timaReloadedWithTma)
         {
            tima = value;
         }

         // Writing to TIMA during the delay will prevent the TMA copy and the interrupt
         timaOverloaded = false;
         break;
      case 0xFF06:
         tma = value;

         if (timaReloadedWithTma)
         {
            tima = value;
         }
         break;
      case 0xFF07:
         tac = value & 0x07;
         break;
      case 0xFF0F: // IF
         ifr = value & 0x1F;

         // Writing to IF during the delay between TIMA overflow and interrupt request overrides the IF change
         ifWritten = true;
         break;
      default:
         break;
      }
      break;
   // Sound
   case 0x0010:
   case 0x0020:
   case 0x0030:
      soundController.write(address, value);
      break;
   // LCD
   case 0x0040:
      lcdController.write(address, value);
      break;
   // Bootstrap flag
   case 0x0050:
#if GBC_WITH_BOOTSTRAP
      switch (address)
      {
      case 0xFF50:
         if (value == 0x01)
         {
            booting = false;
            bootstrap.clear();
         }
         break;
      default:
         break;
      }
      break;
#endif // GBC_WITH_BOOTSTRAP
   case 0x0060:
   case 0x0070:
      break;
   // High RAM area
   case 0x0080:
   case 0x0090:
   case 0x00A0:
   case 0x00B0:
   case 0x00C0:
   case 0x00D0:
   case 0x00E0:
      ramh[address - 0xFF80] = value;
      break;
   case 0x00F0:
      switch (address & 0x000F)
      {
      default:
         ramh[address - 0xFF80] = value;
         break;
      // Interrupt enable register
      case 0x000F:
         ie = value;
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }
}

uint8_t GameBoy::readDirect(uint16_t address) const
{
   uint8_t value = kInvalidAddressByte;

   switch (address & 0xF000)
   {
   // Permanently-mapped ROM bank
   case 0x0000:
#if GBC_WITH_BOOTSTRAP
      if (address <= 0x00FF && booting && !bootstrap.empty())
      {
         ASSERT(bootstrap.size() == 256);
         value = bootstrap[address];
         break;
      }
#endif // GBC_WITH_BOOTSTRAP
   case 0x1000:
   case 0x2000:
   case 0x3000:
   // Switchable ROM bank
   case 0x4000:
   case 0x5000:
   case 0x6000:
   case 0x7000:
      if (cart)
      {
         value = cart->read(address);
      }
      break;
   // Video RAM
   case 0x8000:
   case 0x9000:
      value = lcdController.read(address);
      break;
   // Switchable external RAM bank
   case 0xA000:
   case 0xB000:
      if (cart)
      {
         value = cart->read(address);
      }
      break;
   // Working RAM bank 0
   case 0xC000:
      value = ram0[address - 0xC000];
      break;
   // Working RAM bank 1
   case 0xD000:
      value = ram1[address - 0xD000];
      break;
   // Mirror of working ram
   case 0xE000:
      value = ram0[address - 0xE000];
      break;
   case 0xF000:
      switch (address & 0x0F00)
      {
      // Mirror of working ram
      default:
         value = ram1[address - 0xF000];
         break;
      // Sprite attribute table
      case 0x0E00:
         value = lcdController.read(address);
         break;
      // I/O device mappings
      case 0x0F00:
         value = readIO(address);
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }

   return value;
}

void GameBoy::writeDirect(uint16_t address, uint8_t value)
{
   switch (address & 0xF000)
   {
   // Permanently-mapped ROM bank
   case 0x0000:
#if GBC_WITH_BOOTSTRAP
      if (address <= 0x00FF && booting)
      {
         // Bootstrap is read only
         break;
      }
#endif // GBC_WITH_BOOTSTRAP
   case 0x1000:
   case 0x2000:
   case 0x3000:
   // Switchable ROM bank
   case 0x4000:
   case 0x5000:
   case 0x6000:
   case 0x7000:
      if (cart)
      {
         cart->write(address, value);
      }
      break;
   // Video RAM
   case 0x8000:
   case 0x9000:
      lcdController.write(address, value);
      break;
   // Switchable external RAM bank
   case 0xA000:
   case 0xB000:
      if (cart)
      {
         cart->write(address, value);
      }
      break;
   // Working RAM bank 0
   case 0xC000:
      ram0[address - 0xC000] = value;
      break;
   // Working RAM bank 1
   case 0xD000:
      ram1[address - 0xD000] = value;
      break;
   // Mirror of working ram
   case 0xE000:
      ram0[address - 0xE000] = value;
      break;
   case 0xF000:
      switch (address & 0x0F00)
      {
      // Mirror of working ram
      default:
         ram1[address - 0xF000] = value;
         break;
      // Sprite attribute table
      case 0x0E00:
         lcdController.write(address, value);
         break;
      // I/O device mappings
      case 0x0F00:
         writeIO(address, value);
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }
}

} // namespace GBC
