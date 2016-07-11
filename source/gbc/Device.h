#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "CPU.h"
#include "Memory.h"

namespace GBC {

class Device {
public:
   Device();

   void tick(float dt);

private:
   Memory memory;
   CPU cpu;
};

} // namespace GBC

#endif
