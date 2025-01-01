/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file filled_circle_2d.cpp
 * @brief
 */

#include "filled_circle_2d.h"
#include "../ray.h"
#include <wx/debug.h>


FILLED_CIRCLE_2D::FILLED_CIRCLE_2D( const SFVEC2F& aCenter, float aRadius,
                                    const BOARD_ITEM& aBoardItem ) :
        OBJECT_2D( OBJECT_2D_TYPE::FILLED_CIRCLE, aBoardItem )
{
    wxASSERT( aRadius > 0.0f ); // If that happens, it should be handled before create this circle

    m_center = aCenter;
    m_radius = aRadius;
    m_radius_squared = aRadius * aRadius;

    m_bbox.Reset();
    m_bbox.Set( m_center - SFVEC2F( aRadius, aRadius ),
                m_center + SFVEC2F( aRadius, aRadius ) );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    wxASSERT( m_bbox.IsInitialized() );
}


bool FILLED_CIRCLE_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // NOT IMPLEMENTED, why?
    return false;
}


bool FILLED_CIRCLE_2D::Intersects( const BBOX_2D& aBBox ) const
{
    return aBBox.Intersects( m_center, m_radius_squared );
}


bool FILLED_CIRCLE_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    // This code used directly from Steve Marschner's CS667 framework
    // http://cs665pd.googlecode.com/svn/trunk/photon/sphere.cpp

    // Compute some factors used in computation
    const float qx = aSegRay.m_Start.x - m_center.x;
    const float qy = aSegRay.m_Start.y - m_center.y;

    const float qd = qx * aSegRay.m_Dir.x + qy * aSegRay.m_Dir.y;
    const float qq = qx * qx + qy * qy;

    // solving the quadratic equation for t at the pts of intersection
    // dd*t^2 + (2*qd)*t + (qq-r^2) = 0
    const float discriminantsqr = ( qd * qd - ( qq - m_radius_squared ) );

    // If the discriminant is less than zero, there is no intersection
    if( discriminantsqr < FLT_EPSILON )
        return false;

    // Otherwise check and make sure that the intersections occur on the ray (t > 0) and
    // return the closer one.
    const float discriminant = sqrt( discriminantsqr );
    const float t1 = ( -qd - discriminant );
    const float t2 = ( -qd + discriminant );
    float       t;

    if( ( t1 > 0.0f ) && ( t1 < aSegRay.m_Length ) )
    {
        t = t1;
    }
    else
    {
        if( ( t2 > 0.0f ) && ( t2 < aSegRay.m_Length ) )
            t = t2;
        else
            return false; // Neither intersection was in the ray's half line.
    }

    wxASSERT( ( t > 0.0f ) && ( t <= aSegRay.m_Length ) );

    // Convert the intersection to a normalized 0.0 .. 1.0
    if( aOutT )
        *aOutT = t / aSegRay.m_Length;

    const SFVEC2F hitPoint = aSegRay.at( t );

    if( aNormalOut )
        *aNormalOut = (hitPoint - m_center) / m_radius;

    return true;
}


INTERSECTION_RESULT FILLED_CIRCLE_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return INTERSECTION_RESULT::MISSES;

    SFVEC2F v[4];

    v[0] = aBBox.Min() - m_center;
    v[1] = aBBox.Max() - m_center;
    v[2] = SFVEC2F( aBBox.Min().x, aBBox.Max().y ) - m_center;
    v[3] = SFVEC2F( aBBox.Max().x, aBBox.Min().y ) - m_center;

    float s[4];

    s[0] = v[0].x * v[0].x + v[0].y * v[0].y;
    s[1] = v[1].x * v[1].x + v[1].y * v[1].y;
    s[2] = v[2].x * v[2].x + v[2].y * v[2].y;
    s[3] = v[3].x * v[3].x + v[3].y * v[3].y;

    bool isInside[4];

    isInside[0] = s[0] <= m_radius_squared;
    isInside[1] = s[1] <= m_radius_squared;
    isInside[2] = s[2] <= m_radius_squared;
    isInside[3] = s[3] <= m_radius_squared;

    // Check if all points are inside the circle
    if( isInside[0] && isInside[1] && isInside[2] && isInside[3] )
        return INTERSECTION_RESULT::FULL_INSIDE;

    // Check if any point is inside the circle
    if( isInside[0] || isInside[1] || isInside[2] || isInside[3] )
        return INTERSECTION_RESULT::INTERSECTS;

    return INTERSECTION_RESULT::MISSES;
}


bool FILLED_CIRCLE_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    const SFVEC2F v = m_center - aPoint;

    if( ( v.x * v.x + v.y * v.y ) <= m_radius_squared )
        return true;

    return false;
}
