// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_CHILD_HPP_INCLUDED
#define MOCK_CHILD_HPP_INCLUDED

#include "../config.hpp"
#include "parent.hpp"
#include "type_name.hpp"
#include <boost/optional.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <ostream>

namespace mock { namespace detail {
    class child
    {
    public:
        child() : parent_(0) {}
        void update(parent& p,
                    boost::unit_test::const_string instance,
                    boost::optional<type_name> type,
                    boost::unit_test::const_string name)
        {
            if(instance != "?." || name_.empty())
                p = parent(instance, type);
            parent_ = &p;
            name_ = name;
        }
        friend std::ostream& operator<<(std::ostream& s, const child& c)
        {
            if(c.parent_)
                s << *c.parent_;
            return s << c.name_;
        }

    private:
        const parent* parent_;
        boost::unit_test::const_string name_;
    };
}} // namespace mock::detail

#endif // MOCK_CHILD_HPP_INCLUDED
