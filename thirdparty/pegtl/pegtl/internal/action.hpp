// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_ACTION_HPP
#define TAO_PEGTL_INTERNAL_ACTION_HPP

#include "../config.hpp"

#include "enable_control.hpp"
#include "seq.hpp"
#include "success.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"
#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< template< typename... > class Action, typename... Rules >
   struct action
      : action< Action, seq< Rules... > >
   {};

   template< template< typename... > class Action >
   struct action< Action >
      : success
   {};

   template< template< typename... > class Action, typename Rule >
   struct action< Action, Rule >
   {
      using rule_t = action;
      using subs_t = type_list< Rule >;

      template< apply_mode A,
                rewind_mode M,
                template< typename... >
                class,
                template< typename... >
                class Control,
                typename ParseInput,
                typename... States >
      [[nodiscard]] static bool match( ParseInput& in, States&&... st )
      {
         return Control< Rule >::template match< A, M, Action, Control >( in, st... );
      }
   };

   template< template< typename... > class Action, typename... Rules >
   inline constexpr bool enable_control< action< Action, Rules... > > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
