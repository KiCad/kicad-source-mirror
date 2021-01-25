// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_EXCEPTION_HPP_INCLUDED
#define MOCK_EXCEPTION_HPP_INCLUDED

#include "config.hpp"
#ifdef MOCK_USE_BOOST_TEST
#    include <boost/test/execution_monitor.hpp>

namespace mock {
struct exception : virtual boost::execution_aborted
{};
} // namespace mock

#endif // MOCK_USE_BOOST_TEST

#endif // MOCK_EXCEPTION_HPP_INCLUDED
