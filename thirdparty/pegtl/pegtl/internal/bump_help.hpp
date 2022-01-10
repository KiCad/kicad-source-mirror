// Copyright (c) 2015-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_BUMP_HELP_HPP
#define TAO_PEGTL_INTERNAL_BUMP_HELP_HPP

#include <cstddef>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   template< typename Rule, typename ParseInput >
   void bump_help( ParseInput& in, const std::size_t count )
   {
      if constexpr( Rule::template can_match_eol< ParseInput::eol_t::ch > ) {
         in.bump( count );
      }
      else {
         in.bump_in_this_line( count );
      }
   }

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
