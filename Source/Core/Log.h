#pragma once

#include <boxer/boxer.h>
#include <logging.h>

#include <cstring>
#include <ctime>
#include <string>

// Delimiter for splitting the title and message of logs to be written by the boxer_write_policy
#define LOG_BOXER_MESSAGE_SPLIT "|"

// Severity names
#define LOG_SEV_DEBUG "Debug"
#define LOG_SEV_INFO "Information"
#define LOG_SEV_MESSAGE "Message"
#define LOG_SEV_WARNING "Warning"
#define LOG_SEV_ERROR "Error"
#define LOG_SEV_FATAL "Fatal Error"

namespace Log
{
   boxer::Style boxerStyle(const std::string& sev);
   std::string center(const std::string& input, int width);
   std::string formatTime(const tm& time);
   std::string hex(uint8_t value);
   std::string hex(uint16_t value);
   tm getCurrentTime();

   // Text formatting policy
   class text_formating_policy : public templog::formating_policy_base<text_formating_policy>
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

   // Message box formatting policy
   class boxer_formating_policy : public templog::formating_policy_base<boxer_formating_policy>
   {
   public:
      template<class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_>
      static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN_IGNORE, const ParamList_& parameters)
      {
         write_params<WritePolicy_>(token, parameters);
      }
   };

   // Message box writing policy
   class boxer_write_policy : public templog::non_incremental_write_policy_base<boxer_write_policy, false>
   {
   public:
      template<int Sev_, int Aud_>
      struct writes { enum { result = true }; };

      static bool is_writing(int /*sev*/, int /*aud*/) { return true; }

      static void write_str(const std::string& str)
      {
         size_t split = str.find(LOG_BOXER_MESSAGE_SPLIT);

         std::string sevName = LOG_SEV_ERROR;
         std::string message = str;
         // Should always be found, but in some off chance that it isn't, we don't want to break here
         if (split != std::string::npos)
         {
            sevName = str.substr(0, split);
            message = str.substr(split + std::strlen(LOG_BOXER_MESSAGE_SPLIT));
         }

         boxer::show(message.c_str(), sevName.c_str(), boxerStyle(sevName));
      }
   };

#if SWAP_DEBUG
#  define LOG_CERR_SEV_THRESHOLD templog::sev_debug
#  define LOG_MSG_BOX_SEV_THRESHOLD templog::sev_debug
#else // SWAP_DEBUG
// Prevent text logging in release builds
#  define LOG_CERR_SEV_THRESHOLD templog::sev_fatal + 1
// Prevent debug messages boxes in release builds
#  define LOG_MSG_BOX_SEV_THRESHOLD templog::sev_info
#endif // SWAP_DEBUG

   using cerr_logger = templog::logger<templog::non_filtering_logger<text_formating_policy, templog::std_write_policy>
                                       , LOG_CERR_SEV_THRESHOLD
                                       , templog::audience_list<templog::aud_developer>>;

   using boxer_logger = templog::logger<templog::non_filtering_logger<boxer_formating_policy, boxer_write_policy>
                                       , LOG_MSG_BOX_SEV_THRESHOLD
                                       , templog::audience_list<templog::aud_user>>; // Only show message boxes when the user should be notified
}

// Simplify logging calls

// Debug   : Used to check values, locations, etc. (sort of a replacement for printf)
// Info    : For logging interesting, but expected, information
// Message : For logging more detailed informational messages
// Warning : For logging information of concern, which may cause issues
// Error   : For logging errors that do not prevent the program from continuing
// Fatal   : For logging fatal errors that prevent the program from continuing

#define LOG(_log_message_, _log_title_, _log_severity_) \
do \
{ \
   TEMPLOG_LOG(Log::cerr_logger, _log_severity_, templog::aud_developer) << _log_message_; \
} while (0)

#define LOG_DEBUG(_log_message_) LOG(_log_message_, LOG_SEV_DEBUG, templog::sev_debug)
#define LOG_INFO(_log_message_) LOG(_log_message_, LOG_SEV_INFO, templog::sev_info)
#define LOG_MESSAGE(_log_message_) LOG(_log_message_, LOG_SEV_MESSAGE, templog::sev_message)
#define LOG_WARNING(_log_message_) LOG(_log_message_, LOG_SEV_WARNING, templog::sev_warning)
#define LOG_ERROR(_log_message_) LOG(_log_message_, LOG_SEV_ERROR, templog::sev_error)

// Easy calls to create message boxes

#define LOG_MSG_BOX(_log_message_, _log_title_, _log_severity_) \
do \
{ \
   TEMPLOG_LOG(Log::boxer_logger, _log_severity_, templog::aud_user) << _log_title_ << LOG_BOXER_MESSAGE_SPLIT << _log_message_; \
} while (0)

#define LOG_DEBUG_MSG_BOX(_log_message_) LOG_MSG_BOX(_log_message_, LOG_SEV_DEBUG, templog::sev_debug)
#define LOG_INFO_MSG_BOX(_log_message_) LOG_MSG_BOX(_log_message_, LOG_SEV_INFO, templog::sev_info)
#define LOG_MESSAGE_MSG_BOX(_log_message_) LOG_MSG_BOX(_log_message_, LOG_SEV_MESSAGE, templog::sev_message)
#define LOG_WARNING_MSG_BOX(_log_message_) LOG_MSG_BOX(_log_message_, LOG_SEV_WARNING, templog::sev_warning)
#define LOG_ERROR_MSG_BOX(_log_message_) LOG_MSG_BOX(_log_message_, LOG_SEV_ERROR, templog::sev_error)

// Fatal errors (kill the program, even in release)

#define LOG_FATAL(_log_message_) \
do \
{ \
   LOG(_log_message_, LOG_SEV_FATAL, templog::sev_fatal); \
   abort(); \
} while(0)
#define LOG_FATAL_MSG_BOX(_log_message_) \
do \
{ \
   LOG_MSG_BOX(_log_message_, LOG_SEV_FATAL, templog::sev_fatal); \
   abort(); \
} while(0)
