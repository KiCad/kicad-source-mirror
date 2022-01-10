// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_PARSE_HPP
#define TAO_PEGTL_PARSE_HPP

#include <type_traits>

#include "apply_mode.hpp"
#include "config.hpp"
#include "normal.hpp"
#include "nothing.hpp"
#include "parse_error.hpp"
#include "position.hpp"
#include "rewind_mode.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   namespace internal
   {
      [[nodiscard]] inline auto get_position( const position& p ) noexcept( std::is_nothrow_copy_constructible_v< position > )
      {
         return p;
      }

      template< typename ParseInput >
      [[nodiscard]] position get_position( const ParseInput& in ) noexcept( noexcept( position( in.position() ) ) )
      {
         return in.position();
      }

   }  // namespace internal

   template< typename Rule,
             template< typename... > class Action = nothing,
             template< typename... > class Control = normal,
             apply_mode A = apply_mode::action,
             rewind_mode M = rewind_mode::required,
             typename ParseInput,
             typename... States >
   auto parse( ParseInput&& in, States&&... st )
   {
      return Control< Rule >::template match< A, M, Action, Control >( in, st... );
   }

   template< typename Rule,
             template< typename... > class Action = nothing,
             template< typename... > class Control = normal,
             apply_mode A = apply_mode::action,
             rewind_mode M = rewind_mode::required,
             typename Outer,
             typename ParseInput,
             typename... States >
   auto parse_nested( const Outer& o, ParseInput&& in, States&&... st )
   {
#if defined( __cpp_exceptions )
      try {
         return parse< Rule, Action, Control, A, M >( in, st... );
      }
      catch( parse_error& e ) {
         e.add_position( internal::get_position( o ) );
         throw;
      }
#else
      (void)o;
      return parse< Rule, Action, Control, A, M >( in, st... );
#endif
   }

}  // namespace TAO_PEGTL_NAMESPACE

#endif
