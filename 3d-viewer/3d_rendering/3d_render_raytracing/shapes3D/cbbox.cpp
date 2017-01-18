/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cbbox.cpp
 * @brief Bounding Box class implementation
 */
#include "3d_fastmath.h"

#include "cbbox.h"
#include <stdio.h>
#include <wx/debug.h>   // For the wxASSERT


CBBOX::CBBOX()
{
    Reset();
}


CBBOX::CBBOX( const SFVEC3F &aPbInit )
{
    m_min = aPbInit;
    m_max = aPbInit;
}


CBBOX::CBBOX( const SFVEC3F &aPbMin, const SFVEC3F &aPbMax )
{
    Set( aPbMin, aPbMax );
}


CBBOX::~CBBOX()
{
}


void CBBOX::Set( const SFVEC3F &aPoint )
{
    m_min = aPoint;
    m_max = aPoint;
}


void CBBOX::Set( const SFVEC3F &aPbMin, const SFVEC3F &aPbMax )
{
    m_min.x =  fminf( aPbMin.x, aPbMax.x );
    m_min.y =  fminf( aPbMin.y, aPbMax.y );
    m_min.z =  fminf( aPbMin.z, aPbMax.z );

    m_max.x =  fmaxf( aPbMin.x, aPbMax.x );
    m_max.y =  fmaxf( aPbMin.y, aPbMax.y );
    m_max.z =  fmaxf( aPbMin.z, aPbMax.z );
}


void CBBOX::Set( const CBBOX &aBBox )
{
    wxASSERT( aBBox.IsInitialized() );

    Set( aBBox.Min(), aBBox.Max() );
}


bool CBBOX::IsInitialized() const
{
    return !( ( FLT_MAX == m_min.x) ||
              ( FLT_MAX == m_min.y) ||
              ( FLT_MAX == m_min.z) ||
              (-FLT_MAX == m_max.x) ||
              (-FLT_MAX == m_max.y) ||
              (-FLT_MAX == m_max.z) );
}


void CBBOX::Reset()
{
    m_min = SFVEC3F( FLT_MAX, FLT_MAX, FLT_MAX );
    m_max = SFVEC3F(-FLT_MAX,-FLT_MAX,-FLT_MAX );
}


void CBBOX::Union( const SFVEC3F &aPoint )
{
    // get the minimun value between the added point and the existent bounding box
    m_min.x =  fminf( m_min.x, aPoint.x );
    m_min.y =  fminf( m_min.y, aPoint.y );
    m_min.z =  fminf( m_min.z, aPoint.z );

    // get the maximun value between the added point and the existent bounding box
    m_max.x =  fmaxf( m_max.x, aPoint.x );
    m_max.y =  fmaxf( m_max.y, aPoint.y );
    m_max.z =  fmaxf( m_max.z, aPoint.z );
}


void CBBOX::Union( const CBBOX &aBBox )
{
    wxASSERT( aBBox.IsInitialized() );

    // get the minimun value between the added bounding box and the existent bounding box
    m_min.x =  fmin( m_min.x, aBBox.m_min.x );
    m_min.y =  fmin( m_min.y, aBBox.m_min.y );
    m_min.z =  fmin( m_min.z, aBBox.m_min.z );

    // get the maximun value between the added bounding box and the existent bounding box
    m_max.x =  fmax( m_max.x, aBBox.m_max.x );
    m_max.y =  fmax( m_max.y, aBBox.m_max.y );
    m_max.z =  fmax( m_max.z, aBBox.m_max.z );
}


SFVEC3F CBBOX::GetCenter() const
{
    return (m_max + m_min) * 0.5f;
}


float CBBOX::GetCenter( unsigned int aAxis ) const
{
    wxASSERT( aAxis < 3 );
    return (m_max[aAxis] + m_min[aAxis]) * 0.5f;
}


const SFVEC3F CBBOX::GetExtent() const
{
    return m_max - m_min;
}


