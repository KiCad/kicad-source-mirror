/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  mortoncodes.cpp
 * @brief Implements Morton Codes base on the implementation of Fabian “ryg” Giesen
 * https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
 */

#include "mortoncodes.h"


// "Insert" a 0 bit after each of the 16 low bits of x
uint32_t Part1By1( uint32_t x )
{
    x &= 0x0000ffff;                     // x = ---- ---- ---- ---- fedc ba98 7654 3210
    x = ( x ^ ( x << 8 ) ) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = ( x ^ ( x << 4 ) ) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = ( x ^ ( x << 2 ) ) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = ( x ^ ( x << 1 ) ) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0

    return x;
}


// "Insert" two 0 bits after each of the 10 low bits of x
uint32_t Part1By2( uint32_t x )
{
    x &= 0x000003ff;                      // x = ---- ---- ---- ---- ---- --98 7654 3210
    x = ( x ^ ( x << 16 ) ) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = ( x ^ ( x << 8 ) ) & 0x0300f00f;  // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = ( x ^ ( x << 4 ) ) & 0x030c30c3;  // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = ( x ^ ( x << 2 ) ) & 0x09249249;  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0

    return x;
}


// Inverse of Part1By1 - "delete" all odd-indexed bits
uint32_t Compact1By1( uint32_t x )
{
    x &= 0x55555555;                     // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    x = ( x ^ ( x >> 1 ) ) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = ( x ^ ( x >> 2 ) ) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = ( x ^ ( x >> 4 ) ) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = ( x ^ ( x >> 8 ) ) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210

    return x;
}


// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
uint32_t Compact1By2( uint32_t x )
{
    x &= 0x09249249;                      // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    x = ( x ^ ( x >> 2 ) ) & 0x030c30c3;  // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = ( x ^ ( x >> 4 ) ) & 0x0300f00f;  // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = ( x ^ ( x >> 8 ) ) & 0xff0000ff;  // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = ( x ^ ( x >> 16 ) ) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210

    return x;
}


uint32_t EncodeMorton2( uint32_t x, uint32_t y )
{
    return ( Part1By1( y ) << 1 ) + Part1By1( x );
}


uint32_t EncodeMorton3( uint32_t x, uint32_t y, uint32_t z )
{
    return ( Part1By2( z ) << 2 ) + ( Part1By2( y ) << 1 ) + Part1By2( x );
}


uint32_t DecodeMorton2X( uint32_t code )
{
    return Compact1By1( code >> 0 );
}


uint32_t DecodeMorton2Y( uint32_t code )
{
    return Compact1By1( code >> 1 );
}


uint32_t DecodeMorton3X( uint32_t code )
{
    return Compact1By2( code >> 0 );
}


uint32_t DecodeMorton3Y( uint32_t code )
{
    return Compact1By2( code >> 1 );
}


uint32_t DecodeMorton3Z( uint32_t code )
{
    return Compact1By2( code >> 2 );
}
