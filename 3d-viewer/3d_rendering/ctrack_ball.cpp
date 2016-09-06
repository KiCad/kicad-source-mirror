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
 * @file  ctrack_ball.cpp
 * @brief Implementation of a track ball camera. A track ball is placed in the
 * center of the screen and rotates the camera.
 */

#include "ctrack_ball.h"
#include "trackball.h"
#include "../3d_math.h"
#include <wx/log.h>


CTRACK_BALL::CTRACK_BALL( float aRangeScale ) : CCAMERA( aRangeScale )
{
    wxLogTrace( m_logTrace, wxT( "CTRACK_BALL::CTRACK_BALL" ) );

    memset( m_quat, 0, sizeof( m_quat ) );
    memset( m_quat_t0, 0, sizeof( m_quat_t0 ) );
    memset( m_quat_t1, 0, sizeof( m_quat_t1 ) );

    trackball( m_quat, 0.0, 0.0, 0.0, 0.0 );
    trackball( m_quat_t0, 0.0, 0.0, 0.0, 0.0 );
    trackball( m_quat_t1, 0.0, 0.0, 0.0, 0.0 );
}


void CTRACK_BALL::Drag( const wxPoint &aNewMousePosition )
{
    m_parametersChanged = true;

    double spin_quat[4];

    // "Pass the x and y coordinates of the last and current positions of
    //  the mouse, scaled so they are from (-1.0 ... 1.0)."
    const float zoom = 1.0f;

    trackball( spin_quat,
               zoom * (2.0 * m_lastPosition.x - m_windowSize.x) / m_windowSize.x,
               zoom * (m_windowSize.y - 2.0 * m_lastPosition.y) / m_windowSize.y,
               zoom * (2.0 * aNewMousePosition.x - m_windowSize.x) / m_windowSize.x,
               zoom * ( m_windowSize.y - 2.0 * aNewMousePosition.y ) / m_windowSize.y);

    add_quats( spin_quat, m_quat, m_quat );

    float rotationMatrix[4][4];

    build_rotmatrix( rotationMatrix, m_quat );

    m_rotationMatrix = glm::make_mat4( &rotationMatrix[0][0] );

    updateViewMatrix();

    updateFrustum();
}


void CTRACK_BALL::SetLookAtPos( const SFVEC3F &aLookAtPos )
{
    if( m_lookat_pos != aLookAtPos )
    {
        m_lookat_pos = aLookAtPos;

        updateViewMatrix();
        updateFrustum();

        m_parametersChanged = true;
    }
}


void CTRACK_BALL::Pan( const wxPoint &aNewMousePosition )
{
    m_parametersChanged = true;

    if( m_projectionType == PROJECTION_ORTHO )
    {
        // With the ortographic projection, there is just a zoom factor
        const float panFactor = m_zoom / 37.5f; // Magic number from CCAMERA::rebuildProjection
        m_camera_pos.x -= panFactor * ( m_lastPosition.x - aNewMousePosition.x );
        m_camera_pos.y -= panFactor * ( aNewMousePosition.y - m_lastPosition.y );
    }
    else // PROJECTION_PERSPECTIVE
    {
        // Unproject the coordinates using the precomputed frustum tangent (zoom level dependent)
        const float panFactor = -m_camera_pos.z *  m_frustum.tang * 2;
        m_camera_pos.x -= panFactor * m_frustum.ratio * ( m_lastPosition.x - aNewMousePosition.x ) / m_windowSize.x;
        m_camera_pos.y -= panFactor * ( aNewMousePosition.y - m_lastPosition.y ) / m_windowSize.y;
    }

    updateViewMatrix();
    updateFrustum();
}


void CTRACK_BALL::Pan( const SFVEC3F &aDeltaOffsetInc )
{
    m_parametersChanged = true;

    m_camera_pos += aDeltaOffsetInc;

    updateViewMatrix();
    updateFrustum();
}


void CTRACK_BALL::Pan_T1( const SFVEC3F &aDeltaOffsetInc )
{
    m_camera_pos_t1 = m_camera_pos + aDeltaOffsetInc;
}


void CTRACK_BALL::Reset()
{
    CCAMERA::Reset();

    memset( m_quat, 0, sizeof( m_quat ) );
    trackball( m_quat, 0.0, 0.0, 0.0, 0.0 );
}


void CTRACK_BALL::Reset_T1()
{
    CCAMERA::Reset_T1();

    memset( m_quat_t1, 0, sizeof( m_quat_t1 ) );
    trackball( m_quat_t1, 0.0, 0.0, 0.0, 0.0 );
}


void CTRACK_BALL::SetT0_and_T1_current_T()
{
    CCAMERA::SetT0_and_T1_current_T();

    memcpy( m_quat_t0, m_quat, sizeof( m_quat ) );
    memcpy( m_quat_t1, m_quat, sizeof( m_quat ) );
}


void CTRACK_BALL::Interpolate( float t )
{
    wxASSERT( t >= 0.0f );

    // Limit t o 1.0
    t = (t > 1.0f)?1.0f:t;

    switch( m_interpolation_mode )
    {
    case INTERPOLATION_BEZIER:
        t = BezierBlend( t );
        break;

    case INTERPOLATION_EASING_IN_OUT:
        t = QuadricEasingInOut( t );
        break;

    case INTERPOLATION_LINEAR:
    default:
        break;
    }

    const float t0 = 1.0f - t;

    m_quat[0] = m_quat_t0[0] * t0 + m_quat_t1[0] * t;
    m_quat[1] = m_quat_t0[1] * t0 + m_quat_t1[1] * t;
    m_quat[2] = m_quat_t0[2] * t0 + m_quat_t1[2] * t;
    m_quat[3] = m_quat_t0[3] * t0 + m_quat_t1[3] * t;

    float rotationMatrix[4][4];

    build_rotmatrix( rotationMatrix, m_quat );

    m_rotationMatrix = glm::make_mat4( &rotationMatrix[0][0] );

    CCAMERA::Interpolate( t );
}
