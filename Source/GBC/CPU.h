#pragma once

#include "Core/Assert.h"
#include "Core/Enum.h"

#include "UI/UIFriend.h"

#include <cstdint>

namespace GBC
{

class GameBoy;

enum class Interrupt : uint8_t;

// Instructions
enum class Ins : uint8_t
{
   Invalid,

   LD,
   LDD,
   LDI,
   LDH,
   LDHL,
   PUSH,
   POP,

   ADD,
   ADC,
   SUB,
   SBC,
   AND,
   OR,
   XOR,
   CP,
   INC,
   DEC,

   SWAP,
   DAA,
   CPL,
   CCF,
   SCF,
   NOP,
   HALT,
   STOP,
   DI,
   EI,

   RLCA,
   RLA,
   RRCA,
   RRA,
   RLC,
   RL,
   RRC,
   RR,
   SLA,
   SRA,
   SRL,

   BIT,
   SET,
   RES,

   JP,
   JR,

   CALL,

   RST,

   RET,
   RETI,

   PREFIX
};

// Operands
enum class Opr : uint8_t
{
   None,

   A,
   F,
   B,
   C,
   D,
   E,
   H,
   L,

   AF,
   BC,
   DE,
   HL,
   SP,
   PC,

   Imm8,
   Imm16,
   Imm8Signed,

   DerefC,

   DerefBC,
   DerefDE,
   DerefHL,

   DerefImm8,
   DerefImm16,

   // flags (for jumps)
   FlagC,
   FlagNC,
   FlagZ,
   FlagNZ,

   // bit offsets
   Bit0,
   Bit1,
   Bit2,
   Bit3,
   Bit4,
   Bit5,
   Bit6,
   Bit7,

   // RST values
   Rst00H,
   Rst08H,
   Rst10H,
   Rst18H,
   Rst20H,
   Rst28H,
   Rst30H,
   Rst38H
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

   void step();

   bool isStopped() const
   {
      return stopped;
   }

   void resume()
   {
      stopped = false;
   }

   uint16_t getPC() const
   {
      return reg.pc;
   }

   void setPC(uint16_t address)
   {
      reg.pc = address;
   }

private:
   DECLARE_UI_FRIEND

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

   enum class Flag : uint8_t
   {
      Zero = 1 << 7,      // Zero flag
      Sub = 1 << 6,       // Subtract flag
      HalfCarry = 1 << 5, // Half carry flag
      Carry = 1 << 4,     // Carry flag
   };

   void setFlag(Flag flag, bool value)
   {
      ASSERT(flag == Flag::Zero || flag == Flag::Sub || flag == Flag::HalfCarry || flag == Flag::Carry, "Invalid flag value: %hhu", Enum::cast(flag));

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

      reg.f ^= (-static_cast<int8_t>(value) ^ reg.f) & Enum::cast(flag);
   }

   bool getFlag(Flag flag)
   {
      ASSERT(flag == Flag::Zero || flag == Flag::Sub || flag == Flag::HalfCarry || flag == Flag::Carry, "Invalid flag value: %hhu", Enum::cast(flag));

      return (reg.f & Enum::cast(flag)) != 0;
   }

   uint8_t readPC();
   uint16_t readPC16();

   void push(uint16_t value);
   uint16_t pop();

   bool handleInterrupts();
   bool handleInterrupt(Interrupt interrupt);

   Operation fetch();
   void execute(Operation operation);
   void execute16(Operation operation);

   Registers reg;
   GameBoy& gameBoy;
   bool ime;

   bool halted;
   bool stopped;
   bool interruptEnableRequested;
   bool freezePC;
};

} // namespace GBC
