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
 * @brief This source code comes from the project:
 * https://github.com/sol-prog/Perlin_Noise
 *
 * It was changed to work with floats instead of doubles
 *
 * Original copyright notice:
 *
 * Perlin_Noise
 * "Here you could find the code for "Perlin noise in C++11",
 *  for more information visit the project webpage:
 * http://solarianprogrammer.com/2012/07/18/perlin-noise-cpp-11/
 * You could use this program under the terms of GPL v3, for more details see:
 * http://www.gnu.org/copyleft/gpl.html
 * Copyright 2012 Sol from www.solarianprogrammer.com"
 */

#include <vector>

// THIS CLASS IS A TRANSLATION TO C++11 FROM THE REFERENCE
// JAVA IMPLEMENTATION OF THE IMPROVED PERLIN FUNCTION (see http://mrl.nyu.edu/~perlin/noise/)
// THE ORIGINAL JAVA IMPLEMENTATION IS COPYRIGHT 2002 KEN PERLIN

// I ADDED AN EXTRA METHOD THAT GENERATES A NEW PERMUTATION VECTOR
// (THIS IS NOT PRESENT IN THE ORIGINAL IMPLEMENTATION)

#ifndef PERLINNOISE_H
#define PERLINNOISE_H

class PerlinNoise
{
public:
    /**
     * Initialize with the reference values for the permutation vector.
     */
    PerlinNoise();

    /**
     * Generate a new permutation vector based on the value of seed.
     */
    PerlinNoise( unsigned int seed );

    // Returns between 0.0f and 1.0f
    float noise( float x, float y, float z ) const;
    float noise( float x, float y ) const;

private:
    float fade( float t ) const;
    float lerp( float t, float a, float b ) const;
    float grad( int hash, float x, float y, float z ) const;
    float grad( int hash, float x, float y ) const;

    // The permutation vector
    std::vector<int> p;
};

#endif
