// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2009
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// boost-no-inspect

#ifndef MOCK_CONFIG_HPP_INCLUDED
#define MOCK_CONFIG_HPP_INCLUDED

#include <boost/config.hpp>

#ifndef MOCK_ERROR_POLICY
#    define MOCK_ERROR_POLICY mock::error
#    define MOCK_USE_BOOST_TEST
#endif

#if !defined(BOOST_NO_CXX11_HDR_MUTEX) && !defined(BOOST_NO_0X_HDR_MUTEX)
#    ifndef MOCK_NO_HDR_MUTEX
#        define MOCK_HDR_MUTEX
#    endif
#endif

#if defined(__cpp_lib_uncaught_exceptions) || defined(_MSC_VER) && (_MSC_VER >= 1900)
#    ifndef MOCK_NO_UNCAUGHT_EXCEPTIONS
#        define MOCK_UNCAUGHT_EXCEPTIONS
#    endif
#endif

#endif // MOCK_CONFIG_HPP_INCLUDED
