#pragma once

#include "Core/Archive.h"
#include "Core/Assert.h"
#include "Core/Pointers.h"

#include "GBC/GameBoy.h"
#include "GBC/MemoryBankController.h"

#include "UI/UIFriend.h"

#include <array>
#include <cstdint>
#include <vector>

namespace GBC
{

class Cartridge
{
public:
   static UPtr<Cartridge> fromData(std::vector<uint8_t>&& data, std::string& error);

   const char* title() const
   {
      return cartTitle.data();
   }

   uint8_t data(size_t address) const
   {
      if (address >= cartData.size())
      {
         return GameBoy::kInvalidAddressByte;
      }

      return cartData[address];
   }

   uint8_t read(uint16_t address) const
   {
      ASSERT(controller);
      return controller->read(address);
   }

   void write(uint16_t address, uint8_t value)
   {
      ASSERT(controller);
      controller->write(address, value);
   }

   void tick(double dt)
   {
      ASSERT(controller);
      controller->tick(dt);
   }

   Archive saveRAM() const
   {
      ASSERT(controller);
      return controller->saveRAM();
   }

   bool loadRAM(Archive& ramData)
   {
      ASSERT(controller);
      return controller->loadRAM(ramData);
   }

   bool wroteToRamThisFrame() const
   {
      ASSERT(controller);
      return controller->wroteToRamThisFrame();
   }

   bool hasRAM() const
   {
      return ramPresent;
   }

   bool hasBattery() const
   {
      return batteryPresent;
   }

   bool hasTimer() const
   {
      return timerPresent;
   }

   bool hasRumble() const
   {
      return rumblePresent;
   }

   enum class CGBFlag : uint8_t
   {
      Ignored = 0x00,
      Supported = 0x80,
      Required = 0xC0
   };

   enum class SGBFlag : uint8_t
   {
      Ignored = 0x00,
      Supported = 0x03
   };

   enum class Type : uint8_t
   {
      ROM = 0x00,

      MBC1 = 0x01,
      MBC1PlusRAM = 0x02,
      MBC1PlusRAMPlusBattery = 0x03,

      MBC2 = 0x05,
      MBC2PlusBattery = 0x06,

      ROMPlusRAM = 0x08,
      ROMPlusRAMPlusBattery = 0x09,

      MMM01 = 0x0B,
      MMM01PlusRAM = 0x0C,
      MMM01PlusRAMPlusBattery = 0x0D,

      MBC3PlusTimerPlusBattery = 0x0F,
      MBC3PlusTimerPlusRAMPlusBattery = 0x10,
      MBC3 = 0x11,
      MBC3PlusRAM = 0x12,
      MBC3PlusRAMPlusBattery = 0x13,

      MBC4 = 0x15,
      MBC4PlusRAM = 0x16,
      MBC4PlusRAMPlusBattery = 0x17,

      MBC5 = 0x19,
      MBC5PlusRAM = 0x1A,
      MBC5PlusRAMPlusBattery = 0x1B,
      MBC5PlusRumble = 0x1C,
      MBC5PlusRumblePlusRAM = 0x1D,
      MBC5PlusRumblePlusRAMPlusBattery = 0x1E,

      MBC6 = 0x20,

      MBC7PlusSensorPlusRumblePlusRAMPlusBattery = 0x22,

      PocketCamera = 0xFC,
      BandaiTAMA5 = 0xFD,
      HuC3 = 0xFE,
      HuC1PlusRAMPlusBattery = 0xFF
   };

   enum class ROMSize : uint8_t
   {
      Size32KBytes = 0x00,       // no ROM banking
      Size64KBytes = 0x01,       // 4 banks
      Size128KBytes = 0x02,      // 8 banks
      Size256KBytes = 0x03,      // 16 banks
      Size512KBytes = 0x04,      // 32 banks
      Size1MBytes = 0x05,        // 64 banks - only 63 banks used by MBC1
      Size2MBytes = 0x06,        // 128 banks - only 125 banks used by MBC1
      Size4MBytes = 0x07,        // 256 banks
      Size1Point1MBytes = 0x52,  // 72 banks
      Size1Point2MBytes = 0x53,  // 80 banks
      Size1Point5MBytes = 0x54   // 96 banks
   };

   enum class RAMSize : uint8_t
   {
      None = 0x00,
      Size2KBytes = 0x01,
      Size8KBytes = 0x02,
      Size32KBytes = 0x03,       // 4 banks of 8 KBytes each
      Size128KBytes = 0x04,      // 16 banks of 8 KBytes each
      Size64KBytes = 0x05        // 8 banks of 8 KBytes each
   };

   enum class DestinationCode : uint8_t
   {
      Japanese = 0x00,
      NonJapanese = 0x01
   };

   struct Header
   {
      std::array<uint8_t, 4> entryPoint;
      std::array<uint8_t, 48> nintendoLogo;
      std::array<uint8_t, 11> title;
      std::array<uint8_t, 4> manufacturerCode;
      CGBFlag cgbFlag;
      std::array<uint8_t, 2> newLicenseeCode;
      SGBFlag sgbFlag;
      Type type;
      ROMSize romSize;
      RAMSize ramSize;
      DestinationCode destinationCode;
      uint8_t oldLicenseeCode;
      uint8_t maskRomVersionNumber;
      uint8_t headerChecksum;
      std::array<uint8_t, 2> globalChecksum;
   };

   static const char* getTypeName(Type type);

private:
   DECLARE_UI_FRIEND

   Cartridge(std::vector<uint8_t>&& data, const Header& headerData);

   void setController(UPtr<MemoryBankController> mbc)
   {
      controller = std::move(mbc);
   }

   std::vector<uint8_t> cartData;
   Header header;
   std::array<char, 17> cartTitle;

   const bool ramPresent;
   const bool batteryPresent;
   const bool timerPresent;
   const bool rumblePresent;

   UPtr<MemoryBankController> controller;
};

} // namespace GBC
