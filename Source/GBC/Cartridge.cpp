#include "Core/Assert.h"
#include "Core/Enum.h"
#include "Core/Log.h"

#include "GBC/Cartridge.h"
#include "GBC/Memory.h"

#include <cstring>
#include <ctime>
#include <memory>

namespace GBC
{

namespace
{

using RamBank = std::array<uint8_t, 0x2000>;

const uint16_t kHeaderOffset = 0x0100;
const uint16_t kHeaderSize = 0x0050;

Cartridge::Header parseHeader(const std::vector<uint8_t>& data)
{
   STATIC_ASSERT(sizeof(Cartridge::Header) == kHeaderSize);
   ASSERT(data.size() >= kHeaderOffset + kHeaderSize);

   Cartridge::Header header;
   memcpy(&header, &data.data()[kHeaderOffset], kHeaderSize);

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

int64_t getPlatformTime()
{
   static_assert(sizeof(std::time_t) <= sizeof(int64_t), "std::time_t will not fit in an int64_t");

   return static_cast<int64_t>(std::time(nullptr));
}

} // namespace

class ROM : public MemoryBankController
{
public:
   ROM(const Cartridge& cartridge)
      : MemoryBankController(cartridge)
   {
   }

   uint8_t read(uint16_t address) const override
   {
      if (address < 0x8000)
      {
         return cart.data(address);
      }

      LOG_WARNING("Trying to read invalid cartridge location: " << Log::hex(address));
      return Memory::kInvalidAddressByte;
   }

   void write(uint16_t address, uint8_t value) override
   {
      // No writable memory
      LOG_WARNING("Trying to write to read-only cartridge at location " << Log::hex(address) << ": " << Log::hex(value));
   }
};

class MBC1 : public MemoryBankController
{
public:
   MBC1(const Cartridge& cartridge)
      : MemoryBankController(cartridge)
      , ramEnabled(false)
      , romBankNumber(0x01)
      , ramBankNumber(0x00)
      , bankingMode(BankingMode::ROM)
      , ramBanks({})
   {
   }

   uint8_t read(uint16_t address) const override
   {
      uint8_t value = Memory::kInvalidAddressByte;

      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         case 0x2000:
         case 0x3000:
         {
            // Always contains the first 16Bytes of the ROM
            value = cart.data(address);
            break;
         }
         case 0x4000:
         case 0x5000:
         case 0x6000:
         case 0x7000:
         {
            // Switchable ROM bank
            ASSERT(romBankNumber > 0);
            value = cart.data(address + ((romBankNumber - 1) * 0x4000));
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to read from MBC1 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled)
            {
               uint8_t bankNumber = (bankingMode == BankingMode::RAM) ? ramBankNumber : 0x00;
               value = ramBanks[bankNumber][address - 0xA000];
            }
            else
            {
               LOG_WARNING("Trying to read from RAM when not enabled");
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to read invalid cartridge location: " << Log::hex(address));
            break;
         }
      }

      return value;
   }

   void write(uint16_t address, uint8_t value) override
   {
      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         {
            // RAM enable
            ramEnabled = (value & 0x0A) != 0x00;
            break;
         }
         case 0x2000:
         case 0x3000:
         {
            // ROM bank number
            romBankNumber = value & 0x1F;
            if (romBankNumber == 0x00 || romBankNumber == 0x20 || romBankNumber == 0x40 || romBankNumber == 0x60)
            {
               // Handle banks 0x00, 0x20, 0x40, 0x60
               romBankNumber += 0x01;
            }
            break;
         }
         case 0x4000:
         case 0x5000:
         {
            // RAM bank number or upper bits of ROM bank number
            uint8_t bankNumber = value & 0x03;
            switch (bankingMode)
            {
               case BankingMode::ROM:
                  romBankNumber = (romBankNumber & 0x1F) | (bankNumber << 5);
                  break;
               case BankingMode::RAM:
                  ramBankNumber = bankNumber;
                  break;
               default:
                  ASSERT(false, "Invalid banking mode: %hhu", Enum::cast(bankingMode));
            }
            break;
         }
         case 0x6000:
         case 0x7000:
         {
            // ROM / RAM mode select
            bankingMode = (value & 0x01) == 0x00 ? BankingMode::ROM : BankingMode::RAM;
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to write to MBC1 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled)
            {
               uint8_t bankNumber = (bankingMode == BankingMode::RAM) ? ramBankNumber : 0x00;
               ramBanks[bankNumber][address - 0xA000] = value;
               wroteToRam = true;
            }
            else
            {
               LOG_WARNING("Trying to write to disabled RAM " << Log::hex(address) << ": " << Log::hex(value));
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to write to read-only cartridge location " << Log::hex(address) << ": " << Log::hex(value));
            break;
         }
      };
   }

