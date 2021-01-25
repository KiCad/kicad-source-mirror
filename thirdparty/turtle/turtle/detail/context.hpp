// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CONTEXT_HPP_INCLUDED
#define MOCK_CONTEXT_HPP_INCLUDED

#include "../config.hpp"
#include "type_name.hpp"
#include <boost/optional.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <ostream>

namespace mock { namespace detail {
    class verifiable;

    class context
    {
    public:
        context() = default;
        context(const context&) = delete;
        context& operator=(const context&) = delete;

        virtual ~context() = default;

        virtual void add(const void* p,
                         verifiable& v,
                         boost::unit_test::const_string instance,
                         boost::optional<type_name> type,
                         boost::unit_test::const_string name) = 0;
        virtual void add(verifiable& v) = 0;
        virtual void remove(verifiable& v) = 0;

        virtual void serialize(std::ostream& s, const verifiable& v) const = 0;
    };
}} // namespace mock::detail

#endif // MOCK_CONTEXT_HPP_INCLUDED
