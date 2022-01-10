// Copyright (c) 2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_HAS_ERROR_MESSAGE_HPP
#define TAO_PEGTL_INTERNAL_HAS_ERROR_MESSAGE_HPP

#include "../config.hpp"

#include <type_traits>

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Rule, typename = const char* >
   inline constexpr bool has_error_message = false;

   template< typename Rule >
   inline constexpr bool has_error_message< Rule, std::decay_t< decltype( Rule::error_message ) > > = true;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
