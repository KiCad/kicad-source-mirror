/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_units.h
 * @brief Design units of 100 nm and angles of thousandths of a degree,
 *        anticlockwise from +X with Y up.
 */

#ifndef EASYPC_UNITS_H
#define EASYPC_UNITS_H

#include <cmath>
#include <cstdint>
#include <limits>


namespace EASYPC
{

constexpr int64_t NM_PER_DSU = 100;

constexpr int ANGLE_FULL_TURN = 360000;

/// Half of KiCad's integer range, so shape code rotating or adding coordinates cannot overflow
constexpr double COORD_LIMIT = std::numeric_limits<int>::max() / 2.0;


constexpr int64_t DsuToNm( int64_t aDsu )
{
    return aDsu * NM_PER_DSU;
}


constexpr double AngleToDegrees( int32_t aAngle )
{
    return aAngle / 1000.0;
}


/// Wrap an angle into [0, ANGLE_FULL_TURN)
constexpr int32_t NormalizeAngle( int32_t aAngle )
{
    return ( aAngle % ANGLE_FULL_TURN + ANGLE_FULL_TURN ) % ANGLE_FULL_TURN;
}


/**
 * Rotate aPx, aPy anticlockwise by aAngle about aCx, aCy as Easy-PC places points: in double with exact quarter
 * turns, truncating toward zero, so the operand order of each sum decides the last bit.
 */
inline void PointRotate( double aPx, double aPy, int32_t aAngle, double aCx, double aCy, int32_t& aOutX,
                         int32_t& aOutY )
{
    double x = aPx;
    double y = aPy;

    if( aAngle != 0 )
    {
        double c = std::cos( aAngle * M_PI / 180000.0 );
        double s = std::sin( aAngle * M_PI / 180000.0 );

        if( aAngle == 90000 || aAngle == 270000 )
        {
            c = 0.0;
            s = aAngle == 90000 ? 1.0 : -1.0;
        }
        else if( aAngle == 180000 )
        {
            c = -1.0;
            s = 0.0;
        }

        double dx = aPx - aCx;
        double dy = aPy - aCy;
        x = ( c * dx + aCx ) - s * dy;
        y = s * dx + aCy + c * dy;
    }

    aOutX = static_cast<int32_t>( x );
    aOutY = static_cast<int32_t>( y );
}


inline void PointRotate( int32_t& aX, int32_t& aY, int32_t aAngle, int32_t aCx, int32_t aCy )
{
    PointRotate( aX, aY, aAngle, aCx, aCy, aX, aY );
}

} // namespace EASYPC

#endif // EASYPC_UNITS_H
