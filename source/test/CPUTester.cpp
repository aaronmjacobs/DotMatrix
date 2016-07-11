#include "Constants.h"

#if defined(RUN_TESTS)

#include "CPUTester.h"
#include "FancyAssert.h"
#include "Log.h"

#include "gbc/CPU.h"
#include "gbc/Memory.h"
#include "gbc/Operations.h"

#include <cstring>
#include <random>
#include <sstream>

namespace GBC {

namespace {

std::string hex(uint8_t val) {
   std::ostringstream stream;
   stream << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint16_t>(val);
   return stream.str();
}

std::string hex(uint16_t val) {
   std::ostringstream stream;
   stream << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << val;
   return stream.str();
}

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
   FlagOp zero = getFlagOp(_znhc_[0], final.reg._reg_ == 0 ? kSet : kClear);\
   FlagOp sub = getFlagOp(_znhc_[1], add ? kClear : kSet);\
   FlagOp halfCarry = getFlagOp(_znhc_[2], (add ? didHalfCarryAdd(initial.reg._reg_, final.reg._reg_) : didHalfCarrySub(initial.reg._reg_, final.reg._reg_)) ? kSet : kClear);\
   FlagOp carry = getFlagOp(_znhc_[3], (add ? didCarryAdd(initial.reg._reg_, final.reg._reg_) : didCarrySub(initial.reg._reg_, final.reg._reg_)) ? kSet : kClear);\
   final.reg.f = updateFlagRegister(initial.reg.f, zero, sub, halfCarry, carry);\
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
      out << " " << hex(mem[i]);
   }
   out << "\n";
   LOG_INFO(out.str());
}

void prepareInitial(CPU& cpu, const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed) {
   std::default_random_engine engine(seed);
   std::uniform_int_distribution<uint8_t> distribution8;
   std::uniform_int_distribution<uint16_t> distribution16;

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

   cpu.reg.a = kInitialA;
   cpu.reg.f = kInitialF; // TODO Should the CPU ensure the low bits are never used?
   cpu.reg.b = kInitialB;
   cpu.reg.c = kInitialC;
   cpu.reg.d = kInitialD;
   cpu.reg.e = kInitialE;
   cpu.reg.h = kInitialH;
   cpu.reg.l = kInitialL;

   cpu.reg.sp = kInitialSP;
   cpu.reg.pc = kInitialPC;

   uint16_t pcOffset = 0;
   for (const CPUTest& test : testGroup) {
      Operation operation = kOperations[test.opcode];

      cpu.mem[cpu.reg.pc + pcOffset++] = test.opcode;

      if (usesImm8(operation)) {
         cpu.mem[cpu.reg.pc + pcOffset++] = kInitialMem1;
      } else if (usesImm16(operation)) {
         cpu.mem[cpu.reg.pc + pcOffset++] = kInitialMem1;
         cpu.mem[cpu.reg.pc + pcOffset++] = kInitialMem2;
      }
   }
}

void prepareFinal(CPU& cpu, const CPUTestGroup& testGroup, size_t testIndex, bool randomizeData, uint16_t seed) {
   prepareInitial(cpu, testGroup, randomizeData, seed);

   for (size_t i = 0; i <= testIndex; ++i) {
      const CPUTest& test = testGroup[i];

      Operation operation = kOperations[test.opcode];
      cpu.cycles += operation.cycles;

      ++cpu.reg.pc;
      if (usesImm8(operation)) {
         ++cpu.reg.pc;
      } else if (usesImm16(operation)) {
         cpu.reg.pc += 2;
      }
   }
}

uint8_t imm8(CPU& cpu) {
   return cpu.mem[cpu.reg.pc + 1];
}

