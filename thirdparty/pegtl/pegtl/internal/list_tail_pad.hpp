// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_LIST_TAIL_PAD_HPP
#define TAO_PEGTL_INTERNAL_LIST_TAIL_PAD_HPP

#include "../config.hpp"

#include "list.hpp"
#include "opt.hpp"
#include "pad.hpp"
#include "seq.hpp"
#include "star.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Rule, typename Sep, typename Pad >
   using list_tail_pad = seq< Rule, star< pad< Sep, Pad >, Rule >, opt< star< Pad >, Sep > >;

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
