#pragma once

#if GBC_RUN_TESTS

#include <cstdint>
#include <vector>

namespace GBC
{

class GameBoy;

using CPUTestSetupFunc = void (*)(GameBoy& initialState, GameBoy& finalState);

struct CPUTest
{
   uint8_t opcode;
   CPUTestSetupFunc testSetupFunction;
};

using CPUTestGroup = std::vector<CPUTest>;

class CPUTester
{
public:
   CPUTester();

   void runTests(bool randomizeData);

private:
   void runTestGroup(const CPUTestGroup& testGroup, bool randomizeData, uint16_t seed);

   void runTest(GameBoy& gameBoy, GameBoy& finalGameBoy, const CPUTest& test);

   void init();

   std::vector<CPUTestGroup> testGroups;
};

} // namespace GBC

#endif // GBC_RUN_TESTS
