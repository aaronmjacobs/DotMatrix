#ifndef CPU_H
#define CPU_H

#include <cstdint>

namespace GBC {

union Reg {             // general purpose registers
   struct {
      uint8_t b;
      uint8_t c;
      uint8_t d;
      uint8_t e;
      uint8_t h;
      uint8_t l;
   };
   struct {
      uint16_t bc;
      uint16_t de;
      uint16_t hl;
   };
};

const uint8_t kZero = 1 << 7;       // Zero flag
const uint8_t kSub = 1 << 6;        // Subtract flag
const uint8_t kHalfCarry = 1 << 5;  // Half carry flag
const uint8_t kCarry = 1 << 4;      // Carry flag

class CPU {
private:
   uint8_t a;          // accumulator register
   uint8_t f;          // status register
   uint16_t pc;        // program counter
   uint16_t sp;        // stack pointer
   Reg reg;

public:
   CPU();
   ~CPU();
};

} // namespace GBC

#endif

