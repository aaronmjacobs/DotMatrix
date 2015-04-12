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
* templog type list header
*
* This defines generic type lists and a few useful operations for them. 
*
* This file is used internally by the templog library. There's no need to include this file 
* manually. 
*/

#if !defined(TEMPLOG_TYPELISTS_H)
#define TEMPLOG_TYPELISTS_H

/******************************************************************************/
#include "config.h"
#include "templ_meta.h"

/******************************************************************************/

namespace templog {

	namespace templ_meta {

		/**
		* compile-time type list type
		*/
		template< typename Head_, class Tail_ >
		struct type_list {
			typedef Head_                                   head_type;
			typedef Tail_                                   tail_type;
		};

		/**
		* compile-time type list generator meta function
		*/
		template< typename T01_ = nil, typename T02_ = nil, typename T03_ = nil
		        , typename T04_ = nil, typename T05_ = nil, typename T06_ = nil
		        , typename T07_ = nil, typename T08_ = nil, typename T09_ = nil >
		struct type_list_generator {
		private:
			template< class TypeList_ > 
			struct nil_remover_;
			template< typename Head_, class Tail_ >
			struct nil_remover_< type_list<Head_,Tail_> > {
				typedef type_list<Head_,typename nil_remover_<Tail_>::result_type>
				                                              result_type;
			};
			template< typename Head_ >
			struct nil_remover_< type_list<Head_,nil> > {
				typedef type_list<Head_,nil>                  result_type;
			};
			typedef type_list< T01_
			      , type_list< T02_
			      , type_list< T03_
			      , type_list< T04_
			      , type_list< T05_
			      , type_list< T06_
			      , type_list< T07_
			      , type_list< T08_
			      , type_list< T09_, nil > > > > > > > > >  long_list_type_;
		public:
			typedef typename nil_remover_<long_list_type_>::result_type
			                                                result_type;
		};

		/**
		* compile-time int type list meta function
		*/
		template< int Invalid_ = 0
		        , int I01_ = Invalid_, int I02_ = Invalid_, int I03_ = Invalid_
		        , int I04_ = Invalid_, int I05_ = Invalid_, int I06_ = Invalid_
		        , int I07_ = Invalid_, int I08_ = Invalid_, int I09_ = Invalid_ >
		struct int_type_list_generator 
			: public type_list_generator< int2type<I01_>, int2type<I02_>, int2type<I03_>
			                            , int2type<I04_>, int2type<I05_>, int2type<I06_>
			                            , int2type<I07_>, int2type<I08_>, int2type<I09_> > 
		{};

		/**
		* meta function
		* checks whether a type list includes a specific type
		*/
		template< class TypeList_, typename T >
		struct type_list_includes;

		template< typename T >
		struct type_list_includes< nil, T >
			: public boolean<false> {};

		template< class Tail_, typename T >
		struct type_list_includes< type_list<T,Tail_>, T >
			: public boolean<true> {};

		template< typename Head_, class Tail_, typename T >
		struct type_list_includes< type_list<Head_,Tail_>, T >
			: public type_list_includes<Tail_,T> {};

		/**
		* meta function
		* checks whether an int type list includes a specific vlaue
		*/
		template< class TypeList_, int I >
		struct int_type_list_includes : public type_list_includes< TypeList_, int2type<I> > {};

	}

}

/******************************************************************************/

#endif //defined(TEMPLOG_TYPELISTS_H)

/******************************************************************************/
/* EOF */