   Archive saveRAM() const override
   {
      Archive ramData;

      for (const RamBank& bank : ramBanks)
      {
         ramData.write(bank);
      }

      return ramData;
   }

   bool loadRAM(Archive& ramData) override
   {
      for (RamBank& bank : ramBanks)
      {
         if (!ramData.read(bank))
         {
            return false;
         }
      }

      return true;
   }

private:
   enum class BankingMode : uint8_t
   {
      ROM = 0x00,
      RAM = 0x01
   };

   bool ramEnabled;
   uint8_t romBankNumber;
   uint8_t ramBankNumber;
   BankingMode bankingMode;

   std::array<RamBank, 4> ramBanks;
};

class MBC2 : public MemoryBankController
{
public:
   MBC2(const Cartridge& cartridge)
      : MemoryBankController(cartridge)
      , ramEnabled(false)
      , romBankNumber(0x01)
   {
      ram.fill(0xFF);
   }

   uint8_t read(uint16_t address) const override
   {
      uint8_t value = Memory::kInvalidAddressByte;

      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         case 0x2000:
         case 0x3000:
         {
            // Always contains the first 16Bytes of the ROM
            value = cart.data(address);
            break;
         }
         case 0x4000:
         case 0x5000:
         case 0x6000:
         case 0x7000:
         {
            // Switchable ROM bank
            ASSERT(romBankNumber > 0);
            value = cart.data(address + ((romBankNumber - 1) * 0x4000));
            break;
         }
         case 0xA000:
         {
            if (address > 0xA1FF)
            {
               break;
            }

            // Switchable RAM bank
            if (ramEnabled)
            {
               value = ram[address - 0xA000];
            }
            else
            {
               LOG_WARNING("Trying to read from RAM when not enabled");
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to read invalid cartridge location: " << Log::hex(address));
            break;
         }
      }

      return value;
   }

   void write(uint16_t address, uint8_t value) override
   {
      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         {
            // RAM enable
            // The least significant bit of the upper address byte must be zero to enable/disable cart RAM
            if ((address & 0x0100) == 0x0000)
            {
               ramEnabled = (value & 0x0A) != 0x00;
            }
            break;
         }
         case 0x2000:
         case 0x3000:
         {
            // ROM bank number
            // The least significant bit of the upper address byte must be one to select a ROM bank
            if ((address & 0x0100) != 0x0000)
            {
               romBankNumber = value & 0x0F;
            }
            break;
         }
         case 0xA000:
         {
            ASSERT(cart.hasRAM(), "Trying to write to MBC2 cartridge RAM when it doesn't have any!");
            ASSERT(false);

            if (address > 0xA1FF)
            {
               break;
            }

            // Switchable RAM bank
            if (ramEnabled)
            {
               // only the lower 4 bits of the "bytes" in this memory area are used
               ram[address - 0xA000] = 0xF0 | (value & 0x0F);
               wroteToRam = true;
            }
            else
            {
               LOG_WARNING("Trying to write to disabled RAM " << Log::hex(address) << ": " << Log::hex(value));
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to write to read-only cartridge location " << Log::hex(address) << ": " << Log::hex(value));
            break;
         }
      };
   }

   Archive saveRAM() const override
   {
      Archive ramData;

      ramData.write(ram);

      return ramData;
   }

   bool loadRAM(Archive& ramData) override
   {
      return ramData.read(ram);
   }

private:
   bool ramEnabled;
   uint8_t romBankNumber;

   std::array<uint8_t, 0x0200> ram;
};

