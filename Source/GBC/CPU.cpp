#include "Core/Log.h"

#include "GBC/CPU.h"
#include "GBC/GameBoy.h"
#include "GBC/Operations.h"

namespace GBC
{

namespace
{

bool is16BitOperand(Opr operand)
{
   return operand == Opr::kImm8Signed || operand == Opr::kAF || operand == Opr::kBC || operand == Opr::kDE
       || operand == Opr::kHL || operand == Opr::kSP || operand == Opr::kPC || operand == Opr::kImm16
       || operand == Opr::kFlagC || operand == Opr::kFlagNC || operand == Opr::kFlagZ || operand == Opr::kFlagNZ
       || operand == Opr::k00H || operand == Opr::k08H || operand == Opr::k10H || operand == Opr::k18H
       || operand == Opr::k20H || operand == Opr::k28H || operand == Opr::k30H || operand == Opr::k38H;
}

bool is16BitOperation(Operation operation)
{
   return operation.ins == Ins::kRET // Opcode 0xC9 is a RET with no operands
      || operation.ins == Ins::kRETI // RETI (0xD9) also has no operands
      || is16BitOperand(operation.param1) || is16BitOperand(operation.param2);
}

bool usesImm8(Operation operation)
{
   return operation.param1 == Opr::kImm8 || operation.param2 == Opr::kImm8
      || operation.param1 == Opr::kDrefImm8 || operation.param2 == Opr::kDrefImm8
      || operation.param1 == Opr::kImm8Signed || operation.param2 == Opr::kImm8Signed;
}

bool usesImm16(Operation operation)
{
   return operation.param1 == Opr::kImm16 || operation.param2 == Opr::kImm16
      || operation.param1 == Opr::kDrefImm16 || operation.param2 == Opr::kDrefImm16;
}

// Interpret a uint8_t as an int8_t
// static_cast<int8_t> behavior is platform dependent, memcpy is not
int8_t toSigned(uint8_t value)
{
   int8_t signedVal;
   std::memcpy(&signedVal, &value, sizeof(signedVal));
   return signedVal;
}

constexpr bool checkBitOperand(Opr operand, uint8_t value)
{
   return static_cast<uint8_t>(operand) - value == static_cast<uint8_t>(Opr::k0);
}

// Determine the BIT value from the operand
uint8_t bitOprMask(Opr operand)
{
   STATIC_ASSERT(checkBitOperand(Opr::k1, 1) && checkBitOperand(Opr::k2, 2) && checkBitOperand(Opr::k3, 3)
              && checkBitOperand(Opr::k4, 4) && checkBitOperand(Opr::k5, 5) && checkBitOperand(Opr::k6, 6)
              && checkBitOperand(Opr::k7, 7), "Number operands are in an incorrect order");
   ASSERT(operand >= Opr::k0 && operand <= Opr::k7, "Bad bitOprMask() operand: %hhu", operand);

   uint8_t value = static_cast<uint8_t>(operand) - static_cast<uint8_t>(Opr::k0);
   return 1 << value;
}

// Calculate the restart offset from the operand
uint8_t rstOffset(Opr operand)
{
   switch (operand)
   {
      case Opr::k00H:
         return 0x00;
      case Opr::k08H:
         return 0x08;
      case Opr::k10H:
         return 0x10;
      case Opr::k18H:
         return 0x18;
      case Opr::k20H:
         return 0x20;
      case Opr::k28H:
         return 0x28;
      case Opr::k30H:
         return 0x30;
      case Opr::k38H:
         return 0x38;
      default:
         ASSERT(false);
         return 0x00;
   }
}

// Evaluate whether a jump / call / return should be executed
bool evalJumpCondition(Opr operand, bool zero, bool carry)
{
   switch (operand)
   {
      case Opr::kFlagC:
         return carry;
      case Opr::kFlagNC:
         return !carry;
      case Opr::kFlagZ:
         return zero;
      case Opr::kFlagNZ:
         return !zero;
      default:
         ASSERT(false);
         return false;
   }
}

} // namespace

class CPU::Operand
{
public:
   Operand(CPU::Registers& registers, Memory& memory, Opr op, uint8_t immediate8, uint16_t immediate16)
      : reg(registers)
      , mem(memory)
      , opr(op)
      , imm8(immediate8)
      , imm16(immediate16)
   {
   }

