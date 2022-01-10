// Copyright (c) 2017-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_IDENTIFIER_HPP
#define TAO_PEGTL_INTERNAL_IDENTIFIER_HPP

#include "../config.hpp"

#include "peek_char.hpp"
#include "ranges.hpp"
#include "seq.hpp"
#include "star.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   using identifier_first = ranges< peek_char, 'a', 'z', 'A', 'Z', '_' >;
   using identifier_other = ranges< peek_char, 'a', 'z', 'A', 'Z', '0', '9', '_' >;
   using identifier = seq< identifier_first, star< identifier_other > >;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
