// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_ENABLE_CONTROL_HPP
#define TAO_PEGTL_INTERNAL_ENABLE_CONTROL_HPP

#include <type_traits>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   // This class is a simple tagging mechanism.
   // By default, enable_control< Rule > is  'true'.
   // Each internal (!) rule that should be hidden
   // from the control and action class' callbacks
   // simply specializes enable_control<> to return
   // 'true' for the above expression.

   template< typename Rule >
   inline constexpr bool enable_control = true;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
