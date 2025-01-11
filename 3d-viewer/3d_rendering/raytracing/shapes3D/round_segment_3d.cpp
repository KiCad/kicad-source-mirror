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
 * @file round_segment_3d.cpp
 */

#include "round_segment_3d.h"
#include "../shapes2D/round_segment_2d.h"


ROUND_SEGMENT::ROUND_SEGMENT( const ROUND_SEGMENT_2D& aSeg2D, float aZmin, float aZmax ) :
        OBJECT_3D( OBJECT_3D_TYPE::ROUNDSEG ),
        m_segment( aSeg2D.m_segment )
{
    m_radius = aSeg2D.GetRadius();
    m_radius_squared = m_radius * m_radius;
    m_inv_radius = 1.0f / m_radius;

    m_plane_dir_left  = SFVEC3F( -m_segment.m_Dir.y,  m_segment.m_Dir.x, 0.0f );
    m_plane_dir_right = SFVEC3F(  m_segment.m_Dir.y, -m_segment.m_Dir.x, 0.0f );

    m_bbox.Reset();

    m_bbox.Set( SFVEC3F( m_segment.m_Start.x, m_segment.m_Start.y, aZmin ),
                SFVEC3F( m_segment.m_End.x, m_segment.m_End.y, aZmax ) );

    m_bbox.Set( m_bbox.Min() - SFVEC3F( m_radius, m_radius, 0.0f ),
                m_bbox.Max() + SFVEC3F( m_radius, m_radius, 0.0f ) );

    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    m_center_left  = m_centroid + m_plane_dir_left  * m_radius;
    m_center_right = m_centroid + m_plane_dir_right * m_radius;

    m_seglen_over_two_squared = ( m_segment.m_Length / 2.0f ) * ( m_segment.m_Length / 2.0f );
}


