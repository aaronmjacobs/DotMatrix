#pragma once

#include "Core/Pointers.h"

#include <cstdint>
#include <string>
#include <vector>

namespace IOUtils
{

/**
 * Determines if the file with the given name can be read
 */
bool canRead(const std::string& fileName);

/**
 * Reads the entire contents of the text file with the given name
 */
bool readTextFile(const std::string& fileName, std::string& data);

/**
 * Reads the entire contents of the text file with the given name while under the IOUtils mutex
 */
bool readTextFileLocked(const std::string& fileName, std::string& data);

/**
 * Reads the entire contents of the binary file with the given name
 */
std::vector<uint8_t> readBinaryFile(const std::string& fileName);

/**
 * Reads the entire contents of the binary file with the given name while under the IOUtils mutex
 */
std::vector<uint8_t> readBinaryFileLocked(const std::string& fileName);

/**
 * Writes the contents of the given text to the file with the given name, returning true on success
 */
bool writeTextFile(const std::string& fileName, const std::string& data);

/**
 * Writes the contents of the given text to the file with the given name while under the IOUtils mutex, returning true on success
 */
bool writeTextFileLocked(const std::string& fileName, const std::string& data);

/**
 * Writes the contents of the given vector to the file with the given name, returning true on success
 */
bool writeBinaryFile(const std::string& fileName, const std::vector<uint8_t>& data);

/**
 * Writes the contents of the given vector to the file with the given name while under the IOUtils mutex, returning true on success
 */
bool writeBinaryFileLocked(const std::string& fileName, const std::vector<uint8_t>& data);

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
 * Standardizes path representation
 */
void standardizePath(std::string& path);

} // namespace IOUtils
