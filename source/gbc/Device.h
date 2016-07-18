#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "Cartridge.h"
#include "CPU.h"
#include "Memory.h"
#include "Types.h"

namespace GBC {

class Device {
public:
   Device();

   void tick(float dt);

   void setCartridge(UPtr<Cartridge>&& cartridge);

private:
   Memory memory;
   CPU cpu;

   UPtr<Cartridge> cart;
};

} // namespace GBC

#endif
