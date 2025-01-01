/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file  PerlinNoise.h
 * @brief This source code comes from the project: https://github.com/sol-prog/Perlin_Noise
 *
 * It was changed to work with floats instead of doubles
 *
 * Original copyright notice:
 *
 * Perlin_Noise
 * Here you could find the code for "Perlin noise in C++11", for more information visit the
 * project webpage at http://solarianprogrammer.com/2012/07/18/perlin-noise-cpp-11/
 * You could use this program under the terms of GPL v3, for more details see:
 * http://www.gnu.org/copyleft/gpl.html
 * Copyright 2012 Sol from www.solarianprogrammer.com
 */

#include "PerlinNoise.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

// THIS IS A DIRECT TRANSLATION TO C++11 FROM THE REFERENCE
// JAVA IMPLEMENTATION OF THE IMPROVED PERLIN FUNCTION (see http://mrl.nyu.edu/~perlin/noise/)
// THE ORIGINAL JAVA IMPLEMENTATION IS COPYRIGHT 2002 KEN PERLIN

// I ADDED AN EXTRA METHOD THAT GENERATES A NEW PERMUTATION VECTOR
// (THIS IS NOT PRESENT IN THE ORIGINAL IMPLEMENTATION)

// Initialize with the reference values for the permutation vector
PerlinNoise::PerlinNoise()
{

    // Initialize the permutation vector with the reference values
    p = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54, 65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,
        43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
        97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,
        107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

    // Duplicate the permutation vector
    auto oldsize = p.size();
    p.resize( 2 * p.size() );
    std::copy_n( p.begin(), oldsize, p.begin() + oldsize );
}


PerlinNoise::PerlinNoise( unsigned int seed )
{
    p.resize( 256 );

    // Fill p with values from 0 to 255
    std::iota( p.begin(), p.end(), 0 );

    // Initialize a random engine with seed
    std::default_random_engine engine( seed );

    // Shuffle using the above random engine
    std::shuffle( p.begin(), p.end(), engine );

    // Duplicate the permutation vector
    auto oldsize = p.size();
    p.resize( 2 * p.size() );
    std::copy_n( p.begin(), oldsize, p.begin() + oldsize );
}


float PerlinNoise::noise( float x, float y, float z ) const
{
    // Find the unit cube that contains the point
    int X = static_cast<int>( std::floor( x ) ) & 255;
    int Y = static_cast<int>( std::floor( y ) ) & 255;
    int Z = static_cast<int>( std::floor( z ) ) & 255;

    // Find relative x, y,z of point in cube
    x -= std::floor( x );
    y -= std::floor( y );
    z -= std::floor( z );

    // Compute fade curves for each of x, y, z
    const float u = fade( x );
    const float v = fade( y );
    const float w = fade( z );

    // Hash coordinates of the 8 cube corners
    const int A   = p[X] + Y;
    const int AA  = p[A] + Z;
    const int AB  = p[A + 1] + Z;
    const int B   = p[X + 1] + Y;
    const int BA  = p[B] + Z;
    const int BB  = p[B + 1] + Z;

    // Add blended results from 8 corners of cube
    const float res = lerp(
            w,
            lerp( v, lerp( u, grad( p[AA], x, y, z ), grad( p[BA], x - 1, y, z ) ),
                  lerp( u, grad( p[AB], x, y - 1, z ), grad( p[BB], x - 1, y - 1, z ) ) ),
            lerp( v, lerp( u, grad( p[AA + 1], x, y, z - 1 ), grad( p[BA + 1], x - 1, y, z - 1 ) ),
                  lerp( u, grad( p[AB + 1], x, y - 1, z - 1 ),
                        grad( p[BB + 1], x - 1, y - 1, z - 1 ) ) ) );

    return (res + 1.0f) / 2.0f;
}


float PerlinNoise::noise( float x, float y ) const
{
    // Find the unit cube that contains the point
    int X = static_cast<int>( std::floor( x ) ) & 255;
    int Y = static_cast<int>( std::floor( y ) ) & 255;

    // Find relative x, y,z of point in cube
    x -= std::floor( x );
    y -= std::floor( y );

    // Compute fade curves for each of x, y
    const float u = fade( x );
    const float v = fade( y );

    // Hash coordinates of the 8 cube corners
    const int A   = p[X] + Y;
    const int AA  = p[A] + 0;
    const int AB  = p[A + 1] + 0;
    const int B   = p[X + 1] + Y;
    const int BA  = p[B] + 0;
    const int BB  = p[B + 1] + 0;

    // Add blended results from 8 corners of cube
    const float res = lerp( v, lerp( u, grad( p[AA], x, y ), grad( p[BA], x - 1, y ) ),
                            lerp( u, grad( p[AB], x, y - 1 ), grad( p[BB], x - 1, y - 1 ) ) );

    return ( res + 1.0f ) / 2.0f;
}


float PerlinNoise::fade( float t ) const
{
    return t * t * t * ( t * ( t * 6.0f - 15.0f ) + 10.0f );
}


float PerlinNoise::lerp( float t, float a, float b ) const
{
    return a + t * ( b - a );
}


float PerlinNoise::grad( int hash, float x, float y, float z ) const
{
    const int h = hash & 15;

    // Convert lower 4 bits of hash inot 12 gradient directions
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;

    return ( ( h & 1 ) == 0 ? u : -u ) + ( ( h & 2 ) == 0 ? v : -v );
}


float PerlinNoise::grad( int hash, float x, float y ) const
{
    const int h = hash & 15;

    // Convert lower 4 bits of hash inot 12 gradient directions
    const float u = h < 8 ? x : y;
    const float v = h < 4 ? y : h == 12 || h == 14 ? x : 0.0f;

    return ( ( h & 1 ) == 0 ? u : -u ) + ( ( h & 2 ) == 0 ? v : -v );
}
