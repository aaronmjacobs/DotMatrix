#pragma once

#include "Core/Assert.h"

#include "GBC/Memory.h"

#include <cstdint>

namespace GBC
{

class GameBoy;

// Instructions
enum class Ins : uint8_t
{
   kInvalid,

   kLD,
   kLDD,
   kLDI,
   kLDH,
   kLDHL,
   kPUSH,
   kPOP,

   kADD,
   kADC,
   kSUB,
   kSBC,
   kAND,
   kOR,
   kXOR,
   kCP,
   kINC,
   kDEC,

   kSWAP,
   kDAA,
   kCPL,
   kCCF,
   kSCF,
   kNOP,
   kHALT,
   kSTOP,
   kDI,
   kEI,

   kRLCA,
   kRLA,
   kRRCA,
   kRRA,
   kRLC,
   kRL,
   kRRC,
   kRR,
   kSLA,
   kSRA,
   kSRL,

   kBIT,
   kSET,
   kRES,

   kJP,
   kJR,

   kCALL,

   kRST,

   kRET,
   kRETI,

   kPREFIX
};

// Operands
enum class Opr : uint8_t
{
   kNone,

   kA,
   kF,
   kB,
   kC,
   kD,
   kE,
   kH,
   kL,

   kAF,
   kBC,
   kDE,
   kHL,
   kSP,
   kPC,

   kImm8,
   kImm16,
   kImm8Signed,

   kDrefC,

   kDrefBC,
   kDrefDE,
   kDrefHL,

   kDrefImm8,
   kDrefImm16,

   kCB,

   // flags (for jumps)
   kFlagC,
   kFlagNC,
   kFlagZ,
   kFlagNZ,

   // bit offsets
   k0,
   k1,
   k2,
   k3,
   k4,
   k5,
   k6,
   k7,

   // RST values
   k00H,
   k08H,
   k10H,
   k18H,
   k20H,
   k28H,
   k30H,
   k38H
};

struct Operation
{
   Ins ins;
   Opr param1;
   Opr param2;
   uint8_t cycles;

   Operation(Ins i, Opr p1, Opr p2, uint8_t c)
      : ins(i)
      , param1(p1)
      , param2(p2)
      , cycles(c)
   {
   }
};

class CPU
{
public:
   static const uint8_t kClockCyclesPerMachineCycle = 4;
   static const uint64_t kClockSpeed = 4194304; // 4.194304 MHz TODO handle GBC / SGB
   static const uint64_t kMachineSpeed = kClockSpeed / kClockCyclesPerMachineCycle;

   CPU(GameBoy& gb);

   void tick();

   bool isStopped() const
   {
      return stopped;
   }

   void resume()
   {
      stopped = false;
   }

   uint8_t readPC()
   {
      return mem.read(reg.pc++);
   }

   uint16_t readPC16()
   {
      uint8_t low = readPC();
      uint8_t high = readPC();
      return (high << 8) | low;
   }

private:
   class Operand;

   struct Registers
   {
      union
      {
         struct
         {
#if GBC_IS_BIG_ENDIAN
            uint8_t a;  // accumulator register
            uint8_t f;  // status register

            uint8_t b;
            uint8_t c;

            uint8_t d;
            uint8_t e;

            uint8_t h;
            uint8_t l;
#else
            uint8_t f;  // status register
            uint8_t a;  // accumulator register

            uint8_t c;
            uint8_t b;

            uint8_t e;
            uint8_t d;

            uint8_t l;
            uint8_t h;
#endif
         };
         struct
         {
            uint16_t af;
            uint16_t bc;
            uint16_t de;
            uint16_t hl;
         };
      };

      uint16_t sp;      // stack pointer
      uint16_t pc;      // program counter
   };

   enum Flag : uint8_t
   {
      kZero = 1 << 7,      // Zero flag
      kSub = 1 << 6,       // Subtract flag
      kHalfCarry = 1 << 5, // Half carry flag
      kCarry = 1 << 4,     // Carry flag
   };

   void setFlag(Flag flag, bool value)
   {
      ASSERT(flag == kZero || flag == kSub || flag == kHalfCarry || flag == kCarry, "Invalid flag value: %hhu", flag);

      /*
       * This function is equivalent to the following:
       *
       * if (value)
       * {
       *    reg.f |= flag;
       * }
       * else
       * {
       *    reg.f &= ~flag;
       * }
       *
       * In tests the code below runs ~2x faster
       */

      reg.f ^= (-static_cast<int8_t>(value) ^ reg.f) & flag;
   }

   bool getFlag(Flag flag)
   {
      ASSERT(flag == kZero || flag == kSub || flag == kHalfCarry || flag == kCarry, "Invalid flag value: %hhu", flag);

      return (reg.f & flag) != 0;
   }

   void push(uint16_t value);
   uint16_t pop();

   bool hasInterrupt() const
   {
      return (mem.ie & mem.ifr & 0x1F) != 0;
   }

   bool handleInterrupts();
   bool handleInterrupt(Interrupt::Enum interrupt);

   Operation fetch();
   void execute(Operation operation);
   void execute16(Operation operation);

   Registers reg;
   GameBoy& gameBoy;
   Memory& mem;
   bool ime;

   bool halted;
   bool stopped;
   bool interruptEnableRequested;
   bool freezePC;
};

} // namespace GBC
