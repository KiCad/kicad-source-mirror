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
 * @file  cpolygon4pts2d.cpp
 * @brief
 */

#include "cpolygon4pts2d.h"
#include <wx/debug.h>


CPOLYGON4PTS2D::CPOLYGON4PTS2D( const SFVEC2F &v1,
                                const SFVEC2F &v2,
                                const SFVEC2F &v3,
                                const SFVEC2F &v4,
                                const BOARD_ITEM &aBoardItem ) :
                COBJECT2D( OBJ2D_POLYGON4PT, aBoardItem )
{/*
    if( (v1.x > v2.x) || (v1.y < v2.y) )
    {
        m_segments[0] = v4;
        m_segments[1] = v3;
        m_segments[2] = v2;
        m_segments[3] = v1;
    }
    else
    {*/
        m_segments[0] = v1;
        m_segments[1] = v4;
        m_segments[2] = v3;
        m_segments[3] = v2;
   // }

    unsigned int i;
    unsigned int j = 4 - 1;

    for( i = 0; i < 4; j = i++ )
    {
        SFVEC2F slope = m_segments[j] - m_segments[i];
        m_precalc_slope[i] = slope;
        m_seg_normal[i] = glm::normalize( SFVEC2F( -slope.y, +slope.x ) );
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


bool CPOLYGON4PTS2D::Intersects( const CBBOX2D &aBBox ) const
{
    return m_bbox.Intersects( aBBox );

    // This source code is not working OK.
    /*
    if( !m_bbox.Intersects( aBBox ) )
        return false;

    // Check if the bouding box complety have inside the small bouding box
    if( (aBBox.Max().x > m_bbox.Max().x) &&
        (aBBox.Max().y > m_bbox.Max().x) &&
        (aBBox.Min().x < m_bbox.Min().x) &&
        (aBBox.Min().y < m_bbox.Min().y)
        )
        return true;

    SFVEC2F v[4];

    v[0] = aBBox.Min();
    v[1] = SFVEC2F( aBBox.Min().x, aBBox.Max().y );
    v[2] = aBBox.Max();
    v[3] = SFVEC2F( aBBox.Max().x, aBBox.Min().y );

    for( unsigned int i = 0; i < 4; i++ )
    {
        if( IntersectSegment( m_segments[i], m_precalc_slope[i], v[0], v[1] - v[0] ) )
            return true;
        if( IntersectSegment( m_segments[i], m_precalc_slope[i], v[1], v[2] - v[1] ) )
            return true;
        if( IntersectSegment( m_segments[i], m_precalc_slope[i], v[2], v[3] - v[2] ) )
            return true;
        if( IntersectSegment( m_segments[i], m_precalc_slope[i], v[3], v[0] - v[3] ) )
            return true;
    }

    if( IsPointInside( v[0] ) )
        return true;
    if( IsPointInside( v[1] ) )
        return true;
    if( IsPointInside( v[2] ) )
        return true;
    if( IsPointInside( v[3] ) )
        return true;

    return false;*/
}


bool CPOLYGON4PTS2D::Overlaps( const CBBOX2D &aBBox ) const
{
    // NOT IMPLEMENTED
    return true;
}


bool CPOLYGON4PTS2D::Intersect( const RAYSEG2D &aSegRay,
                                float *aOutT,
                                SFVEC2F *aNormalOut ) const
{
    wxASSERT( aOutT );
    wxASSERT( aNormalOut );

    bool  hited = false;
    unsigned int hitIndex;
    float bestHitT;

    for( unsigned int i = 0; i < 4; i++ )
    {
        float t;

        if( aSegRay.IntersectSegment( m_segments[i], m_precalc_slope[i], &t ) )
            if( (hited == false) || ( t < bestHitT) )
            {
                hited = true;
                hitIndex = i;
                bestHitT = t;
            }
    }

    if( hited )
    {
        wxASSERT( (bestHitT >= 0.0f) && (bestHitT <= 1.0f) );

        *aOutT = bestHitT;
        *aNormalOut = m_seg_normal[hitIndex];

        return true;
    }

    return false;
}


INTERSECTION_RESULT CPOLYGON4PTS2D::IsBBoxInside( const CBBOX2D &aBBox ) const
{
    // !TODO:

    return INTR_MISSES;
}


bool CPOLYGON4PTS2D::IsPointInside( const SFVEC2F &aPoint ) const
{
    unsigned int i;
    unsigned int j = 4 - 1;
    bool  oddNodes = false;

    for( i = 0; i < 4; j = i++ )
    {
        const float polyJY = m_segments[j].y;
        const float polyIY = m_segments[i].y;

        if( ((polyIY <= aPoint.y) && (polyJY >= aPoint.y)) ||
            ((polyJY <= aPoint.y) && (polyIY >= aPoint.y))
          )
        {
            const float polyJX = m_segments[j].x;
            const float polyIX = m_segments[i].x;

            if( (polyIX <= aPoint.x) || (polyJX <= aPoint.x) )
            {
                oddNodes ^= ( ( polyIX +
                                ( ( aPoint.y - polyIY ) / ( polyJY - polyIY ) ) *
                                ( polyJX - polyIX ) ) < aPoint.x );
            }
        }
    }

    return oddNodes;
}
