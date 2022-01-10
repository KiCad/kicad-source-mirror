// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_EOL_HPP
#define TAO_PEGTL_INTERNAL_EOL_HPP

#include "../config.hpp"

#include "enable_control.hpp"

#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct eol
   {
      using rule_t = eol;
      using subs_t = empty_list;

      template< typename ParseInput >
      [[nodiscard]] static bool match( ParseInput& in ) noexcept( noexcept( ParseInput::eol_t::eol_match( in ) ) )
      {
         return ParseInput::eol_t::eol_match( in ).first;
      }
   };

   template<>
   inline constexpr bool enable_control< eol > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
