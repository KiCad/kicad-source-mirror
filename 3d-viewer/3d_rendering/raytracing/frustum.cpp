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


#include "frustum.h"

// !TODO: optimize with SSE
//#if(GLM_ARCH != GLM_ARCH_PURE)
#if 0
#error not implemented
#else


#endif

void FRUSTUM::GenerateFrustum( const RAY& topLeft, const RAY& topRight, const RAY& bottomLeft,
                               const RAY& bottomRight )
{
    m_point[0] = topLeft.m_Origin;
    m_point[1] = topRight.m_Origin;
    m_point[2] = bottomLeft.m_Origin;
    m_point[3] = topLeft.m_Origin;

    SFVEC3F tl_tr = topRight.m_Origin - topLeft.m_Origin;
    SFVEC3F tr_br = bottomRight.m_Origin - topRight.m_Origin;
    SFVEC3F br_bl = bottomLeft.m_Origin - bottomRight.m_Origin;
    SFVEC3F bl_tl = topLeft.m_Origin - bottomLeft.m_Origin;

    m_normals[0] = glm::cross( tl_tr, topLeft.m_Dir );     // TOP
    m_normals[1] = glm::cross( tr_br, topRight.m_Dir );    // RIGHT
    m_normals[2] = glm::cross( br_bl, bottomRight.m_Dir ); // BOTTOM
    m_normals[3] = glm::cross( bl_tl, bottomLeft.m_Dir );  // LEFT
}


// There are multiple implementation of this algorithm on the web,
// this one was based on the one find in:
// https://github.com/nslo/raytracer/blob/2c2e0ff4bbb6082e07804ec7cf0b92673b98dcb1/src/raytracer/geom_utils.cpp#L66
// by Nathan Slobody and Adam Wright
// The frustum test is not exllude all the boxes,
// when a box is behind and if it is intersecting the planes it will not be discardly but should.
bool FRUSTUM::Intersect( const BBOX_3D& aBBox ) const
{
    const SFVEC3F box[8] = { aBBox.Min(),
                             aBBox.Max(),
                             SFVEC3F(aBBox.Min().x, aBBox.Min().y, aBBox.Max().z),
                             SFVEC3F(aBBox.Min().x, aBBox.Max().y, aBBox.Min().z),
                             SFVEC3F(aBBox.Min().x, aBBox.Max().y, aBBox.Max().z),
                             SFVEC3F(aBBox.Max().x, aBBox.Min().y, aBBox.Min().z),
                             SFVEC3F(aBBox.Max().x, aBBox.Min().y, aBBox.Max().z),
                             SFVEC3F(aBBox.Max().x, aBBox.Max().y, aBBox.Min().z) };

    // test each plane of frustum individually; if the point is on the wrong
    // side of the plane, the box is outside the frustum and we can exit
    int out_side = 0;

    for( unsigned int i = 0; i < 4; ++i )
    {
        const SFVEC3F& pointPlane  = m_point[i];
        const SFVEC3F& normalPlane = m_normals[i];

        for( unsigned int j = 0; j < 8; ++j )
        {
            const SFVEC3F OP = pointPlane - box[j];
            const float dot = glm::dot( OP, normalPlane );

            if( dot < FLT_EPSILON )
            {
                out_side++;

                break;
            }
        }
    }

    if( out_side == 4 )
        return true;

    return false;
}
