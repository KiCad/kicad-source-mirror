// Copyright (c) 2017-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP
#define TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP

#include "../config.hpp"

#include <type_traits>

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Action >
   struct apply_single
   {
      template< typename ActionInput, typename... States >
      [[nodiscard]] static auto match( const ActionInput& in, States&&... st ) noexcept( noexcept( Action::apply( in, st... ) ) )
         -> std::enable_if_t< std::is_same_v< decltype( Action::apply( in, st... ) ), void >, bool >
      {
         Action::apply( in, st... );
         return true;
      }

      template< typename ActionInput, typename... States >
      [[nodiscard]] static auto match( const ActionInput& in, States&&... st ) noexcept( noexcept( Action::apply( in, st... ) ) )
         -> std::enable_if_t< std::is_same_v< decltype( Action::apply( in, st... ) ), bool >, bool >
      {
         return Action::apply( in, st... );
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
