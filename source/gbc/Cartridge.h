#ifndef GBC_CARTRIDGE_H
#define GBC_CARTRIDGE_H

#include "Pointers.h"

#include <array>
#include <cstdint>

namespace GBC {

class Cartridge {
public:
   static UPtr<Cartridge> fromData(UPtr<uint8_t[]>&& data, size_t numBytes);

   virtual ~Cartridge() = default;

   const char* getTitle() const {
      return title.data();
   }

   virtual void tick(uint64_t cycles) = 0;

   virtual void load(class Memory& memory) = 0;

protected:
   Cartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes);

   enum Type : uint8_t {
      kROMOnly = 0x00,

      kMBC1 = 0x01,
      kMBC1PlusRAM = 0x02,
      kMBC1PlusRAMPlusBattery = 0x03,

      kMBC2 = 0x05,
      kMBC2PlusBattery = 0x06,

      kROMPlusRAM = 0x08,
      kROMPlusRAMPlusBattery = 0x09,

      kMMM01 = 0x0B,
      kMMM01PlusRAM = 0x0C,
      kMMM01PlusRAMPlusBattery = 0x0D,

      kMBC3PlusTimerPlusBattery = 0x0F,
      kMBC3PlusTimerPlusRAMPlusBattery = 0x10,
      kMBC3 = 0x11,
      kMBC3PlusRAM = 0x12,
      kMBC3PlusRAMPlusBattery = 0x13,

      kMBC4 = 0x15,
      kMBC4PlusRAM = 0x16,
      kMBC4PlusRAMPlusBattery = 0x17,

      kMBC5 = 0x19,
      kMBC5PlusRAM = 0x1A,
      kMBC5PlusRAMPlusBattery = 0x1B,
      kMBC5PlusRumble = 0x1C,
      kMBC5PlusRumblePlusRAM = 0x1D,
      kMBC5PlusRumblePlusRAMPlusBattery = 0x1E,

      kMBC6 = 0x20,

      kMBC7PlusSensorPlusRumblePlusRAMPlusBattery = 0x22,

      kPocketCamera = 0xFC,
      kBandaiTAMA5 = 0xFD,
      kHuC3 = 0xFE,
      kHuC1PlusRAMPlusBattery = 0xFF
   };

   enum ROMSize : uint8_t {
      kROM32KBytes = 0x00,       // no ROM banking
      kROM64KBytes = 0x01,       // 4 banks
      kROM128KBytes = 0x02,      // 8 banks
      kROM256KBytes = 0x03,      // 16 banks
      kROM512KBytes = 0x04,      // 32 banks
      kROM1MBytes = 0x05,        // 64 banks - only 63 banks used by MBC1
      kROM2MBytes = 0x06,        // 128 banks - only 125 banks used by MBC1
      kROM4MBytes = 0x07,        // 256 banks
      kROM1Point1MBytes = 0x52,  // 72 banks
      kROM1Point2MBytes = 0x53,  // 80 banks
      kROM1Point5MBytes = 0x54   // 96 banks
   };

   enum RAMSize : uint8_t {
      kRAMNone = 0x00,
      kRAM2KBytes = 0x01,
      kRAM8KBytes = 0x02,
      kRAM32KBytes = 0x03,       // 4 banks of 8 KBytes each
      kRAM128KBytes = 0x04,      // 16 banks of 8 KBytes each
      kRAM64KBytes = 0x05        // 8 banks of 8 KBytes each
   };

   enum DestinationCode : uint8_t {
      kDestJapanese = 0x00,
      kDestNonJapanese = 0x01
   };

   UPtr<uint8_t[]> data;
   size_t numBytes;
   std::array<char, 16> title;
};

} // namespace GBC

#endif