#if !defined(__APPLE__)
#  error "Trying to compile macOS-only source, but '__APPLE__' isn't defined!"
#endif // !defined(__APPLE__)

#include "Platform/Utils/OSUtils.h"

#import <Foundation/Foundation.h>

#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace OSUtils
{

bool getExecutablePath(std::string& executablePath)
{
   uint32_t size = MAXPATHLEN;
   char rawPath[size];
   if (_NSGetExecutablePath(rawPath, &size) != 0)
   {
      return false;
   }

   char realPath[size];
   if (!realpath(rawPath, realPath))
   {
      return false;
   }

   executablePath = std::string(realPath);
   return true;
}

bool getAppDataPath(const std::string& appName, std::string& appDataPath)
{
   NSURL* appSupportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
                                                          inDomain:NSUserDomainMask
                                                          appropriateForURL:nil
                                                          create:YES
                                                          error:nil];
   if (!appSupportURL)
   {
      return false;
   }
   
   appDataPath = std::string([[appSupportURL path] cStringUsingEncoding:NSASCIIStringEncoding]) + "/" + appName;
   return true;
}

bool setWorkingDirectory(const std::string& dir)
{
   return chdir(dir.c_str()) == 0;
}

bool createDirectory(const std::string& dir)
{
   return mkdir(dir.c_str(), 0755) == 0;
}

} // namespace OSUtils
