#include "FancyAssert.h"

#include "gbc/Cartridge.h"
#include "gbc/Device.h"

#include <array>

namespace GBC {

namespace {

namespace P1 {

enum Enum {
   kP10InPort = 0x01,
   kP11InPort = 0x02,
   kP12InPort = 0x04,
   kP13InPort = 0x08,
   kP14OutPort = 0x10,
   kP15OutPort = 0x20,

   kInMask = 0x0F,
   kOutMask = 0x30
};

} // namespace P1

namespace TAC {

enum Enum {
   kTimerStartStop = 1 << 2,
   kInputClockSelect = (1 << 1) | (1 << 0)
};

const std::array<uint16_t, 4> kCounterMasks = {
   0x0200,  // 4096 Hz, increase every 1024 clocks
   0x0008,  // 262144 Hz, increase every 16 clocks
   0x0020,  // 65536 Hz, increase every 64 clocks
   0x0080   // 16384 Hz, increase every 256 clocks
};

} // namespace TAC

namespace SC {

enum Enum {
   kTransferStartFlag = 1 << 7,
   kClockSpeed = 1 << 1, // CGB only
   kShiftClock = 1 << 0
};

} // namespace SC

} // namespace

Device::Device()
   : memory(*this), cpu(memory), lcdController(memory), cart(nullptr), joypad({}), lastInputVals(P1::kInMask),
     counter(0), lastCounter(0), clocksUntilInterrupt(0), ifOverrideClocks(0), lastTimerBit(false),
     serialCycles(0), serialCallback(nullptr) {
}

void Device::tick(float dt) {
   uint64_t previousCycles = cpu.getCycles();
   uint64_t targetCycles = previousCycles + static_cast<uint64_t>(CPU::kClockSpeed * dt);

   while (cpu.getCycles() < targetCycles) {
      if (!cpu.isStopped()) {
         cpu.tick();
      }
      uint64_t cyclesTicked = cpu.getCycles() - previousCycles;

      tickJoypad();
      tickDiv(cyclesTicked);
      tickTima(cyclesTicked);
      tickSerial(cyclesTicked);
      lcdController.tick(cpu.getCycles(), cpu.isStopped());

      previousCycles = cpu.getCycles();

      if (cpu.isStopped()) {
         // Not going to tick any cycles, so break to avoid an infinite loop
         break;
      }
   }
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   memory.setCartridge(cart.get());
}

std::vector<uint8_t> Device::saveCartRAM() const {
   if (!cart) {
      return {};
   }

   return cart->saveRAM();
}

bool Device::loadCartRAM(const std::vector<uint8_t>& ram) {
   if (!cart) {
      return false;
   }

   return cart->loadRAM(ram);
}

const char* Device::title() const {
   if (!cart) {
      return "none";
   }

   return cart->title();
}

void Device::tickJoypad() {
   // The STOP state is exited when any button is pressed
   if (cpu.isStopped()
      && (joypad.right || joypad.left || joypad.up || joypad.down
      || joypad.a || joypad.b || joypad.select || joypad.start)) {
      cpu.resume();
   }

   uint8_t dpadVals = P1::kInMask;
   if ((memory.p1 & P1::kP14OutPort) == 0x00) {
      uint8_t right = joypad.right ? 0x00 : P1::kP10InPort;
      uint8_t left = joypad.left ? 0x00 : P1::kP11InPort;
      uint8_t up = joypad.up ? 0x00 : P1::kP12InPort;
      uint8_t down = joypad.down ? 0x00 : P1::kP13InPort;

      dpadVals = (right | left | up | down);
   }

   uint8_t faceVals = P1::kInMask;
   if ((memory.p1 & P1::kP15OutPort) == 0x00) {
      uint8_t a = joypad.a ? 0x00 : P1::kP10InPort;
      uint8_t b = joypad.b ? 0x00 : P1::kP11InPort;
      uint8_t select = joypad.select ? 0x00 : P1::kP12InPort;
      uint8_t start = joypad.start ? 0x00 : P1::kP13InPort;

      faceVals = (a | b | select | start);
   }

   uint8_t inputVals = dpadVals & faceVals;
   uint8_t inputChange = inputVals ^ lastInputVals;
   if ((inputVals & inputChange) != inputChange) {
      // At least one bit went from high to low
      memory.ifr |= Interrupt::kJoypad;
   }
   lastInputVals = inputVals;

   memory.p1 = (memory.p1 & P1::kOutMask) | (inputVals & P1::kInMask);
}

void Device::tickDiv(uint64_t cycles) {
   lastCounter = counter;
   counter += static_cast<uint16_t>(cycles);

   // DIV is just the upper 8 bits of the internal counter
   memory.div = (counter & 0xFF00) >> 8;
}

void Device::tickTima(uint64_t cycles) {
   bool enabled = (memory.tac & TAC::kTimerStartStop) != 0;
   uint16_t mask = TAC::kCounterMasks[memory.tac & TAC::kInputClockSelect];

   for (uint16_t c = lastCounter; c != counter; ++c) {
      // Handle interrupt / TMA copy delay
      if (clocksUntilInterrupt > 0) {
         --clocksUntilInterrupt;
         if (clocksUntilInterrupt == 0) {
            memory.tima = memory.tma;

            // If the IF register was written to during the last cycle, in overrides the value set here
            if (ifOverrideClocks == 0) {
               memory.ifr |= Interrupt::kTimer;
            }
         }
      }

      // Handle IF override
      if (ifOverrideClocks > 0) {
         --ifOverrideClocks;
      }

      // Increase TIMA on falling edge
      // This can be caused by a counter increase, counter reset, TAC change, TIMA disable, etc.
      bool timerBit = (c & mask) != 0 && enabled;
      if (lastTimerBit && !timerBit) {
         ++memory.tima;

         if (memory.tima == 0) {
            // Interrupt is delayed by 4 clocks
            clocksUntilInterrupt = 4;
         }
      }
      lastTimerBit = timerBit;
   }
}

void Device::tickSerial(uint64_t cycles) {
   static const uint64_t kSerialFrequency = 8192; // 8192Hz
   static const uint64_t kCyclesPerSerialBit = CPU::kClockSpeed / kSerialFrequency; // 512
   static const uint64_t kCyclesPerSerialByte = kCyclesPerSerialBit * 8; // 4096
   STATIC_ASSERT(CPU::kClockSpeed % kSerialFrequency == 0); // Should divide evenly

   if ((memory.sc & SC::kTransferStartFlag) && (memory.sc & SC::kShiftClock)) {
      serialCycles += cycles;

      if (serialCycles >= kCyclesPerSerialByte) {
         // Transfer done
         serialCycles = 0;

         uint8_t sentVal = memory.sb;
         uint8_t receivedVal = 0xFF;
         if (serialCallback) {
            receivedVal = serialCallback(sentVal);
         }

         memory.sb = receivedVal;
         memory.sc &= ~SC::kTransferStartFlag;
         memory.ifr |= Interrupt::kSerial;
      }
   }
}

} // namespace GBC