   uint8_t read8() const;
   uint16_t read16() const;

   void write8(uint8_t value);
   void write16(uint16_t value);

private:
   CPU::Registers& reg;
   Memory& mem;

   Opr opr;
   uint8_t imm8;
   uint16_t imm16;
};

uint8_t CPU::Operand::read8() const
{
   uint8_t value = Memory::kInvalidAddressByte;

   switch (opr)
   {
      case Opr::kNone:
      case Opr::kCB:
      case Opr::k0:
      case Opr::k1:
      case Opr::k2:
      case Opr::k3:
      case Opr::k4:
      case Opr::k5:
      case Opr::k6:
      case Opr::k7:
         return value;
      case Opr::kA:
         value = reg.a;
         break;
      case Opr::kF:
         value = reg.f;
         break;
      case Opr::kB:
         value = reg.b;
         break;
      case Opr::kC:
         value = reg.c;
         break;
      case Opr::kD:
         value = reg.d;
         break;
      case Opr::kE:
         value = reg.e;
         break;
      case Opr::kH:
         value = reg.h;
         break;
      case Opr::kL:
         value = reg.l;
         break;
      case Opr::kImm8:
      case Opr::kImm8Signed:
         value = imm8;
         break;
      case Opr::kDrefC:
         value = mem.read(0xFF00 + reg.c);
         break;
      case Opr::kDrefBC:
         value = mem.read(reg.bc);
         break;
      case Opr::kDrefDE:
         value = mem.read(reg.de);
         break;
      case Opr::kDrefHL:
         value = mem.read(reg.hl);
         break;
      case Opr::kDrefImm8:
         value = mem.read(0xFF00 + imm8);
         break;
      case Opr::kDrefImm16:
         value = mem.read(imm16);
         break;
      default:
         ASSERT(false, "Invalid 8-bit operand: %hhu", opr);
   }

   return value;
}

uint16_t CPU::Operand::read16() const
{
   uint16_t value = 0xFFFF;

   switch (opr)
   {
      case Opr::kNone:
      case Opr::kImm8Signed: // 8 bit signed value used with 16 bit values - needs to be handled as a special case
      case Opr::kFlagC:
      case Opr::kFlagNC:
      case Opr::kFlagZ:
      case Opr::kFlagNZ:
      case Opr::k00H:
      case Opr::k08H:
      case Opr::k10H:
      case Opr::k18H:
      case Opr::k20H:
      case Opr::k28H:
      case Opr::k30H:
      case Opr::k38H:
      case Opr::kDrefImm16: // Handled as a special case in execute16()
         return value;
      case Opr::kAF:
         value = reg.af;
         break;
      case Opr::kBC:
         value = reg.bc;
         break;
      case Opr::kDE:
         value = reg.de;
         break;
      case Opr::kHL:
         value = reg.hl;
         break;
      case Opr::kSP:
         value = reg.sp;
         break;
      case Opr::kPC:
         value = reg.pc;
         break;
      case Opr::kImm16:
         value = imm16;
         break;
      default:
         ASSERT(false, "Invalid 16-bit operand: %hhu", opr);
   }

   return value;
}

void CPU::Operand::write8(uint8_t value)
{
   switch (opr)
   {
      case Opr::kA:
         reg.a = value;
         break;
      case Opr::kF:
         reg.f = value & 0xF0; // Don't allow any bits in the lower nibble
         break;
      case Opr::kB:
         reg.b = value;
         break;
      case Opr::kC:
         reg.c = value;
         break;
      case Opr::kD:
         reg.d = value;
         break;
      case Opr::kE:
         reg.e = value;
         break;
      case Opr::kH:
         reg.h = value;
         break;
      case Opr::kL:
         reg.l = value;
         break;
      case Opr::kDrefC:
         mem.write(0xFF00 + reg.c, value);
         break;
      case Opr::kDrefBC:
         mem.write(reg.bc, value);
         break;
      case Opr::kDrefDE:
         mem.write(reg.de, value);
         break;
      case Opr::kDrefHL:
         mem.write(reg.hl, value);
         break;
      case Opr::kDrefImm8:
         mem.write(0xFF00 + imm8, value);
         break;
      case Opr::kDrefImm16:
         mem.write(imm16, value);
         break;
      default:
         ASSERT(false, "Invalid / unwritable 8-bit operand: %hhu", opr);
   }
}

void CPU::Operand::write16(uint16_t value)
{
   switch (opr)
   {
      case Opr::kAF:
         reg.af = value & 0xFFF0; // Don't allow any bits in the lower nibble
         break;
      case Opr::kBC:
         reg.bc = value;
         break;
      case Opr::kDE:
         reg.de = value;
         break;
      case Opr::kHL:
         reg.hl = value;
         break;
      case Opr::kSP:
         reg.sp = value;
         break;
      case Opr::kPC:
         reg.pc = value;
         break;
      case Opr::kDrefImm16:
         mem.write(imm16, value & 0x00FF);
         mem.write(imm16 + 1, (value >> 8) & 0x00FF);
         break;
      default:
         ASSERT(false, "Invalid / unwritable 16-bit operand: %hhu", opr);
   }
}

CPU::CPU(GameBoy& gb)
   : reg({})
   , gameBoy(gb)
   , mem(gameBoy.getMemory())
   , ime(false)
   , halted(false)
   , stopped(false)
   , interruptEnableRequested(false)
   , freezePC(false)
{
}

void CPU::tick()
{
   if (halted)
   {
      if (hasInterrupt())
      {
         halted = false;
      }
      else
      {
         gameBoy.machineCycle();
         return;
      }
   }

   Operation operation = fetch();

   if (interruptEnableRequested)
   {
      ime = true;
      interruptEnableRequested = false;
   }

   execute(operation);
}

void CPU::push(uint16_t value)
{
   reg.sp -= 2;
   mem.write(reg.sp + 1, (value & 0xFF00) >> 8);
   mem.write(reg.sp, value & 0x00FF);
}

uint16_t CPU::pop()
{
   uint8_t low = mem.read(reg.sp);
   uint8_t high = mem.read(reg.sp + 1);
   reg.sp += 2;
   return (high << 8) | low;
}

bool CPU::handleInterrupts()
{
   if (!ime && !halted)
   {
      return false;
   }

   if ((mem.ie & Interrupt::kVBlank) && (mem.ifr & Interrupt::kVBlank))
   {
      return handleInterrupt(Interrupt::kVBlank);
   }
   else if ((mem.ie & Interrupt::kLCDState) && (mem.ifr & Interrupt::kLCDState))
   {
      return handleInterrupt(Interrupt::kLCDState);
   }
   else if ((mem.ie & Interrupt::kTimer) && (mem.ifr & Interrupt::kTimer))
   {
      return handleInterrupt(Interrupt::kTimer);
   }
   else if ((mem.ie & Interrupt::kSerial) && (mem.ifr & Interrupt::kSerial))
   {
      return handleInterrupt(Interrupt::kSerial);
   }
   else if ((mem.ie & Interrupt::kJoypad) && (mem.ifr & Interrupt::kJoypad))
   {
      return handleInterrupt(Interrupt::kJoypad);
   }

   return false;
}

bool CPU::handleInterrupt(Interrupt::Enum interrupt)
{
   ASSERT((ime || halted) && (mem.ie & interrupt) && (mem.ifr & interrupt));

   if (halted && !ime)
   {
      // The HALT state is left when an enabled interrupt occurs, no matter if the IME is enabled or not.
      // However, if IME is disabled the interrupt is not serviced.
      halted = false;
      return false;
   }

   ime = false;
   interruptEnableRequested = false;
   mem.ifr &= ~interrupt;
   halted = false;

   // two wait states
   gameBoy.machineCycle();
   gameBoy.machineCycle();

   // PC is pushed onto the stack
   push(reg.pc);

   // PC is set to the interrupt handler
   switch (interrupt)
   {
      case Interrupt::kVBlank:
         reg.pc = 0x0040;
         break;
      case Interrupt::kLCDState:
         reg.pc = 0x0048;
         break;
      case Interrupt::kTimer:
         reg.pc = 0x0050;
         break;
      case Interrupt::kSerial:
         reg.pc = 0x0058;
         break;
      case Interrupt::kJoypad:
         reg.pc = 0x0060;
         break;
   }

   gameBoy.machineCycle(); // TODO Correct? https://github.com/Gekkio/mooneye-gb/blob/master/docs/accuracy.markdown suggests it should be grouped with the other 2 above

   return true;
}

GBC::Operation CPU::fetch()
{
   uint8_t opcode = mem.read(reg.pc);

   bool handledInterrupt = handleInterrupts();
   if (handledInterrupt)
   {
      opcode = mem.read(reg.pc);
   }

   if (freezePC)
   {
      freezePC = false;
   }
   else
   {
      ++reg.pc;
   }

   Operation operation = kOperations[opcode];

   // Handle PREFIX CB
   if (operation.ins == Ins::kPREFIX)
   {
      opcode = readPC();
      operation = kCBOperations[opcode];
   }

   return operation;
}

void CPU::execute(Operation operation)
{
   static const uint16_t kHalfCaryMask = 0x0010;
   static const uint16_t kCaryMask = 0x0100;

   // Check if the operation deals with 16-bit values
   if (is16BitOperation(operation))
   {
      execute16(operation);
      return;
   }

   // Prepare immediate values if necessary
   uint8_t imm8 = 0;
   uint16_t imm16 = 0;
   if (usesImm8(operation))
   {
      imm8 = readPC();
   }
   else if (usesImm16(operation))
   {
      imm16 = readPC16();
   }

   Operand param1(reg, mem, operation.param1, imm8, imm16);
   Operand param2(reg, mem, operation.param2, imm8, imm16);

   switch (operation.ins)
   {
      // Loads
      case Ins::kLD:
      {
         // If one of the params is (C), the other param must be A
         ASSERT((operation.param2 != Opr::kDrefC || operation.param1 == Opr::kA)
            && (operation.param1 != Opr::kDrefC || operation.param2 == Opr::kA));

         param1.write8(param2.read8());
         break;
      }
      case Ins::kLDD:
      {
         execute(Operation(Ins::kLD, operation.param1, operation.param2, 0));

         --reg.hl;
         break;
      }
      case Ins::kLDI:
      {
         execute(Operation(Ins::kLD, operation.param1, operation.param2, 0));

         ++reg.hl;
         break;
      }
      case Ins::kLDH:
      {
         // Only valid params are (n) and A
         ASSERT((operation.param1 == Opr::kDrefImm8 && operation.param2 == Opr::kA)
            || (operation.param1 == Opr::kA && operation.param2 == Opr::kDrefImm8));

         param1.write8(param2.read8());
         break;
      }

      // ALU
      case Ins::kADD:
      {
         ASSERT(operation.param1 == Opr::kA);

         uint8_t param1Val = param1.read8();
         uint8_t param2Val = param2.read8();
         uint16_t result = param1Val + param2Val;
         uint16_t carryBits = param1Val ^ param2Val ^ result;

         uint8_t result8 = static_cast<uint8_t>(result);
         param1.write8(result8);

         setFlag(kZero, result8 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kADC:
      {
         ASSERT(operation.param1 == Opr::kA);

         uint16_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t param1Val = param1.read8();
         uint8_t param2Val = param2.read8();
         uint16_t result = param1Val + param2Val + carryVal;
         uint16_t carryBits = param1Val ^ param2Val ^ carryVal ^ result;

         uint8_t result8 = static_cast<uint8_t>(result);
         param1.write8(result8);

         setFlag(kZero, result8 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kSUB:
      {
         uint8_t param1Val = param1.read8();
         uint16_t result = reg.a - param1Val;
         uint16_t carryBits = reg.a ^ param1Val ^ result;

         reg.a = static_cast<uint8_t>(result);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kSBC:
      {
         ASSERT(operation.param1 == Opr::kA);

         uint16_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t param1Val = param1.read8();
         uint8_t param2Val = param2.read8();
         uint16_t result = param1Val - param2Val - carryVal;
         uint16_t carryBits = param1Val ^ param2Val ^ carryVal ^ result;

         uint8_t result8 = static_cast<uint8_t>(result);
         param1.write8(result8);

         setFlag(kZero, result8 == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kAND:
      {
         reg.a &= param1.read8();

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kOR:
      {
         reg.a |= param1.read8();

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kXOR:
      {
         reg.a ^= param1.read8();

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kCP:
      {
         uint8_t param1Val = param1.read8();
         uint16_t result = reg.a - param1Val;
         uint16_t carryBits = reg.a ^ param1Val ^ result;

         setFlag(kZero, result == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kINC:
      {
         uint8_t param1Val = param1.read8();
         uint16_t result = param1Val + 1;
         uint16_t carryBits = param1Val ^ 1 ^ result;

         uint8_t result8 = static_cast<uint8_t>(result);
         param1.write8(result8);

         setFlag(kZero, result8 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         break;
      }
      case Ins::kDEC:
      {
         uint8_t param1Val = param1.read8();
         uint16_t result = param1Val - 1;
         uint16_t carryBits = param1Val ^ 1 ^ result;

         uint8_t result8 = static_cast<uint8_t>(result);
         param1.write8(result8);

         setFlag(kZero, result8 == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         break;
      }

      // Miscellaneous
      case Ins::kSWAP:
      {
         uint8_t param1Val = param1.read8();
         uint8_t result = ((param1Val & 0x0F) << 4) | ((param1Val & 0xF0) >> 4);
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kDAA:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         uint16_t temp = reg.a;

         if (!getFlag(kSub))
         {
            if (getFlag(kHalfCarry) || (temp & 0x0F) > 9)
            {
               temp += 0x06;
            }

            if (getFlag(kCarry) || (temp > 0x9F))
            {
               temp += 0x60;
            }
         }
         else
         {
            if (getFlag(kHalfCarry))
            {
               temp = (temp - 6) & 0xFF;
            }

            if (getFlag(kCarry))
            {
               temp -= 0x60;
            }
         }

         bool carry = getFlag(kCarry) || (temp & 0x0100) == 0x0100;
         reg.a = temp & 0x00FF;

         setFlag(kZero, reg.a == 0);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, carry);
         break;
      }
      case Ins::kCPL:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         reg.a = ~reg.a;

         setFlag(kSub, true);
         setFlag(kHalfCarry, true);
         break;
      }
      case Ins::kCCF:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, !getFlag(kCarry));
         break;
      }
      case Ins::kSCF:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, true);
         break;
      }
      case Ins::kNOP:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         break;
      }
      case Ins::kHALT:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         halted = true;
         if (!ime && hasInterrupt())
         {
            // HALT bug
            freezePC = true;
         }
         break;
      }
      case Ins::kSTOP:
      {
         // STOP should be followed by 0x00 (treated here as an immediate)
         // ASSERT(param1Val == 0x00); // TODO

         stopped = true;
         break;
      }
      case Ins::kDI:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         ime = false;
         break;
      }
      case Ins::kEI:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         interruptEnableRequested = true;
         break;
      }

      // Rotates and shifts
      case Ins::kRLCA:
      {
         reg.a = (reg.a << 1) | (reg.a >> 7);

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x01) != 0);
         break;
      }
      case Ins::kRLA:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = reg.a & 0x80;

         reg.a = (reg.a << 1) | carryVal;

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kRRCA:
      {
         reg.a = (reg.a >> 1) | (reg.a << 7);

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x80) != 0);
         break;
      }
      case Ins::kRRA:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = reg.a & 0x01;

         reg.a = (reg.a >> 1) | (carryVal << 7);

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kRLC:
      {
         uint8_t param1Val = param1.read8();
         uint8_t result = (param1Val << 1) | (param1Val >> 7);
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (result & 0x01) != 0);
         break;
      }
      case Ins::kRL:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t param1Val = param1.read8();
         uint8_t newCarryVal = param1Val & 0x80;

         uint8_t result = (param1Val << 1) | carryVal;
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kRRC:
      {
         uint8_t param1Val = param1.read8();
         uint8_t result = (param1Val >> 1) | (param1Val << 7);
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (result & 0x80) != 0);
         break;
      }
      case Ins::kRR:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t param1Val = param1.read8();
         uint8_t newCarryVal = param1Val & 0x01;

