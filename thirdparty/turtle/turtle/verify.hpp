// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_VERIFY_HPP_INCLUDED
#define MOCK_VERIFY_HPP_INCLUDED

#include "config.hpp"
#include "detail/functor.hpp"
#include "detail/root.hpp"
#include "object.hpp"

namespace mock {
inline bool verify()
{
    return detail::root.verify();
}
inline bool verify(const object& o)
{
    return o.impl_->verify();
}
template<typename Signature>
bool verify(const detail::functor<Signature>& f)
{
    return f.verify();
}
} // namespace mock

#endif // MOCK_VERIFY_HPP_INCLUDED
