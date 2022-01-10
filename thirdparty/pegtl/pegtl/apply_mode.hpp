// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_APPLY_MODE_HPP
#define TAO_PEGTL_APPLY_MODE_HPP

#include "config.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   enum class apply_mode : bool
   {
      action = true,
      nothing = false
   };

}  // namespace TAO_PEGTL_NAMESPACE

#endif
