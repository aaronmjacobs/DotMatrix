// Hack to allow us access to private members of the CPU
#define _ALLOW_KEYWORD_MACROS
#define private public
#include "GameBoy/Cartridge.h"
#include "GameBoy/GameBoy.h"
#undef private
#undef _ALLOW_KEYWORD_MACROS

#include <PlatformUtils/IOUtils.h>
#include <readerwriterqueue.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <sstream>
#include <thread>

namespace
{
   std::vector<std::filesystem::path> getAllFilePathsRecursive(const std::filesystem::path& directory)
   {
      std::vector<std::filesystem::path> filePaths;

      if (std::filesystem::is_directory(directory))
      {
         for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory))
         {
            const std::filesystem::path& path = entry.path();

            if (std::filesystem::is_regular_file(path))
            {
               filePaths.push_back(path);
            }
         }
      }

      return filePaths;
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
      std::filesystem::path cartPath;
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

   template<std::size_t size>
   bool valuesMatch(const std::vector<uint8_t>& values, const std::array<uint8_t, size>& testValues)
   {
      if (values.size() < size)
      {
         return false;
      }

      std::size_t offset = values.size() - size;
      for (std::size_t i = 0; i < size; ++i)
      {
         if (values[offset + i] != testValues[i])
         {
            return false;
         }
      }

      return true;
   }

   Result runTestCart(std::unique_ptr<DotMatrix::Cartridge> cart, float time, bool isMooneye)
   {
      static const std::array<uint8_t, 6> kMooneyeSuccessValues = { 3, 5, 8, 13, 21, 34 };
      static const std::array<uint8_t, 6> kMooneyeFailureValues = { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42 };

      std::unique_ptr<DotMatrix::GameBoy> gameBoy = std::make_unique<DotMatrix::GameBoy>();
      gameBoy->setCartridge(std::move(cart));

      std::vector<uint8_t> serialValues;
      if (isMooneye)
      {
         gameBoy->setSerialCallback([gb = gameBoy.get(), &serialValues](uint8_t value) -> uint8_t
         {
            serialValues.push_back(value);

            if (valuesMatch(serialValues, kMooneyeSuccessValues) || valuesMatch(serialValues, kMooneyeFailureValues))
            {
               gb->onCPUStopped();
            }

            return 0xFF;
         });
      }

      gameBoy->tick(time);

      // Magic numbers signal a successful test
      const DotMatrix::CPU& cpu = gameBoy->cpu;
      bool success = isMooneye ? valuesMatch({ cpu.reg.b, cpu.reg.c, cpu.reg.d, cpu.reg.e, cpu.reg.h, cpu.reg.l }, kMooneyeSuccessValues) : cpu.reg.a == 0;

      return success ? Result::Pass : Result::Fail;
   }

   void runTestCartRange(TestWorkerData data)
   {
      for (std::size_t i = data.firstIndex; i <= data.lastIndex; ++i)
      {
         TestResult& result = data.testResults[i];

         std::string error;
         if (std::optional<std::vector<uint8_t>> cartData = IOUtils::readBinaryFile(result.cartPath))
         {
            if (std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(*cartData), error))
            {
               static const std::string kMooneye = "mooneye";

               bool isMooneye = result.cartPath.generic_string().find(kMooneye) != std::string::npos;
               result.result = runTestCart(std::move(cartridge), data.testTime, isMooneye);
            }
         }

         std::string message = result.cartPath.generic_string() + ": " + getResultName(result.result);
         if (!error.empty())
         {
            message += " (" + error + ")";
         }
         data.messageQueue.enqueue(std::move(message));
      }
   }

   void runTestCartsInPath(std::filesystem::path path, std::filesystem::path resultPath, float time, bool isBlargg)
   {
      std::vector<std::filesystem::path> filePaths = getAllFilePathsRecursive(path);

      // Only try to run .gb files
      filePaths.erase(std::remove_if(filePaths.begin(), filePaths.end(), [](const std::filesystem::path& filePath)
      {
         return filePath.extension() != ".gb";
      }), filePaths.end());

      if (isBlargg)
      {
         // Only run individual tests
         filePaths.erase(std::remove_if(filePaths.begin(), filePaths.end(), [](const std::filesystem::path& filePath)
         {
            if (filePath.has_parent_path())
            {
               std::filesystem::path parentPath = filePath.parent_path();
               std::filesystem::path individualPath = parentPath / "individual";
               std::filesystem::path romSinglesPath = parentPath / "rom_singles";

               if (std::filesystem::is_directory(individualPath) || std::filesystem::is_directory(romSinglesPath))
               {
                  return true;
               }
            }

            return false;
         }), filePaths.end());
      }

      std::sort(filePaths.begin(), filePaths.end());
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

      std::stringstream ss;
      for (const TestResult& result : testResults)
      {
         std::filesystem::path relativePath = result.cartPath.lexically_relative(path);
         ss << '"' << relativePath.generic_string() << '"' << ',' << getResultName(result.result) << '\n';
      }

      IOUtils::writeTextFile(resultPath, ss.str());
   }

   void runProfileInPath(std::filesystem::path path, float time)
   {
      if (std::optional<std::vector<uint8_t>> cartData = IOUtils::readBinaryFile(path))
      {
         std::string error;
         if (std::unique_ptr<DotMatrix::Cartridge> cartridge = DotMatrix::Cartridge::fromData(std::move(*cartData), error))
         {
            std::unique_ptr<DotMatrix::GameBoy> gameBoy = std::make_unique<DotMatrix::GameBoy>();
            gameBoy->setCartridge(std::move(cartridge));

            auto start = std::chrono::high_resolution_clock::now();
            gameBoy->tick(time);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> elapsedSeconds = end - start;
            std::printf("Elapsed time: %f\n", elapsedSeconds.count());
         }
      }
   }
}

