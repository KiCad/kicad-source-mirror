// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_SIGNATURE_TRAITS_HPP_INCLUDED
#define MOCK_SIGNATURE_TRAITS_HPP_INCLUDED

#include "../config.hpp"
#include <cstddef>

namespace mock { namespace detail {
    /// Helper class to store a tuple/list of types
    template<class...>
    struct tuple;

    /// Get the type at the given index in the tuple
    template<std::size_t index, class Tuple>
    struct tuple_element;

    template<std::size_t I, class H, class... T>
    struct tuple_element<I, tuple<H, T...>> : tuple_element<I - 1, tuple<T...>>
    {};

    template<class H, class... T>
    struct tuple_element<0, tuple<H, T...>>
    {
        using type = H;
    };

    /// Provides information about a given function signature
    /// Member types: return_type, args
    /// Member constant: arity
    template<typename Signature>
    struct signature_traits;

    template<typename R, typename... Args>
    struct signature_traits<R(Args...)>
    {
        using return_type = R;
        static constexpr std::size_t arity = sizeof...(Args);
        using args = tuple<Args...>;
    };

    /// Return the result type of the function signature
    template<typename Signature>
    using result_type_t = typename signature_traits<Signature>::return_type;

    /// Return the arity of the function signature
    template<typename Signature>
    using function_arity_t = std::integral_constant<std::size_t, signature_traits<Signature>::arity>;

    /// Return the type at the given index of the function signature
    template<typename Signature, std::size_t idx>
    using parameter_t = typename tuple_element<idx, typename signature_traits<Signature>::args>::type;

}} // namespace mock::detail

#endif // MOCK_SIGNATURE_TRAITS_HPP_INCLUDED
