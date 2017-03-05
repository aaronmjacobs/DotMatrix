#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "Pointers.h"

#include <cstdint>
#include <string>
#include <vector>

namespace IOUtils {

/**
 * Determines if the file with the given name can be read
 */
bool canRead(const std::string& fileName);

/**
 * Reads the entire contents of the text file with the given name
 */
bool readTextFile(const std::string& fileName, std::string& data);

/**
 * Reads the entire contents of the binary file with the given name
 */
std::vector<uint8_t> readBinaryFile(const std::string& fileName);

/**
 * Writes the contents of the given text to the file with the given name, returning true on success
 */
bool writeTextFile(const std::string& fileName, const std::string& data);

/**
 * Writes the contents of the given vector to the file with the given name, returning true on success
 */
bool writeBinaryFile(const std::string& fileName, const std::vector<uint8_t>& data);

/**
 * Gets the absolute path of a resource stored in the app data folder given a relative path and application name,
 * returning true on success
 */
bool appDataPath(const std::string& appName, const std::string& fileName, std::string& path);

/**
 * Ensures the path to the given file exists, returning true on success
 */
bool ensurePathToFileExists(const std::string& path);

/**
 * Recursively gets all file paths for the given directory
 */
std::vector<std::string> getAllFilePathsRecursive(const std::string& directory);

/**
 * Helper class for reading data from / writing data to a byte array
 */
class Archive {
public:
   Archive() : offset(0) {
   }

   Archive(size_t numBytes) : data(numBytes), offset(0) {
   }

   Archive(const std::vector<uint8_t>& inData) : data(inData), offset(0) {
   }

   Archive(const Archive& other) : data(other.data), offset(other.offset) {
   }

   Archive(Archive&& other) : data(std::move(other.data)), offset(std::move(other.offset)) {
      other.offset = 0;
   }

   ~Archive() {
      ASSERT(isAtEnd());
   }

   Archive& operator=(const Archive& other) {
      data = other.data;
      offset = other.offset;

      return *this;
   }

   Archive& operator=(Archive&& other) {
      data = std::move(other.data);
      offset = std::move(other.offset);
      other.offset = 0;

      return *this;
   }

   const std::vector<uint8_t>& getData() const {
      return data;
   }

   bool isAtEnd() const {
      return offset == data.size();
   }

   void reserve(size_t numBytes) {
      data.resize(numBytes);
   }

   template<typename T>
   bool read(T& outVal) {
      size_t numBytes = sizeof(T);
      if (offset + numBytes > data.size()) {
         return false;
      }

      memcpy(&outVal, &data[offset], numBytes);
      offset += numBytes;

      return true;
   }

   template<typename T>
   void write(const T& inVal) {
      size_t numBytes = sizeof(T);
      if (offset + numBytes > data.size()) {
         data.resize(offset + numBytes);
      }

      memcpy(&data[offset], &inVal, numBytes);
      offset += numBytes;
   }

private:
   std::vector<uint8_t> data;
   size_t offset;
};

} // namespace IOUtils

#endif
