#include "FancyAssert.h"

#include "GBC/Bootstrap.h"
#include "GBC/Cartridge.h"
#include "GBC/Device.h"
#include "GBC/Memory.h"

#include <cstring>

namespace GBC {

// static
const uint8_t Memory::kInvalidAddressByte;

Memory::Memory(Device& dev)
   : raw{}, cart(nullptr), device(dev) {
   // If no cartridge is available, all cartridge reads return 0xFF
   romb.fill(0xFF);
   roms.fill(0xFF);
   eram.fill(0xFF);
}

uint8_t Memory::read(uint16_t address) const {
   if (boot == Boot::kBooting && address <= 0x00FF) {
      return kBootstrap[address];
   }

   if (cart && address < 0x8000) {
      return cart->read(address);
   }

   if (cart && address >= 0xA000 && address < 0xC000) {
      return cart->read(address);
   }

   if (address >= 0xFF10 && address < 0xFF40) {
      // Sound registers
      return device.getSoundController().read(address);
   }

   if (address >= 0xE000 && address < 0xFE00) {
      // Mirror of working ram
      address -= 0x2000;
   }

   return raw[address];
}

void Memory::write(uint16_t address, uint8_t value) {
   if (boot == Boot::kBooting && address <= 0x00FF) {
      return; // Bootstrap is read only
   }

   if (cart && address < 0x8000) {
      cart->write(address, value);
      return;
   }

   if (cart && address >= 0xA000 && address < 0xC000) {
      cart->write(address, value);
      return;
   }

   if (address >= 0xFF10 && address < 0xFF40) {
      // Sound registers
      device.getSoundController().write(address, value);
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

   if (address == 0xFF41) {
      // LCD status - mode flag should be unaffected by memory writes
      static const uint8_t kModeFlagMask = 0b00000011;
      value = (value & ~kModeFlagMask) | (stat & kModeFlagMask);
   }

   raw[address] = value;

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
