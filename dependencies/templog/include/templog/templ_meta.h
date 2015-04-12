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
* templog template meta header
*
* This defines some general template meta stuff. 
*
* This file is used internally by the templog library. There's no need to include this file 
* manually. 
*/

#if !defined(TEMPLOG_TEMPL_META_H)
#define TEMPLOG_TEMPL_META_H

/******************************************************************************/
#include "config.h"

/******************************************************************************/

namespace templog {

	namespace templ_meta {

		/**
		* compile-time list NIL type
		*/
		struct nil {};

		/**
		* compile-time boolean type
		*/
		template< bool B_ >
		struct boolean {
			typedef boolean<B_>                             result_type; //!< simplifies some meta code
			enum { result = B_ };
		};

		/**
		* classical int-to-type template
		*/
		template< int I_ >
		struct int2type { enum { result = I_ }; };

		/**
		* compile-time if
		*/
		template< class Bool_, typename T1_, typename T2_ > struct if_;
		template<              typename T1_, typename T2_ > struct if_<boolean<true >,T1_,T2_> {typedef T1_ result_type;};
		template<              typename T1_, typename T2_ > struct if_<boolean<false>,T1_,T2_> {typedef T2_ result_type;};

	}

}

/******************************************************************************/

#endif //defined(TEMPLOG_TEMPL_META_H)

/******************************************************************************/
/* EOF */