         uint8_t result = (param1Val >> 1) | (carryVal << 7);
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kSLA:
      {
         uint8_t param1Val = param1.read8();
         uint8_t newCarryVal = param1Val & 0x80;

         uint8_t result = param1Val << 1;
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kSRA:
      {
         uint8_t param1Val = param1.read8();
         uint8_t newCarryVal = param1Val & 0x01;

         uint8_t result = (param1Val >> 1) | (param1Val & 0x80);
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }
      case Ins::kSRL:
      {
         uint8_t param1Val = param1.read8();
         uint8_t newCarryVal = param1Val & 0x01;

         uint8_t result = param1Val >> 1;
         param1.write8(result);

         setFlag(kZero, result == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal != 0);
         break;
      }

      // Bit operations
      case Ins::kBIT:
      {
         uint8_t mask = bitOprMask(operation.param1);

         setFlag(kZero, (param2.read8() & mask) == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         break;
      }
      case Ins::kSET:
      {
         uint8_t mask = bitOprMask(operation.param1);

         param2.write8(param2.read8() | mask);
         break;
      }
      case Ins::kRES:
      {
         uint8_t mask = bitOprMask(operation.param1);

         param2.write8(param2.read8() & ~mask);
         break;
      }

      // Invalid instruction
      default:
      {
         ASSERT(false, "Invalid 8-bit instruction: %hhu", operation.ins);
         break;
      }
   }
}

void CPU::execute16(Operation operation)
{
   static const uint32_t kHalfCaryMask = 0x00001000;
   static const uint32_t kCaryMask = 0x00010000;

   // Prepare immediate values if necessary
   uint8_t imm8 = 0;
   uint16_t imm16 = 0;
   if (usesImm8(operation))
   {
      imm8 = readPC();
   }
   else if (usesImm16(operation))
   {
      imm16 = readPC16();
   }

   Operand param1(reg, mem, operation.param1, imm8, imm16);
   Operand param2(reg, mem, operation.param2, imm8, imm16);

   switch (operation.ins)
   {
      // Loads
      case Ins::kLD:
      {
         uint16_t param2Val = param2.read16();

         if (operation.param1 == Opr::kDrefImm16)
         {
            ASSERT(operation.param2 == Opr::kSP);

            param1.write16(param2Val);
         }
         else
         {
            param1.write16(param2Val);

            if (operation.param2 == Opr::kHL)
            {
               gameBoy.machineCycle();
            }
         }
         break;
      }
      case Ins::kLDHL:
      {
         ASSERT(operation.param1 == Opr::kSP && operation.param2 == Opr::kImm8Signed);
         // Special case - uses one byte signed immediate value
         int8_t n = toSigned(param2.read8());

         uint16_t param1Val = param1.read16();
         uint32_t result = param1Val + n;
         uint32_t carryBits = param1Val ^ n ^ result;

         reg.hl = static_cast<uint16_t>(result);

         // Special case - treat carry and half carry as if this was an 8 bit add
         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & 0x0010) != 0);
         setFlag(kCarry, (carryBits & 0x0100) != 0);

         gameBoy.machineCycle();
         break;
      }
      case Ins::kPUSH:
      {
         gameBoy.machineCycle();
         push(param1.read16());
         break;
      }
      case Ins::kPOP:
      {
         param1.write16(pop());
         break;
      }

