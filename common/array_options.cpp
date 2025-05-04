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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <array_options.h>

#include <trigo.h>


int ARRAY_GRID_OPTIONS::GetArraySize() const
{
    return m_nx * m_ny;
}


VECTOR2I ARRAY_GRID_OPTIONS::getGridCoords( int n ) const
{
    const int axisSize = m_horizontalThenVertical ? m_nx : m_ny;

    int x = n % axisSize;
    int y = n / axisSize;

    // reverse on this row/col?
    if( m_reverseNumberingAlternate && ( y % 2 ) )
        x = axisSize - x - 1;

    VECTOR2I coords( x, y );

    return coords;
}


VECTOR2I ARRAY_GRID_OPTIONS::gtItemPosRelativeToItem0( int n ) const
{
    VECTOR2I point;

    VECTOR2I coords = getGridCoords( n );

    // swap axes if needed
    if( !m_horizontalThenVertical )
        std::swap( coords.x, coords.y );

    point.x = coords.x * m_delta.x + coords.y * m_offset.x;
    point.y = coords.y * m_delta.y + coords.x * m_offset.y;

    if( std::abs( m_stagger ) > 1 )
    {
        const int  stagger = std::abs( m_stagger );
        const bool sr = m_stagger_rows;
        const int  stagger_idx = ( ( sr ? coords.y : coords.x ) % stagger );

        VECTOR2I stagger_delta( ( sr ? m_delta.x : m_offset.x ), ( sr ? m_offset.y : m_delta.y ) );

        // Stagger to the left/up if the sign of the stagger is negative
        point += stagger_delta * copysign( stagger_idx, m_stagger ) / stagger;
    }

    return point;
}


ARRAY_OPTIONS::TRANSFORM ARRAY_GRID_OPTIONS::GetTransform( int n, const VECTOR2I& aPos ) const
{
    VECTOR2I point = gtItemPosRelativeToItem0( n );

    // Bump the item by half the array size
    if( m_centred )
    {
        // Get the array extents
        const int arrayExtentX = ( m_nx - 1 ) * m_delta.x + ( m_ny - 1 ) * m_offset.x;
        const int arrayExtentY = ( m_ny - 1 ) * m_delta.y + ( m_nx - 1 ) * m_offset.y;

        point -= VECTOR2I( arrayExtentX, arrayExtentY ) / 2;
    }

    return { point, ANGLE_0 };
}


wxString ARRAY_GRID_OPTIONS::GetItemNumber( int n ) const
{
    wxString itemNum;

    if( m_2dArrayNumbering )
    {
        VECTOR2I coords = getGridCoords( n );

        itemNum << m_pri_axis.GetItemNumber( coords.x );
        itemNum << m_sec_axis.GetItemNumber( coords.y );
    }
    else
    {
        itemNum << m_pri_axis.GetItemNumber( n );
    }

    return itemNum;
}


int ARRAY_CIRCULAR_OPTIONS::GetArraySize() const
{
    return m_nPts;
}


ARRAY_OPTIONS::TRANSFORM ARRAY_CIRCULAR_OPTIONS::GetTransform( int n, const VECTOR2I& aPos ) const
{
    EDA_ANGLE angle;

    if( m_angle.IsZero() )
        // angle is zero, divide evenly into m_nPts
        angle = EDA_ANGLE( 360.0 * n / double( m_nPts ), DEGREES_T );
    else
        // n'th step
        angle = EDA_ANGLE( m_angle.AsDegrees() * n, DEGREES_T );

    angle += m_angleOffset;

    if( m_clockwise )
        angle = -angle;

    VECTOR2I new_pos = aPos;
    RotatePoint( new_pos, m_centre, angle );

    // take off the rotation (but not the translation) if needed
    if( !m_rotateItems )
        angle = ANGLE_0;

    return { new_pos - aPos, angle };
}


wxString ARRAY_CIRCULAR_OPTIONS::GetItemNumber( int aN ) const
{
    return m_axis.GetItemNumber( aN );
}
