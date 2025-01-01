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

#ifndef _HITINFO_H_
#define _HITINFO_H_

#include "raypacket.h"

//#define RAYTRACING_RAY_STATISTICS

class OBJECT_3D;

/// Stores the hit information of a ray with a point on the surface of a object
struct HITINFO
{
    SFVEC3F m_HitNormal;                ///< (12) normal at the hit point
    float   m_tHit;                     ///< ( 4) distance

    const OBJECT_3D* pHitObject;        ///< ( 4) Object that was hitted
    SFVEC2F m_UV;                       ///< ( 8) 2-D texture coordinates
    unsigned int m_acc_node_info;       ///< ( 4) The acc stores here the node that it hits

    SFVEC3F m_HitPoint;                 ///< (12) hit position
    float m_ShadowFactor;               ///< ( 4) Shadow attenuation (1.0 no shadow, 0.0f darkness)

#ifdef RAYTRACING_RAY_STATISTICS
    // Statistics
    unsigned int m_NrRayObjTests;       ///< Number of ray-objects tests
    unsigned int m_NrTransversedNodes;  ///< Number of transversed nodes in the acc structure
#endif
};


struct HITINFO_PACKET
{
    bool    m_hitresult;
    HITINFO m_HitInfo;
};

#endif // _HITINFO_H_
