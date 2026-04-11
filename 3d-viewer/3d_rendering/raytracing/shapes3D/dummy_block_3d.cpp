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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file dummy_block_3d.cpp
 * @brief
 */

#include "dummy_block_3d.h"


DUMMY_BLOCK::DUMMY_BLOCK( const BBOX_3D& aBBox ) : OBJECT_3D( OBJECT_3D_TYPE::DUMMYBLOCK )
{
    m_centroid = aBBox.GetCenter();
    m_bbox.Reset();
    m_bbox.Set( aBBox );
}


bool DUMMY_BLOCK::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    float t;

    if( !m_bbox.Intersect( aRay, &t ) )
        return false;

    if( t < aHitInfo.m_tHit )
    {
        aHitInfo.m_tHit = t;
        aHitInfo.m_HitPoint = aRay.at( t );

        // Determine which face was hit by checking which box face
        // the hit point is closest to
        const SFVEC3F hitPt = aHitInfo.m_HitPoint;
        const float   eps = 0.0001f;

        if( std::abs( hitPt.x - m_bbox.Min().x ) < eps )
            aHitInfo.m_HitNormal = SFVEC3F( -1.0f, 0.0f, 0.0f );
        else if( std::abs( hitPt.x - m_bbox.Max().x ) < eps )
            aHitInfo.m_HitNormal = SFVEC3F( 1.0f, 0.0f, 0.0f );
        else if( std::abs( hitPt.y - m_bbox.Min().y ) < eps )
            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, -1.0f, 0.0f );
        else if( std::abs( hitPt.y - m_bbox.Max().y ) < eps )
            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 1.0f, 0.0f );
        else if( aRay.m_dirIsNeg[2] )
            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, 1.0f );
        else
            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, -1.0f );

        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

        aHitInfo.pHitObject = this;

        return true;
    }

    return false;
}


bool DUMMY_BLOCK::IntersectP(const RAY& aRay, float aMaxDistance ) const
{
    float t;

    if( !m_bbox.Intersect( aRay, &t ) )
        return false;

    if( t < aMaxDistance )
        return true;

    return false;
}


bool DUMMY_BLOCK::Intersects( const BBOX_3D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );
}


SFVEC3F DUMMY_BLOCK::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
