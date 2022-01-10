// Copyright (c) 2018-2021 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_CONTRIB_INTERNAL_READ_UINT_HPP
#define TAO_PEGTL_CONTRIB_INTERNAL_READ_UINT_HPP

#include <cstdint>

#include "../../config.hpp"

#include "endian.hpp"

namespace TAO_PEGTL_NAMESPACE::internal
{
   struct read_uint16_be
   {
      using type = std::uint16_t;

      [[nodiscard]] static std::uint16_t read( const void* d ) noexcept
      {
         return be_to_h< std::uint16_t >( d );
      }
   };

   struct read_uint16_le
   {
      using type = std::uint16_t;

      [[nodiscard]] static std::uint16_t read( const void* d ) noexcept
      {
         return le_to_h< std::uint16_t >( d );
      }
   };

   struct read_uint32_be
   {
      using type = std::uint32_t;

      [[nodiscard]] static std::uint32_t read( const void* d ) noexcept
      {
         return be_to_h< std::uint32_t >( d );
      }
   };

   struct read_uint32_le
   {
      using type = std::uint32_t;

      [[nodiscard]] static std::uint32_t read( const void* d ) noexcept
      {
         return le_to_h< std::uint32_t >( d );
      }
   };

   struct read_uint64_be
   {
      using type = std::uint64_t;

      [[nodiscard]] static std::uint64_t read( const void* d ) noexcept
      {
         return be_to_h< std::uint64_t >( d );
      }
   };

   struct read_uint64_le
   {
      using type = std::uint64_t;

      [[nodiscard]] static std::uint64_t read( const void* d ) noexcept
      {
         return le_to_h< std::uint64_t >( d );
      }
   };

}  // namespace TAO_PEGTL_NAMESPACE::internal

#endif
