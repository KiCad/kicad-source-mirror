// Copyright (c) 2020-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_PATH_TO_STRING_HPP
#define TAO_PEGTL_INTERNAL_PATH_TO_STRING_HPP

#include <string>

#include "../config.hpp"
#include "filesystem.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   [[nodiscard]] inline std::string path_to_string( const internal::filesystem::path& path )
   {
#if defined( TAO_PEGTL_BOOST_FILESYSTEM )
      return path.string();
#elif defined( __cpp_char8_t )
      const auto s = path.u8string();
      return { reinterpret_cast< const char* >( s.data() ), s.size() };
#else
      return path.u8string();
#endif
   }

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
