// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_PARENT_HPP_INCLUDED
#define MOCK_PARENT_HPP_INCLUDED

#include "../config.hpp"
#include "type_name.hpp"
#include <boost/optional.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <ostream>

namespace mock { namespace detail {
    class parent
    {
    public:
        parent() = default;
        parent(boost::unit_test::const_string instance, boost::optional<type_name> type)
            : instance_(instance), type_(type)
        {}
        friend std::ostream& operator<<(std::ostream& s, const parent& p)
        {
            s << p.instance_;
            if(p.type_)
                s << *p.type_ << "::";
            return s;
        }

    private:
        boost::unit_test::const_string instance_;
        boost::optional<type_name> type_;
    };
}} // namespace mock::detail

#endif // MOCK_PARENT_HPP_INCLUDED
