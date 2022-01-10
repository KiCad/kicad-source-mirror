// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_FAILURE_HPP
#define TAO_PEGTL_INTERNAL_FAILURE_HPP

#include "../config.hpp"

#include "enable_control.hpp"

#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct failure
   {
      using rule_t = failure;
      using subs_t = empty_list;

      template< typename ParseInput >
      [[nodiscard]] static bool match( ParseInput& /*unused*/ ) noexcept
      {
         return false;
      }
   };

   template<>
   inline constexpr bool enable_control< failure > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
