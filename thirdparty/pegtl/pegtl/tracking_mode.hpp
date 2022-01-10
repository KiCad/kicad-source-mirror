// Copyright (c) 2017-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_TRACKING_MODE_HPP
#define TAO_PEGTL_TRACKING_MODE_HPP

#include "config.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   enum class tracking_mode : bool
   {
      eager,
      lazy
   };

}  // namespace TAO_PEGTL_NAMESPACE

#endif
