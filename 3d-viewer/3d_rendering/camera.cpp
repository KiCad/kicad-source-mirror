/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file camera.cpp
 */

#include "camera.h"
#include <wx/log.h>


// A helper function to normalize aAngle between -2PI and +2PI
inline void normalise2PI( float& aAngle )
{
    while( aAngle > 0.0 )
        aAngle -= static_cast<float>( M_PI * 2.0f );

    while( aAngle < 0.0 )
        aAngle += static_cast<float>( M_PI * 2.0f );
}


/**
 * @ingroup trace_env_vars
 */
const wxChar *CAMERA::m_logTrace = wxT( "KI_TRACE_CAMERA" );


#define DEFAULT_MIN_ZOOM 0.10f
#define DEFAULT_MAX_ZOOM 1.20f


CAMERA::CAMERA( float aInitialDistance )
{
    wxLogTrace( m_logTrace, wxT( "CAMERA::CAMERA" ) );

    m_camera_pos_init       = SFVEC3F( 0.0f, 0.0f, -aInitialDistance );
    m_board_lookat_pos_init = SFVEC3F( 0.0f );
    m_windowSize            = SFVEC2I( 0, 0 );
    m_projectionType        = PROJECTION_TYPE::PERSPECTIVE;
    m_interpolation_mode    = CAMERA_INTERPOLATION::BEZIER;

    m_minZoom = DEFAULT_MIN_ZOOM;
    m_maxZoom = DEFAULT_MAX_ZOOM;

    Reset();
}


void CAMERA::Reset()
{
    m_parametersChanged    = true;
    m_projectionMatrix     = glm::mat4( 1.0f );
    m_projectionMatrixInv  = glm::mat4( 1.0f );
    m_rotationMatrix       = glm::mat4( 1.0f );
    m_rotationMatrixAux    = glm::mat4( 1.0f );
    m_lastPosition         = wxPoint( 0, 0 );

    m_zoom                 = 1.0f;
    m_zoom_t0              = 1.0f;
    m_zoom_t1              = 1.0f;
    m_camera_pos           = m_camera_pos_init;
    m_camera_pos_t0        = m_camera_pos_init;
    m_camera_pos_t1        = m_camera_pos_init;
    m_lookat_pos           = m_board_lookat_pos_init;
    m_lookat_pos_t0        = m_board_lookat_pos_init;
    m_lookat_pos_t1        = m_board_lookat_pos_init;

    m_rotate_aux           = SFVEC3F( 0.0f );
    m_rotate_aux_t0        = SFVEC3F( 0.0f );
    m_rotate_aux_t1        = SFVEC3F( 0.0f );

    updateRotationMatrix();
    updateViewMatrix();
    m_viewMatrixInverse    = glm::inverse( m_viewMatrix );
    m_scr_nX.clear();
    m_scr_nY.clear();
    rebuildProjection();
}


void CAMERA::Reset_T1()
{
    m_camera_pos_t1        = m_camera_pos_init;
    m_zoom_t1              = 1.0f;
    m_rotate_aux_t1        = SFVEC3F( 0.0f );
    m_lookat_pos_t1        = m_board_lookat_pos_init;

    // Since 0 = 2pi, we want to reset the angle to be the closest
    // one to where we currently are. That ensures that we rotate
    // the board around the smallest distance getting there.
    if( m_rotate_aux_t0.x > M_PI )
        m_rotate_aux_t1.x = static_cast<float>( 2.0f * M_PI );

    if( m_rotate_aux_t0.y > M_PI )
        m_rotate_aux_t1.y = static_cast<float>( 2.0f * M_PI );

    if( m_rotate_aux_t0.z > M_PI )
        m_rotate_aux_t1.z = static_cast<float>( 2.0f * M_PI );
}


void CAMERA::zoomChanged()
{
    if( m_zoom < m_minZoom )
        m_zoom = m_minZoom;

    if( m_zoom > m_maxZoom )
        m_zoom = m_maxZoom;

    m_camera_pos.z = m_camera_pos_init.z * m_zoom;

    updateViewMatrix();
    rebuildProjection();
}


