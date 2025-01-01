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
 * @file  3d_fastmath.cpp
 * @brief
 */


#include "3d_fastmath.h"


// Fast Float Random Numbers
// a small and fast implementation for random float numbers in the range [-1,1]
// References : Posted by dominik.ries[AT]gmail[DOT]com
// http://www.musicdsp.org/showone.php?id=273


// set the initial seed to whatever you like
static int s_randSeed = 1;

// fast rand float, using full 32bit precision
// returns in the range [-1, 1] (not confirmed)
float Fast_RandFloat()
{
    s_randSeed *= 16807;

    return (float)s_randSeed * 4.6566129e-010f;
}


// Fast rand, as described here:
// http://wiki.osdev.org/Random_Number_Generator

static unsigned long int s_nextRandSeed = 1;

int Fast_rand( void ) // RAND_MAX assumed to be 32767
{
    s_nextRandSeed = s_nextRandSeed * 1103515245 + 12345;

    return (unsigned int)(s_nextRandSeed >>  16) & 0x7FFF;
}

void Fast_srand( unsigned int seed )
{
    s_nextRandSeed = seed;
}
