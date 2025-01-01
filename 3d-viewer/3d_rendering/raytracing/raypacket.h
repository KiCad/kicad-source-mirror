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

#ifndef RAYPACKET_H
#define RAYPACKET_H

#include "ray.h"
#include "frustum.h"
#include <gal/3d/camera.h>

#define RAYPACKET_DIM (1 << 3)
#define RAYPACKET_MASK (unsigned int) ( ( RAYPACKET_DIM - 1 ) )
#define RAYPACKET_INVMASK (unsigned int) ( ~( RAYPACKET_DIM - 1 ) )
#define RAYPACKET_RAYS_PER_PACKET ( RAYPACKET_DIM * RAYPACKET_DIM )


struct RAYPACKET
{
    RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition );

    RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition,
               const SFVEC3F& aDirectionDisplacementFactor );

    RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition,
               unsigned int aPixelMultiple );

    RAYPACKET( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition );

    RAYPACKET( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition,
               const SFVEC2F& a2DWindowsPosDisplacementFactor );

    FRUSTUM     m_Frustum;
    RAY         m_ray[RAYPACKET_RAYS_PER_PACKET];
};

void RAYPACKET_InitRays( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition, RAY* aRayPck );

void RAYPACKET_InitRays_with2DDisplacement( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition,
                                            const SFVEC2F& a2DWindowsPosDisplacementFactor,
                                            RAY* aRayPck );

#endif // RAYPACKET_H
