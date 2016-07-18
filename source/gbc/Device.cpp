#include "gbc/Cartridge.h"
#include "gbc/Device.h"

namespace GBC {

Device::Device()
   : memory({0}), cpu(memory), cart(nullptr) {
}

void Device::tick(float dt) {
   uint64_t initialCycles = cpu.getCycles();
   uint64_t targetCycles = initialCycles + CPU::kClockSpeed * dt;

   while (cpu.getCycles() < targetCycles) {
      cpu.tick();
   }
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   cart->load(memory);
}

} // namespace GBC
