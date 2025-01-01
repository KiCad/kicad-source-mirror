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
 * @file plane_3d.cpp
 */

#include "plane_3d.h"


XY_PLANE::XY_PLANE( const BBOX_3D& aBBox ) : OBJECT_3D( OBJECT_3D_TYPE::XYPLANE )
{
    m_centerPoint = aBBox.GetCenter();
    m_centroid = m_centerPoint;

    m_bbox.Reset();
    m_bbox.Set( aBBox );
    m_xsize = aBBox.GetExtent().x;
    m_ysize = aBBox.GetExtent().y;
    m_xsize_inv2 = 1.0f / ( 2.0f * m_xsize );
    m_ysize_inv2 = 1.0f / ( 2.0f * m_ysize );
}


XY_PLANE::XY_PLANE( SFVEC3F aCenterPoint, float aXSize, float aYSize )
        : OBJECT_3D( OBJECT_3D_TYPE::XYPLANE )
{
    m_centerPoint = aCenterPoint;
    m_xsize = aXSize;
    m_ysize = aYSize;
    m_xsize_inv2 = 1.0f / ( 2.0f * aXSize );
    m_ysize_inv2 = 1.0f / ( 2.0f * aYSize );
    m_bbox.Set( SFVEC3F( aCenterPoint.x - aXSize / 2.0f, aCenterPoint.y - aYSize / 2.0f,
                         aCenterPoint.z ),
                SFVEC3F( aCenterPoint.x + aXSize / 2.0f, aCenterPoint.y + aYSize / 2.0f,
                         aCenterPoint.z ) );
    m_centroid = aCenterPoint;
}


bool XY_PLANE::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    const float t = ( m_centerPoint.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

    if( ( t < FLT_EPSILON ) || ( t >= aHitInfo.m_tHit ) )
        return false;

    const float vSU = t * aRay.m_Dir.x + aRay.m_Origin.x - m_centerPoint.x;

    if( (vSU < -m_xsize) || (vSU > m_xsize) )
        return false;

    const float vSV = t * aRay.m_Dir.y + aRay.m_Origin.y - m_centerPoint.y;

    if( ( vSV < -m_ysize ) || ( vSV > m_ysize ) )
        return false;

    aHitInfo.m_tHit = t;
    aHitInfo.m_HitPoint = aRay.at( t );
    aHitInfo.pHitObject = this;

    if( aRay.m_dirIsNeg[2] )
        aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, 1.0f );
    else
        aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f,-1.0f );

    m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

    return true;
}


bool XY_PLANE::IntersectP(const RAY& aRay, float aMaxDistance ) const
{
    const float t = ( m_centerPoint.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

    if( ( t < FLT_EPSILON ) || ( t >= aMaxDistance ) )
        return false;

    const float vSU = t * aRay.m_Dir.x + aRay.m_Origin.x - m_centerPoint.x;

    if( ( vSU < -m_xsize ) || ( vSU > m_xsize ) )
        return false;

    const float vSV = t * aRay.m_Dir.y + aRay.m_Origin.y - m_centerPoint.y;

    if( ( vSV < -m_ysize ) || ( vSV > m_ysize ) )
        return false;

    return true;
}


bool XY_PLANE::Intersects( const BBOX_3D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );
}


SFVEC3F XY_PLANE::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
