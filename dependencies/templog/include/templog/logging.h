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
* templog logging header
*
* This file defines all the actual logging stuff. 
*
*/

/**
* 
* \page page_templog_overview Overview 
* 
* \section sec_templog_intro Introduction
* 
* The templog logging library is designed with (portable) efficiency being the most important 
* goal. In order to achieve this, it relies on template meta programming and inlining to allow 
* filtering of log messages at compile-time. Also, log messages use lazy evaluation to allow 
* efficient run-time filtering of those log messages that aren't weeded out at compile-time. 
* 
* The second most important goal was ease of use. Therefor log messages can be created using 
* the stream idiom. 
* 
* \section sec_templog_loggers Loggers
* 
* In order to log messages using the templog library it's necessary to first to define a logger. 
* A logger is an instance of the logger template which forwards log messages. Each logger is 
* passed (by way of template parameters) a minimum severity and a list of audiences. Log 
* messages passed to a logger with a severity less than the logger's severity or with a 
* targeted audience which is not in the logger's audience list will be dropped by the logger at 
* compile-time. 
* 
* Each logger instance is passed (also by way of a template parameter) another logger instance 
* to which, after applying the above mentioned filtering, it forwards its log messages. That 
* instance, after applying filtering according to its minimum severity and its audience list, 
* in turn forwards its log messages to some other logger instance. Several loggers can forward 
* to the same other logger, so that loggers form a direct, acyclic graph. 
* 
* \section sec_templog_policies Formatting and write policies
* 
* At the root of the resulting logger hierarchy always is a non_filtering_logger which uses a 
* formatting policy (see formatting_policy_base, passed as a template argument) to format log 
* messages and a write policy (see write_policy_base, also passed as a template argument) to 
* write them to some log message sink. Write policies have a meta function writes which can be 
* used to globally filter log messages at compile-time and a is_writing() static member 
* function that can be used to globally filter log messages at run-time. 
* 
* There are several predefined formatting and write policies. There are also default policies 
* (see std_formating_policy and std_write_policy) and a type global_logger using these which 
* can be used as the root of a logging hierarchy. User-defined loggers can directly or 
* indirectly forward their log messages to this global logger: 
\verbatim
  typedef templog::logger< templog::global_logger
                         , templog::sev_info
                         , templog::audience_list<aud_developer,aud_support,aud_user> >
                                                    myModuleLogger;
  typedef templog::logger< templog::myModuleLogger
                         , templog::sev_message
                         , templog::audience_list<aud_support> >
                                                    mySupportLogger;
  typedef templog::logger< templog::myModuleLogger
                         , templog::sev_message
                         , templog::audience_list<aud_user> >
                                                    myUserLogger;
\endverbatim
* \section sec_templog_macro The TEMPLOG macro
* 
* In order to log a message using a logger, a log_forwarder obtained from that logger must be 
* assigned a compile-time list of log_intermediate instances referring to the log parameters. 
* An easy way to do this is using the TEMPLOG_LOG macro, passing the logger, the log message's 
* severity, and its intended audience. The result of that macro invocation can be streamed into
* using the common ostream operators. For example, this 
\verbatim
  TEMPLOG_LOG(mySupportLogger,sev_debug,aud_support) << "x = " << x;
\endverbatim
* uses the logger mySupportLogger to log a message with the severity sev_debug, and the 
* targeted audience aud_support that consists of the string "x = " and the value of x. 
* Note that, according to its definition, the logger mySupportLogger will drop this log 
* message, because sev_debug is below its minimum severity sev_message. 
* 
* \section sec_templog_onoff Turning logging volume up or down
* 
* If loggers are created so that their hierarchy resembles a software system's architecture, it 
* is rather easy to either quieten the logging behaviour of whole parts of the system through 
* restricting loggers' filtering, or alternatively make parts of the systems log more verbosely 
* by removing restrictions from their loggers (and, if necessary, have them forwarding directly 
* to the global_logger, so that their log messages won't be discarded by intermediate loggers). 
* 
* \sa logger
* \sa global_logger
* \sa formatting_policy_base
* \sa write_policy_base
* \sa std_formating_policy
* \sa std_write_policy
*/

#if !defined(TEMPLOG_LOGGING_H)
#define TEMPLOG_LOGGING_H

/**********************************************************************************************/
#include "config.h"

