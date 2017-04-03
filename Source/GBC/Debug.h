#ifndef GBC_DEBUG_H
#define GBC_DEBUG_H

#if defined(NDEBUG)
#  error "Trying to include Debug.h in a release build"
#endif

#include "GBC/CPU.h"

namespace GBC {

namespace {

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define INS_NAME(val) case Ins::k##val: return STR(val)

const char* insName(Ins instruction) {
   switch (instruction) {
      INS_NAME(Invalid);
      INS_NAME(LD);
      INS_NAME(LDD);
      INS_NAME(LDI);
      INS_NAME(LDH);
      INS_NAME(LDHL);
      INS_NAME(PUSH);
      INS_NAME(POP);
      INS_NAME(ADD);
      INS_NAME(ADC);
      INS_NAME(SUB);
      INS_NAME(SBC);
      INS_NAME(AND);
      INS_NAME(OR);
      INS_NAME(XOR);
      INS_NAME(CP);
      INS_NAME(INC);
      INS_NAME(DEC);
      INS_NAME(SWAP);
      INS_NAME(DAA);
      INS_NAME(CPL);
      INS_NAME(CCF);
      INS_NAME(SCF);
      INS_NAME(NOP);
      INS_NAME(HALT);
      INS_NAME(STOP);
      INS_NAME(DI);
      INS_NAME(EI);
      INS_NAME(RLCA);
      INS_NAME(RLA);
      INS_NAME(RRCA);
      INS_NAME(RRA);
      INS_NAME(RLC);
      INS_NAME(RL);
      INS_NAME(RRC);
      INS_NAME(RR);
      INS_NAME(SLA);
      INS_NAME(SRA);
      INS_NAME(SRL);
      INS_NAME(BIT);
      INS_NAME(SET);
      INS_NAME(RES);
      INS_NAME(JP);
      INS_NAME(JR);
      INS_NAME(CALL);
      INS_NAME(RST);
      INS_NAME(RET);
      INS_NAME(RETI);
      INS_NAME(PREFIX);
      default: return "Unknown instruction";
   }
}

#undef INS_NAME
#define OPR_NAME(val) case Opr::k##val: return STR(val)

const char* oprName(Opr operand) {
   switch (operand) {
      case Opr::kNone: return "";
      OPR_NAME(A);
      OPR_NAME(F);
      OPR_NAME(B);
      OPR_NAME(C);
      OPR_NAME(D);
      OPR_NAME(E);
      OPR_NAME(H);
      OPR_NAME(L);
      OPR_NAME(AF);
      OPR_NAME(BC);
      OPR_NAME(DE);
      OPR_NAME(HL);
      OPR_NAME(SP);
      OPR_NAME(PC);
      OPR_NAME(Imm8);
      OPR_NAME(Imm16);
      OPR_NAME(Imm8Signed);
      OPR_NAME(DrefC);
      OPR_NAME(DrefBC);
      OPR_NAME(DrefDE);
      OPR_NAME(DrefHL);
      OPR_NAME(DrefImm8);
      OPR_NAME(DrefImm16);
      OPR_NAME(CB);
      OPR_NAME(FlagC);
      OPR_NAME(FlagNC);
      OPR_NAME(FlagZ);
      OPR_NAME(FlagNZ);
      OPR_NAME(0);
      OPR_NAME(1);
      OPR_NAME(2);
      OPR_NAME(3);
      OPR_NAME(4);
      OPR_NAME(5);
      OPR_NAME(6);
      OPR_NAME(7);
      OPR_NAME(00H);
      OPR_NAME(08H);
      OPR_NAME(10H);
      OPR_NAME(18H);
      OPR_NAME(20H);
      OPR_NAME(28H);
      OPR_NAME(30H);
      OPR_NAME(38H);
      default: return "Unknown operand";
   }
}

#undef OPR_NAME

} // namespace

} // namespace GBC

#endif
