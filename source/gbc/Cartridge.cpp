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

Cartridge::Header parseHeader(const std::vector<uint8_t>& data) {
   STATIC_ASSERT(sizeof(Cartridge::Header) == kHeaderSize);
   ASSERT(data.size() >= kHeaderOffset + kHeaderSize);

   Cartridge::Header header;
   memcpy(&header, &data.data()[kHeaderOffset], kHeaderSize);

   return header;
}

bool performHeaderChecksum(const Cartridge::Header& header, const std::vector<uint8_t>& data) {
   // Complement check, program will not run if incorrect
   // x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT
   const uint8_t* mem = data.data();

   uint8_t x = 0;
   for (uint16_t i = 0x0134; i <= 0x014C; ++i) {
      x = x - mem[i] - 1;
   }

   return x == header.headerChecksum;
}

bool performGlobalChecksum(const Cartridge::Header& header, const std::vector<uint8_t>& data) {
   // Checksum (higher byte first) produced by adding all bytes of a cartridge except for two checksum bytes and taking
   // two lower bytes of the result. (GameBoy ignores this value.)
   const uint8_t* mem = data.data();

   uint16_t x = 0;
   for (size_t i = 0; i < data.size(); ++i) {
      x += mem[i];
   }

   x -= header.globalChecksum[0];
   x -= header.globalChecksum[1];

   uint8_t low = x & 0x00FF;
   uint8_t high = (x & 0xFF00) >> 8;

   return header.globalChecksum[0] == high && header.globalChecksum[1] == low;
}

bool cartHasRAM(Cartridge::CartridgeType type) {
   switch (type) {
      case Cartridge::kMBC1PlusRAM:
      case Cartridge::kMBC1PlusRAMPlusBattery:
      case Cartridge::kROMPlusRAM:
      case Cartridge::kROMPlusRAMPlusBattery:
      case Cartridge::kMMM01PlusRAM:
      case Cartridge::kMMM01PlusRAMPlusBattery:
      case Cartridge::kMBC3PlusTimerPlusRAMPlusBattery:
      case Cartridge::kMBC3PlusRAM:
      case Cartridge::kMBC3PlusRAMPlusBattery:
      case Cartridge::kMBC4PlusRAM:
      case Cartridge::kMBC4PlusRAMPlusBattery:
      case Cartridge::kMBC5PlusRAM:
      case Cartridge::kMBC5PlusRAMPlusBattery:
      case Cartridge::kMBC5PlusRumblePlusRAM:
      case Cartridge::kMBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::kMBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      case Cartridge::kHuC1PlusRAMPlusBattery:
         return true;
      default:
         return false;
   }
}

bool cartHasBattery(Cartridge::CartridgeType type) {
   switch (type) {
      case Cartridge::kMBC1PlusRAMPlusBattery:
      case Cartridge::kMBC2PlusBattery:
      case Cartridge::kROMPlusRAMPlusBattery:
      case Cartridge::kMMM01PlusRAMPlusBattery:
      case Cartridge::kMBC3PlusTimerPlusBattery:
      case Cartridge::kMBC3PlusTimerPlusRAMPlusBattery:
      case Cartridge::kMBC3PlusRAMPlusBattery:
      case Cartridge::kMBC4PlusRAMPlusBattery:
      case Cartridge::kMBC5PlusRAMPlusBattery:
      case Cartridge::kMBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::kMBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      case Cartridge::kHuC1PlusRAMPlusBattery:
         return true;
      default:
         return false;
   }
}

bool cartHasTimer(Cartridge::CartridgeType type) {
   switch (type) {
      case Cartridge::kMBC3PlusTimerPlusBattery:
      case Cartridge::kMBC3PlusTimerPlusRAMPlusBattery:
         return true;
      default:
         return false;
   }
}

bool cartHasRumble(Cartridge::CartridgeType type) {
   switch (type) {
      case Cartridge::kMBC5PlusRumble:
      case Cartridge::kMBC5PlusRumblePlusRAM:
      case Cartridge::kMBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::kMBC7PlusSensorPlusRumblePlusRAMPlusBattery:
         return true;
      default:
         return false;
   }
}

} // namespace

class NoneController : public MemoryBankController {
public:
   NoneController(const Cartridge& cartridge)
      : MemoryBankController(cartridge) {
   }

   const uint8_t* get(uint16_t address) const override {
      ASSERT(address < cart.data().size());

      if (address < 0x8000) {
         return &cart.data()[address];
      }

      LOG_WARNING("Trying to read invalid cartridge location: " << hex(address));
      return nullptr;
   }

   void set(uint16_t address, uint8_t val) override {
      ASSERT(address < cart.data().size());

      // No writable memory
      LOG_WARNING("Trying to write to read-only cartridge at location " << hex(address) << ": " << hex(val));
   }
};

