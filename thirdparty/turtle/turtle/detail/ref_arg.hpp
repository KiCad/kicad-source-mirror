// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_REF_ARG_HPP_INCLUDED
#define MOCK_REF_ARG_HPP_INCLUDED

#include <type_traits>

namespace mock { namespace detail {
    /// Make T a reference type:
    /// If T is already a reference type, return T, else return an rvalue reference to T
    /// Useful to pass along arguments keeping the ability to modify them (e.g. move from them)
    template<typename T>
    using ref_arg_t = typename std::conditional<std::is_reference<T>::value, T, std::add_rvalue_reference_t<T>>::type;
}} // namespace mock::detail

#endif // MOCK_REF_ARG_HPP_INCLUDED
