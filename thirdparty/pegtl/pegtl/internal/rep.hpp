// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_REP_HPP
#define TAO_PEGTL_INTERNAL_REP_HPP

#include "../config.hpp"

#include "enable_control.hpp"
#include "seq.hpp"
#include "success.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"
#include "../type_list.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< unsigned Cnt, typename... Rules >
   struct rep
      : rep< Cnt, seq< Rules... > >
   {};

   template< unsigned Cnt >
   struct rep< Cnt >
      : success
   {};

   template< typename Rule >
   struct rep< 0, Rule >
      : success
   {};

   template< unsigned Cnt, typename Rule >
   struct rep< Cnt, Rule >
   {
      using rule_t = rep;
      using subs_t = type_list< Rule >;

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
         auto m = in.template mark< M >();
         using m_t = decltype( m );

         for( unsigned i = 0; i != Cnt; ++i ) {
            if( !Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) {
               return false;
            }
         }
         return m( true );
      }
   };

   template< unsigned Cnt, typename... Rules >
   inline constexpr bool enable_control< rep< Cnt, Rules... > > = false;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