/**********************************************************************************************/
#include <typeinfo>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

/**********************************************************************************************/
#include "type_lists.h"
#include "tuples.h"

/**********************************************************************************************/

namespace templog {

	/******************************************************************************************/
	/* source information */

#	define TEMPLOG_SOURCE                                __FILE__, __LINE__                                             //!< source info parameters
#	define TEMPLOG_SOURCE_SIGN                           const char* const file_, const unsigned long line_             //!< source info parameter signature
#	define TEMPLOG_SOURCE_SIGN_IGNORE                    const char* const , const unsigned long                        //!< source info parameter signature ignoring parameter names
#	define TEMPLOG_SOURCE_PASS                           file_, line_                                                   //!< passing source info parameters
#	define TEMPLOG_SOURCE_INIT                           file_data_(file_), line_data_(line_)                           //!< initializing source info data members with source info parameters
#	define TEMPLOG_SOURCE_ASSIGN(Obj_)                   file_data_=Obj_.file_data_; line_data_=Obj_.line_data_         //!< assigning source info parameters to source info data members

#	define TEMPLOG_SOURCE_FILE                           file_                                                          //!< source info file parameter
#	define TEMPLOG_SOURCE_LINE                           line_                                                          //!< source info line parameter

#	define TEMPLOG_SOURCE_DATA                           const char* const file_data_; const unsigned long line_data_   //!< source info data members
#	define TEMPLOG_SOURCE_DATA_PASS                      file_data_, line_data_                                         //!< passing source info data members
#	define TEMPLOG_SOURCE_DATA_FILE                      file_data_                                                     //!< source file data member
#	define TEMPLOG_SOURCE_DATA_LINE                      line_data_                                                     //!< source line data member

#	define TEMPLOG_SOURCE_ACCESS(Obj_)                   Obj_.file_data_, Obj_.line_data_                               //!< accessing source info data members
#	define TEMPLOG_SOURCE_ACCESS_FILE(Obj_)              Obj_.file_data_                                                //!< accessing source info file data members
#	define TEMPLOG_SOURCE_ACCESS_LINE(Obj_)              Obj_.line_data_                                                //!< accessing source info line data members

	/******************************************************************************************/
	/* severities and audiences */

	/**
	* Logging severity
	*
	* This determines a log statement's severity. Log messages can be filtered by their 
	* severity. 
	*/
	enum severity { sev_debug   = 1 //!< debugging
	              , sev_info        //!< informational
	              , sev_message     //!< messages
	              , sev_warning     //!< warnings
	              , sev_error       //!< errors
	              , sev_fatal       //!< fatal errors
	              };

	/**
	* Logging audience
	*
	* This determines a log statement's target audience. Log messages can be filtered by their 
	* audience. 
	*/
	enum audience { aud_developer = 1 //!< helping developers
	              , aud_support       //!< helping support
	              , aud_user          //!< for users
	              };

	/**
	* Logging audience list creator meta function
	*
	* This creates a compile-time list containing the passed audiences. 
	*/
	template< int Aud1_, int Aud2_ = 0, int Aud3_ = 0 >
	struct audience_list : public templ_meta::int_type_list_generator< 0
		                                                               , Aud1_
		                                                               , Aud2_
		                                                               , Aud3_
		                                                               >::result_type
	{
		typedef typename templ_meta::int_type_list_generator< 0
		                                                    , Aud1_
		                                                    , Aud2_
		                                                    , Aud3_
		                                                    >::result_type
		                                                result_type;
	};

	/**
	* Retrieves a textual description for a severity
	*/
	const char* get_name(severity sev);

	/**
	* Retrieves a short (three letters) textual description for a severity
	*/
	const char* get_short_name(severity sev);

	/**
	* Retrieves a textual description for an audience
	*/
	const char* get_name(audience aud);

	/**
	* Retrieves a short (three letters) textual description for an audience
	*/
	const char* get_short_name(audience aud);

	/******************************************************************************************/
	/* log intermediate */

	/**
	* Log message intermediate type
	* 
	* This is used to store subexpressions of logging statements. 
	*/
	template< int Sev_, int Aud_, bool Forward_, class Params_ >
	struct log_intermediate {
		typedef Params_                                 parameters_type;

		parameters_type                     parameters;

		log_intermediate( const parameters_type& p = parameters_type() )
			: parameters(p) {}

	};

