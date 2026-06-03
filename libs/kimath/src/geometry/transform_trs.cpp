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

#include <geometry/transform_trs.h>
#include <trigo.h>


VECTOR2D TRANSFORM_TRS::Apply( const VECTOR2D& aPoint ) const
{
    VECTOR2D scaled( aPoint.x * m_scaleX, aPoint.y * m_scaleY );
    RotatePoint( scaled, m_rotate );
    return scaled + VECTOR2D( m_translate );
}


VECTOR2I TRANSFORM_TRS::Apply( const VECTOR2I& aPoint ) const
{
    VECTOR2D r = Apply( VECTOR2D( aPoint ) );
    return VECTOR2I( KiROUND( r.x ), KiROUND( r.y ) );
}


VECTOR2D TRANSFORM_TRS::InverseApply( const VECTOR2D& aPoint ) const
{
    VECTOR2D shifted = aPoint - VECTOR2D( m_translate );
    RotatePoint( shifted, -m_rotate );
    return VECTOR2D( shifted.x / m_scaleX, shifted.y / m_scaleY );
}


VECTOR2I TRANSFORM_TRS::InverseApply( const VECTOR2I& aPoint ) const
{
    VECTOR2D r = InverseApply( VECTOR2D( aPoint ) );
    return VECTOR2I( KiROUND( r.x ), KiROUND( r.y ) );
}


TRANSFORM_TRS TRANSFORM_TRS::Invert() const
{
    TRANSFORM_TRS inv;
    inv.m_scaleX = 1.0 / m_scaleX;
    inv.m_scaleY = 1.0 / m_scaleY;
    inv.m_rotate = -m_rotate;

    VECTOR2D t( m_translate );
    RotatePoint( t, -m_rotate );
    inv.m_translate = VECTOR2I( KiROUND( -t.x / m_scaleX ), KiROUND( -t.y / m_scaleY ) );

    return inv;
}


TRANSFORM_TRS TRANSFORM_TRS::Compose( const TRANSFORM_TRS& aOuter ) const
{
    TRANSFORM_TRS result;
    result.m_scaleX = m_scaleX * aOuter.m_scaleX;
    result.m_scaleY = m_scaleY * aOuter.m_scaleY;
    result.m_rotate = m_rotate + aOuter.m_rotate;
    result.m_translate = aOuter.Apply( m_translate );
    return result;
}


TRANSFORM_TRS TRANSFORM_TRS::RescaleAround( const VECTOR2I& aFixedPoint,
                                            double aSx, double aSy ) const
{
    TRANSFORM_TRS result = *this;

    // aSx/aSy are scale multipliers in the footprint's own frame, so the local
    // scale factors scale directly.
    result.m_scaleX = m_scaleX * aSx;
    result.m_scaleY = m_scaleY * aSy;

    // The offset from the fixed point is in board axes. Rotate it into the
    // footprint frame, scale it there, then rotate back (R D R^-1) so the position
    // move matches the local scaling when the footprint is rotated.
    VECTOR2D offset( m_translate.x - aFixedPoint.x, m_translate.y - aFixedPoint.y );
    RotatePoint( offset, -m_rotate );
    offset.x *= aSx;
    offset.y *= aSy;
    RotatePoint( offset, m_rotate );

    result.m_translate = VECTOR2I( KiROUND( aFixedPoint.x + offset.x ), KiROUND( aFixedPoint.y + offset.y ) );
    return result;
}


bool TRANSFORM_TRS::IsIdentity() const
{
    return m_translate == VECTOR2I( 0, 0 )
           && m_rotate.IsZero()
           && m_scaleX == 1.0
           && m_scaleY == 1.0;
}


bool TRANSFORM_TRS::IsUniformScale() const
{
    return m_scaleX == m_scaleY;
}


double TRANSFORM_TRS::ApplyLinearScale( double aLength ) const
{
    return aLength * 0.5 * ( m_scaleX + m_scaleY );
}


bool TRANSFORM_TRS::operator==( const TRANSFORM_TRS& aOther ) const
{
    return m_translate == aOther.m_translate
           && m_rotate == aOther.m_rotate
           && m_scaleX == aOther.m_scaleX
           && m_scaleY == aOther.m_scaleY;
}
