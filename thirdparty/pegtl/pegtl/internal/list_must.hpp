// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_LIST_MUST_HPP
#define TAO_PEGTL_INTERNAL_LIST_MUST_HPP

#if !defined( __cpp_exceptions )
#error "Exception support required for tao/pegtl/internal/list_must.hpp"
#else

#include "../config.hpp"

#include "must.hpp"
#include "seq.hpp"
#include "star.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Rule, typename Sep >
   using list_must = seq< Rule, star< Sep, must< Rule > > >;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
#endif