unsigned int CBBOX::MaxDimension() const
{
    unsigned int result = 0;

    SFVEC3F extent = GetExtent();

    if( extent.y > extent.x )
        result = 1;
    if( extent.z > extent.y )
        result = 2;

    return result;
}


float CBBOX::GetMaxDimension() const
{
    unsigned int max_dimensions_idx = 0;

    SFVEC3F extent = GetExtent();

    if( extent.y > extent.x )
        max_dimensions_idx = 1;
    if( extent.z > extent.y )
        max_dimensions_idx = 2;

    return extent[max_dimensions_idx];
}


float CBBOX::SurfaceArea() const
{
    SFVEC3F extent = GetExtent();

    return 2.0f * ( extent.x * extent.z +
                    extent.x * extent.y +
                    extent.y * extent.z );
}


void CBBOX::Scale( float aScale )
{
    wxASSERT( IsInitialized() );

    SFVEC3F scaleV  = SFVEC3F( aScale, aScale, aScale );
    SFVEC3F centerV = GetCenter();

    m_min = (m_min - centerV) * scaleV + centerV;
    m_max = (m_max - centerV) * scaleV + centerV;
}


void CBBOX::ScaleNextUp()
{
    m_min.x = NextFloatDown( m_min.x );
    m_min.y = NextFloatDown( m_min.y );
    m_min.z = NextFloatDown( m_min.z );

    m_max.x = NextFloatUp( m_max.x );
    m_max.y = NextFloatUp( m_max.y );
    m_max.z = NextFloatUp( m_max.z );
}


void CBBOX::ScaleNextDown()
{
    m_min.x = NextFloatUp( m_min.x );
    m_min.y = NextFloatUp( m_min.y );
    m_min.z = NextFloatUp( m_min.z );

    m_max.x = NextFloatDown( m_max.x );
    m_max.y = NextFloatDown( m_max.y );
    m_max.z = NextFloatDown( m_max.z );
}


bool CBBOX::Intersects( const CBBOX &aBBox ) const
{
    wxASSERT( IsInitialized() );
    wxASSERT( aBBox.IsInitialized() );

    bool x = ( m_max.x >= aBBox.m_min.x ) && ( m_min.x <= aBBox.m_max.x );
    bool y = ( m_max.y >= aBBox.m_min.y ) && ( m_min.y <= aBBox.m_max.y );
    bool z = ( m_max.z >= aBBox.m_min.z ) && ( m_min.z <= aBBox.m_max.z );

    return ( x && y && z );
}


bool CBBOX::Inside( const SFVEC3F &aPoint ) const
{
    wxASSERT( IsInitialized() );

    return (( aPoint.x >= m_min.x ) && ( aPoint.x <= m_max.x ) &&
            ( aPoint.y >= m_min.y ) && ( aPoint.y <= m_max.y ) &&
            ( aPoint.z >= m_min.z ) && ( aPoint.z <= m_max.z ));
}


float CBBOX::Volume() const
{
    wxASSERT( IsInitialized() );

    SFVEC3F extent = GetExtent();

    return extent.x * extent.y * extent.z;
}


SFVEC3F CBBOX::Offset( const SFVEC3F &p ) const
{
    return (p - m_min) / (m_max - m_min);
}


