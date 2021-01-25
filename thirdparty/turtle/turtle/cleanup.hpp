// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CLEANUP_HPP_INCLUDED
#define MOCK_CLEANUP_HPP_INCLUDED

#include "config.hpp"
#include "reset.hpp"
#include "verify.hpp"
#ifdef MOCK_USE_BOOST_TEST
#    include <boost/test/unit_test_suite.hpp>
#endif

namespace mock {
struct cleanup
{
    ~cleanup() { mock::reset(); }
};

#ifdef MOCK_USE_BOOST_TEST
BOOST_GLOBAL_FIXTURE(cleanup)
#    if BOOST_VERSION >= 105900
  ;
#    endif
#endif

} // namespace mock

#endif // MOCK_CLEANUP_HPP_INCLUDED
