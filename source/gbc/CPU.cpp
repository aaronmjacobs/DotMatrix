#include "CPU.h"
#include "Log.h"
#include "Operations.h"

namespace GBC {

namespace {

bool is16BitOperand(Opr operand) {
   return operand == Opr::kImm8Signed || operand == Opr::kAF || operand == Opr::kBC || operand == Opr::kDE
       || operand == Opr::kHL || operand == Opr::kSP || operand == Opr::kPC || operand == Opr::kImm16
       || operand == Opr::kFlagC || operand == Opr::kFlagNC || operand == Opr::kFlagZ || operand == Opr::kFlagNZ
       || operand == Opr::k00H || operand == Opr::k08H || operand == Opr::k10H || operand == Opr::k18H
       || operand == Opr::k20H || operand == Opr::k28H || operand == Opr::k30H || operand == Opr::k38H;
}

bool is16BitOperation(const Operation& operation) {
   return operation.ins == Ins::kRET // Opcode 0xC9 is a RET with no operands
      || is16BitOperand(operation.param1) || is16BitOperand(operation.param2);
}

// Interpret a uint8_t as an int8_t
// static_cast<int8_t> behavior is platform dependent, memcpy is not
int8_t toSigned(uint8_t val) {
   int8_t signedVal;
   std::memcpy(&signedVal, &val, sizeof(signedVal));
   return signedVal;
}

bool checkBitOperand(Opr operand, uint8_t val) {
   return static_cast<uint8_t>(operand) - val == static_cast<uint8_t>(Opr::k0);
}

// Determine the BIT value from the operand
uint8_t bitOprMask(Opr operand) {
   // TODO Make static assert
   ASSERT(checkBitOperand(Opr::k1, 1) && checkBitOperand(Opr::k2, 2) && checkBitOperand(Opr::k3, 3)
       && checkBitOperand(Opr::k4, 4) && checkBitOperand(Opr::k5, 5) && checkBitOperand(Opr::k6, 6)
       && checkBitOperand(Opr::k7, 7), "Number operands are in an incorrect order");
   ASSERT(operand >= Opr::k0 && operand <= Opr::k7, "Bad bitOprMask() operand: %hhu", operand);

   uint8_t val = static_cast<uint8_t>(operand) - static_cast<uint8_t>(Opr::k0);
   return 1 << val;
}

// Calculate the restart offset from the operand
uint8_t rstOffset(Opr operand) {
   switch (operand) {
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
bool evalJumpCondition(Opr operand, bool zero, bool carry) {
   switch (operand) {
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

CPU::CPU(Memory& memory)
   : reg({0}), mem(memory), cycles(0), halted(false), stopped(false), executedPrefixCB(false),
     interruptEnableRequested(false), interruptDisableRequested(false) {
   reg.pc = 0x0100;
   reg.sp = 0xFFFE;
}

void CPU::tick() {
   bool goingToEnableInterrupts = interruptEnableRequested;
   bool goingToDisableInterrupts = interruptDisableRequested;
   interruptEnableRequested = interruptDisableRequested = false;
   ASSERT(!(goingToEnableInterrupts && goingToDisableInterrupts));

   uint8_t opcode = readPC();
   Operation operation = executedPrefixCB ? kCBOperations[opcode] : kOperations[opcode];
   executedPrefixCB = false;

   execute(operation);

   cycles += operation.cycles;

   if (goingToEnableInterrupts) {
      ime = true;
   } else if (goingToDisableInterrupts) {
      ime = false;
   }
}

void CPU::execute(Operation operation) {
   static const uint16_t kHalfCaryMask = 0x0010;
   static const uint16_t kCaryMask = 0x0100;

   // Check if the operation deals with 16-bit values
   if (is16BitOperation(operation)) {
      execute16(operation);
      return;
   }

   // If this is a compound operation, execute each part
   if (operation.ins == Ins::kLDD) {
      execute(Operation(Ins::kLD, operation.param1, operation.param2, 0));
      execute(Operation(Ins::kDEC, Opr::kHL, Opr::kNone, 0));
      return;
   } else if (operation.ins == Ins::kLDI) {
      execute(Operation(Ins::kLD, operation.param1, operation.param2, 0));
      execute(Operation(Ins::kINC, Opr::kHL, Opr::kNone, 0));
      return;
   } else if (operation.ins == Ins::kRETI) {
      execute(Operation(Ins::kRET, Opr::kNone, Opr::kNone, 0));
      execute(Operation(Ins::kEI, Opr::kNone, Opr::kNone, 0));
      return;
   }

   // Prepare immediate values if necessary
   uint8_t imm8 = 0;
   uint16_t imm16 = 0;
   if (operation.param1 == Opr::kImm8 || operation.param2 == Opr::kImm8
      || operation.param1 == Opr::kDrefImm8 || operation.param2 == Opr::kDrefImm8
      || operation.param1 == Opr::kImm8Signed || operation.param2 == Opr::kImm8Signed) {
      imm8 = readPC();
   } else if (operation.param1 == Opr::kImm16 || operation.param2 == Opr::kImm16
      || operation.param1 == Opr::kDrefImm16 || operation.param2 == Opr::kDrefImm16) {
      imm16 = readPC16();
   }

   uint8_t* param1 = addr8(operation.param1, &imm8, &imm16);
   uint8_t* param2 = addr8(operation.param2, &imm8, &imm16);

   switch (operation.ins) {
      // Loads
      case Ins::kLD:
      {
         // If one of the params is (C), the other param must be A
         ASSERT((operation.param2 != Opr::kDrefC || operation.param1 == Opr::kA)
            && (operation.param1 != Opr::kDrefC || operation.param2 == Opr::kA));

         *param1 = *param2;
         break;
      }
      case Ins::kLDH:
      {
         // Only valid params are (n) and A
         ASSERT((operation.param1 == Opr::kDrefImm8 && operation.param2 == Opr::kA)
            || (operation.param1 == Opr::kA && operation.param2 == Opr::kDrefImm8));

         *param1 = *param2;
         break;
      }

      // ALU
      case Ins::kADD:
      {
         ASSERT(operation.param1 == Opr::kA);

         uint16_t result = *param1 + *param2;
         uint16_t carryBits = *param1 ^ *param2 ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kADC:
      {
         ASSERT(operation.param1 == Opr::kA);

         uint16_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint16_t result = *param1 + *param2 + carryVal;
         uint16_t carryBits = *param1 ^ *param2 ^ carryVal ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kSUB:
      {
         uint16_t result = reg.a - *param1;
         uint16_t carryBits = reg.a ^ *param1 ^ result;

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
         uint16_t result = *param1 - *param2 - carryVal;
         uint16_t carryBits = *param1 ^ *param2 ^ carryVal ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kAND:
      {
         reg.a &= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kOR:
      {
         reg.a |= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kXOR:
      {
         reg.a ^= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kCP:
      {
         uint16_t result = reg.a - *param1;
         uint16_t carryBits = reg.a ^ *param1 ^ result;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kINC:
      {
         uint16_t result = *param1 + 1;
         uint16_t carryBits = *param1 ^ 1 ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         break;
      }
      case Ins::kDEC:
      {
         uint16_t result = *param1 - 1;
         uint16_t carryBits = *param1 ^ 1 ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         break;
      }

      // Miscellaneous
      case Ins::kSWAP:
      {
         *param1 = ((*param1 & 0x0F) << 4) | ((*param1 & 0xF0) >> 4);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case Ins::kDAA:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         uint16_t temp = reg.a;

         if (!getFlag(kSub)) {
            if (getFlag(kHalfCarry) || (temp & 0x0F) > 9) {
               temp += 0x06;
            }

            if (getFlag(kCarry) || (temp > 0x9F)) {
               temp += 0x60;
            }
         } else {
            if (getFlag(kHalfCarry)) {
               temp = (temp - 6) & 0xFF;
            }

            if (getFlag(kCarry)) {
               temp -= 0x60;
            }
         }

         bool carry = (temp & 0x0100) == 0x0100;
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

         // TODO
         halted = true;

         break;
      }
      case Ins::kSTOP:
      {
         // STOP should be followed by 0x00 (treated here as an immediate)
         ASSERT(*param1 == 0x00);

         // TODO
         stopped = true;

         break;
      }
      case Ins::kDI:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         interruptDisableRequested = true;
         break;
      }
      case Ins::kEI:
      {
         ASSERT(operation.param1 == Opr::kNone && operation.param2 == Opr::kNone);

         interruptEnableRequested = true;
         break;
      }

      // Rotates and shifts
      case Ins::kRLCA: // TODO Handle as special case above?
      {
         reg.a = (reg.a << 1) | (reg.a >> 7);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x01) == 1);
         break;
      }
      case Ins::kRLA: // TODO Handle as special case above?
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = reg.a & 0x80;
         reg.a = (reg.a << 1) | carryVal;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kRRCA: // TODO Handle as special case above?
      {
         reg.a = (reg.a >> 1) | (reg.a << 7);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x80) == 1);
         break;
      }
      case Ins::kRRA: // TODO Handle as special case above?
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = reg.a & 0x01;
         reg.a = (reg.a >> 1) | (carryVal << 7);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kRLC:
      {
         *param1 = (*param1 << 1) | (*param1 >> 7);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (*param1 & 0x01) == 1);
         break;
      }
      case Ins::kRL:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = *param1 & 0x80;
         *param1 = (*param1 << 1) | carryVal;

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kRRC:
      {
         *param1 = (*param1 >> 1) | (*param1 << 7);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (*param1 & 0x80) == 1);
         break;
      }
      case Ins::kRR:
      {
         uint8_t carryVal = getFlag(kCarry) ? 1 : 0;
         uint8_t newCarryVal = *param1 & 0x01;
         *param1 = (*param1 >> 1) | (carryVal << 7);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kSLA:
      {
         uint8_t newCarryVal = *param1 & 0x80;
         *param1 = *param1 << 1;

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kSRA:
      {
         uint8_t newCarryVal = *param1 & 0x01;
         *param1 = (*param1 >> 1) | (*param1 & 0x08);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case Ins::kSRL:
      {
         uint8_t newCarryVal = *param1 & 0x01;
         *param1 = *param1 >> 1;

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }

      // Bit operations
      case Ins::kBIT:
      {
         uint8_t mask = bitOprMask(operation.param1);

         setFlag(kZero, (*param2 & mask) == 1);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         break;
      }
      case Ins::kSET:
      {
         uint8_t mask = bitOprMask(operation.param1);

         *param2 |= mask;
         break;
      }
      case Ins::kRES:
      {
         uint8_t mask = bitOprMask(operation.param1);

         *param2 &= ~mask;
         break;
      }

      // Prefix CB
      case Ins::kPREFIX:
      {
         executedPrefixCB = true;
         break;
      }

      // Invalid instruction
      default:
      {
         ASSERT(false);
         break;
      }
   }
}

void CPU::execute16(Operation operation) {
   static const uint32_t kHalfCaryMask = 0x00001000;
   static const uint32_t kCaryMask = 0x00010000;

   // Prepare immediate values if necessary
   uint8_t imm8 = 0;
   uint16_t imm16 = 0;
   if (operation.param1 == Opr::kImm8 || operation.param2 == Opr::kImm8
      || operation.param1 == Opr::kDrefImm8 || operation.param2 == Opr::kDrefImm8) {
      imm8 = readPC();
   } else if (operation.param1 == Opr::kImm16 || operation.param2 == Opr::kImm16) {
      imm16 = readPC16();
   }

   uint16_t* param1 = addr16(operation.param1, &imm8, &imm16);
   uint16_t* param2 = addr16(operation.param2, &imm8, &imm16);

   switch (operation.ins) {
      // Loads
      case Ins::kLD:
      {
         if (operation.param1 == Opr::kDrefImm16) {
            ASSERT(operation.param2 == Opr::kSP);

            // TODO Make sure this is right (should be? gameboy is little endian)
            mem.raw[imm16] = *param2 & 0xFF;
            mem.raw[imm16 + 1] = (*param2 >> 8) & 0xFF;
         } else {
            *param1 = *param2;
         }
         break;
      }
      case Ins::kLDHL:
      {
         ASSERT(operation.param1 == Opr::kSP && operation.param2 == Opr::kImm8Signed);
         // Special case - uses one byte signed immediate value
         int8_t n = toSigned(imm8);

         uint32_t result = *param1 + n;
         uint32_t carryBits = *param1 ^ n ^ result; // TODO Correct if n is signed?

         *param1 = static_cast<uint16_t>(result);

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case Ins::kPUSH:
      {
         push(*param1);
         break;
      }
      case Ins::kPOP:
      {
         *param1 = pop();
         break;
      }

      // ALU
      case Ins::kADD:
      {
         ASSERT(operation.param1 == Opr::kHL || operation.param1 == Opr::kSP);

         if (operation.param1 == Opr::kHL) {
            uint32_t result = *param1 + *param2;
            uint32_t carryBits = *param1 ^ *param2 ^ result;

            *param1 = static_cast<uint16_t>(result);

            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
            setFlag(kCarry, (carryBits & kCaryMask) != 0);
         } else {
            ASSERT(operation.param2 == Opr::kImm8Signed);

            // Special case - uses one byte signed immediate value
            int8_t n = toSigned(imm8);

            uint32_t result = *param1 + n;
            uint32_t carryBits = *param1 ^ n ^ result; // TODO Correct if n is signed?

            *param1 = static_cast<uint16_t>(result);

            setFlag(kZero, false);
            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
            setFlag(kCarry, (carryBits & kCaryMask) != 0);
         }
         break;
      }
      case Ins::kINC:
      {
         ++*param1;
         break;
      }
      case Ins::kDEC:
      {
         --*param1;
         break;
      }

      // Jumps
      case Ins::kJP:
      {
         if (operation.param2 == Opr::kNone) {
            ASSERT(operation.param1 == Opr::kImm16 || operation.param1 == Opr::kHL); // TODO not DrefHL?

            reg.pc = *param1;
         } else {
            bool shouldJump = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldJump) {
               reg.pc = *param2;
            }
         }
         break;
      }
      case Ins::kJR:
      {
         // Special case - uses one byte signed immediate value
         int8_t n = toSigned(imm8);

         if (operation.param2 == Opr::kNone) {
            ASSERT(operation.param1 == Opr::kImm8Signed);

            reg.pc += n;
         } else {
            ASSERT(operation.param2 == Opr::kImm8Signed);

            bool shouldJump = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldJump) {
               reg.pc += n;
            }
         }
         break;
      }

      // Calls
      case Ins::kCALL:
      {
         if (operation.param2 == Opr::kNone) {
            push(reg.pc);
            reg.pc = *param1;
         } else {
            bool shouldCall = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldCall) {
               push(reg.pc);
               reg.pc = *param2;
            }
         }
         break;
      }

      // Restarts
      case Ins::kRST:
      {
         push(reg.pc);
         reg.pc = 0x0000 + rstOffset(operation.param1);
         break;
      }

      // Returns
      case Ins::kRET:
      {
         if (operation.param1 == Opr::kNone) {
            reg.pc = pop();
         } else {
            bool shouldReturn = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldReturn) {
               reg.pc = pop();
            }
         }
         break;
      }

      // Invalid instruction
      default:
      {
         ASSERT(false);
         break;
      }
   }
}

void CPU::push(uint16_t val) {
   reg.sp -= 2;
   memcpy(&mem.raw[reg.sp], &val, 2);
}

uint16_t CPU::pop() {
   uint16_t val;

   memcpy(&val, &mem.raw[reg.sp], 2);
   reg.sp += 2;

   return val;
}

uint8_t* CPU::addr8(Opr operand, uint8_t* imm8, uint16_t* imm16) {
   uint8_t* address = nullptr;

   switch (operand) {
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
         break;
      case Opr::kA:
         address = &reg.a;
         break;
      case Opr::kF:
         address = &reg.f;
         break;
      case Opr::kB:
         address = &reg.b;
         break;
      case Opr::kC:
         address = &reg.c;
         break;
      case Opr::kD:
         address = &reg.d;
         break;
      case Opr::kE:
         address = &reg.e;
         break;
      case Opr::kH:
         address = &reg.h;
         break;
      case Opr::kL:
         address = &reg.l;
         break;
      case Opr::kImm8:
         address = imm8;
         break;
      case Opr::kDrefC:
         address = &mem.raw[0xFF00 + reg.c];
         break;
      case Opr::kDrefBC:
         address = &mem.raw[reg.bc];
         break;
      case Opr::kDrefDE:
         address = &mem.raw[reg.de];
         break;
      case Opr::kDrefHL:
         address = &mem.raw[reg.hl];
         break;
      case Opr::kDrefImm8:
         address = &mem.raw[0xFF00 + *imm8];
         break;
      case Opr::kDrefImm16:
         address = &mem.raw[*imm16];
         break;
      default:
         ASSERT(false, "Invalid 8-bit operand: %hhu", operand);
   }

   return address;
}

uint16_t* CPU::addr16(Opr operand, uint8_t* imm8, uint16_t* imm16) {
   uint16_t* address = nullptr;

   switch (operand) {
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
         break;
      case Opr::kAF:
         address = &reg.af;
         break;
      case Opr::kBC:
         address = &reg.bc;
         break;
      case Opr::kDE:
         address = &reg.de;
         break;
      case Opr::kHL:
         address = &reg.hl;
         break;
      case Opr::kSP:
         address = &reg.sp;
         break;
      case Opr::kPC:
         address = &reg.pc;
         break;
      case Opr::kImm16:
         address = imm16;
         break;
      case Opr::kDrefImm16:
         address = reinterpret_cast<uint16_t*>(&mem.raw[*imm16]); // TODO Is this ok? Endian issues?
         break;
      default:
         ASSERT(false, "Invalid 16-bit operand: %hhu", operand);
   }

   return address;
}

} // namespace GBC