bool ROUND_SEGMENT::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    // Top / Bottom plane
    float zPlanePos = aRay.m_dirIsNeg[2]? m_bbox.Max().z : m_bbox.Min().z;

    float tPlane = ( zPlanePos - aRay.m_Origin.z ) * aRay.m_InvDir.z;

    if( ( tPlane >= aHitInfo.m_tHit ) || ( tPlane < FLT_EPSILON ) )
        return false;   // Early exit

    SFVEC2F planeHitPoint2d( aRay.m_Origin.x + aRay.m_Dir.x * tPlane,
                             aRay.m_Origin.y + aRay.m_Dir.y * tPlane );

    float dSquared = m_segment.DistanceToPointSquared( planeHitPoint2d );

    if( dSquared <= m_radius_squared )
    {
        if( tPlane < aHitInfo.m_tHit )
        {
            aHitInfo.m_tHit = tPlane;
            aHitInfo.m_HitPoint = SFVEC3F( planeHitPoint2d.x, planeHitPoint2d.y,
                                           aRay.m_Origin.z + aRay.m_Dir.z * tPlane );
            aHitInfo.m_HitNormal = SFVEC3F( 0.0f, 0.0f, aRay.m_dirIsNeg[2] ? 1.0f : -1.0f );
            aHitInfo.pHitObject = this;

            m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

            return true;
        }

        return false;
    }

    // Test LEFT / RIGHT plane
    float normal_dot_ray = glm::dot( m_plane_dir_right, aRay.m_Dir );

    if( normal_dot_ray < 0.0f ) // If the dot is neg, the it hits the plane
    {
        const float n_dot_ray_origin = glm::dot( m_plane_dir_right,
                                                 m_center_right - aRay.m_Origin );
        const float t = n_dot_ray_origin / normal_dot_ray;

        if( t > 0.0f )
        {
            const SFVEC3F hitP = aRay.at( t );

            const SFVEC3F v = hitP - m_center_right;
            const float len = glm::dot( v, v );

            if( ( len <= m_seglen_over_two_squared ) && ( hitP.z >= m_bbox.Min().z )
              && ( hitP.z <= m_bbox.Max().z ) )
            {
                if( t < aHitInfo.m_tHit )
                {
                    aHitInfo.m_tHit = t;
                    aHitInfo.m_HitPoint = hitP;
                    aHitInfo.m_HitNormal = SFVEC3F( m_plane_dir_right.x, m_plane_dir_right.y,
                                                    0.0f );
                    aHitInfo.pHitObject = this;

                    m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                    return true;
                }

                return false;
            }
        }
    }
    else
    {
        normal_dot_ray = glm::dot( m_plane_dir_left, aRay.m_Dir );

        if( normal_dot_ray < 0.0f ) // If the dot is neg, the it hits the plane
        {
            const float n_dot_ray_origin = glm::dot( m_plane_dir_left,
                                                     m_center_left - aRay.m_Origin );
            const float t = n_dot_ray_origin / normal_dot_ray;

            if( t > 0.0f )
            {
                const SFVEC3F hitP = aRay.at( t );

                const SFVEC3F v = hitP - m_center_left;
                const float len = glm::dot( v, v );

                if( ( len <= m_seglen_over_two_squared ) && ( hitP.z >= m_bbox.Min().z )
                  && ( hitP.z <= m_bbox.Max().z ) )
                {
                    if( t < aHitInfo.m_tHit )
                    {
                        aHitInfo.m_tHit = t;
                        aHitInfo.m_HitPoint = hitP;
                        aHitInfo.m_HitNormal = SFVEC3F( m_plane_dir_left.x, m_plane_dir_left.y,
                                                        0.0f );
                        aHitInfo.pHitObject = this;

                        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                        return true;
                    }

                    return false;
                }
            }
        }
    }

    // Based on: http://www.cs.utah.edu/~lha/Code%206620%20/Ray4/Cylinder.cpp
    // Ray-sphere intersection: geometric
    const double OCx_Start = aRay.m_Origin.x - m_segment.m_Start.x;
    const double OCy_Start = aRay.m_Origin.y - m_segment.m_Start.y;

    const double p_dot_p_Start = OCx_Start * OCx_Start + OCy_Start * OCy_Start;

    const double a = (double)aRay.m_Dir.x * (double)aRay.m_Dir.x +
                     (double)aRay.m_Dir.y * (double)aRay.m_Dir.y;

    const double b_Start = (double)aRay.m_Dir.x * (double)OCx_Start +
                           (double)aRay.m_Dir.y * (double)OCy_Start;

    const double c_Start = p_dot_p_Start - m_radius_squared;

    const float delta_Start = (float) ( b_Start * b_Start - a * c_Start );

    if( delta_Start > FLT_EPSILON )
    {
        const float sdelta = sqrtf( delta_Start );
        const float t = ( -b_Start - sdelta ) / a;
        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aHitInfo.m_tHit )
            {
                aHitInfo.m_tHit = t;
                aHitInfo.m_HitPoint = aRay.at( t );

                const SFVEC2F hitPoint2D = SFVEC2F( aHitInfo.m_HitPoint.x, aHitInfo.m_HitPoint.y );

                aHitInfo.m_HitNormal =
                        SFVEC3F( ( hitPoint2D.x - m_segment.m_Start.x ) * m_inv_radius,
                                 ( hitPoint2D.y - m_segment.m_Start.y ) * m_inv_radius, 0.0f );

                aHitInfo.pHitObject = this;

                m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                return true;
            }

            return false;
        }
    }

    const double OCx_End = aRay.m_Origin.x - m_segment.m_End.x;
    const double OCy_End = aRay.m_Origin.y - m_segment.m_End.y;

    const double p_dot_p_End = OCx_End * OCx_End + OCy_End * OCy_End;

    const double b_End = (double)aRay.m_Dir.x * (double)OCx_End +
                         (double)aRay.m_Dir.y * (double)OCy_End;

    const double c_End = p_dot_p_End - m_radius_squared;

    const float delta_End = (float)(b_End * b_End - a * c_End);

    if( delta_End > FLT_EPSILON )
    {
        const float sdelta = sqrtf( delta_End );
        const float t = ( -b_End - sdelta ) / a;
        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aHitInfo.m_tHit )
            {
                aHitInfo.m_tHit = t;
                aHitInfo.m_HitPoint = aRay.at( t );

                const SFVEC2F hitPoint2D = SFVEC2F( aHitInfo.m_HitPoint.x, aHitInfo.m_HitPoint.y );

                aHitInfo.m_HitNormal =
                        SFVEC3F( ( hitPoint2D.x - m_segment.m_End.x ) * m_inv_radius,
                                 ( hitPoint2D.y - m_segment.m_End.y ) * m_inv_radius, 0.0f );
                aHitInfo.pHitObject = this;

                m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

                return true;
            }

            return false;
        }
    }

    return false;
}


