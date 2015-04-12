#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <boxer/boxer.h>
#include <templog/logging.h>

#include <ctime>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace {

// Width of the severity tag in log output (e.g. [ warning ])
const int SEV_NAME_WIDTH = 9;

// Delimiter for splitting the message and title of logs to be written by the boxer_write_policy
const std::string BOXER_MESSAGE_SPLIT = "|||";
const size_t BOXER_MESSAGE_SPLIT_LEN = 3;

// Used for centering content in the log output
std::string center(std::string input, int width) { 
   return std::string((width - input.length()) / 2, ' ') + input + std::string((width - input.length() + 1) / 2, ' ');
}

// Format time to a string (mm-dd-yyyy hh:mm:ss)
std::string formatTime(struct tm *tm) {
   std::stringstream ss;

   ss << std::setfill('0')
      << std::setw(2) << tm->tm_mon + 1 << '-'
      << std::setw(2) << tm->tm_mday << '-'
      << std::setw(4) << tm->tm_year + 1900 << ' '
      << std::setw(2) << tm->tm_hour << ':'
      << std::setw(2) << tm->tm_min << ':'
      << std::setw(2) << tm->tm_sec;

   return ss.str();
}

// Format time to a string (mm-dd-yyyy-hh)
std::string formatTimeForFilename(struct tm *tm) {
   std::stringstream ss;

   ss << std::setfill('0')
   << std::setw(2) << tm->tm_mon + 1 << '-'
   << std::setw(2) << tm->tm_mday << '-'
   << std::setw(4) << tm->tm_year + 1900 << '-'
   << std::setw(2) << tm->tm_hour;

   return ss.str();
}

// Textual output formatting policy (console, file, etc.)
class text_formating_policy : public templog::formating_policy_base<text_formating_policy> {
public:
   template< class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_ >
   static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN, const ParamList_& parameters) {
      auto t = std::time(nullptr);
      auto tm = std::localtime(&t);

      write_obj<WritePolicy_>(token, '[');
      write_obj<WritePolicy_>(token, center(get_name(static_cast<templog::severity>(Sev_)), SEV_NAME_WIDTH));
      write_obj<WritePolicy_>(token, "] <");
      write_obj<WritePolicy_>(token, formatTime(tm));
      write_obj<WritePolicy_>(token, "> ");
      write_params<WritePolicy_>(token, parameters);
   }
};

// Message box formatting policy
class boxer_formating_policy : public templog::formating_policy_base<boxer_formating_policy> {
public:
   template< class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_ >
   static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN_IGNORE, const ParamList_& parameters) {
      write_params<WritePolicy_>(token, parameters);
   }
};

// File writing policy
class file_write_policy : public templog::non_incremental_write_policy_base<file_write_policy,true> {
public:
   template< int Sev_, int Aud_ >
   struct writes { enum { result = true }; };

   static bool is_writing(int /*sev*/, int /*aud*/){return true;}

   static void write_str(const std::string& str) {
      auto t = std::time(nullptr);
      auto tm = std::localtime(&t);

      std::ofstream out(formatTimeForFilename(tm) + ".log", std::ios_base::app);
      if (out) {
         out << str;
      }
   }
};

// Message box writing policy
template<boxer::Style style>
class boxer_write_policy : public templog::non_incremental_write_policy_base<boxer_write_policy<style>,false> {
public:
   template< int Sev_, int Aud_ >
   struct writes { enum { result = true }; };

   static bool is_writing(int /*sev*/, int /*aud*/){return true;}

   static void write_str(const std::string& str) {
      size_t split = str.find(BOXER_MESSAGE_SPLIT);
      boxer::show(str.substr(0, split).c_str(), str.substr(split + BOXER_MESSAGE_SPLIT_LEN).c_str(), style);
   }
};

// Prevent text logging in release builds
#define CERR_SEV_THRESHOLD templog::sev_fatal + 1
#define FILE_SEV_THRESHOLD templog::sev_fatal + 1

#ifndef NDEBUG
#undef CERR_SEV_THRESHOLD
#define CERR_SEV_THRESHOLD templog::sev_debug

#ifdef LOG_TO_FILE
#undef FILE_SEV_THRESHOLD
#define FILE_SEV_THRESHOLD templog::sev_info
#endif

#endif

typedef templog::logger<templog::non_filtering_logger<text_formating_policy, templog::std_write_policy>
                      , CERR_SEV_THRESHOLD
                      , templog::audience_list<templog::aud_developer, templog::aud_support, templog::aud_user>>
                      cerr_logger;

typedef templog::logger<templog::non_filtering_logger<text_formating_policy, file_write_policy>
                      , FILE_SEV_THRESHOLD
                      , templog::audience_list<templog::aud_developer, templog::aud_support, templog::aud_user>>
                      file_logger;

typedef templog::logger<templog::non_filtering_logger<boxer_formating_policy, boxer_write_policy<boxer::Style::Error>>
                      , templog::sev_error
                      , templog::audience_list<templog::aud_developer, templog::aud_support, templog::aud_user>>
                      boxer_logger;

} // namespace

// Send the log to each of our loggers (and let them do the filtering)
#define LOG(_log_message_, _log_title_, _log_severity_, _log_audience_) \
do { \
   TEMPLOG_LOG(cerr_logger, _log_severity_, _log_audience_) << _log_message_; \
   TEMPLOG_LOG(file_logger, _log_severity_, _log_audience_) << _log_message_; \
   TEMPLOG_LOG(boxer_logger, _log_severity_, _log_audience_) << _log_message_ << BOXER_MESSAGE_SPLIT << _log_title_; \
} while (0)

// Simplify logging calls

// Debug   : Used to check values, locations, etc. (sort of a replacement for printf)
// Info    : For logging interesting, but expected, information
// Message : For logging more detailed informational messages
// Warning : For logging information of concern, which may cause issues
// Error   : For logging errors that do not prevent the program from continuing
// Fatal   : For logging fatal errors that prevent the program from continuing

#define LOG_DEBUG(_log_message_) LOG(_log_message_, "Debug", templog::sev_debug, templog::aud_developer)
#define LOG_INFO(_log_message_) LOG(_log_message_, "Information", templog::sev_info, templog::aud_developer)
#define LOG_MESSAGE(_log_message_) LOG(_log_message_, "Message", templog::sev_message, templog::aud_developer)
#define LOG_WARNING(_log_message_) LOG(_log_message_, "Warning", templog::sev_warning, templog::aud_developer)
#define LOG_ERROR(_log_message_) LOG(_log_message_, "Error", templog::sev_error, templog::aud_user)
#define LOG_FATAL(_log_message_) \
do { \
   LOG(_log_message_, "Fatal Error", templog::sev_fatal, templog::aud_user); \
   abort(); \
} while(0)

#endif