uint16_t imm16(CPU& cpu) {
   return cpu.mem[cpu.reg.pc + 1] | (cpu.mem[cpu.reg.pc + 2] << 8);
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
   Memory memory { 0 };
   Memory finalMemory { 0 };
   CPU cpu(memory);
   CPU finalCPU(finalMemory);

   prepareInitial(cpu, testGroup, randomizeData, seed);

   for (size_t i = 0; i < testGroup.size(); ++i) {
      prepareFinal(finalCPU, testGroup, i, randomizeData, seed);
      runTest(cpu, finalCPU, testGroup[i]);
   }
}

void CPUTester::runTest(CPU& cpu, CPU& finalCPU, const CPUTest& test) {
   test.testSetupFunction(cpu, finalCPU);

   cpu.tickOnce();

   bool registersMatch = memcmp(&finalCPU.reg, &cpu.reg, sizeof(CPU::Registers)) == 0;
   bool memoryMatches = true;
   uint16_t mismatchLocation = 0;
   for (uint16_t i = 0, j = 0; i >= j; j = i++) {
      if (cpu.mem[i] != finalCPU.mem[i]) {
         memoryMatches = false;
         mismatchLocation = i;
         break;
      }
   }

   // TODO Also check other CPU vars (ime, cycles, etc.)

   if (!registersMatch) {
      dumpRegisters(finalCPU.reg, "Expected");
      dumpRegisters(cpu.reg, "Actual");
   }

   if (!memoryMatches) {
      uint16_t memoryLine = mismatchLocation / 8;
      uint16_t previousLine = memoryLine > 0 ? memoryLine - 1 : 0;
      uint16_t nextLine = memoryLine < 8192 ? memoryLine + 1 : 8192;

      dumpMem(finalCPU.mem, previousLine * 8, nextLine * 8 + 8, "Expected");
      dumpMem(cpu.mem, previousLine * 8, nextLine * 8 + 8, "Actual");
   }

   ASSERT(registersMatch && memoryMatches);
}

