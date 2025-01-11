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
 * @file bbox_2d.cpp
 * @brief 2D bounding box class definition.
 */

#include "3d_fastmath.h"

#include "bbox_2d.h"
#include "../ray.h"
#include <wx/debug.h>


BBOX_2D::BBOX_2D()
{
    Reset();
}


BBOX_2D::BBOX_2D( const SFVEC2F& aPbInit )
{
    m_min = aPbInit;
    m_max = aPbInit;
}


BBOX_2D::BBOX_2D( const SFVEC2F& aPbMin, const SFVEC2F& aPbMax )
{
    Set( aPbMin, aPbMax );
}


BBOX_2D::~BBOX_2D()
{
}


void BBOX_2D::Set( const SFVEC2F& aPbMin, const SFVEC2F& aPbMax )
{
    m_min.x =  fminf( aPbMin.x, aPbMax.x );
    m_min.y =  fminf( aPbMin.y, aPbMax.y );

    m_max.x =  fmaxf( aPbMin.x, aPbMax.x );
    m_max.y =  fmaxf( aPbMin.y, aPbMax.y );
}


void BBOX_2D::Set( const BBOX_2D& aBBox )
{
    wxASSERT( aBBox.IsInitialized() );

    Set( aBBox.Min(), aBBox.Max() );
}


bool BBOX_2D::IsInitialized() const
{
    return !( ( FLT_MAX == m_min.x ) || ( FLT_MAX == m_min.y ) || ( -FLT_MAX == m_max.x )
            || ( -FLT_MAX == m_max.y ) );
}


void BBOX_2D::Reset()
{
    m_min = SFVEC2F( FLT_MAX, FLT_MAX );
    m_max = SFVEC2F( -FLT_MAX,-FLT_MAX );
}


void BBOX_2D::Union( const SFVEC2F& aPoint )
{
    // get the minimum value between the added point and the existent bounding box
    m_min.x =  fminf( m_min.x, aPoint.x );
    m_min.y =  fminf( m_min.y, aPoint.y );

    // get the maximum value between the added point and the existent bounding box
    m_max.x =  fmaxf( m_max.x, aPoint.x );
    m_max.y =  fmaxf( m_max.y, aPoint.y );
}


void BBOX_2D::Union( const BBOX_2D& aBBox )
{
    // get the minimum value between the added bounding box and
    // the existent bounding box
    m_min.x =  fminf( m_min.x, aBBox.m_min.x );
    m_min.y =  fminf( m_min.y, aBBox.m_min.y );

    // get the maximum value between the added bounding box and
    // the existent bounding box
    m_max.x =  fmaxf( m_max.x, aBBox.m_max.x );
    m_max.y =  fmaxf( m_max.y, aBBox.m_max.y );
}


SFVEC2F BBOX_2D::GetCenter() const
{
    return ( m_max + m_min ) * 0.5f;
}


SFVEC2F BBOX_2D::GetExtent() const
{
    return m_max - m_min;
}


unsigned int BBOX_2D::MaxDimension() const
{
    unsigned int result = 0;
    const SFVEC2F extent = GetExtent();

    if( extent.y > extent.x ) result = 1;

    return result;
}


float BBOX_2D::Perimeter() const
{
    const SFVEC2F extent = GetExtent();

    return 2.0f * ( extent.x + extent.y );
}


void BBOX_2D::Scale( float aScale )
{
    wxASSERT( IsInitialized() );

    const SFVEC2F scaleV( aScale, aScale );
    const SFVEC2F centerV = GetCenter();

    m_min = ( m_min - centerV ) * scaleV + centerV;
    m_max = ( m_max - centerV ) * scaleV + centerV;
}


void BBOX_2D::ScaleNextUp()
{
    m_min.x = NextFloatDown( m_min.x );
    m_min.y = NextFloatDown( m_min.y );

    m_max.x = NextFloatUp( m_max.x );
    m_max.y = NextFloatUp( m_max.y );
}


void BBOX_2D::ScaleNextDown()
{
    m_min.x = NextFloatUp( m_min.x );
    m_min.y = NextFloatUp( m_min.y );

    m_max.x = NextFloatDown( m_max.x );
    m_max.y = NextFloatDown( m_max.y );
}


