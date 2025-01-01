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
 * @file post_shader.cpp
 * @brief A base class to create post shaders.
 */


#include "post_shader.h"
#include "buffers_debug.h"
#include <wx/debug.h>


POST_SHADER::POST_SHADER( const CAMERA& aCamera ) :
    m_camera( aCamera )
{
    m_size    = SFVEC2UI( 0, 0 );
    m_normals = nullptr;
    m_color   = nullptr;
    m_depth   = nullptr;
    m_wc_hitposition    = nullptr;
    m_shadow_att_factor = nullptr;
    m_tmin    = FLT_MAX;
    m_tmax    = FLT_MIN;
}


POST_SHADER::~POST_SHADER()
{
    destroy_buffers();
}


void POST_SHADER::UpdateSize( unsigned int xSize, unsigned int ySize )
{
    destroy_buffers();

    m_size.x = xSize;
    m_size.y = ySize;

    const unsigned int n_elements = xSize * ySize;

    m_normals = new SFVEC3F[n_elements];
    m_color   = new SFVEC4F[n_elements];
    m_depth   = new float[n_elements];
    m_wc_hitposition = new SFVEC3F[n_elements];
    m_shadow_att_factor = new float[n_elements];
}


void POST_SHADER::UpdateSize( const SFVEC2UI& aSize )
{
    UpdateSize( aSize.x, aSize.y );
}


void POST_SHADER::SetPixelData( unsigned int x, unsigned int y, const SFVEC3F& aNormal,
                                const SFVEC4F& aColor, const SFVEC3F& aHitPosition,
                                float aDepth, float aShadowAttFactor )
{
    wxASSERT( x < m_size.x );
    wxASSERT( y < m_size.y );
    wxASSERT( ( aShadowAttFactor >= 0.0f ) && ( aShadowAttFactor <= 1.0f ) );

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


void POST_SHADER::destroy_buffers()
{
    delete[] m_normals;
    m_normals = nullptr;
    delete[] m_color;
    m_color = nullptr;
    delete[] m_depth;
    m_depth = nullptr;
    delete[] m_shadow_att_factor;
    m_shadow_att_factor = nullptr;
    delete[] m_wc_hitposition;
    m_wc_hitposition = nullptr;
}


const SFVEC3F& POST_SHADER::GetNormalAt( const SFVEC2F& aPos ) const
{
    return m_normals[GetIndex( aPos )];
}


const SFVEC4F& POST_SHADER::GetColorAt( const SFVEC2F& aPos ) const
{
    return m_color[GetIndex( aPos )];
}


float POST_SHADER::GetDepthAt( const SFVEC2F& aPos ) const
{
    return m_depth[GetIndex( aPos )];
}


const SFVEC3F& POST_SHADER::GetPositionAt( const SFVEC2F& aPos ) const
{
    return m_wc_hitposition[GetIndex( aPos )];
}


const SFVEC3F& POST_SHADER::GetNormalAt( const SFVEC2I& aPos ) const
{
    return m_normals[GetIndex( aPos )];
}


const SFVEC4F& POST_SHADER::GetColorAt( const SFVEC2I& aPos ) const
{
    return m_color[GetIndex( aPos )];
}


const SFVEC4F& POST_SHADER::GetColorAtNotProtected( const SFVEC2I& aPos ) const
{
    return m_color[ aPos.x + m_size.x * aPos.y ];
}


float POST_SHADER::GetDepthAt( const SFVEC2I& aPos ) const
{
    return m_depth[GetIndex( aPos )];
}


float POST_SHADER::GetDepthNormalizedAt( const SFVEC2I& aPos ) const
{
    const float depth = m_depth[GetIndex( aPos )];

    if( depth >= m_tmin )
        return (depth - m_tmin) / (m_tmax - m_tmin);

    return 0.0f;
}


const SFVEC3F& POST_SHADER::GetPositionAt( const SFVEC2I& aPos ) const
{
    return m_wc_hitposition[GetIndex( aPos )];
}


const float& POST_SHADER::GetShadowFactorAt( const SFVEC2I& aPos ) const
{
    return m_shadow_att_factor[GetIndex( aPos )];
}


void POST_SHADER::DebugBuffersOutputAsImages() const
{
    DBG_SaveBuffer( wxT( "m_shadow_att_factor" ), m_shadow_att_factor, m_size.x, m_size.y );
    DBG_SaveBuffer( wxT( "m_color" ), m_color, m_size.x, m_size.y );
    DBG_SaveNormalsBuffer( wxT( "m_normals" ), m_normals, m_size.x, m_size.y );

    // Normalize depth
    float *normalizedDepth = (float*) malloc( m_size.x * m_size.y * sizeof( float ) );

    float *normalizedDepthPTr = normalizedDepth;

    for( unsigned int iy = 0; iy < m_size.y; ++iy )
    {
        for( unsigned int ix = 0; ix < m_size.x; ++ix )
        {
            *normalizedDepthPTr = GetDepthNormalizedAt( SFVEC2I( ix, iy) );
            normalizedDepthPTr++;
        }
    }

    DBG_SaveBuffer( wxT( "m_depthNormalized" ), normalizedDepth, m_size.x, m_size.y );

    free( normalizedDepth );
}
