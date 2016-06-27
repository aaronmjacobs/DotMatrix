#ifndef CPU_H
#define CPU_H

#include "FancyAssert.h"
#include "Memory.h"

#include <cstdint>

namespace GBC {

// Instructions
enum class Ins : uint8_t {
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
enum class Opr : uint8_t {
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

struct Operation {
   Ins ins;
   Opr param1;
   Opr param2;
   uint8_t cycles;

   Operation(Ins i, Opr p1, Opr p2, uint8_t c)
      : ins(i), param1(p1), param2(p2), cycles(c) {
   }
};

class CPU {
public:
   CPU(Memory& memory);

   void tick();

private:
   union Registers {
      uint8_t raw[12];

      struct {
         union {
            struct {
               uint8_t a;  // accumulator register
               uint8_t f;  // status register
               uint8_t b;
               uint8_t c;
               uint8_t d;
               uint8_t e;
               uint8_t h;
               uint8_t l;
            };
            struct {
               uint16_t af;
               uint16_t bc;
               uint16_t de;
               uint16_t hl;
            };
         };

         uint16_t sp;      // stack pointer
         uint16_t pc;      // program counter
      };
   };

   enum Flag : uint8_t {
      kZero = 1 << 7,      // Zero flag
      kSub = 1 << 6,       // Subtract flag
      kHalfCarry = 1 << 5, // Half carry flag
      kCarry = 1 << 4,     // Carry flag
   };

   uint8_t readPC() {
      return mem.raw[reg.pc++];
   }

   uint16_t readPC16() {
      // TODO Endianness
      uint8_t first = readPC();
      uint8_t second = readPC();
      uint16_t res = second << 8 | first;
      return res;
   }

   void setFlag(Flag flag, bool val) {
      ASSERT(flag == kZero || flag == kSub || flag == kHalfCarry || flag == kCarry, "Invalid flag value: %hhu", flag);

      uint8_t mask = ~(static_cast<uint8_t>(val) - 1); // 0xFF if true, 0x00 if false
      ASSERT((val && mask == 0xFF) || (!val && mask == 0x00));

      uint8_t setVal = (reg.f | flag) & mask; // Sets the flag if val is true, otherwise results in 0x00
      uint8_t clearVal = reg.f & ~(flag | mask); // Clears the flag if val is false, otherwise results in 0x00
      ASSERT((setVal >= 0xFF && clearVal == 0xFF) || (clearVal >= 0xFF && setVal == 0xFF));

      reg.f = setVal | clearVal; // TODO Test!

      /*if (val) {
         reg.f |= flag;
      } else {
         reg.f &= ~flag;
      }*/
   }

   bool getFlag(Flag flag) {
      ASSERT(flag == kZero || flag == kSub || flag == kHalfCarry || flag == kCarry, "Invalid flag value: %hhu", flag);

      return (reg.f & flag) != 0;
   }

   void execute(Operation operation);

   void execute16(Operation operation);

   void push(uint16_t val);

   uint16_t pop();

   uint8_t* addr8(Opr operand, uint8_t* imm8, uint16_t* imm16);

   uint16_t* addr16(Opr operand, uint8_t* imm8, uint16_t* imm16);

   Registers reg;
   Memory& mem;
   bool ime;

   uint64_t cycles;

   bool halted;
   bool stopped;
   bool executedPrefixCB;
   bool interruptEnableRequested;
   bool interruptDisableRequested;
};

} // namespace GBC

#endif
