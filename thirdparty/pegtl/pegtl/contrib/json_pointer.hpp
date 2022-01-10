// Copyright (c) 2019-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_CONTRIB_JSON_POINTER_HPP
#define TAO_PEGTL_CONTRIB_JSON_POINTER_HPP

#include "../ascii.hpp"
#include "../config.hpp"
#include "../rules.hpp"
#include "../utf8.hpp"

namespace TAO_PEGTL_NAMESPACE::json_pointer
{
   // JSON pointer grammar according to RFC 6901

   // clang-format off
   struct unescaped : utf8::ranges< 0x0, 0x2E, 0x30, 0x7D, 0x7F, 0x10FFFF > {};
   struct escaped : seq< one< '~' >, one< '0', '1' > > {};

   struct reference_token : star< sor< unescaped, escaped > > {};
   struct json_pointer : star< one< '/' >, reference_token > {};
   // clang-format on

   // relative JSON pointer, see ...

   // clang-format off
   struct non_negative_integer : sor< one< '0' >, plus< digit > > {};
   struct relative_json_pointer : seq< non_negative_integer, sor< one< '#' >, json_pointer > > {};
   // clang-format on

}  // namespace TAO_PEGTL_NAMESPACE::json_pointer

#endif