// http://goanna.cs.rmit.edu.au/~gl/teaching/rtr&3dgp/notes/intersection.pdf
// http://www.mrtc.mdh.se/projects/3Dgraphics/paperF.pdf
bool BBOX_2D::Intersects( const SFVEC2F& aCenter, float aRadiusSquared ) const
{
    float fDistSq = 0.0f;

    for( unsigned int i = 0; i < 2; i++ )
    {
        if( aCenter[i] < m_min[i] )
        {
            const float fDist = aCenter[i] - m_min[i];

            fDistSq += fDist * fDist;
        }
        else
        {
            if( aCenter[i] > m_max[i] )
            {
                const float fDist = aCenter[i] - m_max[i];

                fDistSq += fDist * fDist;
            }
        }
    }

    return ( fDistSq <= aRadiusSquared );
}


bool BBOX_2D::Intersects( const BBOX_2D& aBBox ) const
{
    wxASSERT( IsInitialized() );
    wxASSERT( aBBox.IsInitialized() );

    const bool x = ( m_max.x >= aBBox.m_min.x ) && ( m_min.x <= aBBox.m_max.x );
    const bool y = ( m_max.y >= aBBox.m_min.y ) && ( m_min.y <= aBBox.m_max.y );

    return ( x && y );
}


bool BBOX_2D::Inside( const SFVEC2F& aPoint ) const
{
    wxASSERT( IsInitialized() );

    return ( ( aPoint.x >= m_min.x ) && ( aPoint.x <= m_max.x ) &&
             ( aPoint.y >= m_min.y ) && ( aPoint.y <= m_max.y ) );
}


bool BBOX_2D::Inside( const BBOX_2D& aBBox ) const
{
    wxASSERT( IsInitialized() );
    wxASSERT( aBBox.IsInitialized() );

    return Inside( aBBox.Min() ) &&
           Inside( aBBox.Max() );
}


float BBOX_2D::Area() const
{
    SFVEC2F extent = GetExtent();
    return extent.x * extent.y;
}


// http://tavianator.com/fast-branchless-raybounding-box-intersections/
bool BBOX_2D::Intersect( const RAY2D& aRay, float* t ) const
{
    wxASSERT( t );

    const float tx1 = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;
    const float tx2 = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

    float tmin = glm::min( tx1, tx2 );
    float tmax = glm::max( tx1, tx2 );

    const float ty1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;
    const float ty2 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

    tmin = glm::max( tmin, glm::min( ty1, ty2 ) );
    tmax = glm::min( tmax, glm::max( ty1, ty2 ) );

    if( tmin > 0.0f )
        *t = tmin;
    else
        *t = tmax;

    return ( tmax >= 0.0f ) && ( tmax >= tmin );
}


bool BBOX_2D::Intersect( const RAYSEG2D& aRaySeg ) const
{
    const float tx1 = ( m_min.x - aRaySeg.m_Start.x ) * aRaySeg.m_InvDir.x;
    const float tx2 = ( m_max.x - aRaySeg.m_Start.x ) * aRaySeg.m_InvDir.x;

    float tmin = glm::min( tx1, tx2 );
    float tmax = glm::max( tx1, tx2 );

    const float ty1 = ( m_min.y - aRaySeg.m_Start.y ) * aRaySeg.m_InvDir.y;
    const float ty2 = ( m_max.y - aRaySeg.m_Start.y ) * aRaySeg.m_InvDir.y;

    tmin = glm::max( tmin, glm::min( ty1, ty2 ) );
    tmax = glm::min( tmax, glm::max( ty1, ty2 ) );

    if( (tmax >= 0.0f) && (tmax >= tmin) )
    {
        const float t = (tmin > 0.0f)?tmin:tmax;

        return ( t < aRaySeg.m_Length );
    }

    return false;
}


bool BBOX_2D::Intersect( const RAY2D& aRay, float* aOutHitT0, float* aOutHitT1 ) const
{
    wxASSERT( aOutHitT0 );
    wxASSERT( aOutHitT1 );

    const float tx1 = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;
    const float tx2 = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

    float tmin = glm::min( tx1, tx2 );
    float tmax = glm::max( tx1, tx2 );

    const float ty1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;
    const float ty2 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

    tmin = glm::max( tmin, glm::min( ty1, ty2 ) );
    tmax = glm::min( tmax, glm::max( ty1, ty2 ) );

    *aOutHitT0 = ( tmin > 0.0f ) ? tmin : 0.0f;
    *aOutHitT1 = tmax;

    return ( tmax >= 0.0f ) && ( tmax >= tmin );
}
