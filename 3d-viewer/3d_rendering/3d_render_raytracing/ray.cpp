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
 * @file  ray.cpp
 * @brief
 */


#include "ray.h"
#include "../../3d_fastmath.h"
#include <stdio.h>
#include <wx/debug.h>

//static unsigned int gs_next_rayID = 0;

void RAY::Init( const SFVEC3F& o, const SFVEC3F& d )
{
    m_Origin = o;
    m_Dir = d;
    m_InvDir = 1.0f / d;

    //rayID = gs_next_rayID;
    //gs_next_rayID++;

    // An Efficient and Robust Ray–Box Intersection Algorithm
    // Amy Williams Steve Barrus R. Keith Morley Peter Shirley
    // University of Utah
    // http://people.csail.mit.edu/amy/papers/box-jgt.pdf
    m_dirIsNeg[0] = m_Dir.x <= 0.0f;
    m_dirIsNeg[1] = m_Dir.y <= 0.0f;
    m_dirIsNeg[2] = m_Dir.z <= 0.0f;


    // ray slope

    // "Fast Ray / Axis-Aligned Bounding Box Overlap Tests using Ray Slopes"
    // by Martin Eisemann, Thorsten Grosch, Stefan Müller and Marcus Magnor
    // Computer Graphics Lab, TU Braunschweig, Germany and
    // University of Koblenz-Landau, Germany
    // Licence: "This source code is public domain, but please mention us if you use it."
    //
    // https://github.com/rjw57/mcvoxel/tree/master/third-party/rayslope
    // https://github.com/rjw57/mcvoxel/blob/master/third-party/rayslope/ray.cpp

    ibyj = m_Dir.x * m_InvDir.y;
    jbyi = m_Dir.y * m_InvDir.x;
    jbyk = m_Dir.y * m_InvDir.z;
    kbyj = m_Dir.z * m_InvDir.y;
    ibyk = m_Dir.x * m_InvDir.z;
    kbyi = m_Dir.z * m_InvDir.x;
    c_xy = m_Origin.y - jbyi * m_Origin.x;
    c_xz = m_Origin.z - kbyi * m_Origin.x;
    c_yx = m_Origin.x - ibyj * m_Origin.y;
    c_yz = m_Origin.z - kbyj * m_Origin.y;
    c_zx = m_Origin.x - ibyk * m_Origin.z;
    c_zy = m_Origin.y - jbyk * m_Origin.z;

    // ray slope classification
    if( m_Dir.x < 0 )
    {
        if( m_Dir.y < 0 )
        {
            if( m_Dir.z < 0 )
            {
                m_Classification = MMM;
            }
            else if( m_Dir.z > 0 ){
                m_Classification = MMP;
            }
            else//( m_Dir.z >= 0 )
            {
                m_Classification = MMO;
            }
        }
        else//( m_Dir.y >= 0 )
        {
            if( m_Dir.z < 0 )
            {
                m_Classification = MPM;
                if( m_Dir.y == 0 )
                    m_Classification = MOM;
            }
            else//( m_Dir.z >= 0 )
            {
                if( ( m_Dir.y == 0 ) && ( m_Dir.z == 0 ) )
                    m_Classification = MOO;
                else if( m_Dir.z == 0 )
                    m_Classification = MPO;
                else if( m_Dir.y == 0 )
                    m_Classification = MOP;
                else
                    m_Classification = MPP;
            }
        }
    }
    else//( m_Dir.x >= 0 )
    {
        if( m_Dir.y < 0 )
        {
            if( m_Dir.z < 0 )
            {
                m_Classification = PMM;
                if( m_Dir.x == 0 )
                    m_Classification = OMM;
            }
            else//( m_Dir.z >= 0 )
            {
                if( ( m_Dir.x == 0 ) && ( m_Dir.z == 0 ) )
                    m_Classification = OMO;
                else if( m_Dir.z == 0 )
                    m_Classification = PMO;
                else if( m_Dir.x == 0 )
                    m_Classification = OMP;
                else
                    m_Classification = PMP;
            }
        }
        else//( m_Dir.y >= 0 )
        {
            if( m_Dir.z < 0 )
            {
                if( ( m_Dir.x == 0 ) && ( m_Dir.y == 0 ) )
                    m_Classification = OOM;
                else if( m_Dir.x == 0 )
                    m_Classification = OPM;
                else if( m_Dir.y == 0 )
                    m_Classification = POM;
                else
                    m_Classification = PPM;
            }
            else//( m_Dir.z > 0 )
            {
                if( m_Dir.x == 0 )
                {
                    if( m_Dir.y == 0 )
                        m_Classification = OOP;
                    else if( m_Dir.z == 0 )
                        m_Classification = OPO;
                    else
                        m_Classification = OPP;
                }
                else
                {
                    if( ( m_Dir.y == 0 ) && ( m_Dir.z == 0 ) )
                        m_Classification = POO;
                    else if( m_Dir.y == 0 )
                        m_Classification = POP;
                    else if( m_Dir.z == 0 )
                        m_Classification = PPO;
                    else
                        m_Classification = PPP;
                }
            }
        }
    }
}


