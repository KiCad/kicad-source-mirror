// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MOVE_HELPER_HPP_INCLUDED
#define MOCK_MOVE_HELPER_HPP_INCLUDED

#include "../config.hpp"
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_rvalue_reference.hpp>
#include <boost/type_traits/decay.hpp>

namespace mock
{
namespace detail
{
#ifdef MOCK_RVALUE_REFERENCES
    template< typename T >
    struct forward_type
    {
        typedef T type;
    };
    template< typename T >
    struct ref_arg
    {
        typedef typename boost::conditional<
          boost::is_reference< T >::value,
          T,
          typename boost::add_rvalue_reference< T >::type >::type type;
    };

    template< typename T >
    inline T&& move_if_not_lvalue_reference(typename boost::remove_reference< T >::type& t)
    {
        return static_cast< T&& >(t);
    }

    template< typename T >
    inline T&& move_if_not_lvalue_reference(typename boost::remove_reference< T >::type&& t)
    {
      return static_cast< T&& >(t);
    }
#else
    template< typename T >
    struct forward_type
    {
        typedef typename boost::decay< const T >::type type;
    };
    template< class T>
    struct forward_type< boost::rv< T > >
    {
        typedef T type;
    };
    template< typename T >
    struct ref_arg
    {
        typedef typename boost::conditional<
          boost::is_reference< T >::value,
          T,
          const typename boost::add_reference< T >::type >::type type;
    };
    template< typename T >
    inline typename boost::remove_reference< T >::type& move_if_not_lvalue_reference(typename boost::remove_reference< T >::type& t)
    {
        return t;
    }
#endif
}
}

#endif  // MOCK_MOVE_HELPER_HPP_INCLUDED
