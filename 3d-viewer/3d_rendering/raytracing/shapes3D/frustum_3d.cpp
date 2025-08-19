/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file frustum_3d.cpp
 * @brief Implementation of truncated cone ray intersection.
 *
 * The truncated cone is a vertical cone truncated at zMin and zMax with different radii.
 * Ray-cone intersection uses the implicit equation of a cone.
 */

#include "3d_fastmath.h"
#include "frustum_3d.h"
#include <cmath>


TRUNCATED_CONE::TRUNCATED_CONE( SFVEC2F aCenterPoint, float aZmin, float aZmax, float aRadiusMin,
                                float aRadiusMax )
        : OBJECT_3D( OBJECT_3D_TYPE::CYLINDER ) // Reuse CYLINDER type for now
{
    m_center = aCenterPoint;
    m_zMin = aZmin;
    m_zMax = aZmax;
    m_radiusMin = aRadiusMin;
    m_radiusMax = aRadiusMax;
    m_height = aZmax - aZmin;
    m_slope = ( m_height > 0.0f ) ? ( aRadiusMax - aRadiusMin ) / m_height : 0.0f;

    // Bounding box uses the larger radius
    const float maxRadius = std::max( aRadiusMin, aRadiusMax );
    m_bbox.Set( SFVEC3F( aCenterPoint.x - maxRadius, aCenterPoint.y - maxRadius, aZmin ),
                SFVEC3F( aCenterPoint.x + maxRadius, aCenterPoint.y + maxRadius, aZmax ) );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();
}


bool TRUNCATED_CONE::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    // Ray-cone intersection for a vertical cone with apex on the Z axis.
    // The cone surface is defined by: (x-cx)^2 + (y-cy)^2 = r(z)^2
    // where r(z) = radiusMin + slope * (z - zMin)
    //
    // Substituting the ray equation P = O + t*D:
    // (Ox + t*Dx - cx)^2 + (Oy + t*Dy - cy)^2 = (radiusMin + slope*(Oz + t*Dz - zMin))^2

    const double OCx = aRay.m_Origin.x - m_center.x;
    const double OCy = aRay.m_Origin.y - m_center.y;
    const double OCz = aRay.m_Origin.z - m_zMin;  // Z relative to cone base

    const double Dx = aRay.m_Dir.x;
    const double Dy = aRay.m_Dir.y;
    const double Dz = aRay.m_Dir.z;

    const double r0 = m_radiusMin;
    const double k = m_slope;  // dr/dz

    // Expanding the equation gives us a quadratic: a*t^2 + 2*b*t + c = 0
    // Note: we use 2*b to simplify the quadratic formula
    const double a = Dx * Dx + Dy * Dy - k * k * Dz * Dz;
    const double b = OCx * Dx + OCy * Dy - k * ( r0 + k * OCz ) * Dz;
    const double c = OCx * OCx + OCy * OCy - ( r0 + k * OCz ) * ( r0 + k * OCz );

    const double discriminant = b * b - a * c;

    if( discriminant < 0.0 )
        return false;

    bool hitResult = false;
    float tHit = aHitInfo.m_tHit;
    SFVEC3F hitNormal;

    const double sqrtDisc = std::sqrt( discriminant );

    // Check both roots
    for( int i = 0; i < 2; ++i )
    {
        double t;
        if( std::abs( a ) < 1e-10 )
        {
            // Nearly linear case
            if( std::abs( b ) < 1e-10 )
                continue;
            t = -c / ( 2.0 * b );
            if( i == 1 )
                continue;  // Only one solution in linear case
        }
        else
        {
            t = ( -b + ( i == 0 ? -sqrtDisc : sqrtDisc ) ) / a;
        }

        if( t <= 0.0f || t >= tHit )
            continue;

        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( z < m_zMin || z > m_zMax )
            continue;

        // Valid hit on cone surface
        tHit = t;
        hitResult = true;

        const SFVEC3F hitPoint = aRay.at( t );
        const float hitRadius = m_radiusMin + m_slope * ( z - m_zMin );

        if( hitRadius > 1e-6f )
        {
            // Normal points outward from cone surface
            // For a cone, the normal has both radial and vertical components
            const float invR = 1.0f / hitRadius;
            const float nx = -( hitPoint.x - m_center.x ) * invR;
            const float ny = -( hitPoint.y - m_center.y ) * invR;
            // The vertical component depends on the slope
            const float nz = m_slope / std::sqrt( 1.0f + m_slope * m_slope );

            hitNormal = glm::normalize( SFVEC3F( nx, ny, nz ) );
        }
        else
        {
            hitNormal = SFVEC3F( 0.0f, 0.0f, -1.0f );
        }

        break;  // Take the first valid hit (closer one)
    }

    if( hitResult )
    {
        aHitInfo.m_tHit = tHit;
        aHitInfo.m_HitPoint = aRay.at( tHit );
        aHitInfo.m_HitNormal = hitNormal;

        m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

        aHitInfo.pHitObject = this;
    }

    return hitResult;
}


bool TRUNCATED_CONE::IntersectP( const RAY& aRay, float aMaxDistance ) const
{
    const double OCx = aRay.m_Origin.x - m_center.x;
    const double OCy = aRay.m_Origin.y - m_center.y;
    const double OCz = aRay.m_Origin.z - m_zMin;

    const double Dx = aRay.m_Dir.x;
    const double Dy = aRay.m_Dir.y;
    const double Dz = aRay.m_Dir.z;

    const double r0 = m_radiusMin;
    const double k = m_slope;

    const double a = Dx * Dx + Dy * Dy - k * k * Dz * Dz;
    const double b = OCx * Dx + OCy * Dy - k * ( r0 + k * OCz ) * Dz;
    const double c = OCx * OCx + OCy * OCy - ( r0 + k * OCz ) * ( r0 + k * OCz );

    const double discriminant = b * b - a * c;

    if( discriminant < 0.0 )
        return false;

    const double sqrtDisc = std::sqrt( discriminant );

    for( int i = 0; i < 2; ++i )
    {
        double t;
        if( std::abs( a ) < 1e-10 )
        {
            if( std::abs( b ) < 1e-10 )
                continue;
            t = -c / ( 2.0 * b );
            if( i == 1 )
                continue;
        }
        else
        {
            t = ( -b + ( i == 0 ? -sqrtDisc : sqrtDisc ) ) / a;
        }

        if( t <= 0.0f || t >= aMaxDistance )
            continue;

        const float z = aRay.m_Origin.z + t * aRay.m_Dir.z;

        if( z >= m_zMin && z <= m_zMax )
            return true;
    }

    return false;
}


bool TRUNCATED_CONE::Intersects( const BBOX_3D& aBBox ) const
{
    // Simple bounding box check
    return m_bbox.Intersects( aBBox );
}


SFVEC3F TRUNCATED_CONE::GetDiffuseColor( const HITINFO& /* aHitInfo */ ) const
{
    return m_diffusecolor;
}
