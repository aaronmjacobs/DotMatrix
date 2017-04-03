#include "OSUtils.h"

#include <cstdint>
#include <ctime>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // __APPLE__

#ifdef __linux__
#include <cstring>
#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // __linux__

#ifdef _WIN32
#include <codecvt>
#include <cstring>
#include <locale>
#include <ShlObj.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <Windows.h>
#endif // _WIN32

namespace OSUtils {

#ifdef __APPLE__
bool getExecutablePath(std::string& executablePath) {
   uint32_t size = MAXPATHLEN;
   char rawPath[size];
   if (_NSGetExecutablePath(rawPath, &size) != 0) {
      return false;
   }

   char realPath[size];
   if (!realpath(rawPath, realPath)) {
      return false;
   }

   executablePath = std::string(realPath);
   return true;
}

bool getAppDataPath(const std::string& appName, std::string& appDataPath) {
   FSRef ref;
   FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &ref);

   char path[PATH_MAX];
   FSRefMakePath(&ref, (UInt8*)&path, PATH_MAX);

   appDataPath = std::string(path) + "/" + appName;
   return true;
}

bool setWorkingDirectory(const std::string& dir) {
   return chdir(dir.c_str()) == 0;
}

bool createDirectory(const std::string& dir) {
   return mkdir(dir.c_str(), 0755) == 0;
}
#endif // __APPLE__

#ifdef __linux__
bool getExecutablePath(std::string& executablePath) {
   char path[PATH_MAX + 1];

   ssize_t numBytes = readlink("/proc/self/exe", path, PATH_MAX);
   if (numBytes == -1) {
      return false;
   }
   path[numBytes] = '\0';

   executablePath = std::string(path);
   return true;
}

bool getAppDataPath(const std::string& appName, std::string& appDataPath) {
   // First, check the HOME environment variable
   char* homePath = secure_getenv("HOME");

   // If it isn't set, grab the directory from the password entry file
   if (!homePath) {
      homePath = getpwuid(getuid())->pw_dir;
   }

   if (!homePath) {
      return false;
   }

   appDataPath = std::string(homePath) + "/.config/" + appName;
   return true;
}

bool setWorkingDirectory(const std::string& dir) {
   return chdir(dir.c_str()) == 0;
}

bool createDirectory(const std::string& dir) {
   return mkdir(dir.c_str(), 0755) == 0;
}
#endif // __linux__

#ifdef _WIN32
bool getExecutablePath(std::string& executablePath) {
   TCHAR buffer[MAX_PATH + 1];
   DWORD length = GetModuleFileName(NULL, buffer, MAX_PATH);
   buffer[length] = '\0';
   int error = GetLastError();

   if (length == 0 || length == MAX_PATH || error == ERROR_INSUFFICIENT_BUFFER) {
      const DWORD REALLY_BIG = 32767;
      TCHAR unreasonablyLargeBuffer[REALLY_BIG + 1];
      length = GetModuleFileName(NULL, unreasonablyLargeBuffer, REALLY_BIG);
      unreasonablyLargeBuffer[length] = '\0';
      error = GetLastError();

      if (length == 0 || length == REALLY_BIG || error == ERROR_INSUFFICIENT_BUFFER) {
         return false;
      }

      executablePath = std::string(unreasonablyLargeBuffer);
   } else {
      executablePath = std::string(buffer);
   }

   return true;
}

bool getAppDataPath(const std::string& appName, std::string& appDataPath) {
   PWSTR path;
   if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path) != S_OK) {
      CoTaskMemFree(path);
      return false;
   }

   std::wstring widePathStr(path);
   CoTaskMemFree(path);

   std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
   appDataPath = converter.to_bytes(widePathStr) + "/" + appName;
   return true;
}

bool setWorkingDirectory(const std::string& dir) {
   return SetCurrentDirectory(dir.c_str()) != 0;
}

bool createDirectory(const std::string& dir) {
   return CreateDirectory(dir.c_str(), nullptr) != 0;
}
#endif // _WIN32

bool getDirectoryFromPath(const std::string& path, std::string& dir) {
   size_t pos = path.find_last_of("/\\");
   if (pos == std::string::npos) {
      return false;
   }

   dir = path.substr(0, pos);
   return true;
}

bool fixWorkingDirectory() {
   std::string executablePath;
   if (!getExecutablePath(executablePath)) {
      return false;
   }

   std::string executableDir;
   if (!getDirectoryFromPath(executablePath, executableDir)) {
      return false;
   }

   return setWorkingDirectory(executableDir);
}

bool directoryExists(const std::string& dir) {
   struct stat info;

   if(stat(dir.c_str(), &info) != 0) {
      return false;
   }

   return (info.st_mode & S_IFDIR) != 0;
}

long getTime() {
   return static_cast<long>(std::time(nullptr));
}

} // namespace OSUtils
