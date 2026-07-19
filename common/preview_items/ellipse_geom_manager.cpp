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

#include <preview_items/ellipse_geom_manager.h>

#include <trigo.h>
#include <algorithm>

using namespace KIGFX::PREVIEW;


bool ELLIPSE_GEOM_MANAGER::acceptPoint( const VECTOR2I& aPt )
{
    // clang-format off
    switch( GetStep() )
    {
    case SET_BBOX_C1:       return setBboxCorner1( aPt );
    case SET_BBOX_C2:       return setBboxCorner2( aPt );
    case SET_START_ANGLE:   return setStartAngle( aPt );
    case SET_END_ANGLE:     return setEndAngle( aPt );
    case COMPLETE:          return false;
    }
    // clang-format on

    return false;
}


bool ELLIPSE_GEOM_MANAGER::setBboxCorner1( const VECTOR2I& aPt )
{
    m_bboxC1 = aPt;
    m_bboxC2 = aPt;
    return true;
}


bool ELLIPSE_GEOM_MANAGER::setBboxCorner2( const VECTOR2I& aPt )
{
    m_bboxC2 = aPt;

    const VECTOR2I first  = m_bboxC1;
    const VECTOR2I second = m_bboxC2;
    const VECTOR2I center = ( first + second ) / 2;
    const int      halfW = std::abs( second.x - first.x ) / 2;
    const int      halfH = std::abs( second.y - first.y ) / 2;

    if( halfW >= halfH )
    {
        m_majorRadius = std::max( halfW, 1 );
        m_minorRadius = std::max( halfH, 1 );
        m_rotation = ANGLE_0;
    }
    else
    {
        m_majorRadius = std::max( halfH, 1 );
        m_minorRadius = std::max( halfW, 1 );
        m_rotation = ANGLE_90;
    }

    m_center = center;

    // Keep full ellipse preview during bbox building.
    m_startAngle = ANGLE_0;
    m_endAngle   = ANGLE_360;

    return true;
}


bool ELLIPSE_GEOM_MANAGER::setStartAngle( const VECTOR2I& aPt )
{
    const ELLIPSE<double> ellipse( VECTOR2D( m_center ), m_majorRadius, m_minorRadius, m_rotation );

    m_startAngle = ellipse.GetAngleAtPoint( aPt );
    m_endAngle = m_startAngle + ANGLE_360;

    return true;
}


bool ELLIPSE_GEOM_MANAGER::setEndAngle( const VECTOR2I& aPt )
{
    const ELLIPSE<double> ellipse( VECTOR2D( m_center ), m_majorRadius, m_minorRadius, m_rotation );
    EDA_ANGLE             cursorAngle = ellipse.GetAngleAtPoint( aPt );

    // Enforce end > start using 360 wrapping.
    while( cursorAngle <= m_startAngle )
        cursorAngle = cursorAngle + ANGLE_360;

    m_endAngle = cursorAngle;

    return m_endAngle != m_startAngle;
}


ELLIPSE<int> ELLIPSE_GEOM_MANAGER::GetEllipse() const
{
    return ELLIPSE<int>( m_center, m_majorRadius, m_minorRadius, m_rotation, m_startAngle, m_endAngle );
}