void CPUTester::init() {
   testGroups = {
      {
         {
            0x00, // NOP
            [](CPU& initial, CPU& final) {
            }
         }
      },
      {
         {
            0x01, // LD BC,d16
            [](CPU& initial, CPU& final) {
               final.reg.bc = imm16(initial);
            }
         }
      },
      {
         {
            0x02, // LD (BC),A
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.bc] = initial.reg.a;
            }
         }
      },
      {
         {
            0x03, // INC BC
            [](CPU& initial, CPU& final) {
               ++final.reg.bc;
            }
         }
      },
      {
         {
            0x04, // INC B
            [](CPU& initial, CPU& final) {
               ++final.reg.b;

               UPDATE_FLAGS(b, "Z0H-")
            }
         }
      },
      {
         {
            0x05, // DEC B
            [](CPU& initial, CPU& final) {
               --final.reg.b;

               UPDATE_FLAGS(b, "Z1H-")
            }
         }
      },
      {
         {
            0x06, // LD B,d8
            [](CPU& initial, CPU& final) {
               final.reg.b = imm8(initial);
            }
         }
      },
      {
         {
            0x07, // RLCA
            [](CPU& initial, CPU& final) {
               final.reg.a = (initial.reg.a << 1) | (initial.reg.a >> 7);

               UPDATE_FLAGS(c, "000C")
               // Old bit 7 to carry
               if (initial.reg.a & 0x80) {
                  final.reg.f |= CPU::kCarry;
               } else {
                  final.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x08, // LD (a16),SP
            [](CPU& initial, CPU& final) {
               uint8_t spLow = initial.reg.sp & 0xFF;
               uint8_t spHigh = (initial.reg.sp >> 8) & 0xFF;
               uint16_t a16 = imm16(initial);

               final.mem[a16] = spLow;
               final.mem[a16 + 1] = spHigh;
            }
         }
      },
      {
         {
            0x09, // ADD HL,BC
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl + initial.reg.bc;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x0A, // LD A,(BC)
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.mem[initial.reg.bc] | (initial.mem[initial.reg.bc + 1] << 8);
            }
         }
      },
      {
         {
            0x0B, // DEC BC
            [](CPU& initial, CPU& final) {
               final.reg.bc = initial.reg.bc - 1;
            }
         }
      },
      {
         {
            0x0C, // INC C
            [](CPU& initial, CPU& final) {
               final.reg.c = initial.reg.c + 1;

               UPDATE_FLAGS(c, "Z0H-")
            }
         }
      },
      {
         {
            0x0D, // DEC C
            [](CPU& initial, CPU& final) {
               final.reg.c = initial.reg.c - 1;

               UPDATE_FLAGS(c, "Z1H-")
            }
         }
      },
      {
         {
            0x0E, // LD C,d8
            [](CPU& initial, CPU& final) {
               final.reg.c = imm8(initial);
            }
         }
      },
      {
         {
            0x0F, // RRCA
            [](CPU& initial, CPU& final) {
               final.reg.a = (initial.reg.a >> 1) | (initial.reg.a << 7);

               UPDATE_FLAGS(c, "000C")
               // Old bit 0 to carry
               if (initial.reg.a & 0x01) {
                  final.reg.f |= CPU::kCarry;
               } else {
                  final.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },

      {
         {
            0x10, // STOP 0
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.pc + 1] = initial.mem[initial.reg.pc + 1] = 0x00;

               final.stopped = true;
            }
         }
      },
      {
         {
            0x11, // LD DE,d16
            [](CPU& initial, CPU& final) {
               final.reg.de = imm16(initial);
            }
         }
      },
      {
         {
            0x12, // LD (DE),A
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.de] = initial.reg.a;
            }
         }
      },
      {
         {
            0x13, // INC DE
            [](CPU& initial, CPU& final) {
               ++final.reg.de;
            }
         }
      },
      {
         {
            0x14, // INC D
            [](CPU& initial, CPU& final) {
               final.reg.d = initial.reg.d + 1;

               UPDATE_FLAGS(d, "Z0H-")
            }
         }
      },
      {
         {
            0x15, // DEC D
            [](CPU& initial, CPU& final) {
               final.reg.d = initial.reg.d - 1;

               UPDATE_FLAGS(d, "Z1H-")
            }
         }
      },
      {
         {
            0x16, // LD D,d8
            [](CPU& initial, CPU& final) {
               final.reg.d = imm8(initial);
            }
         }
      },
      {
         {
            0x17, // RLA
            [](CPU& initial, CPU& final) {
               final.reg.a = (initial.reg.a << 1) | ((initial.reg.f & CPU::kCarry) ? 0x01 : 0x00);

               UPDATE_FLAGS(c, "000C")
               // Old bit 7 to carry
               if (initial.reg.a & 0x80) {
                  final.reg.f |= CPU::kCarry;
               } else {
                  final.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x18, // JR r8
            [](CPU& initial, CPU& final) {
               uint8_t offset = imm8(initial);
               int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
               final.reg.pc = initial.reg.pc + 2 + signedOffset;
            }
         }
      },
      {
         {
            0x19, // ADD HL,DE
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl + initial.reg.de;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x1A, // LD A,(DE)
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.mem[initial.reg.de];
            }
         }
      },
      {
         {
            0x1B, // DEC DE
            [](CPU& initial, CPU& final) {
               final.reg.de = initial.reg.de - 1;
            }
         }
      },
      {
         {
            0x1C, // INC E
            [](CPU& initial, CPU& final) {
               final.reg.e = initial.reg.e + 1;

               UPDATE_FLAGS(e, "Z0H-")
            }
         }
      },
      {
         {
            0x1D, // DEC E
            [](CPU& initial, CPU& final) {
               final.reg.e = initial.reg.e - 1;

               UPDATE_FLAGS(e, "Z1H-")
            }
         }
      },
      {
         {
            0x1E, // LD E,d8
            [](CPU& initial, CPU& final) {
               final.reg.e = imm8(initial);
            }
         }
      },
      {
         {
            0x1F, // RRA
            [](CPU& initial, CPU& final) {
               final.reg.a = (initial.reg.a >> 1) | ((initial.reg.f & CPU::kCarry) ? 0x80 : 0x00);

               UPDATE_FLAGS(c, "000C")
               // Old bit 0 to carry
               if (initial.reg.a & 0x01) {
                  final.reg.f |= CPU::kCarry;
               } else {
                  final.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },

      {
         {
            0x20, // JR NZ,r8
            [](CPU& initial, CPU& final) {
               if ((initial.reg.f & CPU::kZero) == 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.reg.pc = initial.reg.pc + 2 + signedOffset;
               }
            }
         }
      },
      {
         {
            0x21, // LD HL,d16
            [](CPU& initial, CPU& final) {
               final.reg.hl = imm16(initial);
            }
         }
      },
      {
         {
            0x22, // LD (HL+),A
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.hl] = initial.reg.a;
               final.reg.hl = initial.reg.hl + 1;
            }
         }
      },
      {
         {
            0x23, // INC HL
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl + 1;
            }
         }
      },
      {
         {
            0x24, // INC H
            [](CPU& initial, CPU& final) {
               final.reg.h = initial.reg.h + 1;

               UPDATE_FLAGS(h, "Z0H-")
            }
         }
      },
      {
         {
            0x25, // DEC H
            [](CPU& initial, CPU& final) {
               final.reg.h = initial.reg.h - 1;

               UPDATE_FLAGS(h, "Z1H-")
            }
         }
      },
      {
         {
            0x26, // LD H,d8
            [](CPU& initial, CPU& final) {
               final.reg.h = imm8(initial);
            }
         }
      },
      {
         {
            0x27, // DAA
            [](CPU& initial, CPU& final) {
               uint16_t val = initial.reg.a;

               if (initial.reg.f & CPU::kSub) {
                  if (initial.reg.f & CPU::kHalfCarry) {
                     val = (val - 0x06) & 0xFF;
                  }
                  if (initial.reg.f & CPU::kCarry) {
                     val -= 0x60;
                  }
               } else {
                  if (initial.reg.f & CPU::kHalfCarry || (val & 0x0F) > 9) {
                     val += 0x06;
                  }
                  if (initial.reg.f & CPU::kCarry || val > 0x9F) {
                     val += 0x60;
                  }
               }

               final.reg.a = val;

               UPDATE_FLAGS(a, "Z-0C")
               // See DAA table
               if ((val & 0x0100) == 0x0100) {
                  final.reg.f |= CPU::kCarry;
               } else {
                  final.reg.f &= ~CPU::kCarry;
               }
            }
         }
      },
      {
         {
            0x28, // JR Z,r8
            [](CPU& initial, CPU& final) {
               if ((initial.reg.f & CPU::kZero) != 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.reg.pc = initial.reg.pc + 2 + signedOffset;
               }
            }
         }
      },
      {
         {
            0x29, // ADD HL,HL
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl + initial.reg.hl;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x2A, // LD A,(HL+)
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.mem[initial.reg.hl];
               final.reg.hl = initial.reg.hl + 1;
            }
         }
      },
      {
         {
            0x2B, // DEC HL
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl - 1;
            }
         }
      },
      {
         {
            0x2C, // INC L
            [](CPU& initial, CPU& final) {
               final.reg.l = initial.reg.l + 1;

               UPDATE_FLAGS(l, "Z0H-")
            }
         }
      },
      {
         {
            0x2D, // DEC L
            [](CPU& initial, CPU& final) {
               final.reg.l = initial.reg.l - 1;

               UPDATE_FLAGS(l, "Z1H-")
            }
         }
      },
      {
         {
            0x2E, // LD L,d8
            [](CPU& initial, CPU& final) {
               final.reg.l = imm8(initial);
            }
         }
      },
      {
         {
            0x2F, // CPL
            [](CPU& initial, CPU& final) {
               final.reg.a = ~initial.reg.a;

               UPDATE_FLAGS(a, "-11-")
            }
         }
      },

      {
         {
            0x30, // JR NC,r8
            [](CPU& initial, CPU& final) {
               if ((initial.reg.f & CPU::kCarry) == 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.reg.pc = initial.reg.pc + 2 + signedOffset;
               }
            }
         }
      },
      {
         {
            0x31, // LD SP,d16
            [](CPU& initial, CPU& final) {
               final.reg.sp = imm16(initial);
            }
         }
      },
      {
         {
            0x32, // LD (HL-),A
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.hl] = initial.reg.a;
               final.reg.hl = initial.reg.hl - 1;
            }
         }
      },
      {
         {
            0x33, // INC SP
            [](CPU& initial, CPU& final) {
               final.reg.sp = initial.reg.sp + 1;
            }
         }
      },
      {
         {
            0x34, // INC (HL)
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.hl] = initial.mem[initial.reg.hl] + 1;

               // hacky way to make macro still work - use a to store the result temporarily
               uint8_t initialA = initial.reg.a;
               initial.reg.a = initial.mem[initial.reg.hl];
               final.reg.a = initial.mem[initial.reg.hl] + 1;
               UPDATE_FLAGS(a, "Z0H-")
               initial.reg.a = final.reg.a = initialA;
            }
         }
      },
      {
         {
            0x35, // DEC (HL)
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.hl] = initial.mem[initial.reg.hl] - 1;

               // hacky way to make macro still work - use a to store the result temporarily
               uint8_t initialA = initial.reg.a;
               initial.reg.a = initial.mem[initial.reg.hl];
               final.reg.a = initial.mem[initial.reg.hl] - 1;
               UPDATE_FLAGS(a, "Z1H-")
               initial.reg.a = final.reg.a = initialA;
            }
         }
      },
      {
         {
            0x36, // LD (HL),d8
            [](CPU& initial, CPU& final) {
               final.mem[initial.reg.hl] = imm8(initial);
            }
         }
      },
      {
         {
            0x37, // SCF
            [](CPU& initial, CPU& final) {
               UPDATE_FLAGS(a, "-001")
            }
         }
      },
      {
         {
            0x38, // JR C,r8
            [](CPU& initial, CPU& final) {
               if ((initial.reg.f & CPU::kCarry) != 0x00) {
                  uint8_t offset = imm8(initial);
                  int8_t signedOffset = *reinterpret_cast<int8_t*>(&offset);
                  final.reg.pc = initial.reg.pc + 2 + signedOffset;
               }
            }
         }
      },
      {
         {
            0x39, // ADD HL,SP
            [](CPU& initial, CPU& final) {
               final.reg.hl = initial.reg.hl + initial.reg.sp;

               UPDATE_FLAGS(hl, "-0HC")
            }
         }
      },
      {
         {
            0x3A, // LD A,(HL-)
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.mem[initial.reg.hl];
               final.reg.hl = initial.reg.hl - 1;
            }
         }
      },
      {
         {
            0x3B, // DEC SP
            [](CPU& initial, CPU& final) {
               final.reg.sp = initial.reg.sp - 1;
            }
         }
      },
      {
         {
            0x3C, // INC A
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.reg.a + 1;
               UPDATE_FLAGS(a, "Z0H-")
            }
         }
      },
      {
         {
            0x3D, // DEC A
            [](CPU& initial, CPU& final) {
               final.reg.a = initial.reg.a - 1;
               UPDATE_FLAGS(a, "Z1H-")
            }
         }
      },
      {
         {
            0x3E, // LD A,d8
            [](CPU& initial, CPU& final) {
               final.reg.a = imm8(initial);
            }
         }
      },
      {
         {
            0x3F, // CCF
            [](CPU& initial, CPU& final) {
               UPDATE_FLAGS(a, "-00C")
               // Set carry to flipped initial carry
               final.reg.f = (final.reg.f & 0xE0) | ((initial.reg.f & 0x10) ^ 0x10);
            }
         }
      },
   };
}

} // namespace GBC

#endif // defined(RUN_TESTS)
