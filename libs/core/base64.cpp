/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Seth Hillbrand <seth@kipro-pcb.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <core/base64.h>

#include <cstdint>
#include <cstddef>
#include <limits>
#include <vector>

namespace {

static const uint8_t ENCODE_BASE64[64] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
    0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x2F,
};

static const uint8_t E = -1;

static const uint8_t DECODE_BASE64[128] = {
/*  0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
     E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,
     E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E,
     E,  E,  E,  E,  E,  E,  E,  E,  E,  E,  E, 62,  E,  E,  E, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  E,  E,  E,  E,  E,  E,
     E,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  E,  E,  E,  E,  E,
     E, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  E,  E,  E,  E,  E,
};

} // namespace

namespace base64
{
    size_t encode_length( size_t aInputLen )
    {
        return 4 * ( ( aInputLen + 2 ) / 3 ) + ( aInputLen + 2 ) % 3 - 2;
    }


    size_t decode_length( size_t aInputLen )
    {
        if( aInputLen % 4 == 1 )
            return 0;
        else
            return 3 * ( ( aInputLen + 2 ) / 4 ) + ( aInputLen + 2 ) % 4 - 2;
    }


    void encode( const std::vector<uint8_t>& aInput, std::vector<uint8_t>& aOutput )
    {
        size_t end = ( aInput.size() / 3 ) * 3;
        aOutput.reserve( encode_length( aInput.size() ) );

        for( size_t i = 0; i < end; i += 3 )
        {
            unsigned value = ( aInput[i] << 16 ) | ( aInput[i + 1] << 8 ) | aInput[i + 2] ;

            aOutput.emplace_back( ENCODE_BASE64[( value >> 18 ) & 0x3F] );
            aOutput.emplace_back( ENCODE_BASE64[( value >> 12 ) & 0x3F] );
            aOutput.emplace_back( ENCODE_BASE64[( value >> 6 ) & 0x3F] );
            aOutput.emplace_back( ENCODE_BASE64[value & 0x3F] );
        }

        size_t remainder = aInput.size() - end;

        if( remainder )
        {
            unsigned value = aInput[end];

            if( remainder == 2 )
            {
                value = ( value << 10 ) | ( aInput[end + 1] << 2 );
                aOutput.emplace_back( ENCODE_BASE64[( value >> 12 ) & 0x3F] );
            }
            else
            {
                value <<= 4;
            }

            aOutput.emplace_back( ENCODE_BASE64[( value >> 6 ) & 0x3F] );
            aOutput.emplace_back( ENCODE_BASE64[value & 0x3F] );
        }
    }


    void decode(const std::vector<uint8_t>& aInput, std::vector<uint8_t>& aOutput )
    {
        size_t end = ( aInput.size() / 4 ) * 4;
        size_t decode_size = decode_length( aInput.size() );

        if( !decode_size )
            return;

        aOutput.reserve( decode_size );

        for( size_t i = 0; i < end; i++ )
        {
            unsigned value = ( DECODE_BASE64[aInput[i] & 0x7F] << 18 ) |
                             ( DECODE_BASE64[aInput[i + 1] & 0x7F] << 12 ) |
                             ( DECODE_BASE64[aInput[i + 2] & 0x7F] << 6 ) |
                               DECODE_BASE64[aInput[i + 3] & 0x7F];

            aOutput.emplace_back( ( value >> 16 ) & 0xff );
            aOutput.emplace_back( ( value >> 8 ) & 0xff );
            aOutput.emplace_back( value & 0xff );
        }

        size_t remainder = aInput.size() - end;

        if( remainder )
        {
            unsigned value = ( ( DECODE_BASE64[aInput[end] & 0x7F] ) << 6 ) |
                             ( DECODE_BASE64[aInput[end + 1] & 0x7F] );

            if( remainder == 3 )
            {
                value = ( value << 6 ) | ( DECODE_BASE64[aInput[end + 2] & 0x7F] );
                value >>= 2;

                aOutput.emplace_back( ( value >> 8 ) & 0xff );
            }
            else
            {
                value >>= 4;
            }
            aOutput[0] = value & 0xff ;
        }
    }

} // base64