	/**
	* Adds a parameter to a subexpression of a log statement
	* 
	* This version is used for log statements that will be discarded at compile time.
	*/
	template< int Sev_, int Aud_, class ParamList_, typename Param_ >
	inline
	log_intermediate< Sev_, Aud_, false, templ_meta::nil >
	operator<<( const log_intermediate<Sev_,Aud_,false,ParamList_>&, const Param_& )
	{
		typedef log_intermediate< Sev_, Aud_
		                        , false
		                        , templ_meta::nil >     log_intermediate_t;
		return log_intermediate_t();
	}

	/**
	* Adds a parameter to a subexpression of a log statement
	* 
	* This version is used for log statements that will not be discarded at compile time.
	*/
	template< int Sev_, int Aud_, class ParamList_, typename Param_ >
	inline
	log_intermediate< Sev_, Aud_, true, templ_meta::tuple<const Param_*,ParamList_> >
	operator<<( const log_intermediate<Sev_,Aud_,true,ParamList_>& lim, const Param_& p )
	{
		typedef log_intermediate< Sev_, Aud_
		                        , true
		                        , templ_meta::tuple<const Param_*,ParamList_> >
		                                                log_intermediate_t;
		return log_intermediate_t( templ_meta::tuple<const Param_*,ParamList_>(&p,lim.parameters) );
	}

	/******************************************************************************************/
	/* log forwarder */

	/**
	* Log statement forwarder type
	* 
	* Log forwarder objects store a log messages source info and, upon assignment of a 
	* log_intermediate object, feed the latter to the logger type's fwd() * function. 
	*/
	template< class Logger_ >
	struct log_forwarder {
		TEMPLOG_SOURCE_DATA;

		log_forwarder(TEMPLOG_SOURCE_SIGN)
			: TEMPLOG_SOURCE_INIT {}

		template< int Sev_, int Aud_, bool Forward_, class Params_ >
		void operator=(const log_intermediate<Sev_,Aud_,Forward_,Params_>& li)
		{ Logger_::fwd(TEMPLOG_SOURCE_DATA_PASS,li); }

		log_forwarder& operator=(const log_forwarder& rhs)
		{ TEMPLOG_SOURCE_ASSIGN(rhs); return *this; }

	};

	/******************************************************************************************/
	/* logger */

	/**
	* Logger type
	* 
	* Loggers forward log messages to other loggers forming a hierarchy as a directed, acyclic 
	* graph. There's global logger type at the root of this hierarchy. 
	* 
	* Before forwarding, messages are filtered (at compile-time) by their severity and their 
	* intended audience. Only log messages with a severity equal or above the logger's minimal 
	* severity and with a targeted audience that's included in the logger's audience list will 
	* be forwarded. 
	* 
	* A log message that isn't eliminated by any logger's filtering will eventually end up in 
	* the global logger. The global logger will then perform run-time filtering. 
	* 
	*/
	template< class Logger_
	        , int MinSeverity_
	        , class AudienceList_ >
	class logger;

	template< class Logger_
	        , int MinSeverity_
	        , int Aud1_, int Aud2_, int Aud3_ >
	class logger< Logger_, MinSeverity_, audience_list<Aud1_,Aud2_,Aud3_> > {
	private:
		typedef typename audience_list<Aud1_,Aud2_,Aud3_>::result_type
		                                                audience_list_type;
		template< int S_, int A_ >
		struct evaluate_params {
			enum { sev = ( MinSeverity_ <= S_ )
			     , aud = templ_meta::int_type_list_includes<audience_list_type,A_>::result
			     , result = (sev && aud) };
		};
	public:
		/**
		* this logger's type
		*/
		typedef logger< Logger_, MinSeverity_, audience_list<Aud1_,Aud2_,Aud3_> >
		                                                logger_type;
		/**
		* the logger this one forwards to
		*/
		typedef Logger_                                 forward_logger_type;

		/**
		* creates a log_forwarder object for this logger
		*/
		static
		log_forwarder<logger_type>
		get_forwarder(TEMPLOG_SOURCE_SIGN)
		{ return log_forwarder<logger_type>(TEMPLOG_SOURCE_PASS); }

