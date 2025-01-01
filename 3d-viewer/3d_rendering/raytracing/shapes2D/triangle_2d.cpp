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
 * @file triangle_2d.cpp
 */

#include "triangle_2d.h"
#include "../ray.h"
#include <wx/debug.h>
#include <geometry/polygon_triangulation.h>
#include "../../../3d_fastmath.h"


TRIANGLE_2D::TRIANGLE_2D( const SFVEC2F& aV1, const SFVEC2F& aV2, const SFVEC2F& aV3,
                          const BOARD_ITEM& aBoardItem ) :
        OBJECT_2D( OBJECT_2D_TYPE::TRIANGLE, aBoardItem )
{
    p1 = aV1;
    p2 = aV2;
    p3 = aV3;

    // Pre-Calc values
    m_inv_denominator = 1.0f / ( ( p2.y - p3.y ) * ( p1.x - p3.x ) +
                                 ( p3.x - p2.x ) * ( p1.y - p3.y ) );
    m_p2y_minus_p3y = ( p2.y - p3.y );
    m_p3x_minus_p2x = ( p3.x - p2.x );
    m_p3y_minus_p1y = ( p3.y - p1.y );
    m_p1x_minus_p3x = ( p1.x - p3.x );

    m_bbox.Reset();
    m_bbox.Union( aV1 );
    m_bbox.Union( aV2 );
    m_bbox.Union( aV3 );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    wxASSERT( m_bbox.IsInitialized() );
}


bool TRIANGLE_2D::Intersects( const BBOX_2D& aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return false;

    //!TODO: Optimize
    return true;
}


bool TRIANGLE_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // NOT IMPLEMENTED
    return false;
}


bool TRIANGLE_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    return false;
}


INTERSECTION_RESULT TRIANGLE_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return INTERSECTION_RESULT::MISSES;

    // !TODO:
    return INTERSECTION_RESULT::MISSES;
}


bool TRIANGLE_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    // http://totologic.blogspot.co.uk/2014/01/accurate-point-in-triangle-test.html

    SFVEC2F point_minus_p3 = aPoint - p3;

    // barycentric coordinate system
    const float a = ( m_p2y_minus_p3y * point_minus_p3.x +
                      m_p3x_minus_p2x * point_minus_p3.y ) * m_inv_denominator;

    if( 0.0f > a || a > 1.0f )
        return false;

    const float b = ( m_p3y_minus_p1y * point_minus_p3.x +
                      m_p1x_minus_p3x * point_minus_p3.y ) * m_inv_denominator;

    if( 0.0f > b || b > 1.0f )
        return false;

    const float c = 1.0f - a - b;

    return 0.0f <= c && c <= 1.0f;
}


void ConvertPolygonToTriangles( const SHAPE_POLY_SET& aPolyList, CONTAINER_2D_BASE& aDstContainer,
                                float aBiuTo3dUnitsScale, const BOARD_ITEM& aBoardItem )
{
    VECTOR2I a;
    VECTOR2I b;
    VECTOR2I c;

    const_cast<SHAPE_POLY_SET&>( aPolyList ).CacheTriangulation( false );
    const double conver_d = (double)aBiuTo3dUnitsScale;

    for( unsigned int j = 0; j < aPolyList.TriangulatedPolyCount(); j++ )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* triPoly = aPolyList.TriangulatedPolygon( j );

        for( size_t i = 0; i < triPoly->GetTriangleCount(); i++ )
        {
            triPoly->GetTriangle( i, a, b, c );

            aDstContainer.Add( new TRIANGLE_2D( SFVEC2F( a.x * conver_d, -a.y * conver_d ),
                                                SFVEC2F( b.x * conver_d, -b.y * conver_d ),
                                                SFVEC2F( c.x * conver_d, -c.y * conver_d ),
                                                aBoardItem ) );
        }
    }
}
