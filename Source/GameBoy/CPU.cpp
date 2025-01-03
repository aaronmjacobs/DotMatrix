#include "Core/Enum.h"
#include "Core/Log.h"
#include "Core/Math.h"

#include "GameBoy/CPU.h"
#include "GameBoy/GameBoy.h"
#include "GameBoy/Operations.h"

namespace DotMatrix
{

namespace
{
   bool is16BitOperand(Opr operand)
   {
      return operand == Opr::AF || operand == Opr::BC || operand == Opr::DE || operand == Opr::HL
          || operand == Opr::SP || operand == Opr::PC || operand == Opr::Imm16 || operand == Opr::Imm8Signed
          || operand == Opr::FlagC || operand == Opr::FlagNC || operand == Opr::FlagZ || operand == Opr::FlagNZ
          || operand == Opr::Rst00H || operand == Opr::Rst08H || operand == Opr::Rst10H || operand == Opr::Rst18H
          || operand == Opr::Rst20H || operand == Opr::Rst28H || operand == Opr::Rst30H || operand == Opr::Rst38H;
   }

   bool is16BitOperation(Operation operation)
   {
      return operation.ins == Ins::RET // Opcode 0xC9 is a RET with no operands
         || operation.ins == Ins::RETI // RETI (0xD9) also has no operands
         || is16BitOperand(operation.param1) || is16BitOperand(operation.param2);
   }

   bool usesImm8(Operation operation)
   {
      return operation.param1 == Opr::Imm8 || operation.param2 == Opr::Imm8
         || operation.param1 == Opr::DerefImm8 || operation.param2 == Opr::DerefImm8
         || operation.param1 == Opr::Imm8Signed || operation.param2 == Opr::Imm8Signed;
   }

   bool usesImm16(Operation operation)
   {
      return operation.param1 == Opr::Imm16 || operation.param2 == Opr::Imm16
         || operation.param1 == Opr::DerefImm16 || operation.param2 == Opr::DerefImm16;
   }

   constexpr bool checkBitOperand(Opr operand, uint8_t value)
   {
      return Enum::cast(operand) - value == Enum::cast(Opr::Bit0);
   }

   // Determine the BIT value from the operand
   uint8_t bitOprMask(Opr operand)
   {
      DM_STATIC_ASSERT(checkBitOperand(Opr::Bit1, 1) && checkBitOperand(Opr::Bit2, 2) && checkBitOperand(Opr::Bit3, 3)
                    && checkBitOperand(Opr::Bit4, 4) && checkBitOperand(Opr::Bit5, 5) && checkBitOperand(Opr::Bit6, 6)
                    && checkBitOperand(Opr::Bit7, 7), "Number operands are in an incorrect order");
      DM_ASSERT(operand >= Opr::Bit0 && operand <= Opr::Bit7, "Bad bitOprMask() operand: %hhu", Enum::cast(operand));

      uint8_t value = Enum::cast(operand) - Enum::cast(Opr::Bit0);
      return 1 << value;
   }

   // Calculate the restart offset from the operand
   uint8_t rstOffset(Opr operand)
   {
      switch (operand)
      {
      case Opr::Rst00H:
         return 0x00;
      case Opr::Rst08H:
         return 0x08;
      case Opr::Rst10H:
         return 0x10;
      case Opr::Rst18H:
         return 0x18;
      case Opr::Rst20H:
         return 0x20;
      case Opr::Rst28H:
         return 0x28;
      case Opr::Rst30H:
         return 0x30;
      case Opr::Rst38H:
         return 0x38;
      default:
         DM_ASSERT(false);
         return 0x00;
      }
   }

   // Evaluate whether a jump / call / return should be executed
   bool evalJumpCondition(Opr operand, bool zero, bool carry)
   {
      switch (operand)
      {
      case Opr::FlagC:
         return carry;
      case Opr::FlagNC:
         return !carry;
      case Opr::FlagZ:
         return zero;
      case Opr::FlagNZ:
         return !zero;
      default:
         DM_ASSERT(false);
         return false;
      }
   }
}

class CPU::Operand
{
public:
   Operand(CPU::Registers& registers, GameBoy& gameBoy, Opr op, uint8_t immediate8, uint16_t immediate16)
      : reg(registers)
      , gb(gameBoy)
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
   GameBoy& gb;

