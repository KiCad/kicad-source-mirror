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

#include "raypacket.h"
#include "../3d_fastmath.h"
#include <wx/debug.h>


static void RAYPACKET_GenerateFrustum( FRUSTUM* m_Frustum, RAY* m_ray )
{
    m_Frustum->GenerateFrustum(
                m_ray[                   0 * RAYPACKET_DIM +                   0 ],
                m_ray[                   0 * RAYPACKET_DIM + (RAYPACKET_DIM - 1) ],
                m_ray[ (RAYPACKET_DIM - 1) * RAYPACKET_DIM +                   0 ],
                m_ray[ (RAYPACKET_DIM - 1) * RAYPACKET_DIM + (RAYPACKET_DIM - 1) ] );
}


RAYPACKET::RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition )
{
    unsigned int i = 0;

    for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
        {
            SFVEC3F rayOrigin;
            SFVEC3F rayDir;

            aCamera.MakeRay( SFVEC2I( aWindowsPosition.x + x, aWindowsPosition.y + y ),
                             rayOrigin, rayDir );

            m_ray[i].Init( rayOrigin, rayDir );

            i++;
        }
    }

    wxASSERT( i == RAYPACKET_RAYS_PER_PACKET );

    RAYPACKET_GenerateFrustum( &m_Frustum, m_ray );
}


RAYPACKET::RAYPACKET( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition )
{
    RAYPACKET_InitRays( aCamera, aWindowsPosition, m_ray );

    RAYPACKET_GenerateFrustum( &m_Frustum, m_ray );
}


RAYPACKET::RAYPACKET( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition,
                      const SFVEC2F& a2DWindowsPosDisplacementFactor )
{
    RAYPACKET_InitRays_with2DDisplacement( aCamera, aWindowsPosition,
                                           a2DWindowsPosDisplacementFactor, m_ray );

    RAYPACKET_GenerateFrustum( &m_Frustum, m_ray );
}


RAYPACKET::RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition,
                      const SFVEC3F& aDirectionDisplacementFactor )
{
    unsigned int i = 0;

    for( unsigned int y = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x )
        {
            SFVEC3F rayOrigin;
            SFVEC3F rayDir;

            aCamera.MakeRay( SFVEC2I( aWindowsPosition.x + x, aWindowsPosition.y + y ),
                             rayOrigin, rayDir );

            const SFVEC3F randVector = SFVEC3F( Fast_RandFloat() * aDirectionDisplacementFactor.x,
                                                Fast_RandFloat() * aDirectionDisplacementFactor.y,
                                                Fast_RandFloat() * aDirectionDisplacementFactor.z );

            m_ray[i].Init( rayOrigin, glm::normalize( rayDir + randVector ) );

            i++;
        }
    }

    wxASSERT( i == RAYPACKET_RAYS_PER_PACKET );

    RAYPACKET_GenerateFrustum( &m_Frustum, m_ray );
}


RAYPACKET::RAYPACKET( const CAMERA& aCamera, const SFVEC2I& aWindowsPosition,
                      unsigned int aPixelMultiple )
{
    unsigned int i = 0;

    for( unsigned int y = 0; y < RAYPACKET_DIM; y++ )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; x++ )
        {
            SFVEC3F rayOrigin;
            SFVEC3F rayDir;

            aCamera.MakeRay( SFVEC2I( aWindowsPosition.x + x * aPixelMultiple,
                                      aWindowsPosition.y + y * aPixelMultiple),
                             rayOrigin, rayDir );

            m_ray[i].Init( rayOrigin, rayDir );

            i++;
        }
    }

    wxASSERT( i == RAYPACKET_RAYS_PER_PACKET );

    RAYPACKET_GenerateFrustum( &m_Frustum, m_ray );
}


void RAYPACKET_InitRays( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition, RAY* aRayPck )
{
    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            SFVEC3F rayOrigin;
            SFVEC3F rayDir;

            aCamera.MakeRay( SFVEC2F( aWindowsPosition.x + (float)x,
                                      aWindowsPosition.y + (float)y ),
                             rayOrigin, rayDir );

            aRayPck[i].Init( rayOrigin, rayDir );
        }
    }
}


void RAYPACKET_InitRays_with2DDisplacement( const CAMERA& aCamera, const SFVEC2F& aWindowsPosition,
                                            const SFVEC2F& a2DWindowsPosDisplacementFactor,
                                            RAY* aRayPck )
{
    for( unsigned int y = 0, i = 0; y < RAYPACKET_DIM; ++y )
    {
        for( unsigned int x = 0; x < RAYPACKET_DIM; ++x, ++i )
        {
            SFVEC3F rayOrigin;
            SFVEC3F rayDir;

            aCamera.MakeRay( SFVEC2F( aWindowsPosition.x +(float)x +
                                      Fast_RandFloat() * a2DWindowsPosDisplacementFactor.x,
                                      aWindowsPosition.y + (float)y +
                                      Fast_RandFloat() * a2DWindowsPosDisplacementFactor.y ),
                             rayOrigin, rayDir );

            aRayPck[i].Init( rayOrigin, rayDir );
        }
    }
}
