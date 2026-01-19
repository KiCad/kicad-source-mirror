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

#include <geometry/arc_chord_params.h>

#include <algorithm>
#include <cmath>


bool ARC_CHORD_PARAMS::Compute( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    m_valid = false;

    const double dx = static_cast<double>( aEnd.x ) - aStart.x;
    const double dy = static_cast<double>( aEnd.y ) - aStart.y;
    m_chordLen = std::sqrt( dx * dx + dy * dy );

    if( m_chordLen <= 0.0 )
        return false;

    const double mx = static_cast<double>( aMid.x ) - aStart.x;
    const double my = static_cast<double>( aMid.y ) - aStart.y;
    const double cross = mx * dy - my * dx;

    if( cross == 0.0 )
        return false;

    m_sagitta = std::abs( cross ) / m_chordLen;

    if( m_sagitta <= 0.0 )
        return false;

    m_halfChord = m_chordLen / 2.0;
    m_radius = ( m_halfChord * m_halfChord + m_sagitta * m_sagitta ) / ( 2.0 * m_sagitta );

    if( m_radius <= 0.0 )
        return false;

    m_ux = dx / m_chordLen;
    m_uy = dy / m_chordLen;
    m_nx = -m_uy;
    m_ny = m_ux;

    if( cross < 0.0 )
    {
        m_nx = -m_nx;
        m_ny = -m_ny;
    }

    m_centerOffset = m_radius - m_sagitta;
    m_midx = ( static_cast<double>( aStart.x ) + aEnd.x ) * 0.5;
    m_midy = ( static_cast<double>( aStart.y ) + aEnd.y ) * 0.5;

    m_valid = true;
    return true;
}


VECTOR2D ARC_CHORD_PARAMS::GetCenterPoint() const
{
    // Center is at chord midpoint + center_offset along the normal direction
    return VECTOR2D( m_midx + m_nx * m_centerOffset, m_midy + m_ny * m_centerOffset );
}


EDA_ANGLE ARC_CHORD_PARAMS::GetStartAngle() const
{
    double sin_half = m_halfChord / m_radius;
    double cos_half = m_centerOffset / m_radius;

    // Direction from center to start: -sin(half_angle) * u - cos(half_angle) * n
    return EDA_ANGLE( VECTOR2D( -sin_half * m_ux - cos_half * m_nx,
                                -sin_half * m_uy - cos_half * m_ny ) );
}


EDA_ANGLE ARC_CHORD_PARAMS::GetEndAngle() const
{
    double sin_half = m_halfChord / m_radius;
    double cos_half = m_centerOffset / m_radius;

    // Direction from center to end: sin(half_angle) * u - cos(half_angle) * n
    return EDA_ANGLE( VECTOR2D( sin_half * m_ux - cos_half * m_nx,
                                sin_half * m_uy - cos_half * m_ny ) );
}


double ARC_CHORD_PARAMS::GetArcAngle() const
{
    double ratio = std::clamp( m_halfChord / m_radius, 0.0, 1.0 );
    double base_angle = 2.0 * std::asin( ratio );

    if( m_sagitta > m_radius )
        return 2.0 * M_PI - base_angle;

    return base_angle;
}
