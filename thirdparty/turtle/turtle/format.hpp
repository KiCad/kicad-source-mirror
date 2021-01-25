// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FORMAT_HPP_INCLUDED
#define MOCK_FORMAT_HPP_INCLUDED

#include "config.hpp"
#include "detail/formatter.hpp"

namespace mock {
template<typename T>
detail::formatter<T> format(const T& t)
{
    return detail::formatter<T>(t);
}

} // namespace mock

#endif // MOCK_FORMAT_HPP_INCLUDED