   Opr opr;
   uint8_t imm8;
   uint16_t imm16;
};

uint8_t CPU::Operand::read8() const
{
   uint8_t value = GameBoy::kInvalidAddressByte;

   switch (opr)
   {
   case Opr::A:
      value = reg.a;
      break;
   case Opr::F:
      value = reg.f;
      break;
   case Opr::B:
      value = reg.b;
      break;
   case Opr::C:
      value = reg.c;
      break;
   case Opr::D:
      value = reg.d;
      break;
   case Opr::E:
      value = reg.e;
      break;
   case Opr::H:
      value = reg.h;
      break;
   case Opr::L:
      value = reg.l;
      break;
   case Opr::Imm8:
   case Opr::Imm8Signed:
      value = imm8;
      break;
   case Opr::DerefC:
      value = gb.read(0xFF00 + reg.c);
      break;
   case Opr::DerefBC:
      value = gb.read(reg.bc);
      break;
   case Opr::DerefDE:
      value = gb.read(reg.de);
      break;
   case Opr::DerefHL:
      value = gb.read(reg.hl);
      break;
   case Opr::DerefImm8:
      value = gb.read(0xFF00 + imm8);
      break;
   case Opr::DerefImm16:
      value = gb.read(imm16);
      break;
   default:
      DM_ASSERT(false, "Invalid 8-bit read operand: %hhu", Enum::cast(opr));
      break;
   }

   return value;
}

uint16_t CPU::Operand::read16() const
{
   static const uint16_t kInvalidAddressBytes = (GameBoy::kInvalidAddressByte << 8) | GameBoy::kInvalidAddressByte;

   uint16_t value = kInvalidAddressBytes;

   switch (opr)
   {
   case Opr::AF:
      value = reg.af;
      break;
   case Opr::BC:
      value = reg.bc;
      break;
   case Opr::DE:
      value = reg.de;
      break;
   case Opr::HL:
      value = reg.hl;
      break;
   case Opr::SP:
      value = reg.sp;
      break;
   case Opr::PC:
      value = reg.pc;
      break;
   case Opr::Imm16:
      value = imm16;
      break;
   default:
      DM_ASSERT(false, "Invalid 16-bit read operand: %hhu", Enum::cast(opr));
      break;
   }

   return value;
}

void CPU::Operand::write8(uint8_t value)
{
   switch (opr)
   {
   case Opr::A:
      reg.a = value;
      break;
   case Opr::F:
      reg.f = value & 0xF0; // Don't allow any bits in the lower nibble
      break;
   case Opr::B:
      reg.b = value;
      break;
   case Opr::C:
      reg.c = value;
      break;
   case Opr::D:
      reg.d = value;
      break;
   case Opr::E:
      reg.e = value;
      break;
   case Opr::H:
      reg.h = value;
      break;
   case Opr::L:
      reg.l = value;
      break;
   case Opr::DerefC:
      gb.write(0xFF00 + reg.c, value);
      break;
   case Opr::DerefBC:
      gb.write(reg.bc, value);
      break;
   case Opr::DerefDE:
      gb.write(reg.de, value);
      break;
   case Opr::DerefHL:
      gb.write(reg.hl, value);
      break;
   case Opr::DerefImm8:
      gb.write(0xFF00 + imm8, value);
      break;
   case Opr::DerefImm16:
      gb.write(imm16, value);
      break;
   default:
      DM_ASSERT(false, "Invalid 8-bit write operand: %hhu", Enum::cast(opr));
      break;
   }
}

void CPU::Operand::write16(uint16_t value)
{
   switch (opr)
   {
   case Opr::AF:
      reg.af = value & 0xFFF0; // Don't allow any bits in the lower nibble
      break;
   case Opr::BC:
      reg.bc = value;
      break;
   case Opr::DE:
      reg.de = value;
      break;
   case Opr::HL:
      reg.hl = value;
      break;
   case Opr::SP:
      reg.sp = value;
      break;
   case Opr::PC:
      reg.pc = value;
      break;
   case Opr::DerefImm16:
      gb.write(imm16, value & 0x00FF);
      gb.write(imm16 + 1, (value >> 8) & 0x00FF);
      break;
   default:
      DM_ASSERT(false, "Invalid 16-bit write operand: %hhu", Enum::cast(opr));
      break;
   }
}

CPU::CPU(GameBoy& gb)
   : reg({})
   , gameBoy(gb)
   , ime(false)
   , halted(false)
   , stopped(false)
   , interruptEnableRequested(false)
   , freezePC(false)
{
   reg.af = 0x01B0;
   reg.bc = 0x0013;
   reg.de = 0x00D8;
   reg.hl = 0x014D;
   reg.sp = 0xFFFE;
   reg.pc = 0x0100;
}

void CPU::step()
{
   DM_ASSERT(!stopped);

   if (halted && !gameBoy.isAnyInterruptActive())
   {
      gameBoy.machineCycle();
      return;
   }

   Operation operation = fetch();

   if (interruptEnableRequested)
   {
      ime = true;
      interruptEnableRequested = false;
   }

   if (is16BitOperation(operation))
   {
      execute16(operation);
   }
   else
   {
      execute8(operation);
   }
}

uint8_t CPU::readPC()
{
   return gameBoy.read(reg.pc++);
}

uint16_t CPU::readPC16()
{
   uint8_t low = readPC();
   uint8_t high = readPC();
   return (high << 8) | low;
}

void CPU::push(uint16_t value)
{
   reg.sp -= 2;
   gameBoy.write(reg.sp + 1, (value & 0xFF00) >> 8);
   gameBoy.write(reg.sp, value & 0x00FF);
}

uint16_t CPU::pop()
{
   uint8_t low = gameBoy.read(reg.sp);
   uint8_t high = gameBoy.read(reg.sp + 1);
   reg.sp += 2;
   return (high << 8) | low;
}

bool CPU::handleInterrupts()
{
   if (ime || halted)
   {
      if (gameBoy.isInterruptActive(Interrupt::VBlank))
      {
         return handleInterrupt(Interrupt::VBlank);
      }
      else if (gameBoy.isInterruptActive(Interrupt::LCDState))
      {
         return handleInterrupt(Interrupt::LCDState);
      }
      else if (gameBoy.isInterruptActive(Interrupt::Timer))
      {
         return handleInterrupt(Interrupt::Timer);
      }
      else if (gameBoy.isInterruptActive(Interrupt::Serial))
      {
         return handleInterrupt(Interrupt::Serial);
      }
      else if (gameBoy.isInterruptActive(Interrupt::Joypad))
      {
         return handleInterrupt(Interrupt::Joypad);
      }

      DM_ASSERT(!halted); // handleInterrupts() should only be called while halted if at least one interrupt is active
   }

   return false;
}

bool CPU::handleInterrupt(Interrupt interrupt)
{
   DM_ASSERT((ime || halted) && gameBoy.isInterruptActive(interrupt));

   bool wasHalted = halted;
   halted = false;

   if (wasHalted && !ime)
   {
      // The HALT state is left when an enabled interrupt occurs, no matter if the IME is enabled or not.
      // However, if IME is disabled the interrupt is not serviced.
      return false;
   }

   ime = false;
   interruptEnableRequested = false;
   gameBoy.clearInterruptRequest(interrupt);

   // two wait states
   gameBoy.machineCycle();
   gameBoy.machineCycle();

   // PC is pushed onto the stack
   push(reg.pc);

   // PC is set to the interrupt handler
   switch (interrupt)
   {
   case Interrupt::VBlank:
      reg.pc = 0x0040;
      break;
   case Interrupt::LCDState:
      reg.pc = 0x0048;
      break;
   case Interrupt::Timer:
      reg.pc = 0x0050;
      break;
   case Interrupt::Serial:
      reg.pc = 0x0058;
      break;
   case Interrupt::Joypad:
      reg.pc = 0x0060;
      break;
   }

   gameBoy.machineCycle(); // TODO Correct? https://github.com/Gekkio/mooneye-gb/blob/master/docs/accuracy.markdown suggests it should be grouped with the other 2 above

   return true;
}

DotMatrix::Operation CPU::fetch()
{
   uint8_t opcode = gameBoy.read(reg.pc);

   bool handledInterrupt = handleInterrupts();
   if (handledInterrupt)
   {
      opcode = gameBoy.read(reg.pc);
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
   if (operation.ins == Ins::PREFIX)
   {
      opcode = readPC();
      operation = kCBOperations[opcode];
   }

   return operation;
}

void CPU::execute8(Operation operation)
{
   static const uint16_t kHalfCarryMask = 0x0010;
   static const uint16_t kCarryMask = 0x0100;

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

   Operand param1(reg, gameBoy, operation.param1, imm8, imm16);
   Operand param2(reg, gameBoy, operation.param2, imm8, imm16);

   switch (operation.ins)
   {
   // Loads
   case Ins::LD:
   {
      // If one of the params is (C), the other param must be A
      DM_ASSERT((operation.param2 != Opr::DerefC || operation.param1 == Opr::A)
         && (operation.param1 != Opr::DerefC || operation.param2 == Opr::A));

      param1.write8(param2.read8());
      break;
   }
   case Ins::LDD:
   {
      execute8(Operation(Ins::LD, operation.param1, operation.param2, 0));

      --reg.hl;
      break;
   }
   case Ins::LDI:
   {
      execute8(Operation(Ins::LD, operation.param1, operation.param2, 0));

      ++reg.hl;
      break;
   }
   case Ins::LDH:
   {
      // Only valid params are (n) and A
      DM_ASSERT((operation.param1 == Opr::DerefImm8 && operation.param2 == Opr::A)
         || (operation.param1 == Opr::A && operation.param2 == Opr::DerefImm8));

      param1.write8(param2.read8());
      break;
   }

   // ALU
   case Ins::ADD:
   {
      DM_ASSERT(operation.param1 == Opr::A);

      uint8_t param1Val = param1.read8();
      uint8_t param2Val = param2.read8();
      uint16_t result = param1Val + param2Val;
      uint16_t carryBits = param1Val ^ param2Val ^ result;

      uint8_t result8 = static_cast<uint8_t>(result);
      param1.write8(result8);

      setFlag(Flag::Zero, result8 == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);
      break;
   }
   case Ins::ADC:
   {
      DM_ASSERT(operation.param1 == Opr::A);

      uint16_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t param1Val = param1.read8();
      uint8_t param2Val = param2.read8();
      uint16_t result = param1Val + param2Val + carryVal;
      uint16_t carryBits = param1Val ^ param2Val ^ carryVal ^ result;

      uint8_t result8 = static_cast<uint8_t>(result);
      param1.write8(result8);

      setFlag(Flag::Zero, result8 == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);
      break;
   }
   case Ins::SUB:
   {
      uint8_t param1Val = param1.read8();
      uint16_t result = reg.a - param1Val;
      uint16_t carryBits = reg.a ^ param1Val ^ result;

      reg.a = static_cast<uint8_t>(result);

      setFlag(Flag::Zero, reg.a == 0);
      setFlag(Flag::Sub, true);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);
      break;
   }
   case Ins::SBC:
   {
      DM_ASSERT(operation.param1 == Opr::A);

      uint16_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t param1Val = param1.read8();
      uint8_t param2Val = param2.read8();
      uint16_t result = param1Val - param2Val - carryVal;
      uint16_t carryBits = param1Val ^ param2Val ^ carryVal ^ result;

      uint8_t result8 = static_cast<uint8_t>(result);
      param1.write8(result8);

      setFlag(Flag::Zero, result8 == 0);
      setFlag(Flag::Sub, true);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);
      break;
   }
   case Ins::AND:
   {
      reg.a &= param1.read8();

      setFlag(Flag::Zero, reg.a == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, true);
      setFlag(Flag::Carry, false);
      break;
   }
   case Ins::OR:
   {
      reg.a |= param1.read8();

      setFlag(Flag::Zero, reg.a == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, false);
      break;
   }
   case Ins::XOR:
   {
      reg.a ^= param1.read8();

      setFlag(Flag::Zero, reg.a == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, false);
      break;
   }
   case Ins::CP:
   {
      uint8_t param1Val = param1.read8();
      uint16_t result = reg.a - param1Val;
      uint16_t carryBits = reg.a ^ param1Val ^ result;

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, true);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);
      break;
   }
   case Ins::INC:
   {
      uint8_t param1Val = param1.read8();
      uint16_t result = param1Val + 1;
      uint16_t carryBits = param1Val ^ 1 ^ result;

      uint8_t result8 = static_cast<uint8_t>(result);
      param1.write8(result8);

      setFlag(Flag::Zero, result8 == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      break;
   }
   case Ins::DEC:
   {
      uint8_t param1Val = param1.read8();
      uint16_t result = param1Val - 1;
      uint16_t carryBits = param1Val ^ 1 ^ result;

      uint8_t result8 = static_cast<uint8_t>(result);
      param1.write8(result8);

      setFlag(Flag::Zero, result8 == 0);
      setFlag(Flag::Sub, true);
      setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
      break;
   }

   // Miscellaneous
   case Ins::SWAP:
   {
      uint8_t param1Val = param1.read8();
      uint8_t result = ((param1Val & 0x0F) << 4) | ((param1Val & 0xF0) >> 4);
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, false);
      break;
   }
   case Ins::DAA:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      uint16_t temp = reg.a;

      if (!getFlag(Flag::Sub))
      {
         if (getFlag(Flag::HalfCarry) || (temp & 0x0F) > 9)
         {
            temp += 0x06;
         }

         if (getFlag(Flag::Carry) || (temp > 0x9F))
         {
            temp += 0x60;
         }
      }
      else
      {
         if (getFlag(Flag::HalfCarry))
         {
            temp = (temp - 6) & 0xFF;
         }

         if (getFlag(Flag::Carry))
         {
            temp -= 0x60;
         }
      }

      bool carry = getFlag(Flag::Carry) || (temp & 0x0100) == 0x0100;
      reg.a = temp & 0x00FF;

      setFlag(Flag::Zero, reg.a == 0);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, carry);
      break;
   }
   case Ins::CPL:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      reg.a = ~reg.a;

      setFlag(Flag::Sub, true);
      setFlag(Flag::HalfCarry, true);
      break;
   }
   case Ins::CCF:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, !getFlag(Flag::Carry));
      break;
   }
   case Ins::SCF:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, true);
      break;
   }
   case Ins::NOP:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      break;
   }
   case Ins::HALT:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      halted = true;
      if (!ime && gameBoy.isAnyInterruptActive())
      {
         // HALT bug
         freezePC = true;
      }
      break;
   }
   case Ins::STOP:
   {
      // STOP should be followed by 0x00 (treated here as an immediate)
      // DM_ASSERT(param1Val == 0x00); // TODO

      stopped = true;
      gameBoy.onCPUStopped();
      break;
   }
   case Ins::DI:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      ime = false;
      break;
   }
   case Ins::EI:
   {
      DM_ASSERT(operation.param1 == Opr::None && operation.param2 == Opr::None);

      interruptEnableRequested = true;
      break;
   }

   // Rotates and shifts
   case Ins::RLCA:
   {
      reg.a = (reg.a << 1) | (reg.a >> 7);

      setFlag(Flag::Zero, false);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, (reg.a & 0x01) != 0);
      break;
   }
   case Ins::RLA:
   {
      uint8_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t newCarryVal = reg.a & 0x80;

      reg.a = (reg.a << 1) | carryVal;

      setFlag(Flag::Zero, false);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::RRCA:
   {
      reg.a = (reg.a >> 1) | (reg.a << 7);

      setFlag(Flag::Zero, false);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, (reg.a & 0x80) != 0);
      break;
   }
   case Ins::RRA:
   {
      uint8_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t newCarryVal = reg.a & 0x01;

      reg.a = (reg.a >> 1) | (carryVal << 7);

      setFlag(Flag::Zero, false);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::RLC:
   {
      uint8_t param1Val = param1.read8();
      uint8_t result = (param1Val << 1) | (param1Val >> 7);
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, (result & 0x01) != 0);
      break;
   }
   case Ins::RL:
   {
      uint8_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t param1Val = param1.read8();
      uint8_t newCarryVal = param1Val & 0x80;

      uint8_t result = (param1Val << 1) | carryVal;
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::RRC:
   {
      uint8_t param1Val = param1.read8();
      uint8_t result = (param1Val >> 1) | (param1Val << 7);
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, (result & 0x80) != 0);
      break;
   }
   case Ins::RR:
   {
      uint8_t carryVal = getFlag(Flag::Carry) ? 1 : 0;
      uint8_t param1Val = param1.read8();
      uint8_t newCarryVal = param1Val & 0x01;

      uint8_t result = (param1Val >> 1) | (carryVal << 7);
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::SLA:
   {
      uint8_t param1Val = param1.read8();
      uint8_t newCarryVal = param1Val & 0x80;

      uint8_t result = param1Val << 1;
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::SRA:
   {
      uint8_t param1Val = param1.read8();
      uint8_t newCarryVal = param1Val & 0x01;

      uint8_t result = (param1Val >> 1) | (param1Val & 0x80);
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }
   case Ins::SRL:
   {
      uint8_t param1Val = param1.read8();
      uint8_t newCarryVal = param1Val & 0x01;

      uint8_t result = param1Val >> 1;
      param1.write8(result);

      setFlag(Flag::Zero, result == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, false);
      setFlag(Flag::Carry, newCarryVal != 0);
      break;
   }

   // Bit operations
   case Ins::BIT:
   {
      uint8_t mask = bitOprMask(operation.param1);

      setFlag(Flag::Zero, (param2.read8() & mask) == 0);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, true);
      break;
   }
   case Ins::SET:
   {
      uint8_t mask = bitOprMask(operation.param1);

      param2.write8(param2.read8() | mask);
      break;
   }
   case Ins::RES:
   {
      uint8_t mask = bitOprMask(operation.param1);

      param2.write8(param2.read8() & ~mask);
      break;
   }

   // Invalid instruction
   default:
   {
      DM_ASSERT(false, "Invalid 8-bit instruction: %hhu", Enum::cast(operation.ins));
      break;
   }
   }
}