void CAMERA::updateViewMatrix()
{
    m_viewMatrix = glm::translate( glm::mat4( 1.0f ), m_camera_pos ) *
                   m_rotationMatrix * m_rotationMatrixAux *
                   glm::translate( glm::mat4( 1.0f ), -m_lookat_pos );
}


void CAMERA::updateRotationMatrix()
{
    m_rotationMatrixAux = glm::rotate( glm::mat4( 1.0f ), m_rotate_aux.x,
                                       SFVEC3F( 1.0f, 0.0f, 0.0f ) );
    normalise2PI( m_rotate_aux.x );

    m_rotationMatrixAux = glm::rotate( m_rotationMatrixAux, m_rotate_aux.y,
                                       SFVEC3F( 0.0f, 1.0f, 0.0f ) );
    normalise2PI( m_rotate_aux.y );

    m_rotationMatrixAux = glm::rotate( m_rotationMatrixAux, m_rotate_aux.z,
                                       SFVEC3F( 0.0f, 0.0f, 1.0f ) );
    normalise2PI( m_rotate_aux.z );

    m_parametersChanged = true;

    updateViewMatrix();
    updateFrustum();
}


const glm::mat4 CAMERA::GetRotationMatrix() const
{
    return m_rotationMatrix * m_rotationMatrixAux;
}


void CAMERA::rebuildProjection()
{
    if( ( m_windowSize.x == 0 ) || ( m_windowSize.y == 0 ) )
        return;

    m_frustum.ratio = (float) m_windowSize.x / (float)m_windowSize.y;
    m_frustum.farD = glm::length( m_camera_pos_init ) * m_maxZoom * 2.0f;

    switch( m_projectionType )
    {
    default:
    case PROJECTION_TYPE::PERSPECTIVE:

        m_frustum.nearD = 0.10f;

        // Ratio width / height of the window display
        m_frustum.angle = 45.0f;

        m_projectionMatrix = glm::perspective( glm::radians( m_frustum.angle ), m_frustum.ratio,
                                               m_frustum.nearD, m_frustum.farD );

        m_projectionMatrixInv = glm::inverse( m_projectionMatrix );

        m_frustum.tang = glm::tan( glm::radians( m_frustum.angle ) * 0.5f );

        m_focalLen.x = ( (float)m_windowSize.y / (float)m_windowSize.x ) / m_frustum.tang;
        m_focalLen.y = 1.0f / m_frustum.tang;

        m_frustum.nh = m_frustum.nearD * m_frustum.tang;
        m_frustum.nw = m_frustum.nh * m_frustum.ratio;
        m_frustum.fh = m_frustum.farD * m_frustum.tang;
        m_frustum.fw = m_frustum.fh * m_frustum.ratio;
        break;

    case PROJECTION_TYPE::ORTHO:

        m_frustum.nearD = -m_frustum.farD; // Use a symmetrical clip plane for ortho projection

        // This formula was found by trial and error
        const float orthoReductionFactor = glm::length( m_camera_pos_init ) *
                                           m_zoom * m_zoom * 0.5f;

        // Initialize Projection Matrix for Orthographic View
        m_projectionMatrix = glm::ortho( -m_frustum.ratio * orthoReductionFactor,
                                          m_frustum.ratio * orthoReductionFactor,
                                         -orthoReductionFactor,
                                          orthoReductionFactor,
                                          m_frustum.nearD, m_frustum.farD );

        m_projectionMatrixInv = glm::inverse( m_projectionMatrix );

        m_frustum.nw = orthoReductionFactor * 2.0f * m_frustum.ratio;
        m_frustum.nh = orthoReductionFactor * 2.0f;
        m_frustum.fw = m_frustum.nw;
        m_frustum.fh = m_frustum.nh;

        break;
    }

    if( ( m_windowSize.x > 0 ) && ( m_windowSize.y > 0 ) )
    {
        m_scr_nX.resize( m_windowSize.x + 1 );
        m_scr_nY.resize( m_windowSize.y + 1 );

        // Precalc X values for camera -> ray generation
        for( unsigned int x = 0; x < (unsigned int)m_windowSize.x + 1; ++x )
        {
            // Converts 0.0 .. 1.0
            const float xNormalizedDeviceCoordinates = ( ( (float)x + 0.5f ) /
                                                         (m_windowSize.x - 0.0f) );

            // Converts -1.0 .. 1.0
            m_scr_nX[x] = 2.0f * xNormalizedDeviceCoordinates - 1.0f;
        }

        // Precalc Y values for camera -> ray generation
        for( unsigned int y = 0; y < (unsigned int)m_windowSize.y + 1 ; ++y )
        {
            // Converts 0.0 .. 1.0
            const float yNormalizedDeviceCoordinates = ( ( (float)y + 0.5f ) /
                                                         (m_windowSize.y - 0.0f) );

            // Converts -1.0 .. 1.0
            m_scr_nY[y] = 2.0f * yNormalizedDeviceCoordinates - 1.0f;
        }

        updateFrustum();
    }
}


