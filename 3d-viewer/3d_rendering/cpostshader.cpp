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
 * @file cpostshader.cpp
 * @brief a base class to create post shaders
 */


#include "cpostshader.h"
#include "buffers_debug.h"
#include <wx/debug.h>


CPOSTSHADER::CPOSTSHADER( const CCAMERA &aCamera ) : m_camera(aCamera)
{
    m_size    = SFVEC2UI( 0, 0 );
    m_normals = NULL;
    m_color   = NULL;
    m_depth   = NULL;
    m_wc_hitposition    = NULL;
    m_shadow_att_factor = NULL;
    m_tmin    = FLT_MAX;
    m_tmax    = FLT_MIN;
}


CPOSTSHADER::~CPOSTSHADER()
{
    destroy_buffers();
}


void CPOSTSHADER::UpdateSize( unsigned int xSize, unsigned int ySize )
{
    destroy_buffers();

    m_size.x = xSize;
    m_size.y = ySize;

    const unsigned int n_elements = xSize * ySize;

    m_normals = new SFVEC3F[n_elements];
    m_color   = new SFVEC3F[n_elements];
    m_depth   = new float[n_elements];
    m_wc_hitposition = new SFVEC3F[n_elements];
    m_shadow_att_factor = new float[n_elements];
}


void CPOSTSHADER::UpdateSize( const SFVEC2UI &aSize )
{
    UpdateSize( aSize.x, aSize.y );
}


void CPOSTSHADER::SetPixelData( unsigned int x,
                                unsigned int y,
                                const SFVEC3F &aNormal,
                                const SFVEC3F &aColor,
                                const SFVEC3F &aHitPosition,
                                float aDepth,
                                float aShadowAttFactor )
{
    wxASSERT( x < m_size.x );
    wxASSERT( y < m_size.y );
    wxASSERT( (aShadowAttFactor >= 0.0f) && (aShadowAttFactor <= 1.0f) );

    const unsigned int idx = x + y * m_size.x;

    m_normals[ idx ] = aNormal;
    m_color  [ idx ] = aColor;
    m_depth  [ idx ] = aDepth;
    m_shadow_att_factor [ idx ] = aShadowAttFactor;
    m_wc_hitposition[ idx ] = aHitPosition;


    if( aDepth > FLT_EPSILON )
    {
        if( aDepth < m_tmin )
            m_tmin = aDepth;

        if( aDepth > m_tmax )
            m_tmax = aDepth;
    }
}


void CPOSTSHADER::destroy_buffers()
{
    delete m_normals;           m_normals = 0;
    delete m_color;             m_color = 0;
    delete m_depth;             m_depth = 0;
    delete m_shadow_att_factor; m_shadow_att_factor = 0;
    delete m_wc_hitposition;    m_wc_hitposition = 0;
}


const SFVEC3F &CPOSTSHADER::GetNormalAt( const SFVEC2F &aPos ) const
{
    return m_normals[ getIndex( aPos ) ];
}


const SFVEC3F &CPOSTSHADER::GetColorAt( const SFVEC2F &aPos ) const
{
    return m_color[ getIndex( aPos ) ];
}


float CPOSTSHADER::GetDepthAt( const SFVEC2F &aPos ) const
{
    return m_depth[ getIndex( aPos ) ];
}


const SFVEC3F &CPOSTSHADER::GetPositionAt( const SFVEC2F &aPos ) const
{
    return m_wc_hitposition[ getIndex( aPos ) ];
}


const SFVEC3F &CPOSTSHADER::GetNormalAt( const SFVEC2I &aPos ) const
{
    return m_normals[ getIndex( aPos ) ];
}


const SFVEC3F &CPOSTSHADER::GetColorAt( const SFVEC2I &aPos ) const
{
    return m_color[ getIndex( aPos ) ];
}


const SFVEC3F &CPOSTSHADER::GetColorAtNotProtected( const SFVEC2I &aPos ) const
{
    return m_color[ aPos.x + m_size.x * aPos.y ];
}


float CPOSTSHADER::GetDepthAt( const SFVEC2I &aPos ) const
{
    return m_depth[ getIndex( aPos ) ];
}


float CPOSTSHADER::GetDepthNormalizedAt( const SFVEC2I &aPos ) const
{
    const float depth = m_depth[ getIndex( aPos ) ];

    if( depth >= m_tmin )
        return (depth - m_tmin) / (m_tmax - m_tmin);

    return 0.0f;
}


const SFVEC3F &CPOSTSHADER::GetPositionAt( const SFVEC2I &aPos ) const
{
    return m_wc_hitposition[ getIndex( aPos ) ];
}


const float &CPOSTSHADER::GetShadowFactorAt( const SFVEC2I &aPos ) const
{
    return m_shadow_att_factor[ getIndex( aPos ) ];
}


void CPOSTSHADER::DebugBuffersOutputAsImages() const
{
    DBG_SaveBuffer( "m_shadow_att_factor", m_shadow_att_factor, m_size.x, m_size.y );
    DBG_SaveBuffer( "m_color", m_color, m_size.x, m_size.y );
    DBG_SaveNormalsBuffer( "m_normals", m_normals, m_size.x, m_size.y );

    // Normalize depth
    // /////////////////////////////////////////////////////////////////////////
    float *normalizedDepth = (float*) malloc( m_size.x * m_size.y * sizeof( float ) );

    float *normalizedDepthPTr = normalizedDepth;

    for( unsigned int iy = 0; iy < m_size.y; ++iy )
    for( unsigned int ix = 0; ix < m_size.x; ++ix )
    {
        *normalizedDepthPTr =  GetDepthNormalizedAt( SFVEC2I( ix, iy) );
        normalizedDepthPTr++;
    }

    DBG_SaveBuffer( "m_depthNormalized", normalizedDepth, m_size.x, m_size.y );

    free( normalizedDepth );
}
