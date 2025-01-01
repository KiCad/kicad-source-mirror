/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef RAY_H
#define RAY_H

#include <plugins/3dapi/xv3d_types.h>


enum class RAY_CLASSIFICATION
{
    MMM,
    MMP,
    MPM,
    MPP,
    PMM,
    PMP,
    PPM,
    PPP,
    POO,
    MOO,
    OPO,
    OMO,
    OOP,
    OOM,
    OMM,
    OMP,
    OPM,
    OPP,
    MOM,
    MOP,
    POM,
    POP,
    MMO,
    MPO,
    PMO,
    PPO
};


struct RAY
{
    SFVEC3F m_Origin;
    unsigned int rayID; ///< unique ray ID - not used - dummy

    SFVEC3F m_Dir;
    RAY_CLASSIFICATION  m_Classification;

    SFVEC3F m_InvDir;

    float ibyj, jbyi, kbyj, jbyk, ibyk, kbyi;   // slope
    float c_xy, c_xz, c_yx, c_yz, c_zx, c_zy;

    unsigned int m_dirIsNeg[3];

    void Init( const SFVEC3F& o, const SFVEC3F& d );

    bool IntersectSphere( const SFVEC3F &aCenter,
                          float aRadius,
                          float &aOutT0,
                          float &aOutT1 ) const;

    SFVEC3F at( float t ) const { return m_Origin + m_Dir * t; }

    SFVEC2F at2D( float t ) const
    {
        return SFVEC2F( m_Origin.x + m_Dir.x * t, m_Origin.y + m_Dir.y * t );
    }
};


struct RAY2D
{
    SFVEC2F m_Origin;
    SFVEC2F m_Dir;
    SFVEC2F m_InvDir;

    RAY2D( const SFVEC2F& o, const SFVEC2F& d ) { m_Origin = o; m_Dir = d; m_InvDir = (1.0f / d); }

    SFVEC2F at( float t ) const { return m_Origin + m_Dir * t; }
};


struct RAYSEG2D
{
    SFVEC2F m_Start;
    SFVEC2F m_End;
    SFVEC2F m_End_minus_start;
    SFVEC2F m_Dir;
    SFVEC2F m_InvDir;
    float   m_Length;
    float   m_DOT_End_minus_start; ///< dot( m_End_minus_start, m_End_minus_start)

    RAYSEG2D( const SFVEC2F& s, const SFVEC2F& e );

    bool IntersectSegment( const SFVEC2F &aStart,
                           const SFVEC2F &aEnd_minus_start,
                           float *aOutT ) const;

    bool IntersectCircle( const SFVEC2F &aCenter,
                          float aRadius,
                          float *aOutT0,
                          float *aOutT1,
                          SFVEC2F *aOutNormalT0,
                          SFVEC2F *aOutNormalT1 ) const;

    float DistanceToPointSquared( const SFVEC2F &aPoint ) const;

    /**
     * Return the position at \a t.
     *
     * t - value 0.0 ... 1.0
     */
    SFVEC2F atNormalized( float t ) const { return m_Start + m_End_minus_start * t; }

    SFVEC2F at( float t ) const { return m_Start + m_Dir * t; }
};


bool IntersectSegment( const SFVEC2F &aStartA, const SFVEC2F &aEnd_minus_startA,
                       const SFVEC2F &aStartB, const SFVEC2F &aEnd_minus_startB );

/*
#if(GLM_ARCH != GLM_ARCH_PURE)
struct RAY4
{
    glm::simdVec4 m_orgX;   ///< x coordinate of ray origin
    glm::simdVec4 m_orgy;   ///< y coordinate of ray origin
    glm::simdVec4 m_orgz;   ///< z coordinate of ray origin

    glm::simdVec4 m_dirX;   ///< x direction of ray
    glm::simdVec4 m_diry;   ///< y direction of ray
    glm::simdVec4 m_dirz;   ///< z direction of ray

    glm::simdVec4 m_tnear;  ///< Start of ray segment
    glm::simdVec4 m_tfar;   ///< End of ray segment
};

#endif
*/


#endif // RAY_H
