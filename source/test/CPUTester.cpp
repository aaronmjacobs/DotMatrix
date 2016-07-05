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
      out << " " << hex(mem.raw[i]);
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

      cpu.mem.raw[cpu.reg.pc + pcOffset++] = test.opcode;

      if (usesImm8(operation)) {
         cpu.mem.raw[cpu.reg.pc + pcOffset++] = kInitialMem1;
      } else if (usesImm16(operation)) {
         cpu.mem.raw[cpu.reg.pc + pcOffset++] = kInitialMem1;
         cpu.mem.raw[cpu.reg.pc + pcOffset++] = kInitialMem2;
      }
   }
}

void prepareFinal(CPU& cpu, const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed) {
   prepareInitial(cpu, testGroup, randomizeData, seed);

   for (const CPUTest& test : testGroup) {
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
   return cpu.mem.raw[cpu.reg.pc + 1];
}

uint16_t imm16(CPU& cpu) {
   return cpu.mem.raw[cpu.reg.pc + 1] | (cpu.mem.raw[cpu.reg.pc + 2] << 8);
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
   prepareFinal(finalCPU, testGroup, randomizeData, seed);

   for (const CPUTest& test : testGroup) {
      runTest(cpu, finalCPU, test);
   }
}

void CPUTester::runTest(CPU& cpu, CPU& finalCPU, const CPUTest& test) {
   test.testSetupFunction(cpu, finalCPU);

   cpu.tick();

   bool registersMatch = memcmp(&finalCPU.reg, &cpu.reg, sizeof(CPU::Registers)) == 0;
   bool memoryMatches = true;
   uint16_t mismatchLocation = 0;
   for (uint16_t i = 0, j = 0; i >= j; j = i++) {
      if (cpu.mem.raw[i] != finalCPU.mem.raw[i]) {
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
   };
}

} // namespace GBC

#endif // defined(RUN_TESTS)
