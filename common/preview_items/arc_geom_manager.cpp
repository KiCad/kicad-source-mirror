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

#include <preview_items/arc_geom_manager.h>

#include <math/util.h>      // for KiROUND
#include <geometry/eda_angle.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;


///< Snap an angle to the nearest 45 degrees
static EDA_ANGLE snapAngle( const EDA_ANGLE& aAngle )
{
    return ANGLE_45 * KiROUND( aAngle / ANGLE_45 );
}


bool ARC_GEOM_MANAGER::acceptPoint( const VECTOR2I& aPt )
{
    switch( getStep() )
    {
    case SET_ORIGIN: return setOrigin( aPt );
    case SET_START:  return setStart( aPt );
    case SET_ANGLE:  return setEnd( aPt );
    case COMPLETE:   return false;
    }

    return false;
}


void ARC_GEOM_MANAGER::SetClockwise( bool aCw )
{
    m_clockwise = aCw;
    m_directionLocked = true;
    setGeometryChanged();
}


void ARC_GEOM_MANAGER::ToggleClockwise()
{
    m_clockwise = !m_clockwise;
    m_directionLocked = true;
    setGeometryChanged();
}


VECTOR2I ARC_GEOM_MANAGER::GetOrigin() const
{
    return m_origin;
}


VECTOR2I ARC_GEOM_MANAGER::GetStartRadiusEnd() const
{
    VECTOR2I vec( static_cast<int>( m_radius ), 0 );
    RotatePoint( vec, -m_startAngle );
    return m_origin +vec;
}


VECTOR2I ARC_GEOM_MANAGER::GetEndRadiusEnd() const
{
    VECTOR2I vec( static_cast<int>( m_radius ), 0 );
    RotatePoint( vec, -m_endAngle );
    return m_origin + vec;
}


double ARC_GEOM_MANAGER::GetRadius() const
{
    return m_radius;
}


EDA_ANGLE ARC_GEOM_MANAGER::GetStartAngle() const
{
    EDA_ANGLE angle = m_startAngle;

    if( m_clockwise )
        angle -= ANGLE_360;

    return -angle;
}


EDA_ANGLE ARC_GEOM_MANAGER::GetSubtended() const
{
    EDA_ANGLE angle = m_endAngle - m_startAngle;

    if( m_endAngle <= m_startAngle )
        angle += ANGLE_360;

    if( m_clockwise )
        angle -= ANGLE_360;

    return -angle;
}


bool ARC_GEOM_MANAGER::setOrigin( const VECTOR2I& aOrigin )
{
    m_origin     = aOrigin;
    m_startAngle = ANGLE_0;
    m_endAngle   = ANGLE_0;

    return true;
}


bool ARC_GEOM_MANAGER::setStart( const VECTOR2I& aEnd )
{
    const VECTOR2I radVec = aEnd - m_origin;

    m_radius = radVec.EuclideanNorm();
    m_startAngle = EDA_ANGLE( radVec );

    if( m_angleSnap )
        m_startAngle = snapAngle( m_startAngle );

    // normalise to 0..360
    while( m_startAngle < ANGLE_0 )
        m_startAngle += ANGLE_360;

    m_endAngle = m_startAngle;

    return m_radius != 0.0;
}


bool ARC_GEOM_MANAGER::setEnd( const VECTOR2I& aCursor )
{
    const VECTOR2I radVec = aCursor - m_origin;

    m_endAngle = EDA_ANGLE( radVec );

    if( m_angleSnap )
        m_endAngle = snapAngle( m_endAngle );

    // normalise to 0..360
    while( m_endAngle < ANGLE_0 )
        m_endAngle += ANGLE_360;

    if( !m_directionLocked )
    {
        EDA_ANGLE ccwAngle = m_endAngle - m_startAngle;

        if( m_endAngle <= m_startAngle )
            ccwAngle += ANGLE_360;

        EDA_ANGLE cwAngle = std::abs( ccwAngle - ANGLE_360 );

        if( std::min( ccwAngle, cwAngle ) >= ANGLE_90 )
            m_directionLocked = true;
        else
            m_clockwise = cwAngle < ccwAngle;
    }
    else if( std::abs( GetSubtended() ) < ANGLE_90 )
    {
        m_directionLocked = false;
    }

    // if the end is the same as the start, this is a bad point
    return m_endAngle != m_startAngle;
}
