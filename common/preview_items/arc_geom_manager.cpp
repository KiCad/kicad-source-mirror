/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/arc_geom_manager.h>

#include <common.h> // KiROUND

using namespace KIGFX::PREVIEW;


///> Snap an angle to the nearest 45 degrees
static double snapAngle( double aAngle )
{
    return KiROUND( aAngle / M_PI_4 ) * M_PI_4;
}


bool ARC_GEOM_MANAGER::acceptPoint( const VECTOR2I& aPt )
{
    switch( getStep() )
    {
    case SET_ORIGIN:
        return setOrigin( aPt );
    case SET_START:
        return setStart( aPt );
    case SET_ANGLE:
        return setEnd( aPt );
    case COMPLETE:
        break;
    }

    return false;
}


void ARC_GEOM_MANAGER::SetClockwise( bool aCw )
{
    m_clockwise = aCw;
    setGeometryChanged();
}


void ARC_GEOM_MANAGER::ToggleClockwise()
{
    m_clockwise = !m_clockwise;
    setGeometryChanged();
}


VECTOR2I ARC_GEOM_MANAGER::GetOrigin() const
{
    return m_origin;
}


VECTOR2I ARC_GEOM_MANAGER::GetStartRadiusEnd() const
{
    return m_origin + VECTOR2I( m_radius, 0 ).Rotate( m_startAngle );
}


VECTOR2I ARC_GEOM_MANAGER::GetEndRadiusEnd() const
{
    return m_origin + VECTOR2I( m_radius, 0 ).Rotate( m_endAngle );
}


double ARC_GEOM_MANAGER::GetRadius() const
{
    return m_radius;
}


double ARC_GEOM_MANAGER::GetStartAngle() const
{
    double angle = m_startAngle;

    if( m_clockwise )
        angle -= 2 * M_PI;

    return -angle;
}


double ARC_GEOM_MANAGER::GetSubtended() const
{
    double angle = m_endAngle - m_startAngle;

    if( m_endAngle <= m_startAngle )
        angle += 2 * M_PI;

    if( m_clockwise )
        angle -= 2 * M_PI;

    return -angle;
}


bool ARC_GEOM_MANAGER::setOrigin( const VECTOR2I& aOrigin )
{
    m_origin    = aOrigin;
    m_startAngle = 0.0;
    m_endAngle = 0.0;

    return true;
}


bool ARC_GEOM_MANAGER::setStart( const VECTOR2I& aEnd )
{
    const auto radVec = aEnd - m_origin;

    m_radius = radVec.EuclideanNorm();
    m_startAngle = radVec.Angle();

    if( m_angleSnap )
        m_startAngle = snapAngle( m_startAngle );

    // normalise into 0-2Pi
    while( m_startAngle < 0 )
        m_startAngle += M_PI * 2;

    m_endAngle = m_startAngle;

    return m_radius != 0.0;
}


bool ARC_GEOM_MANAGER::setEnd( const VECTOR2I& aCursor )
{
    const auto radVec = aCursor - m_origin;

    m_endAngle = radVec.Angle();

    if( m_angleSnap )
        m_endAngle = snapAngle( m_endAngle );

    // normalise into 0-2Pi
    while( m_endAngle < 0 )
        m_endAngle += M_PI * 2;

    // if the end is the same as the start, this is a bad point
    return m_endAngle != m_startAngle;
}
