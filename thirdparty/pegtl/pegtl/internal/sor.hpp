// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_SOR_HPP
#define TAO_PEGTL_INTERNAL_SOR_HPP

#include <utility>

#include "../config.hpp"

#include "enable_control.hpp"
#include "failure.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"
#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename... Rules >
   struct sor;

   template<>
   struct sor<>
      : failure
   {};

   template< typename... Rules >
   struct sor
   {
      using rule_t = sor;
      using subs_t = type_list< Rules... >;

      template< apply_mode A,
                rewind_mode M,
                template< typename... >
                class Action,
                template< typename... >
                class Control,
                std::size_t... Indices,
                typename ParseInput,
                typename... States >
      [[nodiscard]] static bool match( std::index_sequence< Indices... > /*unused*/, ParseInput& in, States&&... st )
      {
         return ( Control< Rules >::template match< A, ( ( Indices == ( sizeof...( Rules ) - 1 ) ) ? M : rewind_mode::required ), Action, Control >( in, st... ) || ... );
      }

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
         return match< A, M, Action, Control >( std::index_sequence_for< Rules... >(), in, st... );
      }
   };

   template< typename... Rules >
   inline constexpr bool enable_control< sor< Rules... > > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
