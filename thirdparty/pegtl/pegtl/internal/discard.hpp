// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_DISCARD_HPP
#define TAO_PEGTL_INTERNAL_DISCARD_HPP

#include "../config.hpp"

#include "enable_control.hpp"

#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct discard
   {
      using rule_t = discard;
      using subs_t = empty_list;

      template< typename ParseInput >
      [[nodiscard]] static bool match( ParseInput& in ) noexcept
      {
         static_assert( noexcept( in.discard() ) );
         in.discard();
         return true;
      }
   };

   template<>
   inline constexpr bool enable_control< discard > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
