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

} // namespace

Device::Device()
   : cpu(memory), lcdController(memory), cart(nullptr), divCycles(0), timaCycles(0) {
}

void Device::tick(float dt) {
   uint64_t previousCycles = cpu.getCycles();
   uint64_t targetCycles = previousCycles + static_cast<uint64_t>(CPU::kClockSpeed * dt);

   while (cpu.getCycles() < targetCycles) {
      cpu.tick();
      uint64_t cyclesTicked = cpu.getCycles() - previousCycles;

      tickDiv(cyclesTicked);
      tickTima(cyclesTicked);
      lcdController.tick(cpu.getCycles());

      previousCycles = cpu.getCycles();
   }
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   cart->load(memory);
}

void Device::tickDiv(uint64_t cycles) {
   static const uint64_t kDivFrequency = 16384; // 16384Hz
   static const uint64_t kCyclesPerDiv = CPU::kClockSpeed / kDivFrequency; // 256
   STATIC_ASSERT(CPU::kClockSpeed % kDivFrequency == 0); // Should divide evenly

   // TODO divider register - writing any value sets it to $00
   divCycles += cycles;
   uint8_t divTicks = divCycles / kCyclesPerDiv;

   divCycles %= kCyclesPerDiv;
   memory.div += divTicks;
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

} // namespace GBC
