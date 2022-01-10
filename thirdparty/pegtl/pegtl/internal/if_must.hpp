// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_IF_MUST_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_HPP

#if !defined( __cpp_exceptions )
#error "Exception support required for tao/pegtl/internal/if_must.hpp"
#else

#include "../config.hpp"

#include "enable_control.hpp"
#include "must.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"
#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< bool Default, typename Cond, typename... Rules >
   struct if_must
   {
      using rule_t = if_must;
      using subs_t = type_list< Cond, must< Rules... > >;

      template< apply_mode A,
                rewind_mode M,
                template< typename... >
                class Action,
                template< typename... >
                class Control,
                typename ParseInput,
                typename... States >
      [[nodiscard]] static bool match( ParseInput& in, States&&... st )
      {
         if( Control< Cond >::template match< A, M, Action, Control >( in, st... ) ) {
            (void)Control< must< Rules... > >::template match< A, M, Action, Control >( in, st... );
            return true;
         }
         return Default;
      }
   };

   template< bool Default, typename Cond, typename... Rules >
   inline constexpr bool enable_control< if_must< Default, Cond, Rules... > > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
#endif
