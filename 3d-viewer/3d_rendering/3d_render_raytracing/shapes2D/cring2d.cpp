/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cring2d.cpp
 * @brief
 */

#include "cring2d.h"
#include "../../../3d_fastmath.h"
#include <wx/debug.h>


CRING2D::CRING2D( const SFVEC2F &aCenter, float aInnerRadius, float aOuterRadius,
                  const BOARD_ITEM &aBoardItem ) : COBJECT2D( OBJ2D_RING, aBoardItem )
{
    wxASSERT( aInnerRadius < aOuterRadius );

    m_center = aCenter;
    m_inner_radius = aInnerRadius;
    m_outer_radius = aOuterRadius;

    m_inner_radius_squared = aInnerRadius * aInnerRadius;
    m_outer_radius_squared = aOuterRadius * aOuterRadius;

    m_bbox.Reset();
    m_bbox.Set( m_center - SFVEC2F( aOuterRadius, aOuterRadius ),
                m_center + SFVEC2F( aOuterRadius, aOuterRadius ) );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();


    wxASSERT( m_bbox.IsInitialized() );
}


bool CRING2D::Overlaps( const CBBOX2D &aBBox ) const
{
    // NOT IMPLEMENTED
    return false;
}


bool CRING2D::Intersects( const CBBOX2D &aBBox ) const
{
    // !TODO: check the inside for a great improovment
    return aBBox.Intersects( m_center, m_outer_radius_squared );
}


bool CRING2D::Intersect( const RAYSEG2D &aSegRay,
                         float *aOutT,
                         SFVEC2F *aNormalOut ) const
{
    // This code used directly from Steve Marschner's CS667 framework
    // http://cs665pd.googlecode.com/svn/trunk/photon/sphere.cpp

    // Compute some factors used in computation
    const float qx = (aSegRay.m_Start.x - m_center.x);
    const float qy = (aSegRay.m_Start.y - m_center.y);

    const float qd = qx * aSegRay.m_Dir.x + qy * aSegRay.m_Dir.y;
    const float qq = qx * qx + qy * qy;

    // solving the quadratic equation for t at the pts of intersection
    // dd*t^2 + (2*qd)*t + (qq-r^2) = 0

    const float discriminantsqr = qd * qd - qq;
    const float discriminantsqr_outter = discriminantsqr + m_outer_radius_squared;

    // If the discriminant is less than zero, there is no intersection
    if( discriminantsqr_outter < FLT_EPSILON )
        return false;

    // Otherwise check and make sure that the intersections occur on the ray (t
    // > 0) and return the closer one
    const float discriminant = sqrt( discriminantsqr_outter );
    float t = (-qd - discriminant);

    if( (t > FLT_EPSILON) && (t < aSegRay.m_Length) )
    {
        SFVEC2F hitPoint = aSegRay.at( t );
        *aNormalOut = (hitPoint - m_center) / m_outer_radius;
    }
    else
    {
        const float discriminantsqr_inter = discriminantsqr + m_inner_radius_squared;

        if( discriminantsqr_inter > FLT_EPSILON )
        {
            const float discriminant_inner = sqrt( discriminantsqr_inter );

            const float t2_inner = (-qd + discriminant_inner);

            if( (t2_inner > FLT_EPSILON) && (t2_inner < aSegRay.m_Length) )
            {
                t = t2_inner;

                const SFVEC2F hitPoint = aSegRay.at( t2_inner );

                *aNormalOut = (m_center - hitPoint) / m_inner_radius;
            }
            else
                return false;
        }
        else
            return false;
    }

    wxASSERT( (t > 0.0f) && (t <= aSegRay.m_Length) );

    // Convert the intersection to a normalized 0.0 .. 1.0
    *aOutT = t / aSegRay.m_Length;

    return true;
}


INTERSECTION_RESULT CRING2D::IsBBoxInside( const CBBOX2D &aBBox ) const
{
    /*
    if( !m_bbox.Overlaps( aBBox ) )
        return INTR_MISSES;

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
    if( isInside[0] &&
        isInside[1] &&
        isInside[2] &&
        isInside[3] )
        return INTR_FULL_INSIDE;

    // Check if any point is inside the circle
    if( isInside[0] ||
        isInside[1] ||
        isInside[2] ||
        isInside[3] )
        return INTR_INTERSECTS;
*/
    return INTR_MISSES;
}


bool CRING2D::IsPointInside( const SFVEC2F &aPoint ) const
{
    const SFVEC2F v = m_center - aPoint;

    const float dot = glm::dot( v, v );

    if( (dot <= m_outer_radius_squared) &&
        (dot >= m_inner_radius_squared) )
        return true;

    return false;
}
