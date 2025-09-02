/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <sstream>

#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <convert_basic_shapes_to_polygon.h>


SHAPE_SEGMENT SHAPE_SEGMENT::BySizeAndCenter( const VECTOR2I& aOverallSize, const VECTOR2I& aCenter,
                                              const EDA_ANGLE& aRotation )
{
    VECTOR2I segVec{ 0, 0 };
    int      width;

    // Find the major axis, without endcaps
    if( aOverallSize.x > aOverallSize.y )
    {
        width = aOverallSize.y;
        segVec.x = aOverallSize.x - width;
    }
    else
    {
        width = aOverallSize.x;
        segVec.y = aOverallSize.y - width;
    }

    RotatePoint( segVec, aRotation );

    return SHAPE_SEGMENT( aCenter - segVec / 2, aCenter + segVec / 2, width );
}


const std::string SHAPE_SEGMENT::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    if( aCplusPlus )
    {
    ss << "SHAPE_SEGMENT( VECTOR2I( ";
    ss << m_seg.A.x;
    ss << ", ";
    ss << m_seg.A.y;
    ss << "), VECTOR2I( ";
    ss << m_seg.B.x;
    ss << ", ";
    ss << m_seg.B.y;
    ss << "), ";
    ss << m_width;
    ss << "); ";
    }
    else
    {
        ss << SHAPE::Format( aCplusPlus ) << " ";
        ss << m_seg.A.x;
        ss << " ";
        ss << m_seg.A.y;
        ss << " ";
        ss << m_seg.B.x;
        ss << " ";
        ss << m_seg.B.y;
        ss << " ";
        ss << m_width;
    }

    return ss.str();
}

const std::string SHAPE_CIRCLE::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    if( aCplusPlus )
    {
        ss << "SHAPE_CIRCLE( VECTOR2I( ";
        ss << m_circle.Center.x;
        ss << ", ";
        ss << m_circle.Center.y;
        ss << "), ";
        ss << m_circle.Radius;
        ss << "); ";
    }   else
    {
        ss << SHAPE::Format( aCplusPlus ) << " ";
        ss << m_circle.Center.x;
        ss << " ";
        ss << m_circle.Center.y;
        ss << " ";
        ss << m_circle.Radius;
    }
    return ss.str();
}


void SHAPE_CIRCLE::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                                       ERROR_LOC aErrorLoc ) const
{
    TransformCircleToPolygon( aBuffer, m_circle.Center, m_circle.Radius, aError, aErrorLoc );
}


bool SHAPE_SEGMENT::Is45Degree( EDA_ANGLE aTollerance ) const
{
    EDA_ANGLE mag = EDA_ANGLE( m_seg.A - m_seg.B ).Normalize180();

    double f = fmod( mag.AsDegrees(), 45.0 );
    double d = aTollerance.AsDegrees();

    if( f >= 45.0 - d || f <= d )
    {
        return true;
    }

    return false;
}


void SHAPE_SEGMENT::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                                        ERROR_LOC aErrorLoc ) const
{
    TransformOvalToPolygon( aBuffer, m_seg.A, m_seg.B, m_width, aError, aErrorLoc );
}
