// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon and Ovanes Markarian 2017
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CATCH_HPP_INCLUDED
#define MOCK_CATCH_HPP_INCLUDED

#include <catch.hpp>

template<typename Result>
struct catch_mock_error_policy
{
    static Result abort()
    {
        FAIL("Aborted");
        throw std::runtime_error("unreachable");
    }

    template<typename Context>
    static void fail(const char* message,
                     const Context& context,
                     const char* file = "file://unknown-location",
                     int line = 0)
    {
        CAPTURE(context);
        FAIL_CHECK(message << " in: " << file << ":" << line);
    }

    template<typename Context>
    static void call(const Context& context, const char* file, int line)
    {
        CAPTURE(context);
        INFO(file << ":" << line);
    }

    static void pass(const char* file, int line) { INFO(file << ":" << line); }
};

#define MOCK_ERROR_POLICY catch_mock_error_policy
#include "mock.hpp"

#endif // MOCK_CATCH_HPP_INCLUDED
