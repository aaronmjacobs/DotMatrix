#ifndef GBC_CARTRIDGE_H
#define GBC_CARTRIDGE_H

#include "FancyAssert.h"
#include "Pointers.h"

#include <array>
#include <cstdint>
#include <vector>

namespace GBC {

class MemoryBankController {
public:
   MemoryBankController(const class Cartridge& cartridge)
      : cart(cartridge) {
   }

   virtual ~MemoryBankController() = default;

   virtual const uint8_t* get(uint16_t address) const = 0;

   virtual void set(uint16_t address, uint8_t val) = 0;

protected:
   const class Cartridge& cart;
};

class Cartridge {
public:
   static UPtr<Cartridge> fromData(std::vector<uint8_t>&& data);

   const char* title() const {
      return cartTitle.data();
   }

   const std::vector<uint8_t>& data() const {
      return cartData;
   }

   const uint8_t* get(uint16_t address) const {
      ASSERT(controller);
      return controller->get(address);
   }

   void set(uint16_t address, uint8_t val) {
      ASSERT(controller);
      controller->set(address, val);
   }

   bool hasRAM() const {
      return ram;
   }

   bool hasBattery() const {
      return battery;
   }

   bool hasTimer() const {
      return timer;
   }

   bool hasRumble() const {
      return rumble;
   }

   enum CGBFlag : uint8_t {
      kCBGSupported = 0x80,
      kCGBRequired = 0xC0
   };

   enum SGBFlag : uint8_t {
      kNoSGBFunctions = 0x00,
      kSupportsSGBFunctions = 0x03
   };

   enum CartridgeType : uint8_t {
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

   struct Header {
      std::array<uint8_t, 4> entryPoint;
      std::array<uint8_t, 48> nintendoLogo;
      std::array<uint8_t, 11> title;
      std::array<uint8_t, 4> manufacturerCode;
      CGBFlag cgbFlag;
      std::array<uint8_t, 2> newLicenseeCode;
      SGBFlag sgbFlag;
      CartridgeType type;
      ROMSize romSize;
      RAMSize ramSize;
      DestinationCode destinationCode;
      uint8_t oldLicenseeCode;
      uint8_t maskRomVersionNumber;
      uint8_t headerChecksum;
      std::array<uint8_t, 2> globalChecksum;
   };

private:
   Cartridge(std::vector<uint8_t>&& data, const Header& headerData);

   void setController(UPtr<MemoryBankController> mbc) {
      controller = std::move(mbc);
   }

   std::vector<uint8_t> cartData;
   Header header;
   std::array<char, 12> cartTitle;

   bool ram;
   bool battery;
   bool timer;
   bool rumble;

   UPtr<MemoryBankController> controller;
};

} // namespace GBC

#endif