// Intersection code based on the book:
// "Physical Based Ray Tracing" (by Matt Pharr and Greg Humphrey)
// https://github.com/mmp/pbrt-v2/blob/master/src/core/geometry.cpp#L68
// /////////////////////////////////////////////////////////////////////////
#if 0
bool CBBOX::Intersect( const RAY &aRay, float *aOutHitt0, float *aOutHitt1 )
{
    float t0 = 0.0f;
    float t1 = FLT_MAX;

    for( unsigned int i = 0; i < 3; ++i )
    {
        // Update interval for _i_th bounding box slab
        float tNear = (m_min[i] - aRay.m_Origin[i]) * aRay.m_InvDir[i];
        float tFar  = (m_max[i] - aRay.m_Origin[i]) * aRay.m_InvDir[i];

        // Update parametric interval from slab intersection
        if( tNear > tFar )
        {
            // Swap
            float ftemp = tNear;
            tNear = tFar;
            tFar = ftemp;
        }

        t0 = tNear > t0 ? tNear : t0;
        t1 = tFar  < t1 ? tFar  : t1;

        if( t0 > t1 )
            return false;
    }

    if( aOutHitt0 )
        *aOutHitt0 = t0;
    if( aOutHitt1 )
        *aOutHitt1 = t1;

    return true;
}
#else
// https://github.com/mmp/pbrt-v2/blob/master/src/accelerators/bvh.cpp#L126
bool CBBOX::Intersect( const RAY &aRay,
                       float *aOutHitt0,
                       float *aOutHitt1 ) const
{
    wxASSERT( aOutHitt0 );
    wxASSERT( aOutHitt1 );

    const SFVEC3F bounds[2] = {m_min, m_max};

    // Check for ray intersection against x and y slabs
          float tmin  = (bounds[    aRay.m_dirIsNeg[0]].x - aRay.m_Origin.x) * aRay.m_InvDir.x;
          float tmax  = (bounds[1 - aRay.m_dirIsNeg[0]].x - aRay.m_Origin.x) * aRay.m_InvDir.x;
    const float tymin = (bounds[    aRay.m_dirIsNeg[1]].y - aRay.m_Origin.y) * aRay.m_InvDir.y;
    const float tymax = (bounds[1 - aRay.m_dirIsNeg[1]].y - aRay.m_Origin.y) * aRay.m_InvDir.y;

    if( (tmin > tymax) || (tymin > tmax) )
        return false;

    tmin = (tymin > tmin)? tymin : tmin;
    tmax = (tymax < tmax)? tymax : tmax;

    // Check for ray intersection against z slab
    const float tzmin = (bounds[    aRay.m_dirIsNeg[2]].z - aRay.m_Origin.z) * aRay.m_InvDir.z;
    const float tzmax = (bounds[1 - aRay.m_dirIsNeg[2]].z - aRay.m_Origin.z) * aRay.m_InvDir.z;

    if( (tmin > tzmax) || (tzmin > tmax) )
        return false;

    tmin = (tzmin > tmin)? tzmin : tmin;
    tmin = ( tmin < 0.0f)? 0.0f  : tmin;

    tmax = (tzmax < tmax)? tzmax : tmax;

    *aOutHitt0 = tmin;
    *aOutHitt1 = tmax;

    return true;
}
#endif


void CBBOX::ApplyTransformation( glm::mat4 aTransformMatrix )
{
    wxASSERT( IsInitialized() );

    const SFVEC3F v1 = SFVEC3F( aTransformMatrix *
                                glm::vec4( m_min.x, m_min.y, m_min.z, 1.0f ) );

    const SFVEC3F v2 = SFVEC3F( aTransformMatrix *
                                glm::vec4( m_max.x, m_max.y, m_max.z, 1.0f ) );

    Reset();
    Union( v1 );
    Union( v2 );
}


void CBBOX::ApplyTransformationAA( glm::mat4 aTransformMatrix )
{
    wxASSERT( IsInitialized() );

    // apply the transformation matrix for each of vertices of the bounding box
    // and make a union with all vertices
    CBBOX tmpBBox = CBBOX(
                   SFVEC3F( aTransformMatrix *
                            glm::vec4( m_min.x, m_min.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_max.x, m_min.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_min.x, m_max.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_min.x, m_min.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_min.x, m_max.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_max.x, m_max.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_max.x, m_min.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union( SFVEC3F( aTransformMatrix *
                            glm::vec4( m_max.x, m_max.y, m_max.z, 1.0f ) ) );

    m_min = tmpBBox.m_min;
    m_max = tmpBBox.m_max;
}


void CBBOX::debug() const
{
    printf( "min(%f, %f, %f) - max(%f, %f, %f)\n", m_min.x, m_min.y, m_min.z,
                                                   m_max.x, m_max.y, m_max.z );
}
