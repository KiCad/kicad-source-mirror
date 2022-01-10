// Copyright (c) 2019-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_DISCARD_INPUT_ON_FAILURE_HPP
#define TAO_PEGTL_DISCARD_INPUT_ON_FAILURE_HPP

#include "apply_mode.hpp"
#include "config.hpp"
#include "match.hpp"
#include "nothing.hpp"
#include "rewind_mode.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   struct discard_input_on_failure
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
         const bool result = TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
         if( !result ) {
            in.discard();
         }
         return result;
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE

#endif
