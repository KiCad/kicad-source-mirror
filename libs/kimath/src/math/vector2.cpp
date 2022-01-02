/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math/vector2d.h>


size_t std::hash<VECTOR2I>::operator()( const VECTOR2I& k ) const
{
    auto xhash = std::hash<int>()( k.x );

    // 0x9e3779b9 is 2^33 / ( 1 + sqrt(5) )
    // Adding this value ensures that consecutive bits of y will not be close to each other
    // decreasing the likelihood of hash collision in similar values of x and y
    return xhash ^ ( std::hash<int>()( k.y ) + 0x9e3779b9 + ( xhash << 6 ) + ( xhash >> 2 ) );
}


bool std::less<VECTOR2I>::operator()( const VECTOR2I& aA, const VECTOR2I& aB ) const
{
    if( aA.x == aB.x )
        return aA.y < aB.y;

    return aA.x < aB.x;
}