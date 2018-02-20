#if GBC_RUN_TESTS

#include "FancyAssert.h"
#include "Log.h"

// Hack to allow us access to private members of the CPU
#define _ALLOW_KEYWORD_MACROS
#define private public
#include "GBC/CPU.h"
#include "GBC/Device.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include "GBC/Memory.h"
#include "GBC/Operations.h"

#include "Test/CPUTester.h"

#include <cstring>
#include <random>
#include <sstream>

namespace GBC {

namespace {

// Flag handling

template<typename T>
bool didCarryAdd(T before, T after) {
   return after < before;
}

template<typename T>
bool didCarrySub(T before, T after) {
   return after > before;
}

bool didHalfCarryAdd(uint8_t before, uint8_t after) {
   return (after & 0x0F) < (before & 0x0F);
}

bool didHalfCarryAdd(uint16_t before, uint16_t after) {
   return (after & 0x0FFF) < (before & 0x0FFF);
}

bool didHalfCarrySub(uint8_t before, uint8_t after) {
   return (after & 0x0F) > (before & 0x0F);
}

bool didHalfCarrySub(uint16_t before, uint16_t after) {
   return (after & 0x0FFF) > (before & 0x0FFF);
}

enum FlagOp {
   kKeep,
   kSet,
   kClear
};

FlagOp getFlagOp(char option, FlagOp defaultOp) {
   switch (option) {
      case '-':
         return kKeep;
      case '0':
         return kClear;
      case '1':
         return kSet;
      default:
         return defaultOp;
   }
}

uint8_t updateFlagRegister(uint8_t f, FlagOp z, FlagOp n, FlagOp h, FlagOp c) {
   if (z == kSet) {
      f |= CPU::kZero;
   } else if (z == kClear) {
      f &= ~CPU::kZero;
   }

   if (n == kSet) {
      f |= CPU::kSub;
   } else if (n == kClear) {
      f &= ~CPU::kSub;
   }

   if (h == kSet) {
      f |= CPU::kHalfCarry;
   } else if (h == kClear) {
      f &= ~CPU::kHalfCarry;
   }

   if (c == kSet) {
      f |= CPU::kCarry;
   } else if (c == kClear) {
      f &= ~CPU::kCarry;
   }

   return f;
}

#define UPDATE_FLAGS(_reg_, _znhc_)\
{\
   bool add = _znhc_[1] != '1';\
   FlagOp zero = getFlagOp(_znhc_[0], final.cpu.reg._reg_ == 0 ? kSet : kClear);\
   FlagOp sub = getFlagOp(_znhc_[1], add ? kClear : kSet);\
   FlagOp halfCarry = getFlagOp(_znhc_[2], (add ? didHalfCarryAdd(initial.cpu.reg._reg_, final.cpu.reg._reg_) : didHalfCarrySub(initial.cpu.reg._reg_, final.cpu.reg._reg_)) ? kSet : kClear);\
   FlagOp carry = getFlagOp(_znhc_[3], (add ? didCarryAdd(initial.cpu.reg._reg_, final.cpu.reg._reg_) : didCarrySub(initial.cpu.reg._reg_, final.cpu.reg._reg_)) ? kSet : kClear);\
   final.cpu.reg.f = updateFlagRegister(initial.cpu.reg.f, zero, sub, halfCarry, carry);\
}

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

bool usesImm8(Operation operation) {
   return operation.param1 == Opr::kImm8 || operation.param2 == Opr::kImm8
      || operation.param1 == Opr::kDrefImm8 || operation.param2 == Opr::kDrefImm8
      || operation.param1 == Opr::kImm8Signed || operation.param2 == Opr::kImm8Signed;
}

bool usesImm16(Operation operation) {
   return operation.param1 == Opr::kImm16 || operation.param2 == Opr::kImm16
      || operation.param1 == Opr::kDrefImm16 || operation.param2 == Opr::kDrefImm16;
}

bool miscFieldsMatch(const Device& first, const Device& second) {
   return first.cpu.ime == second.cpu.ime && first.totalCycles == second.totalCycles && first.cpu.halted == second.cpu.halted
      && first.cpu.stopped == second.cpu.stopped && first.cpu.interruptEnableRequested == second.cpu.interruptEnableRequested
      && first.cpu.freezePC == second.cpu.freezePC;
}

void dumpFields(const Device& device, const char* name = nullptr) {
   std::ostringstream out;
   out << "Fields";
   if (name) {
      out << "\n" << name;
   }
   out << "\nIME: " << std::boolalpha << device.cpu.ime
      << "\ncycles: " << device.totalCycles
      << "\nhalted: " << device.cpu.halted
      << "\nstopped: " << device.cpu.stopped
      << "\ninterruptEnableRequested: " << device.cpu.interruptEnableRequested
      << "\nfreezePC: " << device.cpu.freezePC << "\n";
   LOG_INFO(out.str());
}

void dumpRegisters(const CPU::Registers& reg, const char* name = nullptr) {
   std::ostringstream out;
   out << "Registers";
   if (name) {
      out << "\n" << name;
   }
   out << "\na: " << hex(reg.a) << " f: " << hex(reg.f) << " af: " << hex(reg.af)
      << "\nb: " << hex(reg.b) << " c: " << hex(reg.c) << " bc: " << hex(reg.bc)
      << "\nd: " << hex(reg.d) << " e: " << hex(reg.e) << " de: " << hex(reg.de)
      << "\nh: " << hex(reg.h) << " l: " << hex(reg.l) << " hl: " << hex(reg.hl)
      << "\nsp: " << hex(reg.sp) << " pc: " << hex(reg.pc) << "\n";
   LOG_INFO(out.str());
}

void dumpMem(const Memory& mem, uint16_t start, uint16_t end, const char* name = nullptr) {
   std::ostringstream out;
   out << "Memory";
   if (name) {
      out << "\n" << name;
   }
   for (uint16_t i = start; i < end; ++i) {
      if (i % 8 == 0) {
         out << "\n" << hex(i) << " |";
      }
      out << " " << hex(mem.read(i));
   }
   out << "\n";
   LOG_INFO(out.str());
}

void prepareInitial(Device& device, const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed) {
   std::default_random_engine engine(seed);
   std::uniform_int_distribution<uint16_t> distribution16;

   std::uniform_int_distribution<uint16_t> distribution8Hack; // Unfortunately, std::uniform_int_distribution<uint8_t> is not supported
   auto distribution8 = [&distribution8Hack](std::default_random_engine& engine) {
      return distribution8Hack(engine) & 0x00FF;
   };

   const uint8_t kInitialA = randomizeData ? distribution8(engine) : 0x01;
   const uint8_t kInitialF = (randomizeData ? distribution8(engine) : 0x00) & 0xF0;
   const uint8_t kInitialB = randomizeData ? distribution8(engine) : 0x03;
   const uint8_t kInitialC = randomizeData ? distribution8(engine) : 0x04;
   const uint8_t kInitialD = randomizeData ? distribution8(engine) : 0x05;
   const uint8_t kInitialE = randomizeData ? distribution8(engine) : 0x06;
   const uint8_t kInitialH = randomizeData ? distribution8(engine) : 0x07;
   const uint8_t kInitialL = randomizeData ? distribution8(engine) : 0x08;

   const uint16_t kInitialSP = randomizeData ? distribution16(engine) : 0xFFFE;
   const uint16_t kInitialPC = randomizeData ? distribution16(engine) : 0x0100;

   const uint8_t kInitialMem1 = randomizeData ? distribution8(engine) : 0xAB;
   const uint8_t kInitialMem2 = randomizeData ? distribution8(engine) : 0xCD;

   device.cpu.reg.a = kInitialA;
   device.cpu.reg.f = kInitialF; // TODO Should the CPU ensure the low bits are never used?
   device.cpu.reg.b = kInitialB;
   device.cpu.reg.c = kInitialC;
   device.cpu.reg.d = kInitialD;
   device.cpu.reg.e = kInitialE;
   device.cpu.reg.h = kInitialH;
   device.cpu.reg.l = kInitialL;

   device.cpu.reg.sp = kInitialSP;
   device.cpu.reg.pc = kInitialPC;

   uint16_t pcOffset = 0;
   for (const CPUTest& test : testGroup) {
      Operation operation = kOperations[test.opcode];

      device.memory.write(device.cpu.reg.pc + pcOffset++, test.opcode);

      if (usesImm8(operation)) {
         device.memory.write(device.cpu.reg.pc + pcOffset++, kInitialMem1);
      } else if (usesImm16(operation)) {
         device.memory.write(device.cpu.reg.pc + pcOffset++, kInitialMem1);
         device.memory.write(device.cpu.reg.pc + pcOffset++, kInitialMem2);
      }
   }
}

void prepareFinal(Device& device, const CPUTestGroup& testGroup, size_t testIndex, bool randomizeData, uint16_t seed) {
   prepareInitial(device, testGroup, randomizeData, seed);

   for (size_t i = 0; i <= testIndex; ++i) {
      const CPUTest& test = testGroup[i];

      Operation operation = kOperations[test.opcode];
      device.totalCycles += operation.cycles;

      ++device.cpu.reg.pc;
      if (usesImm8(operation)) {
         ++device.cpu.reg.pc;
      } else if (usesImm16(operation)) {
         device.cpu.reg.pc += 2;
      }
   }
}

uint16_t read16(Device& device, uint16_t address) {
   uint8_t low = device.memory.read(address);
   uint8_t high = device.memory.read(address + 1);
   return ((high << 8) | low);
}

uint8_t imm8(Device& device) {
   return device.memory.read(device.cpu.reg.pc + 1);
}

uint16_t imm16(Device& device) {
   return read16(device, device.cpu.reg.pc + 1);
}

} // namespace

CPUTester::CPUTester() {
   init();
}

void CPUTester::runTests(bool randomizeData) {
   uint16_t seed = 0x0000;
   if (randomizeData) {
      std::random_device rd;
      std::default_random_engine engine(rd());
      std::uniform_int_distribution<uint16_t> distribution;

      seed = distribution(engine);
   }

   for (const CPUTestGroup& testGroup : testGroups) {
      runTestGroup(testGroup, randomizeData, seed);
   }
}

void CPUTester::runTestGroup(const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed) {
   Device device;
   Device finalDevice;
   device.memory.boot = finalDevice.memory.boot = Boot::kNotBooting; // Don't run the bootstrap program
   device.ignoreMachineCycles = finalDevice.ignoreMachineCycles = true;

   prepareInitial(device, testGroup, randomizeData, seed);

   for (size_t i = 0; i < testGroup.size(); ++i) {
      prepareFinal(finalDevice, testGroup, i, randomizeData, seed);
      runTest(device, finalDevice, testGroup[i]);
   }
}

void CPUTester::runTest(Device& device, Device& finalDevice, const CPUTest& test) {
   test.testSetupFunction(device, finalDevice);

   device.ignoreMachineCycles = false;
   device.cpu.tick();
   device.ignoreMachineCycles = true;

   bool fieldsMatch = miscFieldsMatch(finalDevice, device);
   bool registersMatch = memcmp(&finalDevice.cpu.reg, &device.cpu.reg, sizeof(CPU::Registers)) == 0;
   bool memoryMatches = true;
   uint16_t mismatchLocation = 0;
   for (uint16_t i = 0, j = 0; i >= j; j = i++) {
      if (device.cpu.mem.read(i) != finalDevice.cpu.mem.read(i)) {
         memoryMatches = false;
         mismatchLocation = i;
         break;
      }
   }

   if (!fieldsMatch || !registersMatch || !memoryMatches) {
      std::ostringstream out;
      out << "Opcode: " << hex(test.opcode);
      LOG_INFO(out.str());
   }

   if (!fieldsMatch) {
      dumpFields(finalDevice, "Expected");
      dumpFields(device, "Actual");
   }

   if (!registersMatch) {
      dumpRegisters(finalDevice.cpu.reg, "Expected");
      dumpRegisters(device.cpu.reg, "Actual");
   }

   if (!memoryMatches) {
      uint16_t memoryLine = mismatchLocation / 8;
      uint16_t previousLine = memoryLine > 0 ? memoryLine - 1 : 0;
      uint16_t nextLine = memoryLine < 8192 ? memoryLine + 1 : 8192;

      dumpMem(finalDevice.memory, previousLine * 8, nextLine * 8 + 8, "Expected");
      dumpMem(device.memory, previousLine * 8, nextLine * 8 + 8, "Actual");
   }

   ASSERT(fieldsMatch && registersMatch && memoryMatches);
}

void CPUTester::init() {
   testGroups = {
      {
         {
            0x00, // NOP
            [](Device& initial, Device& final) {
            }
         }
      },
      {
         {
            0x01, // LD BC,d16
            [](Device& initial, Device& final) {
               final.cpu.reg.bc = imm16(initial);
            }
         }
      },
      {
         {
            0x02, // LD (BC),A
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.bc, initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0x03, // INC BC
            [](Device& initial, Device& final) {
               ++final.cpu.reg.bc;
            }
         }
      },
      {
         {
            0x04, // INC B
            [](Device& initial, Device& final) {
               ++final.cpu.reg.b;

               UPDATE_FLAGS(b, "Z0H-")
            }
         }
      },
      {
         {
            0x05, // DEC B
            [](Device& initial, Device& final) {
               --final.cpu.reg.b;

               UPDATE_FLAGS(b, "Z1H-")
            }
         }
      },
      {
         {
            0x06, // LD B,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.b = imm8(initial);
            }
         }
      },
      {
         {
            0x07, // RLCA
            [](Device& initial, Device& final) {
               final.cpu.reg.a = (initial.cpu.reg.a << 1) | (initial.cpu.reg.a >> 7);

               UPDATE_FLAGS(c, "000C")
               // Old bit 7 to carry
               if (initial.cpu.reg.a & 0x80) {
                  final.cpu.reg.f |= CPU::kCarry;
               } else {
                  final.cpu.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x08, // LD (a16),SP
            [](Device& initial, Device& final) {
               uint8_t spLow = initial.cpu.reg.sp & 0xFF;
               uint8_t spHigh = (initial.cpu.reg.sp >> 8) & 0xFF;
               uint16_t a16 = imm16(initial);

               final.memory.write(a16, spLow);
               final.memory.write(a16 + 1, spHigh);
            }
         }
      },
      {
         {
            0x09, // ADD HL,BC
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl + initial.cpu.reg.bc;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x0A, // LD A,(BC)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.bc) | (initial.memory.read(initial.cpu.reg.bc + 1) << 8);
            }
         }
      },
      {
         {
            0x0B, // DEC BC
            [](Device& initial, Device& final) {
               final.cpu.reg.bc = initial.cpu.reg.bc - 1;
            }
         }
      },
      {
         {
            0x0C, // INC C
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.c + 1;

               UPDATE_FLAGS(c, "Z0H-")
            }
         }
      },
      {
         {
            0x0D, // DEC C
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.c - 1;

               UPDATE_FLAGS(c, "Z1H-")
            }
         }
      },
      {
         {
            0x0E, // LD C,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.c = imm8(initial);
            }
         }
      },
      {
         {
            0x0F, // RRCA
            [](Device& initial, Device& final) {
               final.cpu.reg.a = (initial.cpu.reg.a >> 1) | (initial.cpu.reg.a << 7);

               UPDATE_FLAGS(c, "000C")
               // Old bit 0 to carry
               if (initial.cpu.reg.a & 0x01) {
                  final.cpu.reg.f |= CPU::kCarry;
               } else {
                  final.cpu.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },

      {
         {
            0x10, // STOP 0
            [](Device& initial, Device& final) {
               initial.memory.write(initial.cpu.reg.pc + 1, 0x00);
               final.memory.write(initial.cpu.reg.pc + 1, 0x00);

               final.cpu.stopped = true;
            }
         }
      },
      {
         {
            0x11, // LD DE,d16
            [](Device& initial, Device& final) {
               final.cpu.reg.de = imm16(initial);
            }
         }
      },
      {
         {
            0x12, // LD (DE),A
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.de, initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0x13, // INC DE
            [](Device& initial, Device& final) {
               ++final.cpu.reg.de;
            }
         }
      },
      {
         {
            0x14, // INC D
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.d + 1;

               UPDATE_FLAGS(d, "Z0H-")
            }
         }
      },
      {
         {
            0x15, // DEC D
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.d - 1;

               UPDATE_FLAGS(d, "Z1H-")
            }
         }
      },
      {
         {
            0x16, // LD D,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.d = imm8(initial);
            }
         }
      },
      {
         {
            0x17, // RLA
            [](Device& initial, Device& final) {
               final.cpu.reg.a = (initial.cpu.reg.a << 1) | ((initial.cpu.reg.f & CPU::kCarry) ? 0x01 : 0x00);

               UPDATE_FLAGS(c, "000C")
               // Old bit 7 to carry
               if (initial.cpu.reg.a & 0x80) {
                  final.cpu.reg.f |= CPU::kCarry;
               } else {
                  final.cpu.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x18, // JR r8
            [](Device& initial, Device& final) {
               uint8_t offset = imm8(initial);
               int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
               final.cpu.reg.pc = initial.cpu.reg.pc + 2 + signedOffset;
            }
         }
      },
      {
         {
            0x19, // ADD HL,DE
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl + initial.cpu.reg.de;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x1A, // LD A,(DE)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.de);
            }
         }
      },
      {
         {
            0x1B, // DEC DE
            [](Device& initial, Device& final) {
               final.cpu.reg.de = initial.cpu.reg.de - 1;
            }
         }
      },
      {
         {
            0x1C, // INC E
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.e + 1;

               UPDATE_FLAGS(e, "Z0H-")
            }
         }
      },
      {
         {
            0x1D, // DEC E
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.e - 1;

               UPDATE_FLAGS(e, "Z1H-")
            }
         }
      },
      {
         {
            0x1E, // LD E,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.e = imm8(initial);
            }
         }
      },
      {
         {
            0x1F, // RRA
            [](Device& initial, Device& final) {
               final.cpu.reg.a = (initial.cpu.reg.a >> 1) | ((initial.cpu.reg.f & CPU::kCarry) ? 0x80 : 0x00);

               UPDATE_FLAGS(c, "000C")
               // Old bit 0 to carry
               if (initial.cpu.reg.a & 0x01) {
                  final.cpu.reg.f |= CPU::kCarry;
               } else {
                  final.cpu.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },

      {
         {
            0x20, // JR NZ,r8
            [](Device& initial, Device& final) {
               if ((initial.cpu.reg.f & CPU::kZero) == 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.cpu.reg.pc = initial.cpu.reg.pc + 2 + signedOffset;
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0x21, // LD HL,d16
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = imm16(initial);
            }
         }
      },
      {
         {
            0x22, // LD (HL+),A
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.a);
               final.cpu.reg.hl = initial.cpu.reg.hl + 1;
            }
         }
      },
      {
         {
            0x23, // INC HL
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl + 1;
            }
         }
      },
      {
         {
            0x24, // INC H
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.h + 1;

               UPDATE_FLAGS(h, "Z0H-")
            }
         }
      },
      {
         {
            0x25, // DEC H
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.h - 1;

               UPDATE_FLAGS(h, "Z1H-")
            }
         }
      },
      {
         {
            0x26, // LD H,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.h = imm8(initial);
            }
         }
      },
      {
         {
            0x27, // DAA
            [](Device& initial, Device& final) {
               uint16_t val = initial.cpu.reg.a;

               if (initial.cpu.reg.f & CPU::kSub) {
                  if (initial.cpu.reg.f & CPU::kHalfCarry) {
                     val = (val - 0x06) & 0xFF;
                  }
                  if (initial.cpu.reg.f & CPU::kCarry) {
                     val -= 0x60;
                  }
               } else {
                  if (initial.cpu.reg.f & CPU::kHalfCarry || (val & 0x0F) > 9) {
                     val += 0x06;
                  }
                  if (initial.cpu.reg.f & CPU::kCarry || val > 0x9F) {
                     val += 0x60;
                  }
               }

               final.cpu.reg.a = static_cast<uint8_t>(val);

               bool carryWasSet = initial.cpu.getFlag(CPU::kCarry);
               UPDATE_FLAGS(a, "Z-0C")
               // See DAA table
               if (carryWasSet || (val & 0x0100) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               } else {
                  final.cpu.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x28, // JR Z,r8
            [](Device& initial, Device& final) {
               if ((initial.cpu.reg.f & CPU::kZero) != 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.cpu.reg.pc = initial.cpu.reg.pc + 2 + signedOffset;
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0x29, // ADD HL,HL
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl + initial.cpu.reg.hl;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x2A, // LD A,(HL+)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl);
               final.cpu.reg.hl = initial.cpu.reg.hl + 1;
            }
         }
      },
      {
         {
            0x2B, // DEC HL
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl - 1;
            }
         }
      },
      {
         {
            0x2C, // INC L
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.l + 1;

               UPDATE_FLAGS(l, "Z0H-")
            }
         }
      },
      {
         {
            0x2D, // DEC L
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.l - 1;

               UPDATE_FLAGS(l, "Z1H-")
            }
         }
      },
      {
         {
            0x2E, // LD L,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.l = imm8(initial);
            }
         }
      },
      {
         {
            0x2F, // CPL
            [](Device& initial, Device& final) {
               final.cpu.reg.a = ~initial.cpu.reg.a;

               UPDATE_FLAGS(a, "-11-")
            }
         }
      },

      {
         {
            0x30, // JR NC,r8
            [](Device& initial, Device& final) {
               if ((initial.cpu.reg.f & CPU::kCarry) == 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.cpu.reg.pc = initial.cpu.reg.pc + 2 + signedOffset;
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0x31, // LD SP,d16
            [](Device& initial, Device& final) {
               final.cpu.reg.sp = imm16(initial);
            }
         }
      },
      {
         {
            0x32, // LD (HL-),A
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.a);
               final.cpu.reg.hl = initial.cpu.reg.hl - 1;
            }
         }
      },
      {
         {
            0x33, // INC SP
            [](Device& initial, Device& final) {
               final.cpu.reg.sp = initial.cpu.reg.sp + 1;
            }
         }
      },
      {
         {
            0x34, // INC (HL)
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.memory.read(initial.cpu.reg.hl) + 1);

               // hacky way to make macro still work - use a to store the result temporarily
               uint8_t initialA = initial.cpu.reg.a;
               initial.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl);
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl) + 1;
               UPDATE_FLAGS(a, "Z0H-")
               initial.cpu.reg.a = final.cpu.reg.a = initialA;
            }
         }
      },
      {
         {
            0x35, // DEC (HL)
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.memory.read(initial.cpu.reg.hl) - 1);

               // hacky way to make macro still work - use a to store the result temporarily
               uint8_t initialA = initial.cpu.reg.a;
               initial.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl);
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl) - 1;
               UPDATE_FLAGS(a, "Z1H-")
               initial.cpu.reg.a = final.cpu.reg.a = initialA;
            }
         }
      },
      {
         {
            0x36, // LD (HL),d8
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, imm8(initial));
            }
         }
      },
      {
         {
            0x37, // SCF
            [](Device& initial, Device& final) {
               UPDATE_FLAGS(a, "-001")
            }
         }
      },
      {
         {
            0x38, // JR C,r8
            [](Device& initial, Device& final) {
               if ((initial.cpu.reg.f & CPU::kCarry) != 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.cpu.reg.pc = initial.cpu.reg.pc + 2 + signedOffset;
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0x39, // ADD HL,SP
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = initial.cpu.reg.hl + initial.cpu.reg.sp;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x3A, // LD A,(HL-)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl);
               final.cpu.reg.hl = initial.cpu.reg.hl - 1;
            }
         }
      },
      {
         {
            0x3B, // DEC SP
            [](Device& initial, Device& final) {
               final.cpu.reg.sp = initial.cpu.reg.sp - 1;
            }
         }
      },
      {
         {
            0x3C, // INC A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + 1;
               UPDATE_FLAGS(a, "Z0H-")
            }
         }
      },
      {
         {
            0x3D, // DEC A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - 1;
               UPDATE_FLAGS(a, "Z1H-")
            }
         }
      },
      {
         {
            0x3E, // LD A,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = imm8(initial);
            }
         }
      },
      {
         {
            0x3F, // CCF
            [](Device& initial, Device& final) {
               UPDATE_FLAGS(a, "-00C")
               // Set carry to flipped initial carry
               final.cpu.reg.f = (final.cpu.reg.f & 0xE0) | ((initial.cpu.reg.f & CPU::kCarry) ^ CPU::kCarry);
            }
         }
      },

      {
         {
            0x40, // LD B,B
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x41, // LD B,C
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x42, // LD B,D
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x43, // LD B,E
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x44, // LD B,H
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x45, // LD B,L
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x46, // LD B,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x47, // LD B,A
            [](Device& initial, Device& final) {
               final.cpu.reg.b = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0x48, // LD C,B
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x49, // LD C,C
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x4A, // LD C,D
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x4B, // LD C,E
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x4C, // LD C,H
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x4D, // LD C,L
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x4E, // LD C,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x4F, // LD C,A
            [](Device& initial, Device& final) {
               final.cpu.reg.c = initial.cpu.reg.a;
            }
         }
      },

      {
         {
            0x50, // LD D,B
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x51, // LD D,C
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x52, // LD D,D
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x53, // LD D,E
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x54, // LD D,H
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x55, // LD D,L
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x56, // LD D,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x57, // LD D,A
            [](Device& initial, Device& final) {
               final.cpu.reg.d = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0x58, // LD E,B
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x59, // LD E,C
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x5A, // LD E,D
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x5B, // LD E,E
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x5C, // LD E,H
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x5D, // LD E,L
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x5E, // LD E,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x5F, // LD E,A
            [](Device& initial, Device& final) {
               final.cpu.reg.e = initial.cpu.reg.a;
            }
         }
      },

      {
         {
            0x60, // LD H,B
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x61, // LD H,C
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x62, // LD H,D
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x63, // LD H,E
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x64, // LD H,H
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x65, // LD H,L
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x66, // LD H,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x67, // LD H,A
            [](Device& initial, Device& final) {
               final.cpu.reg.h = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0x68, // LD L,B
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x69, // LD L,C
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x6A, // LD L,D
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x6B, // LD L,E
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x6C, // LD L,H
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x6D, // LD L,L
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x6E, // LD L,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x6F, // LD L,A
            [](Device& initial, Device& final) {
               final.cpu.reg.l = initial.cpu.reg.a;
            }
         }
      },

      {
         {
            0x70, // LD (HL),B
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.b);
            }
         }
      },
      {
         {
            0x71, // LD (HL),C
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.c);
            }
         }
      },
      {
         {
            0x72, // LD (HL),D
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.d);
            }
         }
      },
      {
         {
            0x73, // LD (HL),E
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.e);
            }
         }
      },
      {
         {
            0x74, // LD (HL),H
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.h);
            }
         }
      },
      {
         {
            0x75, // LD (HL),L
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.l);
            }
         }
      },
      {
         {
            0x76, // HALT
            [](Device& initial, Device& final) {
               final.cpu.halted = true;
            }
         }
      },
      {
         {
            0x77, // LD (HL),A
            [](Device& initial, Device& final) {
               final.memory.write(initial.cpu.reg.hl, initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0x78, // LD A,B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.b;
            }
         }
      },
      {
         {
            0x79, // LD A,C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.c;
            }
         }
      },
      {
         {
            0x7A, // LD A,D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.d;
            }
         }
      },
      {
         {
            0x7B, // LD A,E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.e;
            }
         }
      },
      {
         {
            0x7C, // LD A,H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.h;
            }
         }
      },
      {
         {
            0x7D, // LD A,L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.l;
            }
         }
      },
      {
         {
            0x7E, // LD A,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(initial.cpu.reg.hl);
            }
         }
      },
      {
         {
            0x7F, // LD A,A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },

      {
         {
            0x80, // ADD A,B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x81, // ADD A,C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x82, // ADD A,D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x83, // ADD A,E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x84, // ADD A,H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x85, // ADD A,L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x86, // ADD A,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x87, // ADD A,A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z0HC")
            }
         }
      },
      {
         {
            0x88, // ADC A,B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.b + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.b + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.b & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x89, // ADC A,C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.c + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.c + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.c & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8A, // ADC A,D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.d + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.d + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.d & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8B, // ADC A,E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.e + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.e + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.e & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8C, // ADC A,H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.h + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.h + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.h & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8D, // ADC A,L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.l + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.l + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.l & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8E, // ADC A,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.memory.read(initial.cpu.reg.hl) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.memory.read(initial.cpu.reg.hl) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.memory.read(initial.cpu.reg.hl) & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x8F, // ADC A,A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + initial.cpu.reg.a + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.a + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.a & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },

      {
         {
            0x90, // SUB B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x91, // SUB C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x92, // SUB D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x93, // SUB E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x94, // SUB H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x95, // SUB L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x96, // SUB (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x97, // SUB A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z1HC")
            }
         }
      },
      {
         {
            0x98, // SBC A,B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.b - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.b + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.b & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x99, // SBC A,C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.c - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.c + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.c & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9A, // SBC A,D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.d - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.d + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.d & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9B, // SBC A,E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.e - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.e + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.e & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9C, // SBC A,H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.h - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.h + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.h & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9D, // SBC A,L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.l - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.l + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.l & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9E, // SBC A,(HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.memory.read(initial.cpu.reg.hl) - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.memory.read(initial.cpu.reg.hl) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.memory.read(initial.cpu.reg.hl) & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0x9F, // SBC A,A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.a - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (initial.cpu.reg.a + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((initial.cpu.reg.a & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },

      {
         {
            0xA0, // AND B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA1, // AND C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA2, // AND D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA3, // AND E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA4, // AND H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA5, // AND L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA6, // AND (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA7, // AND A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z010")
            }
         }
      },
      {
         {
            0xA8, // XOR B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xA9, // XOR C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAA, // XOR D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAB, // XOR E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAC, // XOR H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAD, // XOR L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAE, // XOR (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xAF, // XOR A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },

      {
         {
            0xB0, // OR B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB1, // OR C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB2, // OR D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB3, // OR E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB4, // OR H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB5, // OR L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB6, // OR (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB7, // OR A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xB8, // CP B
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.b;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xB9, // CP C
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.c;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBA, // CP D
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.d;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBB, // CP E
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.e;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBC, // CP H
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.h;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBD, // CP L
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.l;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBE, // CP (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.memory.read(initial.cpu.reg.hl);

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xBF, // CP A
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - initial.cpu.reg.a;

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },

      {
         {
            0xC0, // RET NZ
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kZero)) {
                  final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
                  final.cpu.reg.sp += 2;
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xC1, // POP BC
            [](Device& initial, Device& final) {
               final.cpu.reg.bc = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.sp += 2;
            }
         }
      },
      {
         {
            0xC2, // JP NZ,a16
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kZero)) {
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0xC3, // JP a16
            [](Device& initial, Device& final) {
               final.cpu.reg.pc = imm16(initial);
            }
         }
      },
      {
         {
            0xC4, // CALL NZ,a16
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kZero)) {
                  final.cpu.reg.sp -= 2;
                  final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
                  final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xC5, // PUSH BC
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, initial.cpu.reg.c);
               final.memory.write(final.cpu.reg.sp + 1, initial.cpu.reg.b);
            }
         }
      },
      {
         {
            0xC6, // ADD A,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + imm8(initial);

               UPDATE_FLAGS(a, "Z0HC");
            }
         }
      },
      {
         {
            0xC7, // RST 00H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0000;
            }
         }
      },
      {
         {
            0xC8, // RET Z
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kZero) {
                  final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
                  final.cpu.reg.sp += 2;
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xC9, // RET
            [](Device& initial, Device& final) {
               final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.sp += 2;
            }
         }
      },
      {
         {
            0xCA, // JP Z,a16
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kZero) {
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      /*{
         {
            0xCB, // PREFIX CB
            [](Device& initial, Device& final) {
            }
         }
      },*/
      {
         {
            0xCC, // CALL Z,a16
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kZero) {
                  final.cpu.reg.sp -= 2;
                  final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
                  final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xCD, // CALL a16
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = imm16(initial);
            }
         }
      },
      {
         {
            0xCE, // ADC A,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a + imm8(initial) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z0HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (imm8(initial) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((imm8(initial) & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0xCF, // RST 08H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0008;
            }
         }
      },

      {
         {
            0xD0, // RET NC
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kCarry)) {
                  final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
                  final.cpu.reg.sp += 2;
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xD1, // POP DE
            [](Device& initial, Device& final) {
               final.cpu.reg.de = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.sp += 2;
            }
         }
      },
      {
         {
            0xD2, // JP NC,a16
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kCarry)) {
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0xD4, // CALL NC,a16
            [](Device& initial, Device& final) {
               if (!(initial.cpu.reg.f & CPU::kCarry)) {
                  final.cpu.reg.sp -= 2;
                  final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
                  final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xD5, // PUSH DE
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, initial.cpu.reg.e);
               final.memory.write(final.cpu.reg.sp + 1, initial.cpu.reg.d);
            }
         }
      },
      {
         {
            0xD6, // SUB d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - imm8(initial);

               UPDATE_FLAGS(a, "Z1HC");
            }
         }
      },
      {
         {
            0xD7, // RST 10H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0010;
            }
         }
      },
      {
         {
            0xD8, // RET C
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kCarry) {
                  final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
                  final.cpu.reg.sp += 2;
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xD9, // RETI
            [](Device& initial, Device& final) {
               final.cpu.reg.pc = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.sp += 2;

               // RETI doesn't delay enabling the IME like EI does
               final.cpu.ime = true;
            }
         }
      },
      {
         {
            0xDA, // JP C,a16
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kCarry) {
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 4;
               }
            }
         }
      },
      {
         {
            0xDC, // CALL C,a16
            [](Device& initial, Device& final) {
               if (initial.cpu.reg.f & CPU::kCarry) {
                  final.cpu.reg.sp -= 2;
                  final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
                  final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
                  final.cpu.reg.pc = imm16(initial);
               } else {
                  final.totalCycles -= 12;
               }
            }
         }
      },
      {
         {
            0xDE, // SBC A,d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - imm8(initial) - ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0);

               UPDATE_FLAGS(a, "Z1HC")

               // Edge cases not handled by UPDATE_FLAGS macro - value wraps all the way around
               if (imm8(initial) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0100) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
               if ((imm8(initial) & 0x0F) + ((initial.cpu.reg.f & CPU::kCarry) ? 1 : 0) == 0x0010) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
            }
         }
      },
      {
         {
            0xDF, // RST 18H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0018;
            }
         }
      },

      {
         {
            0xE0, // LDH (a8),A
            [](Device& initial, Device& final) {
               final.memory.write(0xFF00 + imm8(initial), initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0xE1, // POP HL
            [](Device& initial, Device& final) {
               final.cpu.reg.hl = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.sp += 2;
            }
         }
      },
      {
         {
            0xE2, // LD (C),A
            [](Device& initial, Device& final) {
               final.memory.write(0xFF00 + initial.cpu.reg.c, initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0xE5, // PUSH HL
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, initial.cpu.reg.l);
               final.memory.write(final.cpu.reg.sp + 1, initial.cpu.reg.h);
            }
         }
      },
      {
         {
            0xE6, // AND d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a & imm8(initial);

               UPDATE_FLAGS(a, "Z010");
            }
         }
      },
      {
         {
            0xE7, // RST 20H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0020;
            }
         }
      },
      {
         {
            0xE8, // ADD SP,r8
            [](Device& initial, Device& final) {
               uint8_t offset = imm8(initial);
               int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
               final.cpu.reg.sp = initial.cpu.reg.sp + signedOffset;

               // sp is treated as an 8-bit register for the carry and half carry flags (special case)
               // UPDATE_FLAGS(sp, "00HC")
               uint8_t sp8 = static_cast<uint8_t>(initial.cpu.reg.sp);
               uint8_t finalSp8 = static_cast<uint8_t>(final.cpu.reg.sp);
               FlagOp halfCarry = didHalfCarryAdd(sp8, finalSp8) ? kSet : kClear;
               FlagOp carry = didCarryAdd(sp8, finalSp8) ? kSet : kClear;
               final.cpu.reg.f = updateFlagRegister(initial.cpu.reg.f, kClear, kClear, halfCarry, carry);
            }
         }
      },
      {
         {
            0xE9, // JP (HL)
            [](Device& initial, Device& final) {
               final.cpu.reg.pc = initial.cpu.reg.hl;
            }
         }
      },
      {
         {
            0xEA, // LD (a16),A
            [](Device& initial, Device& final) {
               final.memory.write(imm16(initial), initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0xEE, // XOR d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a ^ imm8(initial);

               UPDATE_FLAGS(a, "Z000")
            }
         }
      },
      {
         {
            0xEF, // RST 28H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0028;
            }
         }
      },

      {
         {
            0xF0, // LDH A,(a8)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(0xFF00 + imm8(initial));
            }
         }
      },
      {
         {
            0xF1, // POP AF
            [](Device& initial, Device& final) {
               final.cpu.reg.af = read16(initial, initial.cpu.reg.sp);
               final.cpu.reg.f &= 0xF0;
               final.cpu.reg.sp += 2;
            }
         }
      },
      {
         {
            0xF2, // LD A,(C)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(0xFF00 + initial.cpu.reg.c);
            }
         }
      },
      {
         {
            0xF3, // DI
            [](Device& initial, Device& final) {
               final.cpu.ime = false;
            }
         }
      },
      {
         {
            0xF5, // PUSH AF
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, initial.cpu.reg.f);
               final.memory.write(final.cpu.reg.sp + 1, initial.cpu.reg.a);
            }
         }
      },
      {
         {
            0xF6, // OR d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a | imm8(initial);

               UPDATE_FLAGS(a, "Z000");
            }
         }
      },
      {
         {
            0xF7, // RST 30H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0030;
            }
         }
      },
      {
         {
            0xF8, // LD HL,SP+r8
            [](Device& initial, Device& final) {
               uint8_t offset = imm8(initial);
               int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);

               final.cpu.reg.hl = initial.cpu.reg.sp + signedOffset;

               // Special case - treat carry and half carry as if this was an 8 bit add
               final.cpu.reg.f = 0x00;
               if ((final.cpu.reg.hl & 0x000F) < (initial.cpu.reg.sp & 0x000F)) {
                  final.cpu.reg.f |= CPU::kHalfCarry;
               }
               if ((final.cpu.reg.hl & 0x00FF) < (initial.cpu.reg.sp & 0x00FF)) {
                  final.cpu.reg.f |= CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0xF9, // LD SP,HL
            [](Device& initial, Device& final) {
               final.cpu.reg.sp = initial.cpu.reg.hl;
            }
         }
      },
      {
         {
            0xFA, // LD A,(a16)
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.memory.read(imm16(initial));
            }
         }
      },
      {
         {
            0xFB, // EI
            [](Device& initial, Device& final) {
               final.cpu.interruptEnableRequested = true;
            }
         }
      },
      {
         {
            0xFE, // CP d8
            [](Device& initial, Device& final) {
               final.cpu.reg.a = initial.cpu.reg.a - imm8(initial);

               UPDATE_FLAGS(a, "Z1HC")

               final.cpu.reg.a = initial.cpu.reg.a;
            }
         }
      },
      {
         {
            0xFF, // RST 38H
            [](Device& initial, Device& final) {
               final.cpu.reg.sp -= 2;
               final.memory.write(final.cpu.reg.sp, final.cpu.reg.pc & 0xFF);
               final.memory.write(final.cpu.reg.sp + 1, final.cpu.reg.pc >> 8);
               final.cpu.reg.pc = 0x0038;
            }
         }
      },
   };
}

} // namespace GBC

#endif // GBC_RUN_TESTS