		/**
		* Creates a log_intermediate object for this logger for a specific severity/audience 
		* combination. if that combination is to be filtered out, the intermediate object will 
		* be flagged as not to be forwarded. 
		*/
		template< int Sev_, int Aud_ >
		static 
		log_intermediate<Sev_,Aud_,evaluate_params<Sev_,Aud_>::result,templ_meta::nil>
		get_intermediate()
		{ return log_intermediate< Sev_, Aud_, evaluate_params<Sev_,Aud_>::result, templ_meta::nil >(); }

		/**
		* Discards a log_intermediate object that's flagged as not to be forwarded. 
		*/
		template< int Sev_, int Aud_, class Params_ >
		static void fwd( TEMPLOG_SOURCE_SIGN_IGNORE, const log_intermediate< Sev_, Aud_, false, Params_ >& )
		{}

		/**
		* Forwards a log_intermediate object to this logger's forward_logger_type. 
		*/
		template< int Sev_, int Aud_, class Params_ >
		static void fwd( TEMPLOG_SOURCE_SIGN       , const log_intermediate< Sev_, Aud_, true , Params_ >& lim )
		{ forward_logger_type::fwd(TEMPLOG_SOURCE_PASS,lim); }
	};

	/******************************************************************************************/
	/* formatting policies */

	/**
	* formatting policy base class
	* 
	* Formatting policies describe how log messages are to be written. In order to achieve 
	* maximum performance (by exploring compile-time decisions and heavily inlined code) 
	* formatting policies have to tightly work together with write policies. 
	* 
	* There already are a few pre-defined formatting policies. If you want to write your own, 
	* this class provides all the boiler-plate for writing arbitrary objects as well as all of 
	* the log statement's parameters in collaboration with the write policy. 
	* 
	* If you want to use this code, derive from this class, passing the derived class' type as 
	* a template parameter. In your derived class implement a function with this
	*   template< class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_ >
	*   static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN, const ParamList_& parameters);
	* signature which uses the base class' write_obj() and write_params() functions in order 
	* to write a formatted log message to an arbitrary sink. 
	* 
	* \sa write_policy_base
	* \sa sev_aud_formating_policy
	* \sa visual_studio_formating_policy
	*/
	template< typename FormattingPolicy_ >
	class formating_policy_base {
	public:
		/**
		* Writes a log message using the derived class' write() function. 
		*/
		template< class WritePolicy_, int Sev_, int Aud_, class ParamList_ >
		static void write_msg(TEMPLOG_SOURCE_SIGN, const ParamList_& parameters)
		{
			typedef typename WritePolicy_::incremental_write_support
			                                              incremental_write_support;
			do_write<WritePolicy_,Sev_,Aud_>(TEMPLOG_SOURCE_PASS, parameters, incremental_write_support());
		}

	protected:
		/**
		* Calls this from derived classes in order to write arbitrary objects. 
		* \note Objects must have a output stream operator overloaded. 
		*/
		template< class WritePolicy_, class WriteToken_, typename T >
		static void write_obj(WriteToken_& token, const T& obj)
		{ do_write_obj<WritePolicy_>(token,obj); }

		/**
		* Call this from derived classes in order to write a parameter list. 
		*/
		template< class WritePolicy_, class WriteToken_, class Param_, class ParamList_ >
		static void write_params( WriteToken_& token 
		                        , const templ_meta::tuple<const Param_*,ParamList_>& parameters )
		{ do_write_params_<WritePolicy_>(token,parameters); }

	private:
		template< class WritePolicy_, int Sev_, int Aud_, class ParamList_ >
		static void do_write(TEMPLOG_SOURCE_SIGN, const ParamList_& parameters, templ_meta::boolean<true>)
		{
			templ_meta::nil dummy;
			WritePolicy_::begin_write();
			FormattingPolicy_::template write<WritePolicy_,Sev_,Aud_>(dummy,TEMPLOG_SOURCE_PASS,parameters);
			do_write_endl<WritePolicy_>(dummy,typename WritePolicy_::needs_endl());
			WritePolicy_::end_write();
		}

		template< class WritePolicy_, int Sev_, int Aud_, class ParamList_ >
		static void do_write(TEMPLOG_SOURCE_SIGN, const ParamList_& parameters, templ_meta::boolean<false>)
		{
			std::ostringstream oss;
			FormattingPolicy_::template write<WritePolicy_,Sev_,Aud_>(oss,TEMPLOG_SOURCE_PASS,parameters);
			do_write_endl<WritePolicy_>(oss,typename WritePolicy_::needs_endl());
			WritePolicy_::write_str(oss.str());
		}