bool ROUND_SEGMENT::IntersectP( const RAY& aRay, float aMaxDistance ) const
{
    // Top / Bottom plane
    const float zPlanePos = aRay.m_dirIsNeg[2]? m_bbox.Max().z : m_bbox.Min().z;

    const float tPlane = ( zPlanePos - aRay.m_Origin.z ) * aRay.m_InvDir.z;

    if( ( tPlane >= aMaxDistance) || ( tPlane < FLT_EPSILON ) )
        return false;   // Early exit

    const SFVEC2F planeHitPoint2d( aRay.m_Origin.x + aRay.m_Dir.x * tPlane,
                                   aRay.m_Origin.y + aRay.m_Dir.y * tPlane );

    const float dSquared = m_segment.DistanceToPointSquared( planeHitPoint2d );

    if( dSquared <= m_radius_squared )
    {
        if( tPlane < aMaxDistance )
            return true;

        return false;
    }

    // Since the IntersectP is used for shadows, we are simplifying the test
    // intersection and only consider the top/bottom plane of the segment
    return false;

    /// @todo Either fix the code below or get rid of it.
#if 0
    // Test LEFT / RIGHT plane
    float normal_dot_ray = glm::dot( m_plane_dir_right, aRay.m_Dir );

    if( normal_dot_ray < 0.0f ) // If the dot is neg, the it hits the plane
    {
        float n_dot_ray_origin = glm::dot( m_plane_dir_right, m_center_right - aRay.m_Origin );
        float t = n_dot_ray_origin / normal_dot_ray;

        if( t > 0.0f )
        {
            SFVEC3F hitP = aRay.at( t );

            SFVEC3F v = hitP - m_center_right;
            float len = glm::dot( v, v );

            if( ( len <= m_seglen_over_two_squared ) &&
                ( hitP.z >= m_bbox.Min().z ) && ( hitP.z <= m_bbox.Max().z ) )
            {
                if( t < aMaxDistance )
                    return true;

                return false;
            }
        }
    }
    else
    {
        normal_dot_ray = glm::dot( m_plane_dir_left, aRay.m_Dir );

        if( normal_dot_ray < 0.0f ) // If the dot is neg, the it hits the plane
        {
            const float n_dot_ray_origin = glm::dot( m_plane_dir_left,
                                                     m_center_left - aRay.m_Origin );
            const float t = n_dot_ray_origin / normal_dot_ray;

            if( t > 0.0f )
            {
                SFVEC3F hitP = aRay.at( t );

                SFVEC3F v = hitP - m_center_left;
                float len = glm::dot( v, v );

                if( ( len <= m_seglen_over_two_squared ) &&
                    ( hitP.z >= m_bbox.Min().z ) && ( hitP.z <= m_bbox.Max().z ) )
                {
                    if( t < aMaxDistance )
                        return true;

                    return false;
                }
            }
        }
    }

    // Based on: http://www.cs.utah.edu/~lha/Code%206620%20/Ray4/Cylinder.cpp
    // Ray-sphere intersection: geometric

    double OCx_Start = aRay.m_Origin.x - m_segment.m_Start.x;
    double OCy_Start = aRay.m_Origin.y - m_segment.m_Start.y;

    double p_dot_p_Start = OCx_Start * OCx_Start + OCy_Start * OCy_Start;

    double a = (double)aRay.m_Dir.x * (double)aRay.m_Dir.x +
               (double)aRay.m_Dir.y * (double)aRay.m_Dir.y;

    double b_Start = (double)aRay.m_Dir.x * (double)OCx_Start +
                     (double)aRay.m_Dir.y * (double)OCy_Start;

    double c_Start = p_dot_p_Start - m_radius_squared;

    float delta_Start = (float)( b_Start * b_Start - a * c_Start );

    if( delta_Start > FLT_EPSILON )
    {
        float sdelta = sqrtf( delta_Start );
        float t = ( -b_Start - sdelta ) / a;
        float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aMaxDistance )
                return true;

            return false;
        }
    }

    double OCx_End = aRay.m_Origin.x - m_segment.m_End.x;
    double OCy_End = aRay.m_Origin.y - m_segment.m_End.y;

    double p_dot_p_End = OCx_End * OCx_End + OCy_End * OCy_End;


    double b_End = (double)aRay.m_Dir.x * (double)OCx_End +
                   (double)aRay.m_Dir.y * (double)OCy_End;

    double c_End = p_dot_p_End - m_radius_squared;

    float delta_End = (float)(b_End * b_End - a * c_End);

    if( delta_End > FLT_EPSILON )
    {
        float sdelta = sqrtf( delta_End );
        float t = ( -b_End - sdelta ) / a;
        float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( ( z >= m_bbox.Min().z ) && ( z <= m_bbox.Max().z ) )
        {
            if( t < aMaxDistance )
                return true;

            return false;
        }
    }

    return false;
#endif
}


bool ROUND_SEGMENT::Intersects( const BBOX_3D& aBBox ) const
{
    //!TODO: improve
    return m_bbox.Intersects( aBBox );
}


SFVEC3F ROUND_SEGMENT::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
