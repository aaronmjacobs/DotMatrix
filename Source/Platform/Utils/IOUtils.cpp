#include "Core/Assert.h"

#include "Platform/Utils/IOUtils.h"
#include "Platform/Utils/OSUtils.h"

#if GBC_HAS_FILESYSTEM_SUPPORT
#  include <filesystem>
#endif // GBC_HAS_FILESYSTEM_SUPPORT
#include <fstream>
#include <mutex>

namespace IOUtils
{

std::mutex ioMutex;

bool canRead(const std::string& fileName)
{
   ASSERT(!fileName.empty(), "Trying to check empty file name");
   return !!std::ifstream(fileName);
}

bool readTextFile(const std::string& fileName, std::string& data)
{
   ASSERT(!fileName.empty(), "Trying to read from empty file name");
   std::ifstream in(fileName);
   if (!in)
   {
      return false;
   }

   data = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
   return true;
}

bool readTextFileLocked(const std::string& fileName, std::string& data)
{
   std::lock_guard<std::mutex> lock(ioMutex);
   return readTextFile(fileName, data);
}

std::vector<uint8_t> readBinaryFile(const std::string& fileName)
{
   ASSERT(!fileName.empty(), "Trying to read from empty file name");

   std::ifstream in(fileName, std::ifstream::binary);
   if (!in)
   {
      return std::vector<uint8_t>();
   }

   std::streampos start = in.tellg();
   in.seekg(0, std::ios_base::end);
   std::streamoff size = in.tellg() - start;
   ASSERT(size >= 0, "Invalid file size");
   in.seekg(0, std::ios_base::beg);

   std::vector<uint8_t> data(static_cast<size_t>(size));
   in.read(reinterpret_cast<char*>(data.data()), size);

   return data;
}

std::vector<uint8_t> readBinaryFileLocked(const std::string& fileName)
{
   std::lock_guard<std::mutex> lock(ioMutex);
   return readBinaryFile(fileName);
}

bool writeTextFile(const std::string& fileName, const std::string& data)
{
   ASSERT(!fileName.empty(), "Trying to write to empty file name");

   if (!ensurePathToFileExists(fileName))
   {
      return false;
   }

   std::ofstream out(fileName);
   if (!out)
   {
      return false;
   }

   out << data;
   return true;
}

bool writeTextFileLocked(const std::string& fileName, const std::string& data)
{
   std::lock_guard<std::mutex> lock(ioMutex);
   return writeTextFile(fileName, data);
}

bool writeBinaryFile(const std::string& fileName, const std::vector<uint8_t>& data)
{
   ASSERT(!fileName.empty(), "Trying to write to empty file name");

   if (!ensurePathToFileExists(fileName))
   {
      return false;
   }

   std::ofstream out(fileName, std::ofstream::binary);
   if (!out)
   {
      return false;
   }

   out.write(reinterpret_cast<const char*>(data.data()), data.size());
   return true;
}

bool writeBinaryFileLocked(const std::string& fileName, const std::vector<uint8_t>& data)
{
   std::lock_guard<std::mutex> lock(ioMutex);
   return writeBinaryFile(fileName, data);
}

bool appDataPath(const std::string& appName, const std::string& fileName, std::string& path)
{
   std::string appDataFolder;
   if (!OSUtils::getAppDataPath(appName, appDataFolder))
   {
      return false;
   }

   path = appDataFolder + "/" + fileName;
   return true;
}

bool ensurePathToFileExists(const std::string& path)
{
   std::string directory;
   if (!OSUtils::getDirectoryFromPath(path, directory))
   {
      return false;
   }

   if (OSUtils::directoryExists(directory))
   {
      return true;
   }

   return OSUtils::createDirectory(directory);
}

std::vector<std::string> getAllFilePathsRecursive(const std::string& directory)
{
   std::vector<std::string> filePaths;

#if GBC_HAS_FILESYSTEM_SUPPORT
   for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory))
   {
      const std::filesystem::path& path = entry.path();
      std::string pathString = path.string();

      if (canRead(pathString))
      {
         standardizePath(pathString);
         filePaths.push_back(pathString);
      }
   }
#else
   ASSERT(false, "This build was generated without <filesystem> support, getAllFilePathsRecursive() will return an empty vector!");
#endif

   return filePaths;
}

void standardizePath(std::string& path)
{
   std::replace(path.begin(), path.end(), '\\', '/');
}

} // namespace IOUtils
