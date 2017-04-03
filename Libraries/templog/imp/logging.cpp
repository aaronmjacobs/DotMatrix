/*
*
* templog library
*
* Copyright Hendrik Schober
* Distributed under the Boost Software License, Version 1.0.
*    (See accompanying file LICENSE_1_0.txt or copy at
*     http://www.boost.org/LICENSE_1_0.txt)
*
* see http://templog.sourceforge.net/ for more info
*
*/

/**
* 
* \file
* templog logging implementation
*
*/

/**********************************************************************************************/
#include "templog/logging.h"

/**********************************************************************************************/
// system header
#if defined(_WIN32)
#   include <windows.h>
#endif //defined(_WIN32)

namespace templog {

	/******************************************************************************************/

	const char* get_name(severity sev)
	{
		switch(sev) {
			case sev_debug   : return "debug";
			case sev_info    : return "info";
			case sev_message : return "message";
			case sev_warning : return "warning";
			case sev_error   : return "error";
			case sev_fatal   : return "fatal";
		};
		return "unknown severity";
	}

	const char* get_short_name(severity sev)
	{
		switch(sev) {
			case sev_debug   : return "dbg";
			case sev_info    : return "inf";
			case sev_message : return "msg";
			case sev_warning : return "wrn";
			case sev_error   : return "err";
			case sev_fatal   : return "ftl";
		};
		return "unknown severity";
	}

	const char* get_name(audience aud)
	{
		switch(aud) {
			case aud_developer : return "developer";
			case aud_support   : return "support";
			case aud_user      : return "user";
		}
		return "unknown audience";
	}

	const char* get_short_name(audience aud)
	{
		switch(aud) {
			case aud_developer : return "dev";
			case aud_support   : return "sup";
			case aud_user      : return "usr";
		}
		return "unknown audience";
	}

	/******************************************************************************************/

#	if defined(_WIN32)
		//class windbg_write_policy {
		//public:
			void windbg_write_policy::write_str(const std::string& str)
			{
				::OutputDebugString(str.c_str());
			}
		//};
#	endif //defined(_WIN32)

	/******************************************************************************************/

}

/**********************************************************************************************/
/* EOF */