class MBC1 : public MemoryBankController {
public:
   MBC1(const Cartridge& cartridge)
      : MemoryBankController(cartridge), ramEnabled(false), romBankNumber(0x01), ramBankNumber(0x00),
        bankingMode(kROMBankingMode), ramBanks({}) {
   }

   const uint8_t* get(uint16_t address) const override {
      ASSERT(address < cart.data().size());

      const uint8_t* pointer = nullptr;
      switch (address & 0xF000) {
         case 0x0000:
         case 0x1000:
         case 0x2000:
         case 0x3000:
         {
            // Always contains the first 16Bytes of the ROM
            pointer = &cart.data()[address];
            break;
         }
         case 0x4000:
         case 0x5000:
         case 0x6000:
         case 0x7000:
         {
            // Switchable ROM bank
            ASSERT(romBankNumber > 0);
            pointer = &cart.data()[address + ((romBankNumber - 1) * 0x4000)];
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to read from MBC1 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled) {
               uint8_t ramBank = (bankingMode == kRAMBankingMode) ? ramBankNumber : 0x00;
               pointer = &ramBanks[ramBank][address - 0xA000];
            } else {
               LOG_WARNING("Trying to read from RAM when not enabled");
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to read invalid cartridge location: " << hex(address));
            break;
         }
      }

      return pointer;
   }

   void set(uint16_t address, uint8_t val) override {
      ASSERT(address < cart.data().size());

      switch (address & 0xF000) {
         case 0x0000:
         case 0x1000:
         {
            // RAM enable
            ramEnabled = (val & 0x0A) != 0x00;
            break;
         }
         case 0x2000:
         case 0x3000:
         {
            // ROM bank number
            romBankNumber = val & 0x1F;
            if (romBankNumber == 0x00 || romBankNumber == 0x20 || romBankNumber == 0x40 || romBankNumber == 0x60) {
               // Handle banks 0x00, 0x20, 0x40, 0x60
               romBankNumber += 0x01;
            }
            break;
         }
         case 0x4000:
         case 0x5000:
         {
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
            break;
         }
         case 0x6000:
         case 0x7000:
         {
            // ROM / RAM mode select
            bankingMode = (val & 0x01) == 0x00 ? kROMBankingMode : kRAMBankingMode;
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to write to MBC1 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled) {
               uint8_t ramBank = (bankingMode == kRAMBankingMode) ? ramBankNumber : 0x00;
               ramBanks[ramBank][address - 0xA000] = val;
            } else {
               LOG_WARNING("Trying to write to disabled RAM " << hex(address) << ": " << hex(val));
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to write to read-only cartridge location " << hex(address) << ": " << hex(val));
            break;
         }
      };
   }

private:
   enum BankingMode : uint8_t {
      kROMBankingMode = 0x00,
      kRAMBankingMode = 0x01
   };

   bool ramEnabled;
   uint8_t romBankNumber;
   uint8_t ramBankNumber;
   BankingMode bankingMode;

   std::array<std::array<uint8_t, 0x2000>, 4> ramBanks;
};

// static
UPtr<Cartridge> Cartridge::fromData(std::vector<uint8_t>&& data) {
   if (data.size() < kHeaderOffset + kHeaderSize) {
      LOG_ERROR("Cartridge provided insufficient data");
      return nullptr;
   }

   Header header = parseHeader(data);
   if (!performHeaderChecksum(header, data)) {
      LOG_ERROR("Cartridge data failed header checksum");
      return nullptr;
   }
   if (!performGlobalChecksum(header, data)) {
      LOG_WARNING("Cartridge data failed global checksum");
   }

   UPtr<Cartridge> cart(new Cartridge(std::move(data), header));

   UPtr<MemoryBankController> mbc;
   switch (header.type) {
      case kROMOnly:
         LOG_INFO("ROM only");
         mbc = std::make_unique<NoneController>(*cart);
         break;
      case kMBC1:
      case kMBC1PlusRAM:
      case kMBC1PlusRAMPlusBattery:
         LOG_INFO("MBC1");
         mbc = std::make_unique<MBC1>(*cart);
         break;
      default:
         LOG_ERROR("Invalid cartridge type: " << hex(static_cast<uint8_t>(header.type)));
         return nullptr;
   }

   cart->setController(std::move(mbc));
   return cart;
}

Cartridge::Cartridge(std::vector<uint8_t>&& data, const Header& headerData)
   : cartData(std::move(data)), header(headerData), cartTitle({}), ram(cartHasRAM(header.type)),
     battery(cartHasBattery(header.type)), timer(cartHasTimer(header.type)), rumble(cartHasRumble(header.type)),
     controller(nullptr) {
   // Copy title into separate array to ensure there is a null terminator
   memcpy(cartTitle.data(), header.title.data(), header.title.size());
}

} // namespace GBC