int main(int argc, char *argv[])
{
   if (argc > 2)
   {
      static const float kDefaultTestTime = 30.0f;
      float time = kDefaultTestTime;

      std::string type = argv[1];
      std::string pathArg = argv[2];

      if (type == "-test")
      {
         bool isBlargg = false;

         std::optional<std::filesystem::path> testPath;
         std::optional<std::filesystem::path> resultPath;
         if (pathArg == "mooneye")
         {
            testPath = IOUtils::getAboluteProjectPath("Test/Roms/mooneye-gb_hwtests/acceptance");
            resultPath = IOUtils::getAboluteProjectPath("Test/Results/mooneye.csv");
         }
         else if (pathArg == "blargg")
         {
            isBlargg = true;
            testPath = IOUtils::getAboluteProjectPath("Test/Roms/blargg");
            resultPath = IOUtils::getAboluteProjectPath("Test/Results/blargg.csv");
         }
         else
         {
            testPath = pathArg;
            resultPath = IOUtils::getAboluteProjectPath("Test/Results/misc.csv");
         }

         if (argc > 3)
         {
            std::stringstream ss(argv[3]);
            float parsedTime = 0.0f;
            if (ss >> parsedTime)
            {
               time = parsedTime;
            }
         }

         if (testPath && resultPath)
         {
            runTestCartsInPath(*testPath, *resultPath, time, isBlargg);
            return 0;
         }
      }
      else if (type == "-profile")
      {
         static const float kDefaultProfileTime = 1'000.0f;
         float time = kDefaultProfileTime;

         if (argc > 3)
         {
            std::stringstream ss(argv[3]);
            float parsedTime = 0.0f;
            if (ss >> parsedTime)
            {
               time = parsedTime;
            }
         }

         runProfileInPath(pathArg, time);
         return 0;
      }
   }

   std::printf("Usage: %s {-test {suite_name|tests_dir} [test_time] | -profile cart_path [profile_time]}\n", argv[0]);
   return 0;
}
