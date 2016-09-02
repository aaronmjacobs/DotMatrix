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
 * Reads the entire contents of the binary file with the given name, storing the number of bytes in numBytes if set
 */
std::vector<uint8_t> readBinaryFile(const std::string& fileName);

/**
 * Writes the contents of the given text to the file with the given name, returning true on success
 */
bool writeTextFile(const std::string& fileName, const std::string& data);

/**
 * Writes the contents of the given array to the file with the given name, returning true on success
 */
bool writeBinaryFile(const std::string& fileName, uint8_t* data, size_t numBytes);

/**
 * Gets the absolute path of a resource stored in the app data folder given a relative path and application name,
 * returning true on success
 */
bool appDataPath(const std::string& appName, const std::string& fileName, std::string& path);

} // namespace IOUtils

#endif
