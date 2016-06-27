#include "Device.h"

namespace GBC {

Device::Device()
   : cpu(memory), memory({0}) {
}

void Device::run() {
   for (int i = 0; i < 100; ++i) {
      cpu.tick();
   }
}

} // namespace GBC
