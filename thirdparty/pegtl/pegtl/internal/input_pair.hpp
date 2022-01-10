// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP
#define TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP

#include <cstdint>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Data >
   struct input_pair
   {
      Data data;
      std::uint8_t size;

      using data_t = Data;

      explicit operator bool() const noexcept
      {
         return size > 0;
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
