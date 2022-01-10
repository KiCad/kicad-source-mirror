// Copyright (c) 2020-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_CONTRIB_INSTANTIATE_HPP
#define TAO_PEGTL_CONTRIB_INSTANTIATE_HPP

#include "../config.hpp"

#include "../apply_mode.hpp"
#include "../match.hpp"
#include "../nothing.hpp"
#include "../rewind_mode.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   template< typename T >
   struct instantiate
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
         const T t( static_cast< const ParseInput& >( in ), st... );
         return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE

#endif
