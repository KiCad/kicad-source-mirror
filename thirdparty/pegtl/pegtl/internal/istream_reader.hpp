// Copyright (c) 2016-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_INTERNAL_ISTREAM_READER_HPP
#define TAO_PEGTL_INTERNAL_ISTREAM_READER_HPP

#include <istream>

#if defined( __cpp_exceptions )
#include <system_error>
#else
#include <cstdio>
#include <exception>
#endif

#include "../config.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct istream_reader
   {
      explicit istream_reader( std::istream& s ) noexcept
         : m_istream( s )
      {}

      [[nodiscard]] std::size_t operator()( char* buffer, const std::size_t length )
      {
         m_istream.read( buffer, std::streamsize( length ) );

         if( const auto r = m_istream.gcount() ) {
            return std::size_t( r );
         }
         if( m_istream.eof() ) {
            return 0;
         }
#if defined( __cpp_exceptions )
         const auto ec = errno;
         throw std::system_error( ec, std::system_category(), "std::istream::read() failed" );
#else
         std::fputs( "std::istream::read() failed\n", stderr );
         std::terminate();
#endif
      }

      std::istream& m_istream;
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
