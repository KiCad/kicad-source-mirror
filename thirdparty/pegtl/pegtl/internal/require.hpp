// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_REQUIRE_HPP
#define TAO_PEGTL_INTERNAL_REQUIRE_HPP

#include "../config.hpp"

#include "enable_control.hpp"
#include "success.hpp"

#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< unsigned Amount >
   struct require;

   template<>
   struct require< 0 >
      : success
   {};

   template< unsigned Amount >
   struct require
   {
      using rule_t = require;
      using subs_t = empty_list;

      template< typename ParseInput >
      [[nodiscard]] static bool match( ParseInput& in ) noexcept( noexcept( in.size( 0 ) ) )
      {
         return in.size( Amount ) >= Amount;
      }
   };

   template< unsigned Amount >
   inline constexpr bool enable_control< require< Amount > > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
