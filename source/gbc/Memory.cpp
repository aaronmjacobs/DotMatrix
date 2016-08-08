#include "FancyAssert.h"

#include "gbc/Bootstrap.h"
#include "gbc/Cartridge.h"
#include "gbc/Memory.h"

namespace GBC {

Memory::Memory()
   : raw{}, cart(nullptr) {
}

const uint8_t* Memory::getAddr(uint16_t address) const {
   if (boot == Boot::kBooting && address <= 0x00FF) {
      return &kBootstrap[address];
   }

   if (cart && address < 0x8000) {
      return cart->get(address);
   }

   if (address >= 0xE000 && address < 0xFE00) {
      // Mirror of working ram
      address -= 0x2000;
   }

   return &raw[address];
}

void Memory::set(uint16_t address, uint8_t val) {
   if (boot == Boot::kBooting && address <= 0x00FF) {
      return; // Bootstrap is read only
   }

   if (cart && address < 0x8000) {
      cart->set(address, val);
      return;
   }

   if (address >= 0xE000 && address < 0xFE00) {
      // Mirror of working ram
      address -= 0x2000;
   }

   raw[address] = val;
}

} // namespace GBC
