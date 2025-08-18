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
#include <random>


// Fast Float Random Numbers
// Thread safe random generator returning numbers in the range [-1,1].
// Uses a Mersenne Twister engine per thread to improve quality.

static thread_local std::mt19937 s_rng{ std::random_device{}() };

float Fast_RandFloat()
{
    static thread_local std::uniform_real_distribution<float> dist( -1.0f, 1.0f );
    return dist( s_rng );
}


// Fast rand returning values compatible with the classic rand() interface
// RAND_MAX is assumed to be 32767
int Fast_rand( void )
{
    static thread_local std::uniform_int_distribution<int> dist( 0, 0x7FFF );
    return dist( s_rng );
}

void Fast_srand( unsigned int seed )
{
    s_rng.seed( seed );
}