void CAMERA::updateFrustum()
{
    // Update matrix and vectors
    m_viewMatrixInverse = glm::inverse( m_viewMatrix );

    m_right = glm::normalize( SFVEC3F( m_viewMatrixInverse *
                                       glm::vec4( SFVEC3F( 1.0, 0.0, 0.0 ), 0.0 ) ) );

    m_up    = glm::normalize( SFVEC3F( m_viewMatrixInverse *
                                       glm::vec4( SFVEC3F( 0.0, 1.0, 0.0 ), 0.0 ) ) );

    m_dir   = glm::normalize( SFVEC3F( m_viewMatrixInverse *
                                       glm::vec4( SFVEC3F( 0.0, 0.0, 1.0 ), 0.0 ) ) );

    m_pos   = SFVEC3F( m_viewMatrixInverse * glm::vec4( SFVEC3F( 0.0, 0.0, 0.0 ), 1.0 ) );

    /*
     * Frustum is a implementation based on a tutorial by
     * http://www.lighthouse3d.com/tutorials/view-frustum-culling/
     */

    // compute the centers of the near and far planes
    m_frustum.nc = m_pos - m_dir * m_frustum.nearD;
    m_frustum.fc = m_pos - m_dir * m_frustum.farD;

    // compute the 4 corners of the frustum on the near plane
    m_frustum.ntl = m_frustum.nc + m_up * m_frustum.nh - m_right * m_frustum.nw;
    m_frustum.ntr = m_frustum.nc + m_up * m_frustum.nh + m_right * m_frustum.nw;
    m_frustum.nbl = m_frustum.nc - m_up * m_frustum.nh - m_right * m_frustum.nw;
    m_frustum.nbr = m_frustum.nc - m_up * m_frustum.nh + m_right * m_frustum.nw;

    // compute the 4 corners of the frustum on the far plane
    m_frustum.ftl = m_frustum.fc + m_up * m_frustum.fh - m_right * m_frustum.fw;
    m_frustum.ftr = m_frustum.fc + m_up * m_frustum.fh + m_right * m_frustum.fw;
    m_frustum.fbl = m_frustum.fc - m_up * m_frustum.fh - m_right * m_frustum.fw;
    m_frustum.fbr = m_frustum.fc - m_up * m_frustum.fh + m_right * m_frustum.fw;

    if( ( m_windowSize.x > 0 ) && ( m_windowSize.y > 0 ) )
    {
        // Reserve size for precalc values
        m_right_nX.resize( m_windowSize.x + 1 );
        m_up_nY.resize( m_windowSize.y + 1 );

        // Precalc X values for camera -> ray generation
        const SFVEC3F right_nw = m_right * m_frustum.nw;

        for( unsigned int x = 0; x < ( (unsigned int) m_windowSize.x + 1 ); ++x )
            m_right_nX[x] = right_nw * m_scr_nX[x];

        // Precalc Y values for camera -> ray generation
        const SFVEC3F up_nh = m_up * m_frustum.nh;

        for( unsigned int y = 0; y < ( (unsigned int) m_windowSize.y + 1 ); ++y )
            m_up_nY[y] = up_nh * m_scr_nY[y];
    }
}


