#include "FancyAssert.h"
#include "Log.h"

#include "gbc/Cartridge.h"
#include "gbc/Memory.h"

#if !defined(NDEBUG)
#  include "Debug.h"
#endif // !defined(NDEBUG)

#include <cstring>
#include <memory>

namespace GBC {

namespace {

static const uint16_t kHeaderOffset = 0x0100;
static const uint16_t kHeaderSize = 0x0050;

Cartridge::Header parseHeader(const uint8_t* data, size_t numBytes) {
   STATIC_ASSERT(sizeof(Cartridge::Header) == kHeaderSize);
   ASSERT(numBytes >= kHeaderOffset + kHeaderSize);

   Cartridge::Header header;
   memcpy(&header, data + kHeaderOffset, kHeaderSize);

   return header;
}

bool performHeaderChecksum(const Cartridge::Header& header, const uint8_t* mem) {
   // Complement check, program will not run if incorrect
   // x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT

   uint8_t x = 0;
   for (uint16_t i = 0x0134; i <= 0x014C; ++i) {
      x = x - mem[i] - 1;
   }

   return x == header.headerChecksum;
}

bool performGlobalChecksum(const Cartridge::Header& header, const uint8_t* mem, size_t numBytes) {
   // Checksum (higher byte first) produced by adding all bytes of a cartridge except for two checksum bytes and taking
   // two lower bytes of the result. (GameBoy ignores this value.)

   uint16_t x = 0;
   for (size_t i = 0; i < numBytes; ++i) {
      x += mem[i];
   }

   x -= header.globalChecksum[0];
   x -= header.globalChecksum[1];

   uint8_t low = x & 0x00FF;
   uint8_t high = (x & 0xFF00) >> 8;

   return header.globalChecksum[0] == high && header.globalChecksum[1] == low;
}

} // namespace

class SimpleCartridge : public Cartridge {
public:
   const uint8_t* get(uint16_t address) const override {
      ASSERT(address < numBytes);

      if (address < 0x8000) {
         return &data[address];
      }

      LOG_WARNING("Trying to read invalid cartridge location: " << hex(address));
      return nullptr;
   }

   void set(uint16_t address, uint8_t val) override {
      ASSERT(address < numBytes);

      // No writable memory
      LOG_WARNING("Trying to write to read-only cartridge at location " << hex(address) << ": " << hex(val));
   }

private:
   friend class Cartridge;
   SimpleCartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
      : Cartridge(std::move(cartData), cartNumBytes) {
   }
};

class MBC1Cartridge : public Cartridge {
public:
   const uint8_t* get(uint16_t address) const override {
      ASSERT(address < numBytes);

      if (address < 0x4000) {
         // Always contains the first 16Bytes of the ROM
         return &data[address];
      }

      if (address < 0x8000) {
         // Switchable ROM bank
         return &data[(address - 0x4000) + (romBankNumber * 0x4000)];
      }

      if (address >= 0xA000 && address < 0xC000) {
         // Switchable RAM bank
         if (ramEnabled) {
            uint8_t ramBank = (bankingMode == kRAMBankingMode) ? ramBankNumber : 0x00;
            return &ramBanks[ramBank][address - 0xA000];
         } else {
            LOG_WARNING("Trying to read from RAM when not enabled");
            return nullptr;
         }
      }

      LOG_WARNING("Trying to read invalid cartridge location: " << hex(address));
      return nullptr;
   }

   void set(uint16_t address, uint8_t val) override {
      ASSERT(address < numBytes);

      if (address < 0x2000) {
         // RAM enable
         ramEnabled = (val & 0x0A) != 0x00;
         return;
      }

      if (address < 0x4000) {
         // ROM bank number
         romBankNumber = val & 0x1F;
         if (romBankNumber == 0x00 || romBankNumber == 0x20 || romBankNumber == 0x40 || romBankNumber == 0x60) {
            // Handle banks 0x00, 0x20, 0x40, 0x60
            romBankNumber += 0x01;
         }
         return;
      }

      if (address < 0x6000) {
         // RAM bank number or upper bits of ROM bank number
         uint8_t bankNumber = val & 0x03;
         switch (bankingMode) {
            case kROMBankingMode:
               romBankNumber = (romBankNumber & 0x1F) | (bankNumber << 5);
               break;
            case kRAMBankingMode:
               ramBankNumber = bankNumber;
               break;
            default:
               ASSERT(false, "Invalid banking mode: %hhu", bankingMode);
         }
         return;
      }

      if (address < 0x8000) {
         // ROM / RAM mode select
         bankingMode = (val & 0x01) == 0x00 ? kROMBankingMode : kRAMBankingMode;
         return;
      }

      if (address >= 0xA000 && address < 0xC000) {
         // Switchable RAM bank
         if (ramEnabled) {
            uint8_t ramBank = (bankingMode == kRAMBankingMode) ? ramBankNumber : 0x00;
            ramBanks[ramBank][address - 0xA000] = val;
         }
         return;
      }

      LOG_WARNING("Trying to write to read-only cartridge location " << hex(address) << ": " << hex(val));
   }

private:
   enum BankingMode : uint8_t {
      kROMBankingMode = 0x00,
      kRAMBankingMode = 0x01
   };

   friend class Cartridge;
   MBC1Cartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
      : Cartridge(std::move(cartData), cartNumBytes), ramEnabled(false), romBankNumber(0x00),
        bankingMode(kROMBankingMode), ramBanks({}) {
   }

   bool ramEnabled;
   uint8_t romBankNumber;
   uint8_t ramBankNumber;
   BankingMode bankingMode;

   std::array<std::array<uint8_t, 0x2000>, 4> ramBanks;
};

// static
UPtr<Cartridge> Cartridge::fromData(UPtr<uint8_t[]>&& data, size_t numBytes) {
   if (!data || numBytes < kHeaderOffset + kHeaderSize) {
      LOG_ERROR("Cartridge provided insufficient data");
      return nullptr;
   }

   Header header = parseHeader(data.get(), numBytes);
   if (!performHeaderChecksum(header, data.get())) {
      LOG_ERROR("Cartridge data failed header checksum");
      return nullptr;
   }
   if (!performGlobalChecksum(header, data.get(), numBytes)) {
      LOG_WARNING("Cartridge data failed global checksum");
   }

   switch (header.cartridgeType) {
      case kROMOnly:
         return UPtr<Cartridge>(new SimpleCartridge(std::move(data), numBytes));
      case kMBC1:
         return UPtr<Cartridge>(new MBC1Cartridge(std::move(data), numBytes));
      default:
         LOG_ERROR("Invalid cartridge type: " << hex(header.cartridgeType));
         return nullptr;
   }
}

Cartridge::Cartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
   : data(std::move(cartData)), numBytes(cartNumBytes), header(parseHeader(data.get(), numBytes)), title({}) {
   ASSERT(data);

   // Copy title into separate array to ensure there is a null terminator
   memcpy(title.data(), header.title.data(), header.title.size());
}

} // namespace GBC
