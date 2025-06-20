#pragma once

// macOS defines nil in MacTypes.h
#if defined(nil)
#undef nil
#endif
#include <logging.h>

#include <cstring>
#include <ctime>
#include <string>
#if DM_PROJECT_PLAYDATE
#include <sstream>
#endif // DM_PROJECT_PLAYDATE

namespace DotMatrix
{

namespace Log
{
   std::string center(const std::string& input, int width);
   std::string formatTime(const tm& time);
   std::string hex(uint8_t value);
   std::string hex(uint16_t value);
   tm getCurrentTime();

   // Text formatting policy
   class text_formatting_policy : public templog::formating_policy_base<text_formatting_policy>
   {
   public:
      template<class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_>
      static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN, const ParamList_& parameters)
      {
         // Width of the severity tag in log output (e.g. [ warning ])
         static const int kSevNameWidth = 9;

         write_obj<WritePolicy_>(token, TEMPLOG_SOURCE_FILE);
         write_obj<WritePolicy_>(token, "(");
         write_obj<WritePolicy_>(token, TEMPLOG_SOURCE_LINE);
         write_obj<WritePolicy_>(token, "): ");
         write_obj<WritePolicy_>(token, "[");
         write_obj<WritePolicy_>(token, center(get_name(static_cast<templog::severity>(Sev_)), kSevNameWidth));
         write_obj<WritePolicy_>(token, "] <");
         write_obj<WritePolicy_>(token, formatTime(getCurrentTime()));
         write_obj<WritePolicy_>(token, "> ");

         write_params<WritePolicy_>(token, parameters);
      }
   };

#if DM_WITH_DEBUG_UTILS
#  define DM_LOG_CERR_SEV_THRESHOLD templog::sev_debug
#else
// Prevent text logging in release builds
#  define DM_LOG_CERR_SEV_THRESHOLD templog::sev_fatal + 1
#endif

#if DM_PROJECT_PLAYDATE
   class playdate_write_policy : public templog::incremental_write_policy_base<playdate_write_policy, false>
   {
   public:
      template< int Sev_, int Aud_ >
      struct writes { enum { result = true }; };

      static bool is_writing(int /*sev*/, int /*aud*/) { return true; }

      static void begin_write() { ss = std::ostringstream{}; }

      template< typename T >
      static void write_obj(const T& obj) { ss << obj; }

      static void end_write();

   private:
      static std::ostringstream ss;
   };

   using dm_write_policy = playdate_write_policy;
#else
   using dm_write_policy = templog::std_write_policy;
#endif // DM_PROJECT_PLAYDATE

   using dm_logger = templog::logger<templog::non_filtering_logger<text_formatting_policy, dm_write_policy>
      , DM_LOG_CERR_SEV_THRESHOLD
      , templog::audience_list<templog::aud_developer>>;

#undef DM_LOG_CERR_SEV_THRESHOLD
}

} // namespace DotMatrix

// Simplify logging calls

// Debug   : Used to check values, locations, etc. (sort of a replacement for printf)
// Info    : For logging interesting, but expected, information
// Message : For logging more detailed informational messages
// Warning : For logging information of concern, which may cause issues
// Error   : For logging errors that do not prevent the program from continuing
// Fatal   : For logging fatal errors that prevent the program from continuing

#if DM_WITH_DEBUG_UTILS
#  define DM_LOG(log_message, log_severity) do { TEMPLOG_LOG(DotMatrix::Log::dm_logger, log_severity, templog::aud_developer) << log_message; } while (0)
#else
#  define DM_LOG(_log_message_, log_severity) do {} while (0)
#endif

#define DM_LOG_DEBUG(log_message) DM_LOG(log_message, templog::sev_debug)
#define DM_LOG_INFO(log_message) DM_LOG(log_message, templog::sev_info)
#define DM_LOG_MESSAGE(log_message) DM_LOG(log_message, templog::sev_message)
#define DM_LOG_WARNING(log_message) DM_LOG(log_message, templog::sev_warning)
#define DM_LOG_ERROR(log_message) DM_LOG(log_message, templog::sev_error)

// Fatal errors (kill the program, even in release)

#define DM_LOG_FATAL(log_message) \
do \
{ \
   LOG(log_message, templog::sev_fatal); \
   abort(); \
} while(0)
