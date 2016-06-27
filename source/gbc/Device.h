#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "CPU.h"
#include "Memory.h"

namespace GBC {

class Device {
public:
   Device();

   void run();

private:
   CPU cpu;
   Memory memory;
};

} // namespace GBC

#endif
