// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_IF_MUST_ELSE_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_ELSE_HPP

#if !defined( __cpp_exceptions )
#error "Exception support required for tao/pegtl/internal/if_must_else.hpp"
#else

#include "../config.hpp"

#include "if_then_else.hpp"
#include "must.hpp"

#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Cond, typename Then, typename Else >
   using if_must_else = if_then_else< Cond, must< Then >, must< Else > >;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
#endif
