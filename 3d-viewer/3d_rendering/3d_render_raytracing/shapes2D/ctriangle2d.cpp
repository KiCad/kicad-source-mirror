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
 * @file  ctriangle2d.cpp
 * @brief
 */

#include "ctriangle2d.h"
#include <map>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/shared_array.hpp>
#include <wx/debug.h>

#include <wx/glcanvas.h>    // CALLBACK definition, needed on Windows
                            // alse needed on OSX to define __DARWIN__
#include <geometry/polygon_triangulation.h>
#include "../../../3d_fastmath.h"



CTRIANGLE2D::CTRIANGLE2D ( const SFVEC2F &aV1,
                           const SFVEC2F &aV2,
                           const SFVEC2F &aV3,
                           const BOARD_ITEM &aBoardItem ) : COBJECT2D( OBJ2D_TRIANGLE,
                                                                       aBoardItem )
{
    p1 = aV1;
    p2 = aV2;
    p3 = aV3;

    // Pre-Calc values
    m_inv_denominator = 1.0f / ( (p2.y - p3.y) * (p1.x - p3.x) +
                                 (p3.x - p2.x) * (p1.y - p3.y));
    m_p2y_minus_p3y = (p2.y - p3.y);
    m_p3x_minus_p2x = (p3.x - p2.x);
    m_p3y_minus_p1y = (p3.y - p1.y);
    m_p1x_minus_p3x = (p1.x - p3.x);

    m_bbox.Reset();
    m_bbox.Union( aV1 );
    m_bbox.Union( aV2 );
    m_bbox.Union( aV3 );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    wxASSERT( m_bbox.IsInitialized() );
}


bool CTRIANGLE2D::Intersects( const CBBOX2D &aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return false;
    //!TODO: Optimize
    return true;
}


bool CTRIANGLE2D::Overlaps( const CBBOX2D &aBBox ) const
{
    // NOT IMPLEMENTED
    return false;
}


bool CTRIANGLE2D::Intersect( const RAYSEG2D &aSegRay,
                             float *aOutT,
                             SFVEC2F *aNormalOut ) const
{
    wxASSERT( aOutT );
    wxASSERT( aNormalOut );
    return false;
}


INTERSECTION_RESULT CTRIANGLE2D::IsBBoxInside( const CBBOX2D &aBBox ) const
{
    if( !m_bbox.Intersects( aBBox ) )
        return INTR_MISSES;
    // !TODO:
    return INTR_MISSES;
}


bool CTRIANGLE2D::IsPointInside( const SFVEC2F &aPoint ) const
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


void Convert_shape_line_polygon_to_triangles( SHAPE_POLY_SET &aPolyList,
                                              CGENERICCONTAINER2D &aDstContainer,
                                              float aBiuTo3DunitsScale ,
                                              const BOARD_ITEM &aBoardItem )
{

    aPolyList.CacheTriangulation();
    const double conver_d = (double)aBiuTo3DunitsScale;

    for( unsigned int j = 0; j < aPolyList.TriangulatedPolyCount(); j++ )
    {
        auto triPoly = aPolyList.TriangulatedPolygon( j );

        for( size_t i = 0; i < triPoly->GetTriangleCount(); i++ )
        {
            VECTOR2I a;
            VECTOR2I b;
            VECTOR2I c;
            triPoly->GetTriangle( i, a, b, c );

            aDstContainer.Add( new CTRIANGLE2D( SFVEC2F( a.x * conver_d,
                                                        -a.y * conver_d ),
                                                SFVEC2F( b.x * conver_d,
                                                        -b.y * conver_d ),
                                                SFVEC2F( c.x * conver_d,
                                                        -c.y * conver_d ),
                                                aBoardItem ) );
        }

    }
}
