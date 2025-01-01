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
 * @file  cylinder_3d.cpp
 */

#include "3d_fastmath.h"
#include "cylinder_3d.h"


CYLINDER::CYLINDER( SFVEC2F aCenterPoint, float aZmin, float aZmax, float aRadius )
        : OBJECT_3D( OBJECT_3D_TYPE::CYLINDER )
{
    m_center = aCenterPoint;
    m_radius_squared = aRadius * aRadius;
    m_inv_radius = 1.0f / aRadius;

    m_bbox.Set( SFVEC3F( aCenterPoint.x - aRadius, aCenterPoint.y - aRadius, aZmin ),
                SFVEC3F( aCenterPoint.x + aRadius, aCenterPoint.y + aRadius, aZmax ) );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();
}


bool CYLINDER::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    // Based on: http://www.cs.utah.edu/~lha/Code%206620%20/Ray4/Cylinder.cpp
    // Ray-sphere intersection: geometric
    const double OCx_Start = aRay.m_Origin.x - m_center.x;
    const double OCy_Start = aRay.m_Origin.y - m_center.y;

    const double p_dot_p = OCx_Start * OCx_Start + OCy_Start * OCy_Start;

    const double a = (double)aRay.m_Dir.x * (double)aRay.m_Dir.x +
                     (double)aRay.m_Dir.y * (double)aRay.m_Dir.y;
    const double b = (double)aRay.m_Dir.x * (double)OCx_Start +
                     (double)aRay.m_Dir.y * (double)OCy_Start;
    const double c = p_dot_p - m_radius_squared;

    const float delta = (float) ( b * b - a * c );

    bool hitResult = false;

    if( delta > FLT_EPSILON )
    {
        const float inv_a = 1.0 / a;

        const float sdelta = sqrtf( delta );
        const float t = (-b - sdelta) * inv_a;
        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aHitInfo.m_tHit )
            {
                hitResult = true;
                aHitInfo.m_tHit = t;
            }
        }

        if( !hitResult )
        {
            const float t1 = (-b + sdelta) * inv_a;
            const float z1 = aRay.m_Origin.z + t1 * aRay.m_Dir.z;

            if( ( z1 > m_bbox.Min().z ) && ( z1 < m_bbox.Max().z ) )
            {
                if( t1 < aHitInfo.m_tHit )
                {
                    hitResult = true;
                    aHitInfo.m_tHit = t1;
                }
            }
        }
    }

    if( hitResult )
    {
        aHitInfo.m_HitPoint = aRay.at( aHitInfo.m_tHit );

        const SFVEC2F hitPoint2D = SFVEC2F( aHitInfo.m_HitPoint.x, aHitInfo.m_HitPoint.y );

        aHitInfo.m_HitNormal = SFVEC3F( -( hitPoint2D.x - m_center.x ) * m_inv_radius,
                                        -( hitPoint2D.y - m_center.y ) * m_inv_radius, 0.0f );

        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

        aHitInfo.pHitObject = this;
    }

    return hitResult;
}


bool CYLINDER::IntersectP(const RAY& aRay , float aMaxDistance ) const
{
    // Based on: http://www.cs.utah.edu/~lha/Code%206620%20/Ray4/Cylinder.cpp
    // Ray-sphere intersection: geometric
    const double OCx_Start = aRay.m_Origin.x - m_center.x;
    const double OCy_Start = aRay.m_Origin.y - m_center.y;

    const double p_dot_p = OCx_Start * OCx_Start + OCy_Start * OCy_Start;

    const double a = (double)aRay.m_Dir.x * (double)aRay.m_Dir.x +
                     (double)aRay.m_Dir.y * (double)aRay.m_Dir.y;
    const double b = (double)aRay.m_Dir.x * (double)OCx_Start +
                     (double)aRay.m_Dir.y * (double)OCy_Start;
    const double c = p_dot_p - m_radius_squared;

    const float delta = (float) ( b * b - a * c );

    if( delta > FLT_EPSILON )
    {
        const float inv_a = 1.0 / a;

        const float sdelta = sqrtf( delta );
        const float t = ( -b - sdelta ) * inv_a;
        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aMaxDistance )
                return true;
        }

        const float t1 = ( -b + sdelta ) * inv_a;
        const float z1 = aRay.m_Origin.z + t1 * aRay.m_Dir.z;

        if( ( z1 > m_bbox.Min().z ) && ( z1 < m_bbox.Max().z ) )
        {
            if( t1 < aMaxDistance )
                return true;
        }
    }

    return false;
}


bool CYLINDER::Intersects( const BBOX_3D& aBBox ) const
{
    // !TODO: improve
    return m_bbox.Intersects( aBBox );
}


SFVEC3F CYLINDER::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
