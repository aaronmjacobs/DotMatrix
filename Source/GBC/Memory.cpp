#include "Core/Assert.h"

#include "GBC/Bootstrap.h"
#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"
#include "GBC/Memory.h"

#include <cstring>

namespace GBC
{

// static
const uint8_t Memory::kInvalidAddressByte;

Memory::Memory(GameBoy& gb)
   : raw{}
   , cart(nullptr)
   , gameBoy(gb)
   , dmaRequested(false)
   , dmaInProgress(false)
   , dmaIndex(0x00)
   , dmaSource(0x0000)
{
   // If no cartridge is available, all cartridge reads return 0xFF
   romb.fill(0xFF);
   roms.fill(0xFF);
   eram.fill(0xFF);
}

void Memory::machineCycle()
{
   if (dmaInProgress)
   {
      if (dmaIndex <= 0x9F)
      {
         sat[dmaIndex] = raw[dmaSource + dmaIndex];
         ++dmaIndex;
      }
      else
      {
         dmaInProgress = false;
         dmaIndex = 0x00;
      }
   }

   if (dmaRequested)
   {
      dmaRequested = false;
      dmaInProgress = true;
      dmaIndex = 0x00;

      ASSERT(dma <= 0xF1);
      dmaSource = dma << 8;
   }
}

uint8_t Memory::read(uint16_t address) const
{
   gameBoy.machineCycle();

   uint8_t value = kInvalidAddressByte;

   if (boot == Boot::kBooting && address <= 0x00FF)
   {
      value = kBootstrap[address];
   }
   else if (cart && address < 0x8000)
   {
      value = cart->read(address);
   }
   else if (cart && address >= 0xA000 && address < 0xC000)
   {
      value = cart->read(address);
   }
   else if (address >= 0xFF10 && address < 0xFF40)
   {
      // Sound registers
      value = gameBoy.getSoundController().read(address);
   }
   else if (address >= 0xFE00 && address < 0xFF00 && !isSpriteAttributeTableAccessible())
   {
      value = kInvalidAddressByte;
   }
   else
   {
      if (address >= 0xE000 && address < 0xFE00)
      {
         // Mirror of working ram
         address -= 0x2000;
      }

      value = raw[address];

      if (address == 0xFF0F)
      {
         // IF reads are masked (upper 3 bits are unused)
         value |= 0xE0;
      }
   }

   return value;
}

void Memory::write(uint16_t address, uint8_t value)
{
   gameBoy.machineCycle();

   if (boot == Boot::kBooting && address <= 0x00FF)
   {
      // Bootstrap is read only
   }
   else if (cart && address < 0x8000)
   {
      cart->write(address, value);
   }
   else if (cart && address >= 0xA000 && address < 0xC000)
   {
      cart->write(address, value);
   }
   else if (address >= 0xFF10 && address < 0xFF40)
   {
      // Sound registers
      gameBoy.getSoundController().write(address, value);
   }
   else if (address >= 0xFE00 && address < 0xFF00 && !isSpriteAttributeTableAccessible())
   {
      // Can't write during OAM
   }
   else
   {
      if (address >= 0xE000 && address < 0xFE00)
      {
         // Mirror of working ram
         address -= 0x2000;
      }

      if (address == 0xFF04)
      {
         // Divider register
         gameBoy.onDivWrite();
      }

      if (address == 0xFF05)
      {
         // TIMA
         gameBoy.onTimaWrite();

         // If TIMA was reloaded with TMA this machine cycle, the write is ignored
         if (gameBoy.wasTimaReloadedWithTma())
         {
            return;
         }
      }

      if (address == 0xFF06 && gameBoy.wasTimaReloadedWithTma())
      {
         // TMA

         // If TIMA was reloaded with TMA this machine cycle, then the value written to TMA gets propagated to TIMA as well
         tima = value;
      }

      if (address == 0xFF0F)
      {
         // IF
         gameBoy.onIfWrite();
      }

      if (address == 0xFF41)
      {
         // LCD status - mode flag should be unaffected by memory writes
         static const uint8_t kModeFlagMask = 0b00000011;
         value = (value & ~kModeFlagMask) | (stat & kModeFlagMask);
      }

      raw[address] = value;

      if (address == 0xFF46)
      {
         dmaRequested = true;
      }
   }
}

} // namespace GBC
