// Copyright (c) 2014-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_POSITION_HPP
#define TAO_PEGTL_POSITION_HPP

#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include "config.hpp"

#include "internal/iterator.hpp"

namespace TAO_PEGTL_NAMESPACE
{
   struct position
   {
      position() = delete;

      position( position&& ) noexcept = default;
      position( const position& ) = default;

      position& operator=( position&& p ) noexcept = default;
      position& operator=( const position& ) = default;

      template< typename T >
      position( const internal::iterator& in_iter, T&& in_source )
         : byte( in_iter.byte ),
           line( in_iter.line ),
           column( in_iter.column ),
           source( std::forward< T >( in_source ) )
      {}

      template< typename T >
      position( const std::size_t in_byte, const std::size_t in_line, const std::size_t in_column, T&& in_source )
         : byte( in_byte ),
           line( in_line ),
           column( in_column ),
           source( std::forward< T >( in_source ) )
      {}

      ~position() = default;

      std::size_t byte;
      std::size_t line;
      std::size_t column;
      std::string source;
   };

   [[nodiscard]] inline bool operator==( const position& lhs, const position& rhs ) noexcept
   {
      return ( lhs.byte == rhs.byte ) && ( lhs.source == rhs.source );
   }

   [[nodiscard]] inline bool operator!=( const position& lhs, const position& rhs ) noexcept
   {
      return !( lhs == rhs );
   }

   inline std::ostream& operator<<( std::ostream& os, const position& p )
   {
      return os << p.source << ':' << p.line << ':' << p.column;
   }

   [[nodiscard]] inline std::string to_string( const position& p )
   {
      std::ostringstream oss;
      oss << p;
      return std::move( oss ).str();
   }

}  // namespace TAO_PEGTL_NAMESPACE

#endif