class MBC3 : public MemoryBankController
{
public:
   MBC3(const Cartridge& cartridge)
      : MemoryBankController(cartridge)
      , ramRTCEnabled(false)
      , rtcLatched(false)
      , latchData(0xFF)
      , romBankNumber(0x01)
      , bankRegisterMode(BankRegisterMode::BankZero)
      , rtc({})
      , rtcLatchedCopy({})
      , tickTime(0.0)
      , ramBanks({})
   {
      STATIC_ASSERT(sizeof(RTC) == 5, "Invalid RTC size (check bitfields)");
   }

   uint8_t read(uint16_t address) const override
   {
      uint8_t value = Memory::kInvalidAddressByte;

      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         case 0x2000:
         case 0x3000:
         {
            // Always contains the first 16Bytes of the ROM
            value = cart.data(address);
            break;
         }
         case 0x4000:
         case 0x5000:
         case 0x6000:
         case 0x7000:
         {
            // Switchable ROM bank
            ASSERT(romBankNumber > 0);
            value = cart.data(address + ((romBankNumber - 1) * 0x4000));
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to read from MBC3 cartridge RAM when it doesn't have any!");

            const RTC& readRTC = rtcLatched ? rtcLatchedCopy : rtc;

            // Switchable RAM bank / RTC
            if (ramRTCEnabled)
            {
               switch (bankRegisterMode)
               {
                  case BankRegisterMode::BankZero:
                  case BankRegisterMode::BankOne:
                  case BankRegisterMode::BankTwo:
                  case BankRegisterMode::BankThree:
                     value = ramBanks[Enum::cast(bankRegisterMode)][address - 0xA000];
                     break;
                  case BankRegisterMode::RTCSeconds:
                     value = readRTC.seconds;
                     break;
                  case BankRegisterMode::RTCMinutes:
                     value = readRTC.minutes;
                     break;
                  case BankRegisterMode::RTCHours:
                     value = readRTC.hours;
                     break;
                  case BankRegisterMode::RTCDaysLow:
                     value = readRTC.daysLow;
                     break;
                  case BankRegisterMode::RTCDaysHigh:
                     value = readRTC.daysHigh;
                     break;
                  default:
                     ASSERT(false, "Invalid RAM bank / RTC selection value: %hhu", Enum::cast(bankRegisterMode));
                     break;
               }
            }
            else
            {
               LOG_WARNING("Trying to read from RAM / RTC when not enabled");
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to read invalid cartridge location: " << Log::hex(address));
            break;
         }
      }

      return value;
   }