void CAMERA::MakeRay( const SFVEC2I& aWindowPos, SFVEC3F& aOutOrigin,
                       SFVEC3F& aOutDirection ) const
{
    wxASSERT( aWindowPos.x < m_windowSize.x );
    wxASSERT( aWindowPos.y < m_windowSize.y );

    const SFVEC3F up_plus_right = m_up_nY[aWindowPos.y] + m_right_nX[aWindowPos.x];

    switch( m_projectionType )
    {
    default:
    case PROJECTION_TYPE::PERSPECTIVE:
        aOutOrigin = up_plus_right + m_frustum.nc;
        aOutDirection = glm::normalize( aOutOrigin - m_pos );
        break;

    case PROJECTION_TYPE::ORTHO:
        aOutOrigin = up_plus_right * 0.5f + m_frustum.nc;
        aOutDirection = -m_dir + SFVEC3F( FLT_EPSILON );
        break;
    }
}


void CAMERA::MakeRay( const SFVEC2F& aWindowPos, SFVEC3F& aOutOrigin,
                       SFVEC3F& aOutDirection ) const
{
    wxASSERT( aWindowPos.x < (float)m_windowSize.x );
    wxASSERT( aWindowPos.y < (float)m_windowSize.y );

    const SFVEC2F floorWinPos_f = glm::floor( aWindowPos );
    const SFVEC2I floorWinPos_i = (SFVEC2I)floorWinPos_f;
    const SFVEC2F relativeWinPos = aWindowPos - floorWinPos_f;

    // Note: size of vectors m_up and m_right are m_windowSize + 1
    const SFVEC3F up_plus_right = m_up_nY[floorWinPos_i.y] * (1.0f - relativeWinPos.y) +
                                  m_up_nY[floorWinPos_i.y + 1] * relativeWinPos.y +
                                  m_right_nX[floorWinPos_i.x] * (1.0f - relativeWinPos.x) +
                                  m_right_nX[floorWinPos_i.x + 1] * relativeWinPos.x;

    switch( m_projectionType )
    {
    default:
    case PROJECTION_TYPE::PERSPECTIVE:
        aOutOrigin = up_plus_right + m_frustum.nc;
        aOutDirection = glm::normalize( aOutOrigin - m_pos );
        break;

    case PROJECTION_TYPE::ORTHO:
        aOutOrigin = up_plus_right * 0.5f + m_frustum.nc;
        aOutDirection = -m_dir + SFVEC3F( FLT_EPSILON );
        break;
    }
}


void CAMERA::MakeRayAtCurrrentMousePosition( SFVEC3F& aOutOrigin, SFVEC3F& aOutDirection ) const
{
    const SFVEC2I windowPos = SFVEC2I( m_lastPosition.x, m_windowSize.y - m_lastPosition.y );

    if( ( 0 < windowPos.x ) && ( windowPos.x < m_windowSize.x ) &&
        ( 0 < windowPos.y ) && ( windowPos.y < m_windowSize.y ) )
    {
        MakeRay( windowPos, aOutOrigin, aOutDirection );
    }
}


const glm::mat4& CAMERA::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}


const glm::mat4& CAMERA::GetProjectionMatrixInv() const
{
    return m_projectionMatrixInv;
}


float CAMERA::GetCameraMinDimension() const
{
    return -m_camera_pos_init.z * m_frustum.tang;
}


void CAMERA::ResetXYpos()
{
    m_parametersChanged = true;
    m_camera_pos.x = 0.0f;
    m_camera_pos.y = 0.0f;

    updateViewMatrix();
    updateFrustum();
}


void CAMERA::ResetXYpos_T1()
{
    m_camera_pos_t1.x = 0.0f;
    m_camera_pos_t1.y = 0.0f;
}


const glm::mat4& CAMERA::GetViewMatrix() const
{
    return m_viewMatrix;
}


const glm::mat4& CAMERA::GetViewMatrix_Inv() const
{
    return m_viewMatrixInverse;
}


