#include "Device.h"

namespace GBC {

Device::Device()
   : memory({0}), cpu(memory), cart(nullptr) {
}

void Device::tick(float dt) {
   cpu.tick(dt);
}

void Device::setCartridge(UPtr<Cartridge>&& cartridge) {
   cart = std::move(cartridge);
   cart->load(memory);
}

} // namespace GBC
