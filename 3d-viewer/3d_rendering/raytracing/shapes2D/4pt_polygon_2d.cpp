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
 * @file 4pt_polygon_2d.cpp
 */

#include "4pt_polygon_2d.h"
#include <wx/debug.h>
#include "../ray.h"


POLYGON_4PT_2D::POLYGON_4PT_2D( const SFVEC2F& v1, const SFVEC2F& v2, const SFVEC2F& v3,
                                const SFVEC2F& v4, const BOARD_ITEM& aBoardItem ) :
        OBJECT_2D( OBJECT_2D_TYPE::POLYGON4PT, aBoardItem )
{
    m_segments[0] = v1;
    m_segments[1] = v4;
    m_segments[2] = v3;
    m_segments[3] = v2;

    unsigned int i;
    unsigned int j = 4 - 1;

    for( i = 0; i < 4; j = i++ )
    {
        SFVEC2F slope      = m_segments[j] - m_segments[i];
        m_precalc_slope[i] = slope;
        m_seg_normal[i]    = glm::normalize( SFVEC2F( -slope.y, +slope.x ) );
    }

    m_bbox.Reset();
    m_bbox.Union( v1 );
    m_bbox.Union( v2 );
    m_bbox.Union( v3 );
    m_bbox.Union( v4 );
    m_bbox.ScaleNextUp();
    m_bbox.ScaleNextUp();
    m_bbox.ScaleNextUp();
    m_bbox.ScaleNextUp();
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    wxASSERT( m_bbox.IsInitialized() );
}


bool POLYGON_4PT_2D::Intersects( const BBOX_2D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );
}


bool POLYGON_4PT_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // NOT IMPLEMENTED
    return true;
}


bool POLYGON_4PT_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    bool         hited = false;
    unsigned int hitIndex;
    float        bestHitT;

    for( unsigned int i = 0; i < 4; i++ )
    {
        float t;

        if( aSegRay.IntersectSegment( m_segments[i], m_precalc_slope[i], &t ) )
        {
            if( ( hited == false ) || ( t < bestHitT ) )
            {
                hited    = true;
                hitIndex = i;
                bestHitT = t;
            }
        }
    }

    if( hited )
    {
        wxASSERT( ( bestHitT >= 0.0f ) && ( bestHitT <= 1.0f ) );

        if( aOutT )
            *aOutT = bestHitT;

        if( aNormalOut )
            *aNormalOut = m_seg_normal[hitIndex];

        return true;
    }

    return false;
}


INTERSECTION_RESULT POLYGON_4PT_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{
    // !TODO:

    return INTERSECTION_RESULT::MISSES;
}


bool POLYGON_4PT_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    unsigned int i;
    unsigned int j        = 4 - 1;
    bool         oddNodes = false;

    for( i = 0; i < 4; j = i++ )
    {
        const float polyJY = m_segments[j].y;
        const float polyIY = m_segments[i].y;

        if( ( ( polyIY <= aPoint.y ) && ( polyJY >= aPoint.y ) )
          || ( ( polyJY <= aPoint.y ) && ( polyIY >= aPoint.y ) ) )
        {
            const float polyJX = m_segments[j].x;
            const float polyIX = m_segments[i].x;

            if( ( polyIX <= aPoint.x ) || ( polyJX <= aPoint.x ) )
            {
                oddNodes ^= ( ( polyIX + ( ( aPoint.y - polyIY ) / ( polyJY - polyIY ) )
                              * ( polyJX - polyIX ) )
                            < aPoint.x );
            }
        }
    }

    return oddNodes;
}
