// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2012
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_PARAMETER_HPP_INCLUDED
#define MOCK_PARAMETER_HPP_INCLUDED

#include "../config.hpp"
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/mpl/at.hpp>

namespace mock
{
namespace detail
{
    template< typename Signature, int n >
    struct parameter
    {
        typedef typename
            boost::mpl::at_c<
                typename
                    boost::function_types::parameter_types< Signature >,
                n
            >::type type;
    };
}
} // mock

#endif // MOCK_PARAMETER_HPP_INCLUDED