bool IntersectSegment( const SFVEC2F &aStartA, const SFVEC2F &aEnd_minus_startA,
                       const SFVEC2F &aStartB, const SFVEC2F &aEnd_minus_startB )
{
    float rxs = aEnd_minus_startA.x *
                aEnd_minus_startB.y - aEnd_minus_startA.y *
                aEnd_minus_startB.x;

    if( fabs(rxs) >  glm::epsilon<float>() )
    {
        float inv_rxs = 1.0f / rxs;

        SFVEC2F pq = aStartB - aStartA;

        float t = (pq.x * aEnd_minus_startB.y - pq.y * aEnd_minus_startB.x) * inv_rxs;

        if( (t < 0.0f) || (t > 1.0f) )
            return false;

        float u = (pq.x * aEnd_minus_startA.y - pq.y * aEnd_minus_startA.x) * inv_rxs;

        if( (u < 0.0f) || (u > 1.0f) )
            return false;

        return true;
    }

    return false;
}


// !TODO: not tested
bool RAY::IntersectSphere( const SFVEC3F &aCenter, float aRadius, float &aOutT0, float &aOutT1 ) const
{
/*
    // Ray-sphere intersection: algebraic

    SFVEC3F CO = m_Origin - aCenter;

    float a = glm::dot( m_Dir, m_Dir );
    float b = 2.0f * glm::dot( CO, m_Dir );
    float c = glm::dot( CO, CO ) - aRadius*aRadius;

    float discriminant = b * b - 4.0f * a * c;

    if( discriminant < 0.0f )
        return false;

    aOutT0 = (-b - sqrtf(discriminant)) / (2.0f * a);
    aOutT1 = (-b + sqrtf(discriminant)) / (2.0f * a);

    if( aOutT0 > aOutT1 )
    {
        float temp = aOutT0;
        aOutT0 = aOutT1;
        aOutT1 = temp;
    }

    return true;
*/

    // Ray-sphere intersection: geometric
    SFVEC3F OC = aCenter - m_Origin;
    float p_dot_d = glm::dot( OC, m_Dir );

    if( p_dot_d < 0.0f )
        return 0.0f;

    float p_dot_p = glm::dot( OC, OC );
    float discriminant = p_dot_p - p_dot_d * p_dot_d;

    if( discriminant > aRadius*aRadius )
        return false;

    discriminant = sqrtf( aRadius*aRadius - discriminant );

    aOutT0 = p_dot_d - discriminant;
    aOutT1 = p_dot_d + discriminant;

    if( aOutT0 > aOutT1 )
    {
        float temp = aOutT0;
        aOutT0 = aOutT1;
        aOutT1 = temp;
    }

    return true;
}


RAYSEG2D::RAYSEG2D( const SFVEC2F& s, const SFVEC2F& e )
{
    m_Start = s;
    m_End = e;
    m_End_minus_start = e - s;
    m_Length = glm::length( m_End_minus_start );
    m_Dir = glm::normalize( m_End_minus_start );
    m_InvDir = (1.0f / m_Dir);

    if( fabs(m_Dir.x) < FLT_EPSILON )
        m_InvDir.x = NextFloatDown(FLT_MAX);

    if( fabs(m_Dir.y) < FLT_EPSILON )
        m_InvDir.y = NextFloatDown(FLT_MAX);

    m_DOT_End_minus_start = glm::dot( m_End_minus_start, m_End_minus_start );
}


