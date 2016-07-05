#ifndef CPUTESTER_H
#define CPUTESTER_H

#include "Constants.h"

#if defined(RUN_TESTS)

#include "gbc/CPU.h"
#include "gbc/Memory.h"

#include <cstdint>
#include <vector>

namespace GBC {

using CPUTestSetupFunc = void (*)(CPU& initialState, CPU& finalState);

struct CPUTest {
   uint8_t opcode;
   CPUTestSetupFunc testSetupFunction;
};

using CPUTestGroup = std::vector<CPUTest>;

class CPUTester {
public:
   CPUTester();

   void runTests(bool randomizeData);

private:
   void runTestGroup(const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed);

   void runTest(CPU& cpu, CPU& finalCPU, const CPUTest& test);

   void init();

   std::vector<CPUTestGroup> testGroups;
};

} // namespace GBC

#endif // defined(RUN_TESTS)

#endif
