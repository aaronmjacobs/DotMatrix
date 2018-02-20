#ifndef CPUTESTER_H
#define CPUTESTER_H

#if GBC_RUN_TESTS

#include <cstdint>
#include <vector>

namespace GBC {

class Device;

using CPUTestSetupFunc = void (*)(Device& initialState, Device& finalState);

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

   void runTest(Device& device, Device& finalDevice, const CPUTest& test);

   void init();

   std::vector<CPUTestGroup> testGroups;
};

} // namespace GBC

#endif // GBC_RUN_TESTS

#endif
