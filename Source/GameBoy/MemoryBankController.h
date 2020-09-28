#pragma once

#include "Core/Archive.h"

#include <array>
#include <cstdint>

namespace DotMatrix
{

class Cartridge;

using RamBank = std::array<uint8_t, 0x2000>;

class MemoryBankController
{
public:
   MemoryBankController(const Cartridge& cartridge)
      : cart(cartridge)
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
   bool wroteToRam = false;
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
   enum class BankingMode : uint8_t
   {
      ROM = 0x00,
      RAM = 0x01
   };

   bool ramEnabled = false;
   uint8_t romBankNumber = 0x01;
   uint8_t ramBankNumber = 0x00;
   BankingMode bankingMode = BankingMode::ROM;

   std::array<RamBank, 4> ramBanks = {};
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
   bool ramEnabled = false;
   uint8_t romBankNumber = 0x01;

   std::array<uint8_t, 0x0200> ram = {};
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
      uint8_t seconds = 0;
      uint8_t minutes = 0;
      uint8_t hours = 0;
      uint8_t daysLow = 0;
      union
      {
         uint8_t daysHigh = 0;
         struct
         {
            uint8_t daysMsb : 1;
            uint8_t pad : 5;
            uint8_t halt : 1;
            uint8_t daysCarry : 1;
         };
      };
   };

private:
   bool ramRTCEnabled = false;
   bool rtcLatched = false;
   uint8_t latchData = 0xFF;
   uint8_t romBankNumber = 0x01;
   BankRegisterMode bankRegisterMode = BankRegisterMode::BankZero;
   RTC rtc;
   RTC rtcLatchedCopy;
   double tickTime = 0.0;

   std::array<RamBank, 4> ramBanks = {};
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
   bool ramEnabled = false;
   uint16_t romBankNumber = 0x0001;
   uint8_t ramBankNumber = 0x00;

   std::array<RamBank, 16> ramBanks = {};
};

} // namespace DotMatrix
