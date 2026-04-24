/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/ellipse.h>


template <typename NumericType>
ELLIPSE<NumericType>::ELLIPSE( const VECTOR2<NumericType>& aCenter, NumericType aMajorRadius,
                               NumericType aMinorRadius, EDA_ANGLE aRotation, EDA_ANGLE aStartAngle,
                               EDA_ANGLE aEndAngle ) :
        Center( aCenter ),
        MajorRadius( aMajorRadius ),
        MinorRadius( aMinorRadius ),
        Rotation( aRotation ),
        StartAngle( aStartAngle ),
        EndAngle( aEndAngle )
{
}


template <typename NumericType>
ELLIPSE<NumericType>::ELLIPSE( const VECTOR2<NumericType>& aCenter,
                               const VECTOR2<NumericType>& aMajor, double aRatio,
                               EDA_ANGLE aStartAngle, EDA_ANGLE aEndAngle ) :
        Center( aCenter ),
        StartAngle( aStartAngle ),
        EndAngle( aEndAngle )
{
    MajorRadius = aMajor.EuclideanNorm();
    MinorRadius = NumericType( MajorRadius * aRatio );
    Rotation = EDA_ANGLE( std::atan2( aMajor.y, aMajor.x ), RADIANS_T );
}


template <typename NumericType>
void ELLIPSE<NumericType>::Mirror( const VECTOR2<NumericType>& aRef, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        Center.x = 2 * aRef.x - Center.x;
    else
        Center.y = 2 * aRef.y - Center.y;

    Rotation = -Rotation;

    const EDA_ANGLE oldStart = StartAngle;
    const EDA_ANGLE oldEnd = EndAngle;

    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        // theta' = pi - theta  ->  [s, e]  ->  [pi - e, pi - s]
        StartAngle = ANGLE_180 - oldEnd;
        EndAngle = ANGLE_180 - oldStart;
    }
    else
    {
        // theta' = -theta  ->  [s, e]  ->  [-e, -s]
        StartAngle = -oldEnd;
        EndAngle = -oldStart;
    }
}


template class ELLIPSE<double>;
template class ELLIPSE<int>;
