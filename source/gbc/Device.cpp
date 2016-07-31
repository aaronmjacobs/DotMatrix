#include "gbc/Cartridge.h"
#include "gbc/Device.h"

namespace GBC {

Device::Device()
   : cpu(memory), cart(nullptr) {
}

void Device::tick(float dt) {
   uint64_t previousCycles = cpu.getCycles();
   uint64_t targetCycles = previousCycles + static_cast<uint64_t>(CPU::kClockSpeed * dt);

   while (cpu.getCycles() < targetCycles) {
      cpu.tick();

      previousCycles = cpu.getCycles();
   }
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   cart->load(memory);
}

} // namespace GBC
