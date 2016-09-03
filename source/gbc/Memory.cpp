#include "FancyAssert.h"

#include "gbc/Bootstrap.h"
#include "gbc/Cartridge.h"
#include "gbc/Device.h"
#include "gbc/Memory.h"

namespace GBC {

Memory::Memory(Device& dev)
   : raw{}, cart(nullptr), device(dev) {
   // If no cartridge is available, all cartridge reads return 0xFF
   romb.fill(0xFF);
   roms.fill(0xFF);
   eram.fill(0xFF);
}

const uint8_t* Memory::getAddr(uint16_t address) const {
   if (boot == Boot::kBooting && address <= 0x00FF) {
      return &kBootstrap[address];
   }

   if (cart && address < 0x8000) {
      return cart->get(address);
   }

   if (cart && address >= 0xA000 && address < 0xC000) {
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

   if (cart && address >= 0xA000 && address < 0xC000) {
      cart->set(address, val);
      return;
   }

   if (address >= 0xE000 && address < 0xFE00) {
      // Mirror of working ram
      address -= 0x2000;
   }

   if (address == 0xFF04) {
      // Divider register
      device.onDivWrite();
   }

   if (address == 0xFF05) {
      // TIMA
      device.onTimaWrite();
   }

   if (address == 0xFF0F) {
      // IF
      device.onIfWrite();
   }

   raw[address] = val;

   if (address == 0xFF46) {
      executeDMATransfer();
   }
}

void Memory::executeDMATransfer() {
   ASSERT(dma <= 0xF1);

   // Copy data into the sprite attribute table
   uint16_t source = dma << 8;
   memcpy(sat.data(), &raw[source], 0x009F);
}

} // namespace GBC
