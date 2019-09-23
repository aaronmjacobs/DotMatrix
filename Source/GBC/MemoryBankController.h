#pragma once

#include "Core/Archive.h"

#include "UI/UIFriend.h"

#include <array>
#include <cstdint>

namespace GBC
{

class Cartridge;

using RamBank = std::array<uint8_t, 0x2000>;

class MemoryBankController
{
public:
   MemoryBankController(const Cartridge& cartridge)
      : cart(cartridge)
      , wroteToRam(false)
   {
   }

   virtual ~MemoryBankController() = default;

   virtual uint8_t read(uint16_t address) const = 0;
   virtual void write(uint16_t address, uint8_t value) = 0;

   virtual void tick(double dt)
   {
      wroteToRam = false;
   }

   virtual Archive saveRAM() const
   {
      return {};
   }

   virtual bool loadRAM(Archive& ramData)
   {
      return false;
   }

   bool wroteToRamThisFrame() const
   {
      return wroteToRam;
   }

protected:
   const Cartridge& cart;
   bool wroteToRam;
};

class MBCNull : public MemoryBankController
{
public:
   MBCNull(const Cartridge& cartridge);

   uint8_t read(uint16_t address) const override;
   void write(uint16_t address, uint8_t value) override;
};

class MBC1 : public MemoryBankController
{
public:
   MBC1(const Cartridge& cartridge);

   uint8_t read(uint16_t address) const override;
   void write(uint16_t address, uint8_t value) override;

   Archive saveRAM() const override;
   bool loadRAM(Archive& ramData) override;

private:
   DECLARE_UI_FRIEND

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
   MBC2(const Cartridge& cartridge);

   uint8_t read(uint16_t address) const override;
   void write(uint16_t address, uint8_t value) override;

   Archive saveRAM() const override;
   bool loadRAM(Archive& ramData) override;

private:
   DECLARE_UI_FRIEND

   bool ramEnabled;
   uint8_t romBankNumber;

   std::array<uint8_t, 0x0200> ram;
};

class MBC3 : public MemoryBankController
{
public:
   MBC3(const Cartridge& cartridge);

   uint8_t read(uint16_t address) const override;
   void write(uint16_t address, uint8_t value) override;

   void tick(double dt) override;

   Archive saveRAM() const override;
   bool loadRAM(Archive& ramData) override;

private:
   DECLARE_UI_FRIEND

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
   MBC5(const Cartridge& cartridge);

   uint8_t read(uint16_t address) const override;
   void write(uint16_t address, uint8_t value) override;

   Archive saveRAM() const override;
   bool loadRAM(Archive& ramData) override;

private:
   DECLARE_UI_FRIEND

   bool ramEnabled;
   uint16_t romBankNumber;
   uint8_t ramBankNumber;

   std::array<RamBank, 16> ramBanks;
};

} // namespace GBC
