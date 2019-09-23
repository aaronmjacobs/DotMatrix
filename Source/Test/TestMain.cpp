// Hack to allow us access to private members of the CPU
#define _ALLOW_KEYWORD_MACROS
#define private public
#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include "Platform/Utils/IOUtils.h"

#include <sstream>

namespace
{

bool endsWith(const std::string& str, const std::string& suffix)
{
   return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

enum class Result
{
   kPass,
   kFail,
   kError
};

const char* getResultName(Result result)
{
   switch (result)
   {
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

struct TestResult
{
   std::string cartPath;
   Result result = Result::kError;
};

Result runTestCart(UPtr<GBC::Cartridge> cart, float time)
{
   UPtr<GBC::GameBoy> gameBoy(std::make_unique<GBC::GameBoy>());
   gameBoy->setCartridge(std::move(cart));

   gameBoy->tick(time);

   // Magic numbers signal a successful test
   return gameBoy->cpu.reg.a == 0 ? Result::kPass : Result::kFail;
   //   && gameBoy.cpu.reg.b == 3 && gameBoy.cpu.reg.c == 5
   //   && gameBoy.cpu.reg.d == 8 && gameBoy.cpu.reg.e == 13
   //   && gameBoy.cpu.reg.h == 21 && gameBoy.cpu.reg.l == 34;
}

std::vector<TestResult> runTestCarts(const std::vector<std::string>& cartPaths, float time)
{
   std::vector<TestResult> results;

   for (const std::string& cartPath : cartPaths)
   {
      std::printf("Running %s...\n", cartPath.c_str());

      TestResult result;
      result.cartPath = cartPath;

      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(cartPath);
      UPtr<GBC::Cartridge> cartridge(GBC::Cartridge::fromData(std::move(cartData)));
      if (cartridge)
      {
         result.result = runTestCart(std::move(cartridge), time);
      }

      results.push_back(result);

      const char* resultName = getResultName(result.result);
      std::printf("%s\n\n", resultName);
   }

   return results;
}

void runTestCartsInPath(std::string path, std::string resultPath, float time)
{
   std::replace(path.begin(), path.end(), '\\', '/');

   std::vector<std::string> filePaths = IOUtils::getAllFilePathsRecursive(path);

   // Only try to run .gb files
   filePaths.erase(std::remove_if(filePaths.begin(), filePaths.end(), [](const std::string& fileName) { return !endsWith(fileName, ".gb"); }), filePaths.end());

   std::vector<TestResult> results = runTestCarts(filePaths, time);

   std::string output;
   for (const TestResult& result : results)
   {
      std::string relativePath = result.cartPath;
      std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

      size_t basePathpos = relativePath.rfind(path);
      if (basePathpos != std::string::npos)
      {
         relativePath = relativePath.substr(basePathpos + path.size());
      }

      output += relativePath + ";" + getResultName(result.result) + "\n";
   }

   IOUtils::writeTextFile(resultPath, output);
}

} // namespace

int main(int argc, char *argv[])
{
   static const float kDefaultTime = 10.0f;

   if (argc > 2)
   {
      float time = kDefaultTime;

      if (argc > 3)
      {
         std::stringstream ss(argv[3]);
         float parsedTime = 0.0f;
         if (ss >> parsedTime)
         {
            time = parsedTime;
         }
      }

      runTestCartsInPath(argv[1], argv[2], time);
   }
   else
   {
      std::printf("Usage: %s carts_path output_path [test_time]\n", argv[0]);
   }

   return 0;
}