		template< class WritePolicy_, typename T >
		static void do_write_obj(templ_meta::nil&, const T& obj)
		{ WritePolicy_::write_obj(obj); }

		template< class WritePolicy_, typename T >
		static void do_write_obj(std::ostream& os, const T& obj)
		{ os << obj; }

		template< class WritePolicy_, class WriteToken_, class Param_, class ParamList_ >
		static void do_write_params_( WriteToken_& token 
		                            , const templ_meta::tuple<const Param_*,ParamList_>& parameters )
		{
			do_write_params_<WritePolicy_>(token,parameters.tail);
			do_write_params_<WritePolicy_>(token,parameters.head);
		}

		template< class WritePolicy_, class WriteToken_, class Param_ >
		static void do_write_params_( WriteToken_& token 
		                            , const Param_* pparameter )
		{ write_obj<WritePolicy_>(token,*pparameter); }

		template< class WritePolicy_, class WriteToken_ >
		static void do_write_params_( WriteToken_& /**token*/ 
		                            , templ_meta::nil /**parameters*/ )
		{}

		template< class WritePolicy_, class WriteToken_ >
		static void do_write_endl( WriteToken_& token 
		                         , templ_meta::boolean<true> )
		{ write_obj<WritePolicy_>(token,'\n'); }

		template< class WritePolicy_, class WriteToken_ >
		static void do_write_endl( WriteToken_& token 
		                         , templ_meta::boolean<false> )
		{}
	};

