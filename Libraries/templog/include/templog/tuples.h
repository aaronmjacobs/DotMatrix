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
* templog tuples header
*
* This defines a simple tuple type and a few useful operations for it. 
*
* This file is used internally by the templog library. There's no need to include this file 
* manually. 
*/

#if !defined(TEMPLOG_TUPLES_H)
#define TEMPLOG_TUPLES_H

/******************************************************************************/
#include "config.h"
#include "templ_meta.h"

/******************************************************************************/

namespace templog {

	namespace templ_meta {

		/**
		* typle type
		*/
		template< typename Head_, class Tail_ >
		struct tuple {
			typedef Head_                                   head_type;
			typedef Tail_                                   tail_type;

			head_type                           head;
			tail_type                           tail;

			tuple( const head_type& h
			     , const tail_type& t )                   : head(h), tail(t) {}
		};

		/**
		* tuple type generator meta function
		*/
		template< typename T01_ = nil, typename T02_ = nil, typename T03_ = nil
		        , typename T04_ = nil, typename T05_ = nil, typename T06_ = nil
		        , typename T07_ = nil, typename T08_ = nil, typename T09_ = nil >
		struct tuple_generator {
		private:
			template< class Tuple_ > 
			struct nil_remover_;
			template< typename Head_, class Tail_ >
			struct nil_remover_< tuple<Head_,Tail_> > {
				typedef tuple<Head_,typename nil_remover_<Tail_>::result_type>
				                                              result_type;
			};
			template< typename Head_ >
			struct nil_remover_< tuple<Head_,nil> > {
				typedef type_list<Head_,nil>                  result_type;
			};
			typedef tuple< T01_
			      , tuple< T02_
			      , tuple< T03_
			      , tuple< T04_
			      , tuple< T05_
			      , tuple< T06_
			      , tuple< T07_
			      , tuple< T08_
			      , tuple< T09_, nil > > > > > > > > >      long_list_type_;
		public:
			typedef typename nil_remover_<long_list_type_>::result_type
			                                                result_type;
		};

		/**
		* meta function
		* checks whether a tuple includes a specific type
		*/
		template< class Tuple_, typename T >
		struct tuple_includes;

		template< typename T >
		struct tuple_includes< nil, T >
			: public boolean<false> {};

		template< class Tail_, typename T >
		struct tuple_includes< tuple<T,Tail_>, T >
			: public boolean<true> {};

		template< typename Head_, class Tail_, typename T >
		struct tuple_includes< tuple<Head_,Tail_>, T >
			: public tuple_includes<Tail_,T> {};

	}

}

/******************************************************************************/

#endif //defined(TEMPLOG_TUPLES_H)

/******************************************************************************/
/* EOF */
