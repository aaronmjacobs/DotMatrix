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

static constexpr std::array<uint64_t, 4> kFrequencies = {
   4096,
   262144,
   65536,
   16384
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
   : cpu(memory), lcdController(memory), cart(nullptr), joypad({}), lastInputVals(P1::kInMask), divCycles(0),
     timaCycles(0), serialCycles(0), serialCallback(nullptr) {
}

void Device::tick(float dt) {
   uint64_t previousCycles = cpu.getCycles();
   uint64_t targetCycles = previousCycles + static_cast<uint64_t>(CPU::kClockSpeed * dt);

   while (cpu.getCycles() < targetCycles) {
      cpu.tick();
      uint64_t cyclesTicked = cpu.getCycles() - previousCycles;

      tickJoypad();
      tickDiv(cyclesTicked);
      tickTima(cyclesTicked);
      tickSerial(cyclesTicked);
      lcdController.tick(cpu.getCycles());

      previousCycles = cpu.getCycles();
   }
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   memory.setCartridge(cart.get());
}

void Device::tickJoypad() {
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
   static const uint64_t kDivFrequency = 16384; // 16384Hz
   static const uint64_t kCyclesPerDiv = CPU::kClockSpeed / kDivFrequency; // 256
   STATIC_ASSERT(CPU::kClockSpeed % kDivFrequency == 0); // Should divide evenly

   divCycles += cycles;

   while (divCycles >= kCyclesPerDiv) {
      divCycles -= kCyclesPerDiv;
      ++memory.div;
   }
}

void Device::tickTima(uint64_t cycles) {
   if (memory.tac & TAC::kTimerStartStop) {
      timaCycles += cycles;

      // All frequencies should evenly divide the clock speed
      STATIC_ASSERT(CPU::kClockSpeed % TAC::kFrequencies[0] == 0);
      STATIC_ASSERT(CPU::kClockSpeed % TAC::kFrequencies[1] == 0);
      STATIC_ASSERT(CPU::kClockSpeed % TAC::kFrequencies[2] == 0);
      STATIC_ASSERT(CPU::kClockSpeed % TAC::kFrequencies[3] == 0);

      uint64_t frequency = TAC::kFrequencies[memory.tac & TAC::kInputClockSelect];
      uint64_t cyclesPerTimerUpdate = CPU::kClockSpeed / frequency;

      while (timaCycles >= cyclesPerTimerUpdate) {
         timaCycles -= cyclesPerTimerUpdate;
         ++memory.tima;

         if (memory.tima == 0) {
            memory.tima = memory.tma;
            memory.ifr |= Interrupt::kTimer;
         }
      }
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
