/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// Based on https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef MMH3_HASH_H_
#define MMH3_HASH_H_

#include <hash_128.h>

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio
#if defined( _MSC_VER )
    #define FORCE_INLINE __forceinline
    #include <stdlib.h>
    #define ROTL64( x, y ) _rotl64( x, y )
    #define BIG_CONSTANT( x ) ( x )
    // Other compilers
#else // defined(_MSC_VER)
    #define FORCE_INLINE inline __attribute__( ( always_inline ) )
    inline uint64_t mmh3_rotl64( uint64_t x, int8_t r )
    {
        return ( x << r ) | ( x >> ( 64 - r ) );
    }
    #define ROTL64( x, y ) mmh3_rotl64( x, y )
    #define BIG_CONSTANT( x ) ( x##LLU )
#endif // !defined(_MSC_VER)


/**
 * A streaming C++ equivalent for MurmurHash3_x64_128.
 */
class MMH3_HASH
{
public:
    MMH3_HASH() { reset( 0 ); };

    MMH3_HASH( uint32_t aSeed ) { reset( aSeed ); }

    FORCE_INLINE void reset( uint32_t aSeed = 0 )
    {
        h1 = aSeed;
        h2 = aSeed;
        len = 0;
    }

    FORCE_INLINE void addData( const uint8_t* data, size_t length )
    {
        size_t remaining = length;

        while( remaining >= 16 )
        {
            memcpy( blocks, data, 16 );
            hashBlock();
            data += 16;
            remaining -= 16;
            len += 16;
        }

        if( remaining > 0 )
        {
            memcpy( blocks, data, remaining );
            size_t padding = 4 - ( remaining + 4 ) % 4;
            memset( reinterpret_cast<uint8_t*>( blocks ) + remaining, 0, padding );
            len += remaining + padding;
        }
    }

    FORCE_INLINE void add( const std::string& input )
    {
        addData( reinterpret_cast<const uint8_t*>( input.data() ), input.length() );
    }

    FORCE_INLINE void add( const std::vector<char>& input )
    {
        addData( reinterpret_cast<const uint8_t*>( input.data() ), input.size() );
    }

    FORCE_INLINE void add( int32_t input )
    {
        blocks[( len % 16 ) / 4] = input;
        len += 4;

        if( len % 16 == 0 )
            hashBlock();
    }

    FORCE_INLINE HASH_128 digest()
    {
        hashTail();

        HASH_128 h128;
        hashFinal( h128 );

        return h128;
    }

private:

    // Block read - if your platform needs to do endian-swapping or can only
    // handle aligned reads, do the conversion here
    FORCE_INLINE uint64_t getblock64( int i )
    {
        return blocks64[i]; //
    }

    FORCE_INLINE void hashBlock()
    {
        static const uint64_t c1 = BIG_CONSTANT( 0x87c37b91114253d5 );
        static const uint64_t c2 = BIG_CONSTANT( 0x4cf5ad432745937f );

        uint64_t k1 = getblock64( 0 );
        uint64_t k2 = getblock64( 1 );

        k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

        h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

        k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

        h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
    }

    FORCE_INLINE void hashTail()
    {
        const uint8_t * tail = (const uint8_t*)(blocks);

        static const uint64_t c1 = BIG_CONSTANT( 0x87c37b91114253d5 );
        static const uint64_t c2 = BIG_CONSTANT( 0x4cf5ad432745937f );

        uint64_t k1 = 0;
        uint64_t k2 = 0;

        switch( len & 15)
        {
        case 15: k2 ^= ((uint64_t)tail[14]) << 48; [[fallthrough]];
        case 14: k2 ^= ((uint64_t)tail[13]) << 40; [[fallthrough]];
        case 13: k2 ^= ((uint64_t)tail[12]) << 32; [[fallthrough]];
        case 12: k2 ^= ((uint64_t)tail[11]) << 24; [[fallthrough]];
        case 11: k2 ^= ((uint64_t)tail[10]) << 16; [[fallthrough]];
        case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;  [[fallthrough]];
        case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
                 k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
                 [[fallthrough]];

        case  8: k1 ^= ((uint64_t)tail[ 7]) << 56; [[fallthrough]];
        case  7: k1 ^= ((uint64_t)tail[ 6]) << 48; [[fallthrough]];
        case  6: k1 ^= ((uint64_t)tail[ 5]) << 40; [[fallthrough]];
        case  5: k1 ^= ((uint64_t)tail[ 4]) << 32; [[fallthrough]];
        case  4: k1 ^= ((uint64_t)tail[ 3]) << 24; [[fallthrough]];
        case  3: k1 ^= ((uint64_t)tail[ 2]) << 16; [[fallthrough]];
        case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;  [[fallthrough]];
        case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
                 k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
        };
    }

    // Finalization mix - force all bits of a hash block to avalanche
    static FORCE_INLINE uint64_t fmix64( uint64_t k )
    {
        k ^= k >> 33;
        k *= BIG_CONSTANT( 0xff51afd7ed558ccd );
        k ^= k >> 33;
        k *= BIG_CONSTANT( 0xc4ceb9fe1a85ec53 );
        k ^= k >> 33;

        return k;
    }

    FORCE_INLINE void hashFinal( HASH_128& out )
    {
        h1 ^= len;
        h2 ^= len;

        h1 += h2;
        h2 += h1;

        h1 = fmix64( h1 );
        h2 = fmix64( h2 );

        h1 += h2;
        h2 += h1;

        out.Value64[0] = h1;
        out.Value64[1] = h2;
    }

private:
    uint64_t h1;
    uint64_t h2;
    union
    {
        uint32_t blocks[4];
        uint64_t blocks64[2];
    };
    uint32_t len;
};


#undef FORCE_INLINE
#undef ROTL64
#undef BIG_CONSTANT


#endif // MMH3_HASH_H_
