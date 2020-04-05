// Hack to allow us access to private members of the CPU
#define _ALLOW_KEYWORD_MACROS
#define private public
#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include "Platform/Utils/IOUtils.h"

#include <readerwriterqueue.h>

#include <future>
#include <sstream>
#include <thread>

namespace
{

bool endsWith(const std::string& str, const std::string& suffix)
{
   return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

enum class Result
{
   Pass,
   Fail,
   Error
};

const char* getResultName(Result result)
{
   switch (result)
   {
   case Result::Pass:
      return "pass";
   case Result::Fail:
      return "fail";
   case Result::Error:
      return "error";
   default:
      return "invalid";
   }
}

struct TestResult
{
   std::string cartPath;
   Result result = Result::Error;
};

struct TestWorkerData
{
   TestWorkerData(std::vector<TestResult>& results, moodycamel::ReaderWriterQueue<std::string>& queue, std::size_t first, std::size_t last, float time)
      : testResults(results)
      , messageQueue(queue)
      , firstIndex(first)
      , lastIndex(last)
      , testTime(time)
   {
   }

   std::vector<TestResult>& testResults;
   moodycamel::ReaderWriterQueue<std::string>& messageQueue;

   std::size_t firstIndex;
   std::size_t lastIndex;
   float testTime;
};

Result runTestCart(UPtr<GBC::Cartridge> cart, float time, bool checkAllRegisters)
{
   UPtr<GBC::GameBoy> gameBoy = std::make_unique<GBC::GameBoy>();
   gameBoy->setCartridge(std::move(cart));

   gameBoy->tick(time);

   // Magic numbers signal a successful test
   const GBC::CPU& cpu = gameBoy->cpu;
   bool success = cpu.reg.a == 0;
   if (checkAllRegisters)
   {
      success = success
         && cpu.reg.b == 3 && cpu.reg.c == 5
         && cpu.reg.d == 8 && cpu.reg.e == 13
         && cpu.reg.h == 21 && cpu.reg.l == 34;
   }

   return success ? Result::Pass : Result::Fail;
}

void runTestCartRange(TestWorkerData data)
{
   for (std::size_t i = data.firstIndex; i <= data.lastIndex; ++i)
   {
      TestResult& result = data.testResults[i];

      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(result.cartPath);
      std::string error;
      if (UPtr<GBC::Cartridge> cartridge = GBC::Cartridge::fromData(std::move(cartData), error))
      {
         static const std::string kMooneye = "mooneye";

         bool checkAllRegisters = result.cartPath.find(kMooneye) != std::string::npos;
         result.result = runTestCart(std::move(cartridge), data.testTime, checkAllRegisters);
      }

      std::string message = result.cartPath + ": " + getResultName(result.result);
      if (!error.empty())
      {
         message += " (" + error + ")";
      }
      data.messageQueue.enqueue(std::move(message));
   }
}

void runTestCartsInPath(std::string path, std::string resultPath, float time)
{
   IOUtils::standardizePath(path);

   // Only try to run .gb files
   std::vector<std::string> filePaths = IOUtils::getAllFilePathsRecursive(path);
   filePaths.erase(std::remove_if(filePaths.begin(), filePaths.end(), [](const std::string& filePath) { return !endsWith(filePath, ".gb"); }), filePaths.end());
   std::size_t numTests = filePaths.size();

   if (numTests == 0)
   {
      std::printf("No tests found\n");
      return;
   }

   std::vector<TestResult> testResults(numTests);
   for (std::size_t i = 0; i < numTests; ++i)
   {
      testResults[i].cartPath = std::move(filePaths[i]);
   }

   std::size_t numWorkers = std::min(numTests, static_cast<std::size_t>(std::max(1U, std::thread::hardware_concurrency())));
   std::size_t numTestsPerWorker = numTests / numWorkers;
   std::size_t numLeftoverTests = numTests % numWorkers;

   std::vector<std::future<void>> futures;
   moodycamel::ReaderWriterQueue<std::string> messageQueue;

   std::size_t firstIndex = 0;
   for (std::size_t i = 0; i < numWorkers; ++i)
   {
      std::size_t lastIndex = firstIndex + numTestsPerWorker - 1;
      if (numLeftoverTests > 0)
      {
         ++lastIndex;
         --numLeftoverTests;
      }

      futures.push_back(std::async(std::launch::async, runTestCartRange, TestWorkerData(testResults, messageQueue, firstIndex, lastIndex, time)));

      firstIndex = lastIndex + 1;
   }

   auto allWorkersDone = [&futures]()
   {
      for (const std::future<void>& future : futures)
      {
         if (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
         {
            return false;
         }
      }

      return true;
   };

   auto processMessages = [&messageQueue]()
   {
      std::string message;
      while (messageQueue.try_dequeue(message))
      {
         std::printf("%s\n", message.c_str());
      }
   };

   bool done = false;
   while (!done)
   {
      done = allWorkersDone();
      processMessages();

      if (!done)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
   }

   std::string output;
   for (const TestResult& result : testResults)
   {
      std::string relativePath = result.cartPath;

      size_t basePathpos = relativePath.rfind(path);
      if (basePathpos != std::string::npos)
      {
         relativePath = relativePath.substr(basePathpos + path.size() + 1);
      }

      output += relativePath + "|" + getResultName(result.result) + "\n";
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