	/**
	* Pre-defined formatting policy. 
	* 
	* This formatting policy write a log message including its severity and audience. 
	*/
	class sev_aud_formating_policy : public formating_policy_base<sev_aud_formating_policy> {
	public:
		template< class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_ >
		static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN_IGNORE, const ParamList_& parameters)
		{
			write_obj<WritePolicy_>( token, '<' );
			write_obj<WritePolicy_>( token, get_short_name(static_cast<severity>(Sev_)) );
			write_obj<WritePolicy_>( token, '|' );
			write_obj<WritePolicy_>( token, get_short_name(static_cast<audience>(Aud_)) );
			write_obj<WritePolicy_>( token, ">: " );
			write_params<WritePolicy_>(token, parameters);
		}
	};

	/**
	* Pre-defined formatting policy. 
	* 
	* This formatting policy write a log message in the format used by the Visual Studio's 
	* output window. (Double-clicking on such a log message in the output window makes the IDE 
	* jump to the source line containing the log statement.) This is especially useful in 
	* conjunction with the windbg_write_policy. 
	* 
	* \sa windbg_write_policy
	*/
	template< class FwdFormattingPolicy_ >
	class visual_studio_formating_policy : public formating_policy_base< visual_studio_formating_policy<FwdFormattingPolicy_> > {
	public:
		template< class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_ >
		static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN, const ParamList_& parameters)
		{
			typedef formating_policy_base< visual_studio_formating_policy<FwdFormattingPolicy_> > base;
			base::template write_obj<WritePolicy_>( token, TEMPLOG_SOURCE_FILE );
			base::template write_obj<WritePolicy_>( token, '(' );
			base::template write_obj<WritePolicy_>( token, TEMPLOG_SOURCE_LINE );
			base::template write_obj<WritePolicy_>( token, ") : " );
			FwdFormattingPolicy_::template write<WritePolicy_,Sev_,Aud_>( token, TEMPLOG_SOURCE_PASS, parameters );
		}
	};

	/******************************************************************************************/
	/* writing policies */

	/**
	* write policy base class
	* 
	* Write policy determine how and where log messages get written to. They need to closely 
	* collaborate with formatting policies, in order to achieve a high degree of inlining and 
	* thus performance. 
	* 
	* Write policies either allow incremental (e.g. stream-like) writing or non-incremental 
	* writing (which need to write a whole log message as one string). 
	* 
	* There already are a few pre-defined write policies. If you want to write your own, this 
	* class provides all the boiler-plate code for doing so. For convenience, you can derive 
	* from either incremental_write_policy_base or non_incremental_write_policy_base. 
	* 
	* \sa formatting_policy_base
	* \sa incremental_write_policy_base
	* \sa non_incremental_write_policy_base
	*/
	template< class WritePolicy_, bool Endl_, bool Incremental_ >
	class write_policy_base {
	public:
		typedef templ_meta::boolean<Incremental_>         incremental_write_support;
		typedef templ_meta::boolean<Endl_>                needs_endl;

		template< class FormattingPolicy_, int Sev_, int Aud_, class ParamList_ >
		static void write_msg(TEMPLOG_SOURCE_SIGN, const ParamList_& parameters)
		{ FormattingPolicy_::template write_msg<WritePolicy_,Sev_,Aud_>(TEMPLOG_SOURCE_PASS, parameters); }
	};

	/**
	* incremental write policy base class
	* 
	* Derive from this class to create your own incremental write policy. Pass your derived 
	* class' type as the first template parameter, and a boolean indicating whether you need a 
	* '\n' character appended to the end of each log message as the second. 
	* 
	* Your derived class needs to implement the meta function
	*   template< int Sev_, int Aud_ >
	*   struct writes { enum { result = true }; };
	* which determines whether any given log message is filtered at compile-time, the static 
	* function 
	*   static bool is_writing(int Sev, int Aud);
	* which determines whether a given log message is filtered at run-time, and the following
	* three functions
	*   static void begin_write();
	*   template< typename T >
	*   static void write_obj(const T& obj);
	*   static void end_write();
	* which are called at the beginning of the output of a log message, for each objects to be
	* written, and at the end of a log message, respectively. 
	* 
	* \sa write_policy_base
	*/
	template< class WritePolicy_, bool Endl_ >
	class incremental_write_policy_base : public write_policy_base<WritePolicy_,Endl_,true > {};

	/**
	* non-incremental write policy base class
	* 
	* Derive from this class to create your own non-incremental write policy. Pass your derived 
	* class' type as the first template parameter, and a boolean indicating whether you need a 
	* '\n' character appended to the end of each log message as the second. 
	* 
	* Your derived class needs to implement the meta function
	*   template< int Sev_, int Aud_ >
	*   struct writes { enum { result = true }; };
	* which determines whether any given log message is filtered at compile-time, the static 
	* function 
	*   static bool is_writing(int Sev, int Aud);
	* which determines whether a given log message is filtered at run-time, and the function
	*   static void write_str(const std::string& str);
	* which writes the log message to the sink. 
	* 
	* \sa write_policy_base
	*/
	template< class WritePolicy_, bool Endl_ >
	class non_incremental_write_policy_base : public write_policy_base<WritePolicy_,Endl_,false> {};

	/**
	* Pre-defined write policy. 
	* 
	* This incremental write policy writes log messages to std::cerr. 
	*/
	class stderr_write_policy : public incremental_write_policy_base<stderr_write_policy,true> {
	public:
		template< int Sev_, int Aud_ >
		struct writes { enum { result = true }; };

		static bool is_writing(int /*sev*/, int /*aud*/){return true;}

		static void begin_write()                       {}

		template< typename T >
		static void write_obj(const T& obj)             {std::cerr << obj;}

		static void end_write()                         {}

	};

	/**
	* Pre-defined write policy. 
	* 
	* This non-incremental write policy writes log messages to Windows' debug console. 
	* This is especially useful in conjunction with the visual_studio_formating_policy. 
	* 
	* \sa visual_studio_formating_policy
	*/
#	if defined(_WIN32)
		class windbg_write_policy : public non_incremental_write_policy_base<windbg_write_policy,true> {
		public:
			template< int Sev_, int Aud_ >
			struct writes { enum { result = true }; };

			static bool is_writing(int /*sev*/, int /*aud*/){return true;}

			static void write_str(const std::string& str);
		};
