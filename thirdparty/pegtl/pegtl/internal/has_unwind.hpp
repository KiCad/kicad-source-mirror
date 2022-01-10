// Copyright (c) 2020-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_HAS_UNWIND_HPP
#define TAO_PEGTL_INTERNAL_HAS_UNWIND_HPP

#include <utility>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename, typename... >
   inline constexpr bool has_unwind = false;

   template< typename C, typename... S >
   inline constexpr bool has_unwind< C, decltype( C::unwind( std::declval< S >()... ) ), S... > = true;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
