#include "Core/Assert.h"
#include "Core/Enum.h"
#include "Core/Log.h"

#include "GBC/Cartridge.h"

#include <cstring>
#include <memory>

namespace GBC
{

namespace
{
   const uint16_t kHeaderOffset = 0x0100;
   const uint16_t kHeaderSize = 0x0050;

   Cartridge::Header parseHeader(const std::vector<uint8_t>& data)
   {
      STATIC_ASSERT(sizeof(Cartridge::Header) == kHeaderSize);
      ASSERT(data.size() >= kHeaderOffset + kHeaderSize);

      Cartridge::Header header;
      std::memcpy(&header, &data.data()[kHeaderOffset], kHeaderSize);

      return header;
   }

   bool performHeaderChecksum(const Cartridge::Header& header, const std::vector<uint8_t>& data)
   {
      // Complement check, program will not run if incorrect
      // x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT
      const uint8_t* mem = data.data();

      uint8_t x = 0;
      for (uint16_t i = 0x0134; i <= 0x014C; ++i)
      {
         x = x - mem[i] - 1;
      }

      return x == header.headerChecksum;
   }

#if GBC_DEBUG
   bool performGlobalChecksum(const Cartridge::Header& header, const std::vector<uint8_t>& data)
   {
      // Checksum (higher byte first) produced by adding all bytes of a cartridge except for two checksum bytes and taking
      // two lower bytes of the result. (GameBoy ignores this value.)
      const uint8_t* mem = data.data();

      uint16_t x = 0;
      for (size_t i = 0; i < data.size(); ++i)
      {
         x += mem[i];
      }

      x -= header.globalChecksum[0];
      x -= header.globalChecksum[1];

      uint8_t low = x & 0x00FF;
      uint8_t high = (x & 0xFF00) >> 8;

      return header.globalChecksum[0] == high && header.globalChecksum[1] == low;
   }
#endif // GBC_DEBUG

   bool cartHasRAM(Cartridge::Type type)
   {
      switch (type)
      {
      case Cartridge::Type::MBC1PlusRAM:
      case Cartridge::Type::MBC1PlusRAMPlusBattery:
      case Cartridge::Type::MBC2:
      case Cartridge::Type::MBC2PlusBattery:
      case Cartridge::Type::ROMPlusRAM:
      case Cartridge::Type::ROMPlusRAMPlusBattery:
      case Cartridge::Type::MMM01PlusRAM:
      case Cartridge::Type::MMM01PlusRAMPlusBattery:
      case Cartridge::Type::MBC3PlusTimerPlusRAMPlusBattery:
      case Cartridge::Type::MBC3PlusRAM:
      case Cartridge::Type::MBC3PlusRAMPlusBattery:
      case Cartridge::Type::MBC4PlusRAM:
      case Cartridge::Type::MBC4PlusRAMPlusBattery:
      case Cartridge::Type::MBC5PlusRAM:
      case Cartridge::Type::MBC5PlusRAMPlusBattery:
      case Cartridge::Type::MBC5PlusRumblePlusRAM:
      case Cartridge::Type::MBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::Type::MBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      case Cartridge::Type::HuC1PlusRAMPlusBattery:
         return true;
      default:
         return false;
      }
   }

   bool cartHasBattery(Cartridge::Type type)
   {
      switch (type)
      {
      case Cartridge::Type::MBC1PlusRAMPlusBattery:
      case Cartridge::Type::MBC2PlusBattery:
      case Cartridge::Type::ROMPlusRAMPlusBattery:
      case Cartridge::Type::MMM01PlusRAMPlusBattery:
      case Cartridge::Type::MBC3PlusTimerPlusBattery:
      case Cartridge::Type::MBC3PlusTimerPlusRAMPlusBattery:
      case Cartridge::Type::MBC3PlusRAMPlusBattery:
      case Cartridge::Type::MBC4PlusRAMPlusBattery:
      case Cartridge::Type::MBC5PlusRAMPlusBattery:
      case Cartridge::Type::MBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::Type::MBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      case Cartridge::Type::HuC1PlusRAMPlusBattery:
         return true;
      default:
         return false;
      }
   }

   bool cartHasTimer(Cartridge::Type type)
   {
      switch (type)
      {
      case Cartridge::Type::MBC3PlusTimerPlusBattery:
      case Cartridge::Type::MBC3PlusTimerPlusRAMPlusBattery:
         return true;
      default:
         return false;
      }
   }

