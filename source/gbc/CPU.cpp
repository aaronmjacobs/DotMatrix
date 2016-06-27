#include "CPU.h"
#include "FancyAssert.h"
#include "Operations.h"

namespace GBC {

namespace {

bool is16BitOperand(Operand operand) {
   return operand == kImm8Signed || operand == kAF || operand == kBC || operand == kDE || operand == kHL
      || operand == kSP || operand == kPC || operand == kImm16;
}

bool is16BitOperation(const Operation& operation) {
   return is16BitOperand(operation.param1) || is16BitOperand(operation.param2);
}

// Interpret a uint8_t as an int8_t
// static_cast<int8_t> behavior is platform dependent, memcpy is not
int8_t toSigned(uint8_t val) {
   int8_t signedVal;
   std::memcpy(&signedVal, &val, sizeof(signedVal));
   return signedVal;
}

// Determine the BIT value from the operand
uint8_t bitVal(Operand operand) {
   // TODO Make static assert
   ASSERT(k1 - 1 == k0 && k2 - 2 == k0 && k3 - 3 == k0 && k4 - 4 == k0 && k5 - 5 == k0 && k6 - 6 == k0 && k7 - 7 == k0,
          "Number operands are in an incorrect order");
   ASSERT(operand >= k0 && operand <= k7, "Bad bitVal() operand: %d", operand);

   return operand - k0;
}

// Calculate the restart offset from the operand
uint8_t rstOffset(Operand operand) {
   switch (operand) {
      case k00H:
         return 0x00;
      case k08H:
         return 0x08;
      case k10H:
         return 0x10;
      case k18H:
         return 0x18;
      case k20H:
         return 0x20;
      case k28H:
         return 0x28;
      case k30H:
         return 0x30;
      case k38H:
         return 0x38;
      default:
         ASSERT(false);
         return 0x00;
   }
}

// Evaluate whether a jump / call / return should be executed
bool evalJumpCondition(Operand operand, bool zero, bool carry) {
   switch (operand) {
      case kFlagC:
         return carry;
      case kFlagNC:
         return !carry;
      case kFlagZ:
         return zero;
      case kFlagNZ:
         return !zero;
      default:
         ASSERT(false);
         return false;
   }
}

} // namespace

CPU::CPU(union Memory& memory)
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
   if (operation.ins == kLDD) {
      execute(Operation(kLD, operation.param1, operation.param2, 0));
      execute(Operation(kDEC, kDrefHL, kNone, 0));
      return;
   } else if (operation.ins == kLDI) {
      execute(Operation(kLD, operation.param1, operation.param2, 0));
      execute(Operation(kINC, kDrefHL, kNone, 0));
      return;
   } else if (operation.ins == kRETI) {
      execute(Operation(kRET, kNone, kNone, 0));
      execute(Operation(kEI, kNone, kNone, 0));
      return;
   }

   // Prepare immediate values if necessary
   uint8_t imm8 = 0;
   uint16_t imm16 = 0;
   if (operation.param1 == kImm8 || operation.param2 == kDrefImm8
      || operation.param2 == kImm8 || operation.param2 == kDrefImm8) {
      imm8 = readPC();
   } else if (operation.param1 == kDrefImm16 || operation.param2 == kDrefImm16) {
      imm16 = readPC16();
   }

   uint8_t* param1 = addr8(operation.param1, &imm8, &imm16);
   uint8_t* param2 = addr8(operation.param2, &imm8, &imm16);

