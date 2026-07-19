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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <geometry/ellipse.h>

#include <type_traits>


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


template <typename NumericType>
EDA_ANGLE ELLIPSE<NumericType>::GetSubtendedAngle() const
{
    EDA_ANGLE subtended = EndAngle - StartAngle;
    return subtended.Normalize();
}


template <typename NumericType>
VECTOR2<NumericType> ELLIPSE<NumericType>::GetPointAtAngle( EDA_ANGLE angle ) const
{
    double t = angle.AsRadians();
    double ex = MajorRadius * std::cos( t );
    double ey = MinorRadius * std::sin( t );

    double cosR = Rotation.Cos();
    double sinR = Rotation.Sin();
    double rx = ex * cosR - ey * sinR;
    double ry = ex * sinR + ey * cosR;

    if constexpr( std::is_integral_v<NumericType> )
        return Center + VECTOR2I( KiROUND( rx ), KiROUND( ry ) );
    else
        return Center + VECTOR2<NumericType>( rx, ry );
}


template <typename NumericType>
EDA_ANGLE ELLIPSE<NumericType>::GetAngleAtPoint( const VECTOR2<NumericType>& aPt ) const
{
    const double a = std::max( 1.0, static_cast<double>( MajorRadius ) );
    const double b = std::max( 1.0, static_cast<double>( MinorRadius ) );

    const double dx = static_cast<double>( aPt.x ) - static_cast<double>( Center.x );
    const double dy = static_cast<double>( aPt.y ) - static_cast<double>( Center.y );

    const double cosRot = Rotation.Cos();
    const double sinRot = Rotation.Sin();
    const double lx = dx * cosRot + dy * sinRot;
    const double ly = -dx * sinRot + dy * cosRot;

    return EDA_ANGLE( std::atan2( ly / b, lx / a ), RADIANS_T );
}


template class ELLIPSE<double>;
template class ELLIPSE<int>;
