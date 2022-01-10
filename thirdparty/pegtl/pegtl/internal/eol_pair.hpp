// Copyright (c) 2017-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_EOL_PAIR_HPP
#define TAO_PEGTL_INTERNAL_EOL_PAIR_HPP

#include <cstddef>
#include <utility>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   using eol_pair = std::pair< bool, std::size_t >;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
