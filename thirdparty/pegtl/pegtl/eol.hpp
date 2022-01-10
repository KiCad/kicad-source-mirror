// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_EOL_HPP
#define TAO_PEGTL_EOL_HPP

#include "config.hpp"

#include "internal/eol.hpp"

#include "internal/cr_crlf_eol.hpp"
#include "internal/cr_eol.hpp"
#include "internal/crlf_eol.hpp"
#include "internal/lf_crlf_eol.hpp"
#include "internal/lf_eol.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   inline namespace ascii
   {
      // Struct eol is both a rule and a pseudo-namespace for the
      // member structs cr, etc. (which are not themselves rules).

      struct eol
         : internal::eol
      {
         // clang-format off
         struct cr : internal::cr_eol {};
         struct cr_crlf : internal::cr_crlf_eol {};
         struct crlf : internal::crlf_eol {};
         struct lf : internal::lf_eol {};
         struct lf_crlf : internal::lf_crlf_eol {};
         // clang-format on
      };

   }  // namespace ascii

}  // namespace TAO_PEGTL_NAMESPACE

#endif
