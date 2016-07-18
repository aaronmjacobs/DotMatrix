#ifndef GBC_DEVICE_H
#define GBC_DEVICE_H

#include "Pointers.h"

#include "gbc/CPU.h"
#include "gbc/Memory.h"

namespace GBC {

class Cartridge;

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
