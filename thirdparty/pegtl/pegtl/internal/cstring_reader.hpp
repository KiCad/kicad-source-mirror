// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_CSTRING_READER_HPP
#define TAO_PEGTL_INTERNAL_CSTRING_READER_HPP

#include <cassert>
#include <cstddef>

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct cstring_reader
   {
      explicit cstring_reader( const char* zero_terminated ) noexcept
         : m_cstring( zero_terminated )
      {
         assert( m_cstring != nullptr );
      }

      [[nodiscard]] std::size_t operator()( char* buffer, const std::size_t length ) noexcept
      {
         std::size_t i = 0;
         char c;

         while( ( i < length ) && ( ( c = m_cstring[ i ] ) != 0 ) ) {
            *buffer++ = c;
            ++i;
         }
         m_cstring += i;
         return i;
      }

      const char* m_cstring;
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
