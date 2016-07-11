#include "Device.h"

namespace GBC {

Device::Device()
   : memory({0}), cpu(memory) {
}

void Device::tick(float dt) {
   cpu.tick(dt);
}

} // namespace GBC
