// http://turtle.sourceforge.net
//
// Copyright Alexander Grund 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_UNWRAP_REFERENCE_HPP_INCLUDED
#define MOCK_UNWRAP_REFERENCE_HPP_INCLUDED

#include <functional>
#include <type_traits>

namespace mock {
template<class T>
struct unwrap_reference
{
    using type = T;
};
template<class T>
struct unwrap_reference<std::reference_wrapper<T>>
{
    using type = T;
};
template<class T>
struct unwrap_reference<const std::reference_wrapper<T>>
{
    using type = T;
};
template<class T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

template<class T>
BOOST_FORCEINLINE unwrap_reference_t<T>& unwrap_ref(T& t) noexcept
{
    return t;
}
} // namespace mock

#endif // MOCK_UNWRAP_REFERENCE_HPP_INCLUDED