   switch (operation.ins) {
      // Loads
      case kLD:
      {
         // If one of the params is (C), the other param must be A
         ASSERT((operation.param2 != kDrefC || operation.param1 == kA)
            && (operation.param1 != kDrefC || operation.param2 == kA));

         *param1 = *param2;
         break;
      }
      case kLDH:
      {
         // Only valid params are (n) and A
         ASSERT((operation.param1 == kDrefImm8 && operation.param2 == kA)
            || (operation.param1 == kA && operation.param2 == kDrefImm8));

         *param1 = *param2;
         break;
      }

      // ALU
      case kADD:
      {
         ASSERT(operation.param1 == kA);

         uint16_t result = *param1 + *param2;
         uint16_t carryBits = *param1 ^ *param2 ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case kADC:
      {
         ASSERT(operation.param1 == kA);

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
      case kSUB:
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
      case kSBC:
      {
         ASSERT(operation.param1 == kA);

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
      case kAND:
      {
         reg.a &= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         setFlag(kCarry, false);
         break;
      }
      case kOR:
      {
         reg.a |= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case kXOR:
      {
         reg.a ^= *param1;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case kCP:
      {
         uint16_t result = reg.a - *param1;
         uint16_t carryBits = reg.a ^ *param1 ^ result;

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, true);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case kINC:
      {
         uint16_t result = *param1 + 1;
         uint16_t carryBits = *param1 ^ 1 ^ result;

         *param1 = static_cast<uint8_t>(result);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         break;
      }
      case kDEC:
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
      case kSWAP:
      {
         *param1 = ((*param1 & 0x0F) << 4) | ((*param1 & 0xF0) >> 4);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, false);
         break;
      }
      case kDAA:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

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
      case kCPL:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         reg.a = ~reg.a;

         setFlag(kSub, true);
         setFlag(kHalfCarry, true);
         break;
      }
      case kCCF:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, !getFlag(kCarry));
         break;
      }
      case kSCF:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, true);
         break;
      }
      case kNOP:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         break;
      }
      case kHALT:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         // TODO
         halted = true;

         break;
      }
      case kSTOP:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         // TODO
         stopped = true;

         break;
      }
      case kDI:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         interruptDisableRequested = true;
         break;
      }
      case kEI:
      {
         ASSERT(operation.param1 == kNone && operation.param2 == kNone);

         interruptEnableRequested = true;
         break;
      }

      // Rotates and shifts
      case kRLCA: // TODO Handle as special case above?
      {
         reg.a = (reg.a << 1) | (reg.a >> 7);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x01) == 1);
         break;
      }
      case kRLA: // TODO Handle as special case above?
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
      case kRRCA: // TODO Handle as special case above?
      {
         reg.a = (reg.a >> 1) | (reg.a << 7);

         setFlag(kZero, reg.a == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (reg.a & 0x80) == 1);
         break;
      }
      case kRRA: // TODO Handle as special case above?
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
      case kRLC:
      {
         *param1 = (*param1 << 1) | (*param1 >> 7);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (*param1 & 0x01) == 1);
         break;
      }
      case kRL:
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
      case kRRC:
      {
         *param1 = (*param1 >> 1) | (*param1 << 7);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, (*param1 & 0x80) == 1);
         break;
      }
      case kRR:
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
      case kSLA:
      {
         uint8_t newCarryVal = *param1 & 0x80;
         *param1 = *param1 << 1;

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case kSRA:
      {
         uint8_t newCarryVal = *param1 & 0x01;
         *param1 = (*param1 >> 1) | (*param1 & 0x08);

         setFlag(kZero, *param1 == 0);
         setFlag(kSub, false);
         setFlag(kHalfCarry, false);
         setFlag(kCarry, newCarryVal == 1);
         break;
      }
      case kSRL:
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
      case kBIT:
      {
         uint8_t val = bitVal(operation.param1);

         setFlag(kZero, (*param2 & (1 << val)) == 1);
         setFlag(kSub, false);
         setFlag(kHalfCarry, true);
         break;
      }
      case kSET:
      {
         uint8_t val = bitVal(operation.param1);

         *param2 |= 1 << val;
         break;
      }
      case kRES:
      {
         uint8_t val = bitVal(operation.param1);

         *param2 &= ~(1 << val);
         break;
      }

      // Prefix CB
      case kPREFIX:
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
   if (operation.param1 == kImm8 || operation.param2 == kDrefImm8
      || operation.param2 == kImm8 || operation.param2 == kDrefImm8) {
      imm8 = readPC();
   } else if (operation.param1 == kImm16 || operation.param2 == kImm16) {
      imm16 = readPC16();
   }

   uint16_t* param1 = addr16(operation.param1, &imm8, &imm16);
   uint16_t* param2 = addr16(operation.param2, &imm8, &imm16);

   switch (operation.ins) {
      // Loads
      case kLD:
      {
         if (operation.param1 == kDrefImm16) {
            ASSERT(operation.param2 == kSP);

            // TODO Make sure this is right (should be? gameboy is little endian)
            mem.raw[imm16] = *param2 = 0xFF;
            mem.raw[imm16 + 1] = (*param2 >> 8) & 0xFF;
         } else {
            *param1 = *param2;
         }
         break;
      }
      case kLDHL:
      {
         ASSERT(operation.param1 == kHL && operation.param2 == kSP);
         // Special case - uses one byte signed immediate value
         int8_t n = toSigned(imm8);

         uint32_t result = *param2 + n;
         uint32_t carryBits = *param2 ^ n ^ result; // TODO Correct if n is signed?

         *param1 = static_cast<uint16_t>(result);

         setFlag(kZero, false);
         setFlag(kSub, false);
         setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
         setFlag(kCarry, (carryBits & kCaryMask) != 0);
         break;
      }
      case kPUSH:
      {
         push(*param1);
         break;
      }
      case kPOP:
      {
         *param1 = pop();
         break;
      }

      // ALU
      case kADD:
      {
         ASSERT(operation.param1 == kHL || operation.param1 == kSP);

         if (operation.param1 == kHL) {
            uint32_t result = *param1 + *param2;
            uint32_t carryBits = *param1 ^ *param2 ^ result;

            *param1 = static_cast<uint16_t>(result);

            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
            setFlag(kCarry, (carryBits & kCaryMask) != 0);
         } else {
            ASSERT(operation.param2 == kImm8Signed);

            // Special case - uses one byte signed immediate value
            int8_t n = toSigned(imm8);

            uint32_t result = *param1 + *param2 + n;
            uint32_t carryBits = *param1 ^ *param2 ^ n ^ result; // TODO Correct if n is signed?

            *param1 = static_cast<uint16_t>(result);

            setFlag(kZero, false);
            setFlag(kSub, false);
            setFlag(kHalfCarry, (carryBits & kHalfCaryMask) != 0);
            setFlag(kCarry, (carryBits & kCaryMask) != 0);
         }
         break;
      }
      case kINC:
      {
         ++*param1;
         break;
      }
      case kDEC:
      {
         --*param1;
         break;
      }

      // Jumps
      case kJP:
      {
         if (operation.param2 == kNone) {
            ASSERT(operation.param1 == kImm16 || operation.param1 == kHL); // TODO not DrefHL?

            reg.pc = *param1;
         } else {
            bool shouldJump = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldJump) {
               reg.pc = *param2;
            }
         }
         break;
      }
      case kJR:
      {
         // Special case - uses one byte signed immediate value
         int8_t n = toSigned(imm8);

         if (operation.param2 == kNone) {
            ASSERT(operation.param1 == kImm8Signed);

            reg.pc += n;
         } else {
            ASSERT(operation.param2 == kImm8Signed);

            bool shouldJump = evalJumpCondition(operation.param1, getFlag(kZero), getFlag(kCarry));
            if (shouldJump) {
               reg.pc += n;
            }
         }
         break;
      }

      // Calls
      case kCALL:
      {
         if (operation.param2 == kNone) {
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
      case kRST:
      {
         push(reg.pc);
         reg.pc = 0x0000 + rstOffset(operation.param1);
         break;
      }

      // Returns
      case kRET:
      {
         if (operation.param1 == kNone) {
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

uint8_t* CPU::addr8(Operand operand, uint8_t* imm8, uint16_t* imm16) {
   uint8_t* address = nullptr;

   switch (operand) {
   case kNone:
   case kCB:
   case kFlagC:
   case kFlagNC:
   case kFlagZ:
   case kFlagNZ:
   case k0:
   case k1:
   case k2:
   case k3:
   case k4:
   case k5:
   case k6:
   case k7:
   case k00H:
   case k08H:
   case k10H:
   case k18H:
   case k20H:
   case k28H:
   case k30H:
   case k38H:
      break;
   case kA:
      address = &reg.a;
      break;
   case kF:
      address = &reg.f;
      break;
   case kB:
      address = &reg.b;
      break;
   case kC:
      address = &reg.c;
      break;
   case kD:
      address = &reg.d;
      break;
   case kE:
      address = &reg.e;
      break;
   case kH:
      address = &reg.h;
      break;
   case kL:
      address = &reg.l;
      break;
   case kImm8:
      address = imm8;
      break;
   case kDrefC:
      address = &mem.raw[0xFF00 + reg.c];
      break;
   case kDrefBC:
      address = &mem.raw[reg.bc];
      break;
   case kDrefDE:
      address = &mem.raw[reg.de];
      break;
   case kDrefHL:
      address = &mem.raw[reg.hl];
      break;
   case kDrefImm8:
      address = &mem.raw[0xFF00 + *imm8];
      break;
   case kDrefImm16:
      address = &mem.raw[*imm16];
      break;
   default:
      ASSERT(false);
   }

   return address;
}

uint16_t* CPU::addr16(Operand operand, uint8_t* imm8, uint16_t* imm16) {
   uint16_t* address = nullptr;

   switch (operand) {
   case kNone:
   case kImm8Signed: // 8 bit signed value used with 16 bit values - needs to be handled as a special case
      break;
   case kAF:
      address = &reg.af;
      break;
   case kBC:
      address = &reg.bc;
      break;
   case kDE:
      address = &reg.de;
      break;
   case kHL:
      address = &reg.hl;
      break;
   case kSP:
      address = &reg.sp;
      break;
   case kPC:
      address = &reg.pc;
      break;
   case kImm16:
      address = imm16;
      break;
   default:
      ASSERT(false);
   }

   return address;
}

} // namespace GBC
