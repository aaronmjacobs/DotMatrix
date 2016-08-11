#include "FancyAssert.h"

#include "gbc/Cartridge.h"
#include "gbc/Device.h"

#include <array>

namespace GBC {

namespace {

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
   : cpu(memory), lcdController(memory), cart(nullptr), divCycles(0), timaCycles(0), serialCycles(0),
     serialCallback(nullptr) {
}

void Device::tick(float dt) {
   uint64_t previousCycles = cpu.getCycles();
   uint64_t targetCycles = previousCycles + static_cast<uint64_t>(CPU::kClockSpeed * dt);

   while (cpu.getCycles() < targetCycles) {
      cpu.tick();
      uint64_t cyclesTicked = cpu.getCycles() - previousCycles;

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

   if (memory.sc & SC::kTransferStartFlag) {
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