void CPU::execute16(Operation operation)
{
   static const uint32_t kHalfCarryMask = 0x00001000;
   static const uint32_t kCarryMask = 0x00010000;

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

   Operand param1(reg, gameBoy, operation.param1, imm8, imm16);
   Operand param2(reg, gameBoy, operation.param2, imm8, imm16);

   switch (operation.ins)
   {
   // Loads
   case Ins::LD:
   {
      uint16_t param2Val = param2.read16();

      if (operation.param1 == Opr::DerefImm16)
      {
         DM_ASSERT(operation.param2 == Opr::SP);

         param1.write16(param2Val);
      }
      else
      {
         param1.write16(param2Val);

         if (operation.param2 == Opr::HL)
         {
            gameBoy.machineCycle();
         }
      }
      break;
   }
   case Ins::LDHL:
   {
      DM_ASSERT(operation.param1 == Opr::SP && operation.param2 == Opr::Imm8Signed);
      // Special case - uses one byte signed immediate value
      int8_t n = Math::reinterpretAsSigned(param2.read8());

      uint16_t param1Val = param1.read16();
      uint32_t result = param1Val + n;
      uint32_t carryBits = param1Val ^ n ^ result;

      reg.hl = static_cast<uint16_t>(result);

      // Special case - treat carry and half carry as if this was an 8 bit add
      setFlag(Flag::Zero, false);
      setFlag(Flag::Sub, false);
      setFlag(Flag::HalfCarry, (carryBits & 0x0010) != 0);
      setFlag(Flag::Carry, (carryBits & 0x0100) != 0);

      gameBoy.machineCycle();
      break;
   }
   case Ins::PUSH:
   {
      gameBoy.machineCycle();
      push(param1.read16());
      break;
   }
   case Ins::POP:
   {
      param1.write16(pop());
      break;
   }

   // ALU
   case Ins::ADD:
   {
      DM_ASSERT(operation.param1 == Opr::HL || operation.param1 == Opr::SP);

      if (operation.param1 == Opr::HL)
      {
         uint16_t param1Val = param1.read16();
         uint16_t param2Val = param2.read16();
         uint32_t result = param1Val + param2Val;
         uint32_t carryBits = param1Val ^ param2Val ^ result;

         param1.write16(static_cast<uint16_t>(result));

         setFlag(Flag::Sub, false);
         setFlag(Flag::HalfCarry, (carryBits & kHalfCarryMask) != 0);
         setFlag(Flag::Carry, (carryBits & kCarryMask) != 0);

         gameBoy.machineCycle();
      }
      else
      {
         DM_ASSERT(operation.param2 == Opr::Imm8Signed);

         // Special case - uses one byte signed immediate value
         int8_t n = Math::reinterpretAsSigned(param2.read8());

         uint16_t param1Val = param1.read16();
         uint32_t result = param1Val + n;
         uint32_t carryBits = param1Val ^ n ^ result;

         param1.write16(static_cast<uint16_t>(result));

         setFlag(Flag::Zero, false);
         setFlag(Flag::Sub, false);
         setFlag(Flag::HalfCarry, (carryBits & 0x0010) != 0);
         setFlag(Flag::Carry, (carryBits & 0x0100) != 0);

         gameBoy.machineCycle();
         gameBoy.machineCycle();
      }
      break;
   }
   case Ins::INC:
   {
      param1.write16(param1.read16() + 1);
      gameBoy.machineCycle();
      break;
   }
   case Ins::DEC:
   {
      param1.write16(param1.read16() - 1);
      gameBoy.machineCycle();
      break;
   }

   // Jumps
   case Ins::JP:
   {
      if (operation.param2 == Opr::None)
      {
         DM_ASSERT(operation.param1 == Opr::Imm16 || operation.param1 == Opr::HL);

         reg.pc = param1.read16();
         if (operation.param1 == Opr::Imm16)
         {
            gameBoy.machineCycle();
         }
      }
      else
      {
         if (evalJumpCondition(operation.param1, getFlag(Flag::Zero), getFlag(Flag::Carry)))
         {
            reg.pc = param2.read16();
            gameBoy.machineCycle();
         }
      }
      break;
   }
   case Ins::JR:
   {
      if (operation.param2 == Opr::None)
      {
         DM_ASSERT(operation.param1 == Opr::Imm8Signed);

         // Special case - uses one byte signed immediate value
         int8_t n = Math::reinterpretAsSigned(param1.read8());

         reg.pc += n;
         gameBoy.machineCycle();
      }
      else
      {
         DM_ASSERT(operation.param2 == Opr::Imm8Signed);

         // Special case - uses one byte signed immediate value
         int8_t n = Math::reinterpretAsSigned(param2.read8());

         if (evalJumpCondition(operation.param1, getFlag(Flag::Zero), getFlag(Flag::Carry)))
         {
            reg.pc += n;
            gameBoy.machineCycle();
         }
      }
      break;
   }

   // Calls
   case Ins::CALL:
   {
      if (operation.param2 == Opr::None)
      {
         gameBoy.machineCycle();
         push(reg.pc);
         reg.pc = param1.read16();
      }
      else
      {
         uint16_t param2Val = param2.read16();
         if (evalJumpCondition(operation.param1, getFlag(Flag::Zero), getFlag(Flag::Carry)))
         {
            gameBoy.machineCycle();
            push(reg.pc);
            reg.pc = param2Val;
         }
      }
      break;
   }

   // Restarts
   case Ins::RST:
   {
      gameBoy.machineCycle();
      push(reg.pc);
      reg.pc = 0x0000 + rstOffset(operation.param1);
      break;
   }

   // Returns
   case Ins::RET:
   {
      if (operation.param1 == Opr::None)
      {
         reg.pc = pop();
         gameBoy.machineCycle();
      }
      else
      {
         gameBoy.machineCycle();
         if (evalJumpCondition(operation.param1, getFlag(Flag::Zero), getFlag(Flag::Carry)))
         {
            reg.pc = pop();
            gameBoy.machineCycle();
         }
      }
      break;
   }
   case Ins::RETI:
   {
      execute16(Operation(Ins::RET, Opr::None, Opr::None, 0));

      // RETI doesn't delay enabling the IME like EI does
      ime = true;
      break;
   }

   // Invalid instruction
   default:
   {
      DM_ASSERT(false, "Invalid 16-bit instruction: %hhu", Enum::cast(operation.ins));
      break;
   }
   }
}

} // namespace DotMatrix
