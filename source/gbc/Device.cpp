#include "Device.h"

namespace GBC {

Device::Device()
   : memory({0}), cpu(memory) {
}

void Device::run() {
   for (int i = 0; i < 100; ++i) {
      cpu.tick();
   }
}

} // namespace GBC