bool RAYSEG2D::IntersectSegment( const SFVEC2F &aStart,
                                 const SFVEC2F &aEnd_minus_start,
                                 float *aOutT ) const
{
    float rxs = m_End_minus_start.x *
                aEnd_minus_start.y - m_End_minus_start.y *
            aEnd_minus_start.x;

    if( fabs( rxs ) >  glm::epsilon<float>() )
    {
        const float inv_rxs = 1.0f / rxs;

        const SFVEC2F pq = aStart - m_Start;

        const float t = (pq.x * aEnd_minus_start.y - pq.y * aEnd_minus_start.x) * inv_rxs;

        if( (t < 0.0f) || (t > 1.0f) )
            return false;

        float u = (pq.x * m_End_minus_start.y - pq.y * m_End_minus_start.x) * inv_rxs;

        if( (u < 0.0f) || (u > 1.0f) )
            return false;

        *aOutT = t;

        return true;
    }

    return false;
}


// http://geomalgorithms.com/a02-_lines.html
float RAYSEG2D::DistanceToPointSquared( const SFVEC2F &aPoint ) const
{
    SFVEC2F p = aPoint - m_Start;

    const float c1 = glm::dot( p, m_End_minus_start );

    if( c1 < FLT_EPSILON )
        return glm::dot( p, p );

    if( m_DOT_End_minus_start <= c1 )
        p = aPoint - m_End;
    else
    {
        const float b = c1 / m_DOT_End_minus_start;
        const SFVEC2F pb = m_Start + m_End_minus_start * b;

        p = aPoint - pb;
    }

    return glm::dot( p, p );
}


bool RAYSEG2D::IntersectCircle( const SFVEC2F &aCenter,
                                float aRadius,
                                float *aOutT0,
                                float *aOutT1,
                                SFVEC2F *aOutNormalT0,
                                SFVEC2F *aOutNormalT1 ) const
{
    // This code used directly from Steve Marschner's CS667 framework
    // http://cs665pd.googlecode.com/svn/trunk/photon/sphere.cpp

    // Compute some factors used in computation
    const float qx = m_Start.x - aCenter.x;
    const float qy = m_Start.y - aCenter.y;

    const float qd = qx * m_Dir.x + qy * m_Dir.y;
    const float qq = qx * qx + qy * qy;

    // solving the quadratic equation for t at the pts of intersection
    // dd*t^2 + (2*qd)*t + (qq-r^2) = 0
    const float discriminantsqr = (qd * qd - (qq - aRadius * aRadius));

    // If the discriminant is less than zero, there is no intersection
    if( discriminantsqr < FLT_EPSILON )
        return false;


    // Otherwise check and make sure that the intersections occur on the ray (t
    // > 0) and return the closer one
    const float discriminant = sqrt( discriminantsqr );
    const float t1 = (-qd - discriminant);
    const float t2 = (-qd + discriminant);

    if( (( t1 < 0.0f ) || ( t1 > m_Length ) ) &&
        (( t2 < 0.0f ) || ( t2 > m_Length ) ) )
        return false;// Neither intersection was in the ray's half line.

    // Convert the intersection to a normalized
    *aOutT0 = t1 / m_Length;
    *aOutT1 = t2 / m_Length;

    SFVEC2F hitPointT1 = at( t1 );
    SFVEC2F hitPointT2 = at( t2 );

    *aOutNormalT0 = (hitPointT1 - aCenter) / aRadius;
    *aOutNormalT1 = (hitPointT2 - aCenter) / aRadius;

    return true;
}


void RAY::debug() const
{
    printf("O(%f, %f, %f) D(%f, %f, %f)\n", m_Origin.x, m_Origin.y, m_Origin.z,
                                            m_Dir.x,    m_Dir.y,    m_Dir.z );
}
