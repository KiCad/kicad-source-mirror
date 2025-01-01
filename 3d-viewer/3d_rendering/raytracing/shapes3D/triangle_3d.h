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
 * @file triangle_3d.h
 * @brief Implement a triangle ray intersection based on article
 * http://www.flipcode.com/archives/Raytracing_Topics_Techniques-Part_7_Kd-Trees_and_More_Speed.shtml
 * by Jacco Bikker, that implement optimizations based on Ingo Wald's thesis.
 */


#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

#include "object_3d.h"

/**
 * A triangle object.
 */
class TRIANGLE : public OBJECT_3D
{
public:
    TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3 );

    TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3,
              const SFVEC3F& aFaceNormal );

    TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3,
              const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3 );

    void SetColor( const SFVEC3F& aColor );

    void SetColor( const SFVEC3F& aVC0, const SFVEC3F& aVC1, const SFVEC3F& aVC2 );

    void SetColor( unsigned int aFaceColorRGBA );

    void SetColor( unsigned int aVertex1ColorRGBA, unsigned int aVertex2ColorRGBA,
                   unsigned int aVertex3ColorRGBA );

    void SetUV( const SFVEC2F& aUV1, const SFVEC2F& aUV2, const SFVEC2F& aUV3 );

    bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const override;
    bool IntersectP(const RAY& aRay, float aMaxDistance ) const override;
    bool Intersects( const BBOX_3D& aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO& aHitInfo ) const override;

private:
    void pre_calc_const();

    SFVEC3F m_normal[3];                // 36
    SFVEC3F m_vertex[3];                // 36
    SFVEC3F m_n;                        // 12
    SFVEC2F m_uv[3];                    // 24
    unsigned int m_vertexColorRGBA[3];  // 12
    float m_nu, m_nv, m_nd;             // 12
    unsigned int m_k;                   // 4
    float m_bnu, m_bnv;                 // 8
    float m_cnu, m_cnv;                 // 8
                                        // 152 bytes (max 160 == 5 * 32)
};

#endif // _TRIANGLE_H_
