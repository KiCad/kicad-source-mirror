// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2013
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_ADDRESSOF_HPP_INCLUDED
#define MOCK_ADDRESSOF_HPP_INCLUDED

#include "../config.hpp"
#include <boost/utility/addressof.hpp>

namespace mock
{
namespace detail
{
    using boost::addressof;

#ifdef MOCK_NULLPTR

    inline const std::nullptr_t* addressof( const std::nullptr_t& p )
    {
        return &p;
    }
    inline std::nullptr_t* addressof( std::nullptr_t& p )
    {
        return &p;
    }

#endif
}
} // mock

#endif // MOCK_ADDRESSOF_HPP_INCLUDED
