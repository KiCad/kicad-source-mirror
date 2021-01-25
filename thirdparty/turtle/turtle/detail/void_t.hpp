// http://turtle.sourceforge.net
//
// Copyright Alexander Grund 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_VOID_T_HPP_INCLUDED
#define MOCK_VOID_T_HPP_INCLUDED

namespace mock { namespace detail {
    template<typename...>
    struct make_void
    {
        using type = void;
    };
    /// Standard helper to implement the detection idiom. Returns always void
    template<typename... Ts>
    using void_t = typename make_void<Ts...>::type;
}} // namespace mock::detail

#endif // MOCK_VOID_T_HPP_INCLUDED
