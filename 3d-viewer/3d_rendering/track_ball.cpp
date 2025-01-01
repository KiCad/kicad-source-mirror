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
 * @file  track_ball.cpp
 * @brief Implementation of a track ball camera. A track ball is placed in the
 * center of the screen and rotates the camera.
 */

#include "track_ball.h"
#include "trackball.h"
#include "../3d_math.h"
#include <wx/log.h>

#include <glm/gtc/quaternion.hpp>

// stdlib
#include <algorithm>


TRACK_BALL::TRACK_BALL( float aInitialDistance ) : CAMERA( aInitialDistance )
{
    wxLogTrace( m_logTrace, wxT( "TRACK_BALL::TRACK_BALL" ) );
    initQuat();
}


TRACK_BALL::TRACK_BALL( SFVEC3F aInitPos, SFVEC3F aLookat, PROJECTION_TYPE aProjectionType ) :
        CAMERA( aInitPos, aLookat, aProjectionType )
{
    wxLogTrace( m_logTrace, wxT( "TRACK_BALL::TRACK_BALL" ) );
    initQuat();
}


void TRACK_BALL::initQuat()
{
    memset( m_quat_t0, 0, sizeof( m_quat_t0 ) );
    memset( m_quat_t1, 0, sizeof( m_quat_t1 ) );

    trackball( m_quat_t0, 0.0, 0.0, 0.0, 0.0 );
    trackball( m_quat_t1, 0.0, 0.0, 0.0, 0.0 );
}


void TRACK_BALL::Drag( const wxPoint& aNewMousePosition )
{
    m_parametersChanged = true;

    double spin_quat[4];

    // "Pass the x and y coordinates of the last and current positions of
    //  the mouse, scaled so they are from (-1.0 ... 1.0)."
    const float zoom = 1.0f;

    trackball( spin_quat, zoom * ( 2.0 * m_lastPosition.x - m_windowSize.x ) / m_windowSize.x,
               zoom * ( m_windowSize.y - 2.0 * m_lastPosition.y ) / m_windowSize.y,
               zoom * ( 2.0 * aNewMousePosition.x - m_windowSize.x ) / m_windowSize.x,
               zoom * ( m_windowSize.y - 2.0 * aNewMousePosition.y ) / m_windowSize.y );

    float spin_matrix[4][4];
    build_rotmatrix( spin_matrix, spin_quat );
    m_rotationMatrix = glm::make_mat4( &spin_matrix[0][0] ) * m_rotationMatrix;

    updateViewMatrix();

    updateFrustum();
}

void TRACK_BALL::Pan( const wxPoint& aNewMousePosition )
{
    m_parametersChanged = true;

    if( m_projectionType == PROJECTION_TYPE::ORTHO )
    {
        m_camera_pos.x -= m_frustum.nw *
                ( m_lastPosition.x - aNewMousePosition.x ) / m_windowSize.x;
        m_camera_pos.y -= m_frustum.nh *
                ( aNewMousePosition.y - m_lastPosition.y ) / m_windowSize.y;
    }
    else // PROJECTION_TYPE::PERSPECTIVE
    {
        // Unproject the coordinates using the precomputed frustum tangent (zoom level dependent)
        const float panFactor = -m_camera_pos.z * m_frustum.tang * 2;
        m_camera_pos.x -= panFactor * m_frustum.ratio *
                ( m_lastPosition.x - aNewMousePosition.x ) / m_windowSize.x;
        m_camera_pos.y -= panFactor * ( aNewMousePosition.y - m_lastPosition.y ) / m_windowSize.y;
    }

    updateViewMatrix();
    updateFrustum();
}

void TRACK_BALL::Pan( const SFVEC3F& aDeltaOffsetInc )
{
    m_parametersChanged = true;

    m_camera_pos += aDeltaOffsetInc;

    updateViewMatrix();
    updateFrustum();
}

void TRACK_BALL::Pan_T1( const SFVEC3F& aDeltaOffsetInc )
{
    m_camera_pos_t1 = m_camera_pos + aDeltaOffsetInc;
}

void TRACK_BALL::Reset_T1()
{
    CAMERA::Reset_T1();

    memset( m_quat_t1, 0, sizeof( m_quat_t1 ) );
    trackball( m_quat_t1, 0.0, 0.0, 0.0, 0.0 );
}

void TRACK_BALL::SetT0_and_T1_current_T()
{
    CAMERA::SetT0_and_T1_current_T();

    double quat[4];

    // Charge the quaternions with the current rotation matrix to allow dual input.
    std::copy_n( glm::value_ptr( glm::conjugate( glm::quat_cast( m_rotationMatrix ) ) ),
                 sizeof( quat ) / sizeof( quat[0] ), quat );

    memcpy( m_quat_t0, quat, sizeof( quat ) );
    memcpy( m_quat_t1, quat, sizeof( quat ) );
}

void TRACK_BALL::Interpolate( float t )
{
    wxASSERT( t >= 0.0f );

    // Limit t o 1.0
    t = ( t > 1.0f ) ? 1.0f : t;

    switch( m_interpolation_mode )
    {
    case CAMERA_INTERPOLATION::BEZIER:
        t = BezierBlend( t );
        break;

    case CAMERA_INTERPOLATION::EASING_IN_OUT:
        t = QuadricEasingInOut( t );
        break;

    case CAMERA_INTERPOLATION::LINEAR:
    default:
        break;
    }

    const float t0 = 1.0f - t;
    double      quat[4];
    quat[0] = m_quat_t0[0] * t0 + m_quat_t1[0] * t;
    quat[1] = m_quat_t0[1] * t0 + m_quat_t1[1] * t;
    quat[2] = m_quat_t0[2] * t0 + m_quat_t1[2] * t;
    quat[3] = m_quat_t0[3] * t0 + m_quat_t1[3] * t;

    float rotationMatrix[4][4];

    build_rotmatrix( rotationMatrix, quat );

    m_rotationMatrix = glm::make_mat4( &rotationMatrix[0][0] );

    CAMERA::Interpolate( t );
}
