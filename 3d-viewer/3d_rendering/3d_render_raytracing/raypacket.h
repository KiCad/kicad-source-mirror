/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  raypacket.h
 * @brief
 */

#ifndef _RAYPACKET_H_
#define _RAYPACKET_H_

#include "ray.h"
#include "cfrustum.h"
#include "../ccamera.h"

#define RAYPACKET_DIM (1 << 3)
#define RAYPACKET_MASK    (unsigned int)( (RAYPACKET_DIM - 1))
#define RAYPACKET_INVMASK (unsigned int)(~(RAYPACKET_DIM - 1))
#define RAYPACKET_RAYS_PER_PACKET (RAYPACKET_DIM * RAYPACKET_DIM)


struct RAYPACKET
{
    CFRUSTUM    m_Frustum;
    RAY         m_ray[RAYPACKET_RAYS_PER_PACKET];

    RAYPACKET( const CCAMERA &aCamera,
               const SFVEC2I &aWindowsPosition );

    RAYPACKET( const CCAMERA &aCamera,
               const SFVEC2I &aWindowsPosition,
               const SFVEC3F &aDirectionDisplacementFactor );

    RAYPACKET( const CCAMERA &aCamera,
               const SFVEC2I &aWindowsPosition,
               unsigned int aPixelMultiple );

    RAYPACKET( const CCAMERA &aCamera,
               const SFVEC2F &aWindowsPosition );

    RAYPACKET( const CCAMERA &aCamera,
               const SFVEC2F &aWindowsPosition,
               const SFVEC2F &a2DWindowsPosDisplacementFactor );
};

void RAYPACKET_InitRays( const CCAMERA &aCamera,
                         const SFVEC2F &aWindowsPosition,
                         RAY *aRayPck );

void RAYPACKET_InitRays_with2DDisplacement( const CCAMERA &aCamera,
                                            const SFVEC2F &aWindowsPosition,
                                            const SFVEC2F &a2DWindowsPosDisplacementFactor,
                                            RAY *aRayPck );

#endif // _RAYPACKET_H_