   void write(uint16_t address, uint8_t value) override
   {
      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         {
            // RAM / RTC enable
            ramRTCEnabled = (value & 0x0A) != 0x00;
            break;
         }
         case 0x2000:
         case 0x3000:
         {
            // ROM bank number
            romBankNumber = value & 0x7F;
            if (romBankNumber == 0x00)
            {
               // Handle bank 0x00
               romBankNumber += 0x01;
            }
            break;
         }
         case 0x4000:
         case 0x5000:
         {
            // RAM bank number or RTC register select
            ASSERT(value <= 0x03 || (value >= 0x08 && value <= 0x0C), "Invalid RAM bank / RTC selection value: %hhu", value);
            bankRegisterMode = static_cast<BankRegisterMode>(value);
            break;
         }
         case 0x6000:
         case 0x7000:
         {
            // Latch clock data
            if (latchData == 0x00 && value == 0x01)
            {
               rtcLatched = !rtcLatched;

               if (rtcLatched)
               {
                  rtcLatchedCopy = rtc;
               }
            }
            latchData = value;
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to write to MBC3 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramRTCEnabled)
            {
               switch (bankRegisterMode)
               {
                  case BankRegisterMode::BankZero:
                  case BankRegisterMode::BankOne:
                  case BankRegisterMode::BankTwo:
                  case BankRegisterMode::BankThree:
                     ramBanks[Enum::cast(bankRegisterMode)][address - 0xA000] = value;
                     break;
                  case BankRegisterMode::RTCSeconds:
                     rtc.seconds = value;
                     break;
                  case BankRegisterMode::RTCMinutes:
                     rtc.minutes = value;
                     break;
                  case BankRegisterMode::RTCHours:
                     rtc.hours = value;
                     break;
                  case BankRegisterMode::RTCDaysLow:
                     rtc.daysLow = value;
                     break;
                  case BankRegisterMode::RTCDaysHigh:
                     rtc.daysHigh = value;
                     break;
                  default:
                     ASSERT(false, "Invalid RAM bank / RTC selection value: %hhu", value);
                     break;
               }
               wroteToRam = true;
            }
            else
            {
               LOG_WARNING("Trying to write to disabled RAM / RTC " << Log::hex(address) << ": " << Log::hex(value));
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to write to read-only cartridge location " << Log::hex(address) << ": " << Log::hex(value));
            break;
         }
      };
   }

   void tick(double dt) override
   {
      MemoryBankController::tick(dt);

      if (rtc.halt || dt < 0.0)
      {
         return;
      }

      tickTime += dt;
      uint32_t newTime = static_cast<uint32_t>(tickTime);
      tickTime -= newTime;

      uint32_t seconds = rtc.seconds + newTime;

      uint32_t minutes = rtc.minutes + seconds / 60;
      seconds %= 60;

      uint32_t hours = rtc.hours + minutes / 60;
      minutes %= 60;

      uint32_t days = rtc.daysLow + (rtc.daysMsb ? 0x0100 : 0) + hours / 24;
      hours %= 24;

      rtc.seconds = static_cast<uint8_t>(seconds);
      rtc.minutes = static_cast<uint8_t>(minutes);
      rtc.hours = static_cast<uint8_t>(hours);
      rtc.daysLow = days % 0x0100;

      uint32_t daysMsb = days / 0x0100;
      rtc.daysMsb = daysMsb % 2; // Bit set first time we hit 0x0100 days, reset on next (overflow), etc.
      rtc.daysCarry = daysMsb > 1; // Carry bit set on overflow, stays until the program resets it
   }

   Archive saveRAM() const override
   {
      Archive ramData;

      for (const RamBank& bank : ramBanks)
      {
         ramData.write(bank);
      }

      ramData.write(rtc);
      ramData.write(getPlatformTime());

      return ramData;
   }

   bool loadRAM(Archive& ramData) override
   {
      for (RamBank& bank : ramBanks)
      {
         if (!ramData.read(bank))
         {
            return false;
         }
      }

      if (!ramData.read(rtc))
      {
         return false;
      }

      int64_t saveTime = 0;
      if (!ramData.read(saveTime))
      {
         return false;
      }

      // Update the RTC with the time between the last save and now
      int64_t now = getPlatformTime();
      double timeDiff = static_cast<double>(now - saveTime);
      tick(timeDiff);

      return true;
   }

private:
   enum class BankRegisterMode : uint8_t
   {
      BankZero = 0x00,
      BankOne = 0x01,
      BankTwo = 0x02,
      BankThree = 0x03,
      RTCSeconds = 0x08,
      RTCMinutes = 0x09,
      RTCHours = 0x0A,
      RTCDaysLow = 0x0B,
      RTCDaysHigh = 0x0C
   };

   // Real time clock
   struct RTC
   {
      uint8_t seconds;
      uint8_t minutes;
      uint8_t hours;
      uint8_t daysLow;
      union
      {
         uint8_t daysHigh;
         struct
         {
            uint8_t daysMsb : 1;
            uint8_t pad : 5;
            uint8_t halt : 1;
            uint8_t daysCarry : 1;
         };
      };
   };

   bool ramRTCEnabled;
   bool rtcLatched;
   uint8_t latchData;
   uint8_t romBankNumber;
   BankRegisterMode bankRegisterMode;
   RTC rtc;
   RTC rtcLatchedCopy;
   double tickTime;

   std::array<RamBank, 4> ramBanks;
};

class MBC5 : public MemoryBankController
{
public:
   MBC5(const Cartridge& cartridge)
      : MemoryBankController(cartridge)
      , ramEnabled(false)
      , romBankNumber(0x0001)
      , ramBankNumber(0x00)
      , ramBanks({})
   {
   }