      // ALU
      case Ins::kADD:
      {
         ASSERT(operation.param1 == Opr::kHL || operation.param1 == Opr::kSP);

         if (operation.param1 == Opr::kHL)
         {
            uint16_t param1Val = param1.read16();
            uint16_t param2Val = param2.read16();
            uint32_t result = param1Val + param2Val;
            uint32_t carryBits = param1Val ^ param2Val ^ result;

            param1.write16(static_cast<uint16_t>(result));

            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
            setFlag(kCarry, (carryBits & kCaryMask) != 0);

            gameBoy.machineCycle();
         }
         else
         {
            ASSERT(operation.param2 == Opr::kImm8Signed);

            // Special case - uses one byte signed immediate value
            int8_t n = toSigned(param2.read8());

            uint16_t param1Val = param1.read16();
            uint32_t result = param1Val + n;
            uint32_t carryBits = param1Val ^ n ^ result;

            param1.write16(static_cast<uint16_t>(result));

            setFlag(kZero, false);
            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & 0x0010) != 0);
            setFlag(kCarry, (carryBits & 0x0100) != 0);

            gameBoy.machineCycle();
            gameBoy.machineCycle();
         }
         break;
      }
      case Ins::kINC:
      {
         param1.write16(param1.read16() + 1);
         gameBoy.machineCycle();
         break;
      }
      case Ins::kDEC:
      {
         param1.write16(param1.read16() - 1);
         gameBoy.machineCycle();
         break;
      }

      // Jumps
      case Ins::kJP:
      {
         if (operation.param2 == Opr::kNone)
         {
            ASSERT(operation.param1 == Opr::kImm16 || operation.param1 == Opr::kHL);

            reg.pc = param1.read16();
            if (operation.param1 == Opr::kImm16)
            {
               gameBoy.machineCycle();
            }
         }
         else
         {
            if (evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry)))
            {
               reg.pc = param2.read16();
               gameBoy.machineCycle();
            }
         }
         break;
      }
      case Ins::kJR:
      {
         if (operation.param2 == Opr::kNone)
         {
            ASSERT(operation.param1 == Opr::kImm8Signed);

            // Special case - uses one byte signed immediate value
            int8_t n = toSigned(param1.read8());

            reg.pc += n;
            gameBoy.machineCycle();
         }
         else
         {
            ASSERT(operation.param2 == Opr::kImm8Signed);

            // Special case - uses one byte signed immediate value
            int8_t n = toSigned(param2.read8());

            if (evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry)))
            {
               reg.pc += n;
               gameBoy.machineCycle();
            }
         }
         break;
      }

      // Calls
      case Ins::kCALL:
      {
         if (operation.param2 == Opr::kNone)
         {
            gameBoy.machineCycle();
            push(reg.pc);
            reg.pc = param1.read16();
         }
         else
         {
            uint16_t param2Val = param2.read16();
            if (evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry)))
            {
               gameBoy.machineCycle();
               push(reg.pc);
               reg.pc = param2Val;
            }
         }
         break;
      }

      // Restarts
      case Ins::kRST:
      {
         gameBoy.machineCycle();
         push(reg.pc);
         reg.pc = 0x0000 + rstOffset(operation.param1);
         break;
      }

      // Returns
      case Ins::kRET:
      {
         if (operation.param1 == Opr::kNone)
         {
            reg.pc = pop();
            gameBoy.machineCycle();
         }
         else
         {
            gameBoy.machineCycle();
            if (evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry)))
            {
               reg.pc = pop();
               gameBoy.machineCycle();
            }
         }
         break;
      }
      case Ins::kRETI:
      {
         execute(Operation(Ins::kRET, Opr::kNone, Opr::kNone, 0));

         // RETI doesn't delay enabling the IME like EI does
         ime = true;
         break;
      }

      // Invalid instruction
      default:
      {
         ASSERT(false, "Invalid 16-bit instruction: %hhu", operation.ins);
         break;
      }
   }
}

} // namespace GBC
