// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2008
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_ERROR_HPP_INCLUDED
#define MOCK_ERROR_HPP_INCLUDED

#include "config.hpp"
#ifdef MOCK_USE_BOOST_TEST
#    include "exception.hpp"
#    include <boost/exception/enable_current_exception.hpp>
#    include <boost/test/framework.hpp>
#    include <boost/test/test_tools.hpp>
#    include <boost/test/unit_test_suite.hpp>
#    include <boost/version.hpp>

namespace mock {
template<typename Result>
struct error
{
    static Result abort()
    {
        boost::unit_test::framework::test_unit_aborted(boost::unit_test::framework::current_test_case());
        throw boost::enable_current_exception(exception());
    }

    static void pass(const char* file, int line)
    {
        boost::unit_test::unit_test_log.set_checkpoint(file, static_cast<std::size_t>(line));
    }

    template<typename Context>
    static void fail(const char* message, const Context& context, const char* file = "unknown location", int line = 0)
    {
        boost::unit_test::framework::assertion_result(
#    if BOOST_VERSION < 105900
          false
#    else
          boost::unit_test::AR_FAILED
#    endif
        );
        boost::unit_test::unit_test_log << boost::unit_test::log::begin(file, static_cast<std::size_t>(line))
                                        << boost::unit_test::log_all_errors << message << ": " << context
                                        << boost::unit_test::log::end();
    }

    template<typename Context>
    static void call(const Context& context, const char* file, int line)
    {
        boost::unit_test::framework::assertion_result(
#    if BOOST_VERSION < 105900
          true
#    else
          boost::unit_test::AR_PASSED
#    endif
        );
        boost::unit_test::unit_test_log << boost::unit_test::log::begin(file, static_cast<std::size_t>(line))
                                        << boost::unit_test::log_successful_tests
                                        << "mock expectation fulfilled: " << context << boost::unit_test::log::end();
    }
};
} // namespace mock

#endif // MOCK_USE_BOOST_TEST

#endif // MOCK_ERROR_HPP_INCLUDED
