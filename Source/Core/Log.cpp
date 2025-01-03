#include "Core/Log.h"

#include <iomanip>
#include <sstream>

#if DM_PROJECT_PLAYDATE
#include <pdcpp/pdnewlib.h>

extern PlaydateAPI* g_pd;
#endif // DM_PROJECT_PLAYDATE

namespace DotMatrix
{

namespace Log
{
   // Used for centering content in the log output
   std::string center(const std::string& input, int width)
   {
      return std::string((width - input.length()) / 2, ' ') + input + std::string((width - input.length() + 1) / 2, ' ');
   }

   // Format time to a string (hh:mm:ss)
   std::string formatTime(const tm& time)
   {
      std::stringstream ss;

      ss << std::setfill('0')
         << std::setw(2) << time.tm_hour << ':'
         << std::setw(2) << time.tm_min << ':'
         << std::setw(2) << time.tm_sec;

      return ss.str();
   }

   std::string hex(uint8_t value)
   {
      std::ostringstream stream;
      stream << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint16_t>(value);
      return stream.str();
   }

   std::string hex(uint16_t value)
   {
      std::ostringstream stream;
      stream << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << value;
      return stream.str();
   }

   tm getCurrentTime()
   {
      time_t now(std::time(nullptr));
      tm time;

#if defined(_POSIX_VERSION)
      localtime_r(&now, &time);
#elif defined(_MSC_VER)
      localtime_s(&time, &now);
#else
      time = *localtime(&now);
#endif

      return time;
   }

#if DM_PROJECT_PLAYDATE
   // static
   std::ostringstream playdate_write_policy::ss;

   // static
   void playdate_write_policy::end_write()
   {
      if (g_pd)
      {
         g_pd->system->logToConsole(ss.str().c_str());
      }
   }
#endif // DM_PROJECT_PLAYDATE
}

} // namespace DotMatrix
