/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JSON_COMMON_H
#define JSON_COMMON_H

/***********************************************************************************************************************
 * If we are compiling on Apple with Clang >= 17, the version of LLVM no longer includes a generic template for
 * char_traits for char types which are not specified in the C++ standard. We define our own here for types required by
 * the JSON library.
 *
 * From: https://github.com/llvm/llvm-project/commit/c3668779c13596e223c26fbd49670d18cd638c40
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 **********************************************************************************************************************/

#ifdef __APPLE__
#if __clang_major__ >= 17

#include <wx/unichar.h>

template <>
struct std::char_traits<wxUniChar>
{
    using char_type = wxUniChar;
    using int_type = int;
    using off_type = std::streamoff;
    using pos_type = std::streampos;
    using state_type = mbstate_t;

    static inline void assign( char_type& __c1, const char_type& __c2 ) noexcept { __c1 = __c2; }

    static inline bool eq( char_type __c1, char_type __c2 ) noexcept { return __c1 == __c2; }

    static inline bool lt( char_type __c1, char_type __c2 ) noexcept { return __c1 < __c2; }

    static constexpr int compare( const char_type* __s1, const char_type* __s2, size_t __n )
    {
        for( ; __n; --__n, ++__s1, ++__s2 )
        {
            if( lt( *__s1, *__s2 ) )
                return -1;

            if( lt( *__s2, *__s1 ) )
                return 1;
        }

        return 0;
    }

    static size_t length( const char_type* __s )
    {
        size_t __len = 0;

        for( ; !eq( *__s, char_type( 0 ) ); ++__s )
            ++__len;

        return __len;
    }

    static constexpr const char_type* find( const char_type* __s, size_t __n, const char_type& __a )
    {
        for( ; __n; --__n )
        {
            if( eq( *__s, __a ) )
                return __s;
            ++__s;
        }

        return nullptr;
    }

    static constexpr char_type* move( char_type* __s1, const char_type* __s2, size_t __n )
    {
        if( __n == 0 )
            return __s1;

        char_type* __r = __s1;

        if( __s1 < __s2 )
        {
            for( ; __n; --__n, ++__s1, ++__s2 )
                assign( *__s1, *__s2 );
        }
        else if( __s2 < __s1 )
        {
            __s1 += __n;
            __s2 += __n;

            for( ; __n; --__n )
                assign( *--__s1, *--__s2 );
        }

        return __r;
    }

    static constexpr char_type* copy( char_type* __s1, const char_type* __s2, size_t __n )
    {
        char_type* __r = __s1;

        for( ; __n; --__n, ++__s1, ++__s2 )
            assign( *__s1, *__s2 );

        return __r;
    }

    static char_type* assign( char_type* __s, size_t __n, char_type __a )
    {
        char_type* __r = __s;

        for( ; __n; --__n, ++__s )
            assign( *__s, __a );

        return __r;
    }

    static inline constexpr int_type not_eof( int_type __c ) noexcept
    {
        return eq_int_type( __c, eof() ) ? ~eof() : __c;
    }

    static inline char_type to_char_type( int_type __c ) noexcept { return char_type( __c ); }

    static inline int_type to_int_type( char_type __c ) noexcept { return static_cast<int_type>( __c ); }

    static inline constexpr bool eq_int_type( int_type __c1, int_type __c2 ) noexcept { return __c1 == __c2; }

    static inline constexpr int_type eof() noexcept { return static_cast<int_type>( EOF ); }
};

#endif
#endif

#include <nlohmann/json.hpp>
#include <kicommon.h>

/**
 * This is simply a "stub" meant to inform MSVC when compiling shared libraries that it can find
 * template instances in kicommon of nlohmann::json's various templates
 */
class KICOMMON_API JSON_COMMON_EXPORT_STUB final : public nlohmann::json
{
};

#endif
