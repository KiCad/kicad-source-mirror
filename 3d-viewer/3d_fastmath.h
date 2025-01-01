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
 * @file  3d_fastmath.h
 * @brief Defines math related functions
 */

#ifndef _3D_FASTMATH_H
#define _3D_FASTMATH_H

#include <cmath>
#include <cstdint>
#include <cstring>


// Define this flag to use fast math optimizations
#define FASTMATH_USE

#define L1_CACHE_LINE_SIZE 64

#ifdef FASTMATH_USE
#define INTFLOORF(s) (lrintf( (s) - (0.5f - FLT_EPSILON) ))
#else
#define INTFLOORF(s) ((int)( floor(s) ))
#endif


// Fast Float Random Numbers
// a small and fast implementation for random float numbers in the range [-1,1]
// This is not thread safe
float Fast_RandFloat();

int Fast_rand( void );
void Fast_srand( unsigned int seed );

/**
 * This part contains some functions from the PBRT 3 source code.
 * https://github.com/mmp/pbrt-v3/blob/master/src/core/pbrt.h
 */

/*
    pbrt source code is Copyright(c) 1998-2015
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// Global Inline Functions
inline uint32_t FloatToBits( float f )
{
    uint32_t ui;

    memcpy( &ui, &f, sizeof( float ) );

    return ui;
}


inline float BitsToFloat( uint32_t ui )
{
    float f;

    memcpy( &f, &ui, sizeof (uint32_t ) );

    return f;
}


inline uint64_t FloatToBits( double f )
{
    uint64_t ui;

    memcpy( &ui, &f, sizeof( double ) );

    return ui;
}


inline double BitsToFloat( uint64_t ui )
{
    double f;

    memcpy( &f, &ui, sizeof( uint64_t ) );

    return f;
}


inline float NextFloatUp( float v )
{
    // Handle infinity and negative zero for _NextFloatUp()_
    if( std::isinf( v ) && (v > 0.) )
        return v;

    if( v == -0.f )
        v = 0.f;

    // Advance _v_ to next higher float
    uint32_t ui = FloatToBits( v );

    if( v >= 0. )
        ++ui;
    else
        --ui;

    return BitsToFloat( ui );
}


inline float NextFloatDown( float v )
{
    // Handle infinity and positive zero for _NextFloatDown()_
    if( std::isinf( v ) && (v < 0.) )
        return v;

    if( v == 0.f )
        v = -0.f;

    uint32_t ui = FloatToBits( v );

    if( v > 0. )
        --ui;
    else
        ++ui;

    return BitsToFloat( ui );
}

#endif // 3D_FASTMATH_H