#	endif //defined(_WIN32)

	/**
	* Pre-defined write policy. 
	* 
	* This non-incremental write policy allows switiching of write policies at run-time. 
	* 
	* \note This disallows filtering at compile-time. All log messages passed to this policy 
	* are filtered at run-time. 
	* 
	* \sa visual_studio_formating_policy
	*/
	class dynamic_write_policy : public non_incremental_write_policy_base<dynamic_write_policy,true> {
	public:
		/**
		* dynamic write policy base class
		*/
		class writer_base {
		public:
			virtual bool is_writing(int, int) const        {return true;}
			virtual void write_str(const std::string& str) const = 0;
			//virtual ~writer_base() {} // virtual dtor not needed, no dynamic instances
		};

		/**
		* dynamic write policy base class
		* 
		* derive from this if you need to create your own dynamic write policies
		*/
		template< class WritePolicy_ >
		class writer : public writer_base {
		public:
			virtual void write_str(const std::string& str) const
			{ do_write_str(str,typename WritePolicy_::incremental_write_support()); }
		private:
			static void do_write_str(const std::string& str, templ_meta::boolean<true > )
			{ WritePolicy_::write_obj(str); }
			static void do_write_str(const std::string& str, templ_meta::boolean<false> )
			{ WritePolicy_::write_str(str); }
		};

		template< int Sev_, int Aud_ >
		struct writes { enum { result = true }; };

		static bool is_writing(int sev, int aud)
		{ const writer_base* ptr=get(); return ptr ? ptr->is_writing(sev,aud) : false; }

		static void write_str(const std::string& str)
		{ const writer_base* ptr=get(); if(ptr) ptr->write_str(str);  }

		/**
		* sets a dynamic write policy as the current writer
		* 
		* \result previous write policy, NULL if there isn't one
		*/
		static const writer_base* set_writer(const writer_base* pw)
		{ std::swap(pw,get()); return pw; }

		/**
		* Automatic dynamic write policy
		* 
		* Instances classes derived from this class will automatically be registered as 
		* the current dynamic write policy. 
		*/
		template< class WritePolicy_ >
		class auto_writer : public writer<WritePolicy_> {
		public:
			auto_writer()                                 {set_writer(this);}
			~auto_writer()                                {set_writer(NULL);}
		};

	private:
		static const writer_base*& get()
		{ static const writer_base* ptr_=NULL; return ptr_; }
	};

	/******************************************************************************************/
	/* default formatting and write policies*/

#	if defined(_WIN32)
		typedef visual_studio_formating_policy<sev_aud_formating_policy>  std_formating_policy;
		typedef windbg_write_policy                                       std_write_policy;
#	else //defined(_WIN32)
		typedef sev_aud_formating_policy                                  std_formating_policy;
		typedef stderr_write_policy                                       std_write_policy;
#	endif //defined(_WIN32)

	/******************************************************************************************/
	/* global logger */

	/**
	* Global logger template
	* 
	* This is the global logger type which, eventually, all other loggers forward log messages 
	* to. 
	* 
	* There's no need to use this directly. 
	*/
	template< class FormattingPolicy_, class WritePolicy_ >
	class non_filtering_logger {
	public:
		template< int Sev_, int Aud_, class Params_ >
		static void fwd( TEMPLOG_SOURCE_SIGN, const log_intermediate< Sev_, Aud_, false , Params_ >& )
		{}

		template< int Sev_, int Aud_, class Params_ >
		static void fwd( TEMPLOG_SOURCE_SIGN, const log_intermediate< Sev_, Aud_, true , Params_ >& lim )
		{ WritePolicy_::template write_msg<FormattingPolicy_,Sev_,Aud_>(TEMPLOG_SOURCE_PASS, lim.parameters); }
	};

	/**
	* Global logger
	* 
	* This is the global logger which, eventually, all other loggers forward log messages * to. 
	* 
	* There's no need to use this directly. 
	*/
	typedef non_filtering_logger< std_formating_policy
	                            , std_write_policy>   global_logger;

	/******************************************************************************************/

}

using templog::sev_debug;
using templog::sev_info;
using templog::sev_message;
using templog::sev_warning;
using templog::sev_error;
using templog::sev_fatal;

using templog::aud_developer;
using templog::aud_support;
using templog::aud_user;

/**
* Logging macro
* 
* Use this to create log messages. The result of the macro can be streamed into: 
*   TEMPLOG_LOG(myLogger,sev_debug,aud_support) << "x = " << x;
* \note Objects streamed into this must be of types that overload the ostream operator in the 
*       usual way. 
*/
#define TEMPLOG_LOG(TLogger_,Sev_,Aud_) TLogger_::get_forwarder(TEMPLOG_SOURCE) = \
                                        TLogger_::template get_intermediate<Sev_,Aud_>()

/**********************************************************************************************/

#endif //defined(TEMPLOG_LOGGING_H)

/**********************************************************************************************/
/* EOF */
