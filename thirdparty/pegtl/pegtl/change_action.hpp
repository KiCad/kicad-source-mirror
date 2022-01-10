// Copyright (c) 2019-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_CHANGE_ACTION_HPP
#define TAO_PEGTL_CHANGE_ACTION_HPP

#include <type_traits>

#include "apply_mode.hpp"
#include "config.hpp"
#include "nothing.hpp"
#include "rewind_mode.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   template< template< typename... > class NewAction >
   struct change_action
      : maybe_nothing
   {
      template< typename Rule,
                apply_mode A,
                rewind_mode M,
                template< typename... >
                class Action,
                template< typename... >
                class Control,
                typename ParseInput,
                typename... States >
      [[nodiscard]] static bool match( ParseInput& in, States&&... st )
      {
         static_assert( !std::is_same_v< Action< void >, NewAction< void > >, "old and new action class templates are identical" );
         return Control< Rule >::template match< A, M, NewAction, Control >( in, st... );
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE

#endif
