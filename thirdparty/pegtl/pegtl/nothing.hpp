// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_NOTHING_HPP
#define TAO_PEGTL_NOTHING_HPP

#include "config.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   template< typename Rule >
   struct nothing
   {};

   using maybe_nothing = nothing< void >;

}  // namespace TAO_PEGTL_NAMESPACE

#endif
