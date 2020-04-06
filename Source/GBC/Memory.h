#pragma once

#include "UI/UIFriend.h"

#include <array>
#include <cstdint>

namespace GBC
{

class Cartridge;
class GameBoy;

namespace Interrupt
{

enum Enum : uint8_t
{
   VBlank = 1 << 0,
   LCDState = 1 << 1,
   Timer = 1 << 2,
   Serial = 1 << 3,
   Joypad = 1 << 4,
};

} // namespace Interrupt

enum class Boot : uint8_t
{
   Booting = 0,
   NotBooting = 1
};

class Memory
{
public:
   static const uint8_t kInvalidAddressByte = 0xFF;

   Memory(GameBoy& gb);

   uint8_t readDirect(uint16_t address) const;
   void writeDirect(uint16_t address, uint8_t value);

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void setCartridge(Cartridge* cartridge)
   {
      cart = cartridge;
   }

   std::array<uint8_t, 0x1000> ram0 = {}; // Working RAM bank 0 (0xC000-0xCFFF)
   std::array<uint8_t, 0x1000> ram1 = {}; // Working RAM bank 1 (0xD000-0xDFFF)
   std::array<uint8_t, 0x007F> ramh = {}; // High RAM area (0xFF80-0xFFFE)

   uint8_t p1 = 0x00; // Joy pad / system info (0xFF00)
   uint8_t sb = 0x00; // Serial transfer data (0xFF01)
   uint8_t sc = 0x00; // Serial IO control (0xFF02)
   uint8_t div = 0x00; // Divider register (0xFF04)
   uint8_t tima = 0x00; // Timer counter (0xFF05)
   uint8_t tma = 0x00; // Timer modulo (0xFF06)
   uint8_t tac = 0x00; // Timer control (0xFF07)
   uint8_t ifr = 0x00; // Interrupt flag (0xFF0F)

   uint8_t boot = 0x00; // Executing bootstrap (0xFF50)
   uint8_t ie = 0x00; // Interrupt enable register (0xFFFF)

private:
   DECLARE_UI_FRIEND

   uint8_t readIO(uint16_t address) const;
   void writeIO(uint16_t address, uint8_t value);

   GameBoy& gameBoy;
   Cartridge* cart = nullptr;
};

} // namespace GBC
