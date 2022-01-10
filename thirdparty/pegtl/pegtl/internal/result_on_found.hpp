// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP
#define TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   enum class result_on_found : bool
   {
      success = true,
      failure = false
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
