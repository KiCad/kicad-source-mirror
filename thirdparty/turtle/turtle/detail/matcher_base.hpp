// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_MATCHER_BASE_HPP_INCLUDED
#define MOCK_MATCHER_BASE_HPP_INCLUDED

#include "ref_arg.hpp"
#include <ostream>

namespace mock { namespace detail {
    template<typename... Args>
    class matcher_base
    {
    public:
        matcher_base() = default;
        matcher_base(const matcher_base&) = delete;
        matcher_base& operator=(const matcher_base&) = delete;
        virtual ~matcher_base() = default;

        virtual bool operator()(ref_arg_t<Args>...) = 0;

        friend std::ostream& operator<<(std::ostream& s, const matcher_base& m)
        {
            m.serialize(s);
            return s;
        }

    private:
        virtual void serialize(std::ostream&) const = 0;
    };
}} // namespace mock::detail

#endif // MOCK_MATCHER_BASE_HPP_INCLUDED
