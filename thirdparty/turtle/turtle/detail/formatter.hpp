// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_FORMATTER_HPP_INCLUDED
#define MOCK_FORMATTER_HPP_INCLUDED

#include "../config.hpp"
#include "../stream.hpp"
#include <memory>

namespace mock { namespace detail {
    template<typename T>
    struct formatter
    {
        explicit formatter(const T& t) : t_(std::addressof(t)) {}
        void serialize(stream& s) const { detail::serialize(s, *t_); }
        const T* t_;
    };

    template<typename T>
    stream& operator<<(stream& s, const formatter<T>& f)
    {
        f.serialize(s);
        return s;
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& s, const formatter<T>& f)
    {
        stream ss(s);
        f.serialize(ss);
        return s;
    }
}} // namespace mock::detail

#endif // MOCK_FORMATTER_HPP_INCLUDED