void CAMERA::SetCurMousePosition( const wxPoint& aNewMousePosition )
{
    m_lastPosition = aNewMousePosition;
}


void CAMERA::ToggleProjection()
{
    if( m_projectionType == PROJECTION_TYPE::ORTHO )
        m_projectionType = PROJECTION_TYPE::PERSPECTIVE;
    else
        m_projectionType = PROJECTION_TYPE::ORTHO;

    rebuildProjection();
}


bool CAMERA::SetCurWindowSize( const wxSize& aSize )
{
    const SFVEC2I newSize = SFVEC2I( aSize.x, aSize.y );

    if( m_windowSize != newSize )
    {
        m_windowSize = newSize;
        rebuildProjection();

        return true;
    }

    return false;
}


void CAMERA::ZoomReset()
{
    m_zoom = 1.0f;

    m_camera_pos.z = m_camera_pos_init.z;

    updateViewMatrix();
    rebuildProjection();
}

bool CAMERA::Zoom( float aFactor )
{
    if( ( m_zoom == m_minZoom && aFactor > 1 ) || ( m_zoom == m_maxZoom && aFactor < 1 )
        || aFactor == 1 )
    {
        return false;
    }

    m_zoom /= aFactor;

    zoomChanged();
    return true;
}


bool CAMERA::Zoom_T1( float aFactor )
{
    if( ( m_zoom == m_minZoom && aFactor > 1 ) || ( m_zoom == m_maxZoom && aFactor < 1 )
        || aFactor == 1 )
    {
        return false;
    }

    m_zoom_t1 = m_zoom / aFactor;

    if( m_zoom_t1 < m_minZoom )
        m_zoom_t1 = m_minZoom;

    if( m_zoom_t1 > m_maxZoom )
        m_zoom_t1 = m_maxZoom;

    m_camera_pos_t1.z = m_camera_pos_init.z * m_zoom_t1;

    return true;
}


void CAMERA::RotateX( float aAngleInRadians )
{
    m_rotate_aux.x += aAngleInRadians;
    updateRotationMatrix();
}


void CAMERA::RotateY( float aAngleInRadians )
{
    m_rotate_aux.y += aAngleInRadians;
    updateRotationMatrix();
}


void CAMERA::RotateZ( float aAngleInRadians )
{
    m_rotate_aux.z += aAngleInRadians;
    updateRotationMatrix();
}


void CAMERA::RotateX_T1( float aAngleInRadians )
{
    m_rotate_aux_t1.x += aAngleInRadians;
}


void CAMERA::RotateY_T1( float aAngleInRadians )
{
    m_rotate_aux_t1.y += aAngleInRadians;
}


void CAMERA::RotateZ_T1( float aAngleInRadians )
{
    m_rotate_aux_t1.z += aAngleInRadians;
}


void CAMERA::SetT0_and_T1_current_T()
{
    m_camera_pos_t0 = m_camera_pos;
    m_lookat_pos_t0 = m_lookat_pos;
    m_rotate_aux_t0 = m_rotate_aux;
    m_zoom_t0       = m_zoom;

    m_camera_pos_t1 = m_camera_pos;
    m_lookat_pos_t1 = m_lookat_pos;
    m_rotate_aux_t1 = m_rotate_aux;
    m_zoom_t1       = m_zoom;
}


void CAMERA::Interpolate( float t )
{
    wxASSERT( t >= 0.0f );

    const float t0 = 1.0f - t;

    m_camera_pos = m_camera_pos_t0 * t0 + m_camera_pos_t1 * t;
    m_lookat_pos = m_lookat_pos_t0 * t0 + m_lookat_pos_t1 * t;
    m_rotate_aux = m_rotate_aux_t0 * t0 + m_rotate_aux_t1 * t;
    m_zoom       = m_zoom_t0       * t0 + m_zoom_t1       * t;

    m_parametersChanged = true;

    updateRotationMatrix();
    rebuildProjection();
}


bool CAMERA::ParametersChanged()
{
    const bool parametersChanged = m_parametersChanged;

    m_parametersChanged = false;

    return parametersChanged;
}
