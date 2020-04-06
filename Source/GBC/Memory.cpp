#include "Core/Assert.h"
#include "Core/Enum.h"

#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"
#include "GBC/Memory.h"

#include <cstring>

namespace GBC
{

namespace
{

const std::array<uint8_t, 256> kBootstrap =
{
   0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32,
   0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
   0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3,
   0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
   0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A,
   0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
   0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06,
   0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
   0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99,
   0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
   0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64,
   0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
   0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90,
   0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
   0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62,
   0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
   0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42,
   0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
   0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04,
   0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
   0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9,
   0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
   0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
   0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
   0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
   0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
   0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
   0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
   0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13,
   0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
   0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20,
   0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50,
};

} // namespace

// static
const uint8_t Memory::kInvalidAddressByte;

Memory::Memory(GameBoy& gb)
   : gameBoy(gb)
{
}

uint8_t Memory::readDirect(uint16_t address) const
{
   uint8_t value = kInvalidAddressByte;

   switch (address & 0xF000)
   {
   // Permanently-mapped ROM bank
   case 0x0000:
      if (address <= 0x00FF && boot == Enum::cast(Boot::Booting))
      {
         value = kBootstrap[address];
         break;
      }
   case 0x1000:
   case 0x2000:
   case 0x3000:
   // Switchable ROM bank
   case 0x4000:
   case 0x5000:
   case 0x6000:
   case 0x7000:
      if (cart)
      {
         value = cart->read(address);
      }
      break;
   // Video RAM
   case 0x8000:
   case 0x9000:
      value = gameBoy.getLCDController().read(address);
      break;
   // Switchable external RAM bank
   case 0xA000:
   case 0xB000:
      if (cart)
      {
         value = cart->read(address);
      }
      break;
   // Working RAM bank 0
   case 0xC000:
      value = ram0[address - 0xC000];
      break;
   // Working RAM bank 1
   case 0xD000:
      value = ram1[address - 0xD000];
      break;
   // Mirror of working ram
   case 0xE000:
      value = ram0[address - 0xE000];
      break;
   case 0xF000:
      switch (address & 0x0F00)
      {
      // Mirror of working ram
      default:
         value = ram1[address - 0xF000];
         break;
      // Sprite attribute table
      case 0x0E00:
         value = gameBoy.getLCDController().read(address);
         break;
      // I/O device mappings
      case 0x0F00:
         value = readIO(address);
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }

   return value;
}

void Memory::writeDirect(uint16_t address, uint8_t value)
{
   switch (address & 0xF000)
   {
   // Permanently-mapped ROM bank
   case 0x0000:
      if (address <= 0x00FF && boot == Enum::cast(Boot::Booting))
      {
         // Bootstrap is read only
         break;
      }
   case 0x1000:
   case 0x2000:
   case 0x3000:
   // Switchable ROM bank
   case 0x4000:
   case 0x5000:
   case 0x6000:
   case 0x7000:
      if (cart)
      {
         cart->write(address, value);
      }
      break;
   // Video RAM
   case 0x8000:
   case 0x9000:
      gameBoy.getLCDController().write(address, value);
      break;
   // Switchable external RAM bank
   case 0xA000:
   case 0xB000:
      if (cart)
      {
         cart->write(address, value);
      }
      break;
   // Working RAM bank 0
   case 0xC000:
      ram0[address - 0xC000] = value;
      break;
   // Working RAM bank 1
   case 0xD000:
      ram1[address - 0xD000] = value;
      break;
   // Mirror of working ram
   case 0xE000:
      ram0[address - 0xE000] = value;
      break;
   case 0xF000:
      switch (address & 0x0F00)
      {
      // Mirror of working ram
      default:
         ram1[address - 0xF000] = value;
         break;
      // Sprite attribute table
      case 0x0E00:
         gameBoy.getLCDController().write(address, value);
         break;
      // I/O device mappings
      case 0x0F00:
         writeIO(address, value);
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }
}

uint8_t Memory::read(uint16_t address) const
{
   gameBoy.machineCycle();

   return readDirect(address);
}

void Memory::write(uint16_t address, uint8_t value)
{
   gameBoy.machineCycle();

   writeDirect(address, value);
}

uint8_t Memory::readIO(uint16_t address) const
{
   ASSERT(address >= 0xFF00);

   uint8_t value = kInvalidAddressByte;

   switch (address & 0x00F0)
   {
   // General
   case 0x0000:
      switch (address)
      {
      case 0xFF00:
         value = p1 | 0xC0;
         break;
      case 0xFF01:
         value = sb;
         break;
      case 0xFF02:
         value = sc | 0x7E;
         break;
      case 0xFF04:
         value = div;
         break;
      case 0xFF05:
         value = tima;
         break;
      case 0xFF06:
         value = tma;
         break;
      case 0xFF07:
         value = tac | 0xF8;
         break;
      case 0xFF0F:
         value = ifr | 0xE0;
         break;
      default:
         break;
      }
      break;
   // Sound
   case 0x0010:
   case 0x0020:
   case 0x0030:
      value = gameBoy.getSoundController().read(address);
      break;
   // LCD
   case 0x0040:
      value = gameBoy.getLCDController().read(address);
      break;
   // Bootstrap
   case 0x0050:
   case 0x0060:
   case 0x0070:
      break;
   // High RAM area
   case 0x0080:
   case 0x0090:
   case 0x00A0:
   case 0x00B0:
   case 0x00C0:
   case 0x00D0:
   case 0x00E0:
      value = ramh[address - 0xFF80];
      break;
   case 0x00F0:
      switch (address & 0x000F)
      {
      default:
         value = ramh[address - 0xFF80];
         break;
      // Interrupt enable register
      case 0x000F:
         value = ie;
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }

   return value;
}

void Memory::writeIO(uint16_t address, uint8_t value)
{
   ASSERT(address >= 0xFF00);

   switch (address & 0x00F0)
   {
   // General
   case 0x0000:
      switch (address)
      {
      case 0xFF00:
         p1 = value & 0x3F;
         break;
      case 0xFF01:
         sb = value;
         break;
      case 0xFF02:
         sc = value & 0x81;
         break;
      case 0xFF04:
         div = value;
         gameBoy.onDivWrite();
         break;
      case 0xFF05:
         // If TIMA was reloaded with TMA this machine cycle, the write is ignored
         if (!gameBoy.wasTimaReloadedWithTma())
         {
            tima = value;
         }

         gameBoy.onTimaWrite();
         break;
      case 0xFF06:
         tma = value;

         if (gameBoy.wasTimaReloadedWithTma())
         {
            tima = value;
         }
         break;
      case 0xFF07:
         tac = value & 0x07;
         break;
      case 0xFF0F:
         ifr = value & 0x1F;
         gameBoy.onIfWrite();
         break;
      default:
         break;
      }
      break;
   // Sound
   case 0x0010:
   case 0x0020:
   case 0x0030:
      gameBoy.getSoundController().write(address, value);
      break;
   // LCD
   case 0x0040:
      gameBoy.getLCDController().write(address, value);
      break;
   // Bootstrap
   case 0x0050:
      switch (address)
      {
      case 0xFF50:
         if (value == 0x01)
         {
            boot = value;
         }
         break;
      default:
         break;
      }
      break;
   case 0x0060:
   case 0x0070:
      break;
   // High RAM area
   case 0x0080:
   case 0x0090:
   case 0x00A0:
   case 0x00B0:
   case 0x00C0:
   case 0x00D0:
   case 0x00E0:
      ramh[address - 0xFF80] = value;
      break;
   case 0x00F0:
      switch (address & 0x000F)
      {
      default:
         ramh[address - 0xFF80] = value;
         break;
      // Interrupt enable register
      case 0x000F:
         ie = value;
         break;
      }
      break;
   default:
      ASSERT(false);
      break;
   }
}

} // namespace GBC