   uint8_t read(uint16_t address) const override
   {
      uint8_t value = Memory::kInvalidAddressByte;

      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         case 0x2000:
         case 0x3000:
         {
            // Always contains the first 16Bytes of the ROM
            value = cart.data(address);
            break;
         }
         case 0x4000:
         case 0x5000:
         case 0x6000:
         case 0x7000:
         {
            // Switchable ROM bank
            ASSERT(romBankNumber <= 0x01E0);
            value = cart.data(address + ((static_cast<int16_t>(romBankNumber) - 1) * 0x4000));
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to read from MBC5 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled)
            {
               value = ramBanks[ramBankNumber][address - 0xA000];
            }
            else
            {
               LOG_WARNING("Trying to read from RAM when not enabled");
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to read invalid cartridge location: " << Log::hex(address));
            break;
         }
      }

      return value;
   }

   void write(uint16_t address, uint8_t value) override
   {
      switch (address & 0xF000)
      {
         case 0x0000:
         case 0x1000:
         {
            // RAM enable
            ramEnabled = (value & 0x0A) != 0x00;
            break;
         }
         case 0x2000:
         {
            // ROM bank number (lower 8 bits)
            romBankNumber = (romBankNumber & 0xFF00) | value;
            break;
         }
         case 0x3000:
         {
            // ROM bank number (upper 9th bit)
            romBankNumber = ((value & 0x01) << 8) | (romBankNumber & 0x00FF);
            break;
         }
         case 0x4000:
         case 0x5000:
         {
            // RAM bank number
            ramBankNumber = value & 0x0F;
            break;
         }
         case 0xA000:
         case 0xB000:
         {
            ASSERT(cart.hasRAM(), "Trying to write to MBC5 cartridge RAM when it doesn't have any!");

            // Switchable RAM bank
            if (ramEnabled)
            {
               ramBanks[ramBankNumber][address - 0xA000] = value;
               wroteToRam = true;
            }
            else
            {
               LOG_WARNING("Trying to write to disabled RAM " << Log::hex(address) << ": " << Log::hex(value));
            }
            break;
         }
         default:
         {
            LOG_WARNING("Trying to write to read-only cartridge location " << Log::hex(address) << ": " << Log::hex(value));
            break;
         }
      };
   }

   Archive saveRAM() const override
   {
      Archive ramData;

      for (const RamBank& bank : ramBanks)
      {
         ramData.write(bank);
      }

      return ramData;
   }

   bool loadRAM(Archive& ramData) override
   {
      for (RamBank& bank : ramBanks)
      {
         if (!ramData.read(bank))
         {
            return false;
         }
      }

      return true;
   }

private:
   bool ramEnabled;
   uint16_t romBankNumber;
   uint8_t ramBankNumber;

   std::array<RamBank, 16> ramBanks;
};

// static
UPtr<Cartridge> Cartridge::fromData(std::vector<uint8_t>&& data)
{
   if (data.size() < kHeaderOffset + kHeaderSize)
   {
      LOG_ERROR("Cartridge provided insufficient data");
      return nullptr;
   }

   Header header = parseHeader(data);
   if (!performHeaderChecksum(header, data))
   {
      LOG_ERROR("Cartridge data failed header checksum");
      return nullptr;
   }
   if (!performGlobalChecksum(header, data))
   {
      LOG_WARNING("Cartridge data failed global checksum");
   }

   UPtr<Cartridge> cart(new Cartridge(std::move(data), header));

   UPtr<MemoryBankController> mbc;
   switch (header.type)
   {
      case Type::ROM:
         LOG_INFO("ROM");
         mbc = std::make_unique<ROM>(*cart);
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
         LOG_ERROR("Invalid cartridge type: " << Log::hex(Enum::cast(header.type)));
         return nullptr;
   }

   cart->setController(std::move(mbc));
   return cart;
}

Cartridge::Cartridge(std::vector<uint8_t>&& data, const Header& headerData)
   : cartData(std::move(data))
   , header(headerData)
   , cartTitle({})
   , ramPresent(cartHasRAM(header.type))
   , batteryPresent(cartHasBattery(header.type))
   , timerPresent(cartHasTimer(header.type))
   , rumblePresent(cartHasRumble(header.type))
   , controller(nullptr)
{
   bool supportsGBC = (header.cgbFlag & kCBGSupported) != 0x00;
   std::size_t titleSize = supportsGBC ? header.title.size() : cartTitle.size() - 1;
   std::memcpy(cartTitle.data(), header.title.data(), titleSize);
}

} // namespace GBC
