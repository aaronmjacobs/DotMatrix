#ifndef OS_UTILS_H
#define OS_UTILS_H

#include <string>

namespace OSUtils {

/**
 * Gets the path to the running executable, returning true on success
 */
bool getExecutablePath(std::string& executablePath);

/**
 * Gets the path to the application's local data / settings / config folder, returning true on success
 */
bool getAppDataPath(const std::string& appName, std::string& appDataPath);

/**
 * Extracts the directory from the given path, returning true on success
 */
bool getDirectoryFromPath(const std::string& path, std::string& dir);

/**
 * Sets the working directory of the application to the given directory, returning true on success
 */
bool setWorkingDirectory(const std::string& dir);

/**
 * Sets the working directory of the application to the directory that the executable is in, returning true on success
 */
bool fixWorkingDirectory();

/**
 * Determines if the given directory exists
 */
bool directoryExists(const std::string& dir);

/**
 * Creates the given directory, returning true on success
 */
bool createDirectory(const std::string& dir);

/**
 * Gets the system (unix) time (in seconds)
 */
long getTime();

} // namespace OSUtils

#endif
