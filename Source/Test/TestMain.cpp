#if GBC_RUN_TESTS

// Hack to allow us access to private members of the CPU
#define _ALLOW_KEYWORD_MACROS
#define private public
#include "GBC/Cartridge.h"
#include "GBC/Device.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include "Test/CPUTester.h"

#include "Wrapper/Platform/IOUtils.h"

#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // defined(_WIN32)

namespace {

bool endsWith(const std::string& str, const std::string& suffix) {
   return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

enum class Result {
   kPass,
   kFail,
   kError
};

const char* getResultName(Result result) {
   switch (result) {
   case Result::kPass:
      return "pass";
   case Result::kFail:
      return "fail";
   case Result::kError:
      return "error";
   default:
      return "invalid";
   }
}

struct TestResult {
   std::string cartPath;
   Result result = Result::kError;
};

Result runTestCart(UPtr<GBC::Cartridge> cart, float time) {
   UPtr<GBC::Device> device(std::make_unique<GBC::Device>());
   device->setCartridge(std::move(cart));

   device->tick(time);

   // Magic numbers signal a successful test
   return device->cpu.reg.a == 0 ? Result::kPass : Result::kFail;
   //   && device.cpu.reg.b == 3 && device.cpu.reg.c == 5
   //   && device.cpu.reg.d == 8 && device.cpu.reg.e == 13
   //   && device.cpu.reg.h == 21 && device.cpu.reg.l == 34;
}

std::vector<TestResult> runTestCarts(const std::vector<std::string>& cartPaths, float time) {
   std::vector<TestResult> results;

   for (const std::string& cartPath : cartPaths) {
      printf("Running %s...\n", cartPath.c_str());

      TestResult result;
      result.cartPath = cartPath;

      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(cartPath);
      UPtr<GBC::Cartridge> cartridge(GBC::Cartridge::fromData(std::move(cartData)));
      if (cartridge) {
         result.result = runTestCart(std::move(cartridge), time);
      }

      results.push_back(result);

      const char* resultName = getResultName(result.result);
      printf("%s\n\n", resultName);
   }

   return results;
}

void runTestCartsInPath(const std::string& path, const std::string& resultPath, float time) {
   std::vector<std::string> filePaths = IOUtils::getAllFilePathsRecursive(path);

   // Only try to run .gb files
   filePaths.erase(std::remove_if(filePaths.begin(), filePaths.end(), [](const std::string& fileName) { return !endsWith(fileName, ".gb"); }), filePaths.end());

   std::vector<TestResult> results = runTestCarts(filePaths, time);

   std::string output;
   for (const TestResult& result : results) {
      std::string relativePath = result.cartPath;
      std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

      size_t basePathpos = relativePath.rfind(path);
      if (basePathpos != std::string::npos) {
         relativePath = relativePath.substr(basePathpos + path.size());
      }

      output += relativePath + ";" + getResultName(result.result) + "\n";
   }

   IOUtils::writeTextFile(resultPath, output);
}

} // namespace

int main(int argc, char *argv[]) {
   static const std::string kRomsCommand = "-roms";
   static const std::string kCpuCommand = "-cpu";
   static const float kDefaultTime = 10.0f;

   if (argc > 1) {
      const char* command = argv[1];

      if (command == kRomsCommand) {
         if (argc > 3) {
            float time = kDefaultTime;

            if (argc > 4) {
               std::stringstream ss(argv[4]);
               float parsedTime = 0.0f;
               if (ss >> parsedTime) {
                  time = parsedTime;
               }
            }

            runTestCartsInPath(argv[2], argv[3], time);
         } else {
            printf("Usage: %s -roms carts_path output_path [test_time]\n", argv[0]);
         }
         
         return 0;
      } else if (command == kCpuCommand) {
         GBC::CPUTester cpuTester;
         for (int i = 0; i < 100; ++i) {
            cpuTester.runTests(true);
         }

         return 0;
      }
   }

   printf("Usage: %s (-roms | -cpu) [options]\n", argv[0]);
   return 0;
}

#if defined(_WIN32)
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
   return main(__argc, __argv);
}
#endif // defined(_WIN32)

#endif // GBC_RUN_TESTS