   bool cartHasRumble(Cartridge::Type type)
   {
      switch (type)
      {
      case Cartridge::Type::MBC5PlusRumble:
      case Cartridge::Type::MBC5PlusRumblePlusRAM:
      case Cartridge::Type::MBC5PlusRumblePlusRAMPlusBattery:
      case Cartridge::Type::MBC7PlusSensorPlusRumblePlusRAMPlusBattery:
         return true;
      default:
         return false;
      }
   }
}

// static
UPtr<Cartridge> Cartridge::fromData(std::vector<uint8_t> data, std::string& error)
{
   if (data.size() < kHeaderOffset + kHeaderSize)
   {
      error = "Cartridge provided insufficient data";
      return nullptr;
   }

   Header header = parseHeader(data);
   if (!performHeaderChecksum(header, data))
   {
      error = "Cartridge failed header checksum";
      return nullptr;
   }

#if GBC_DEBUG
   if (!performGlobalChecksum(header, data))
   {
      LOG_WARNING("Cartridge failed global checksum");
   }
#endif // GBC_DEBUG

   UPtr<Cartridge> cart(new Cartridge(std::move(data), header));

   UPtr<MemoryBankController> mbc;
   switch (header.type)
   {
   case Type::ROM:
      LOG_INFO("Null");
      mbc = std::make_unique<MBCNull>(*cart);
      break;
   case Type::MBC1:
   case Type::MBC1PlusRAM:
   case Type::MBC1PlusRAMPlusBattery:
      LOG_INFO("MBC1");
      mbc = std::make_unique<MBC1>(*cart);
      break;
   case Type::MBC2:
   case Type::MBC2PlusBattery:
      LOG_INFO("MBC2");
      mbc = std::make_unique<MBC2>(*cart);
      break;
   case Type::MBC3PlusTimerPlusBattery:
   case Type::MBC3PlusTimerPlusRAMPlusBattery:
   case Type::MBC3:
   case Type::MBC3PlusRAM:
   case Type::MBC3PlusRAMPlusBattery:
      LOG_INFO("MBC3");
      mbc = std::make_unique<MBC3>(*cart);
      break;
   case Type::MBC5:
   case Type::MBC5PlusRAM:
   case Type::MBC5PlusRAMPlusBattery:
   case Type::MBC5PlusRumble:
   case Type::MBC5PlusRumblePlusRAM:
   case Type::MBC5PlusRumblePlusRAMPlusBattery:
      LOG_INFO("MBC5");
      mbc = std::make_unique<MBC5>(*cart);
      break;
   default:
      error = std::string("Unimplemented cartridge type: ") + getTypeName(header.type);
      return nullptr;
   }

   cart->setController(std::move(mbc));
   return cart;
}

const char* Cartridge::getTypeName(Type type)
{
   switch (type)
   {
   case Type::ROM:
      return "ROM";

   case Type::MBC1:
      return "MBC1";
   case Type::MBC1PlusRAM:
      return "MBC1 + RAM";
   case Type::MBC1PlusRAMPlusBattery:
      return "MBC1 + RAM + Battery";

   case Type::MBC2:
      return "MBC2";
   case Type::MBC2PlusBattery:
      return "MBC2 + Battery";

   case Type::ROMPlusRAM:
      return "ROM + RAM";
   case Type::ROMPlusRAMPlusBattery:
      return "ROM + RAM + Battery";

   case Type::MMM01:
      return "MM01";
   case Type::MMM01PlusRAM:
      return "MM01 + RAM";
   case Type::MMM01PlusRAMPlusBattery:
      return "MM01 + RAM + Battery";

   case Type::MBC3PlusTimerPlusBattery:
      return "MBC3 + Timer + Battery";
   case Type::MBC3PlusTimerPlusRAMPlusBattery:
      return "MBC3 + Timer + RAM + Battery";
   case Type::MBC3:
      return "MBC3";
   case Type::MBC3PlusRAM:
      return "MBC3 + RAM";
   case Type::MBC3PlusRAMPlusBattery:
      return "MBC3 + RAM + Battery";

   case Type::MBC4:
      return "MBC4";
   case Type::MBC4PlusRAM:
      return "MBC4 + RAM";
   case Type::MBC4PlusRAMPlusBattery:
      return "MBC4 + RAM + Battery";

   case Type::MBC5:
      return "MBC5";
   case Type::MBC5PlusRAM:
      return "MBC5 + RAM";
   case Type::MBC5PlusRAMPlusBattery:
      return "MBC5 + RAM + Battery";
   case Type::MBC5PlusRumble:
      return "MBC5 + Rumble";
   case Type::MBC5PlusRumblePlusRAM:
      return "MBC5 + Rumble + RAM";
   case Type::MBC5PlusRumblePlusRAMPlusBattery:
      return "MBC5 + Rumble + RAM + Battery";

   case Type::MBC6:
      return "MBC6";

   case Type::MBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      return "MBC7 + Sensor + Rumble + RAM + Battery";

   case Type::PocketCamera:
      return "Pocket Camera";
   case Type::BandaiTAMA5:
      return "Bandai TAMA 5";
   case Type::HuC3:
      return "HuC3";
   case Type::HuC1PlusRAMPlusBattery:
      return "HuC1 + RAM + Battery";
   default:
      return "Unknown";
   }
}

Cartridge::Cartridge(std::vector<uint8_t> data, const Header& headerData)
   : cartData(std::move(data))
   , header(headerData)
   , cartTitle({})
   , ramPresent(cartHasRAM(header.type))
   , batteryPresent(cartHasBattery(header.type))
   , timerPresent(cartHasTimer(header.type))
   , rumblePresent(cartHasRumble(header.type))
   , controller(nullptr)
{
   bool supportsGBC = (Enum::cast(header.cgbFlag) & Enum::cast(CGBFlag::Supported)) != 0x00;
   std::size_t titleSize = supportsGBC ? header.title.size() : cartTitle.size() - 1;
   std::memcpy(cartTitle.data(), header.title.data(), titleSize);
}

} // namespace GBC
