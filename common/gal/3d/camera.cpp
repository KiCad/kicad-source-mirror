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
 * @file camera.cpp
 */

#include <gal/3d/camera.h>
#include <wx/log.h>
#include <algorithm>
#include <3d_enums.h>

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

const float CAMERA::DEFAULT_MIN_ZOOM = 0.020f;
const float CAMERA::DEFAULT_MAX_ZOOM = 2.0f;


CAMERA::CAMERA( float aInitialDistance ) :
        CAMERA( SFVEC3F( 0.0f, 0.0f, -aInitialDistance ), SFVEC3F( 0.0f ),
                PROJECTION_TYPE::PERSPECTIVE )
{
}


CAMERA::CAMERA( SFVEC3F aInitPos, SFVEC3F aLookat, PROJECTION_TYPE aProjectionType )
{
    wxLogTrace( m_logTrace, wxT( "CAMERA::CAMERA" ) );

    m_camera_pos_init = aInitPos;
    m_board_lookat_pos_init = aLookat;
    m_windowSize = SFVEC2I( 0, 0 );
    m_projectionType = aProjectionType;
    m_interpolation_mode = CAMERA_INTERPOLATION::BEZIER;

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


bool CAMERA::ViewCommand_T1( VIEW3D_TYPE aRequestedView )
{
    switch( aRequestedView )
    {
    case VIEW3D_TYPE::VIEW3D_RIGHT:
        SetT0_and_T1_current_T();
        Reset_T1();
        RotateZ_T1( glm::radians( -90.0f ) );
        RotateX_T1( glm::radians( -90.0f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_LEFT:
        Reset_T1();
        RotateZ_T1( glm::radians(  90.0f ) );
        RotateX_T1( glm::radians( -90.0f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_FRONT:
        Reset_T1();
        RotateX_T1( glm::radians( -90.0f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_BACK:
        Reset_T1();
        RotateX_T1( glm::radians( -90.0f ) );

        // The rotation angle should be 180.
        // We use 179.999 (180 - epsilon) to avoid a full 360 deg rotation when
        // using 180 deg if the previous rotated position was already 180 deg
        RotateZ_T1( glm::radians( 179.999f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_TOP:
        Reset_T1();
        return true;

    case VIEW3D_TYPE::VIEW3D_BOTTOM:
        Reset_T1();
        RotateY_T1( glm::radians( 179.999f ) );    // Rotation = 180 - epsilon
        return true;

    case VIEW3D_TYPE::VIEW3D_FLIP:
        RotateY_T1( glm::radians( 179.999f ) );
        return true;

    default:
        return false;
    }
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


void CAMERA::SetBoardLookAtPos( const SFVEC3F& aBoardPos )
{
    if( m_board_lookat_pos_init != aBoardPos )
    {
        m_board_lookat_pos_init = aBoardPos;
        m_lookat_pos = aBoardPos;

        m_parametersChanged = true;

        updateViewMatrix();
        updateFrustum();
    }
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


glm::mat4 CAMERA::GetRotationMatrix() const
{
    return m_rotationMatrix * m_rotationMatrixAux;
}


void CAMERA::SetRotationMatrix( const glm::mat4& aRotation )
{
    m_parametersChanged = true;
    std::copy_n( glm::value_ptr( aRotation * glm::inverse( m_rotationMatrixAux ) ), 12,
                 glm::value_ptr( m_rotationMatrix ) );
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

        m_frustum.angle = 45.0f;

        m_projectionMatrix = glm::perspective( glm::radians( m_frustum.angle ), m_frustum.ratio,
                                               m_frustum.nearD, m_frustum.farD );

        m_projectionMatrixInv = glm::inverse( m_projectionMatrix );

        m_frustum.tang = glm::tan( glm::radians( m_frustum.angle ) * 0.5f );

        m_focalLen.x = ( (float)m_windowSize.y / (float)m_windowSize.x ) / m_frustum.tang;
        m_focalLen.y = 1.0f / m_frustum.tang;

        m_frustum.nh = 2.0f * m_frustum.nearD * m_frustum.tang;
        m_frustum.nw = m_frustum.nh * m_frustum.ratio;
        m_frustum.fh = 2.0f * m_frustum.farD * m_frustum.tang;
        m_frustum.fw = m_frustum.fh * m_frustum.ratio;
        break;

    case PROJECTION_TYPE::ORTHO:

        // Keep the viewed plane at (m_camera_pos_init * m_zoom) the same dimensions in both
        // projections.
        m_frustum.angle = 45.0f;
        m_frustum.tang = glm::tan( glm::radians( m_frustum.angle ) * 0.5f );

        m_frustum.nearD = -m_frustum.farD; // Use a symmetrical clip plane for ortho projection

        const float orthoReductionFactor =
                glm::length( m_camera_pos_init ) * m_zoom * m_frustum.tang;

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

    const SFVEC3F half_right_nw = m_right * m_frustum.nw * 0.5f;
    const SFVEC3F half_right_fw = m_right * m_frustum.fw * 0.5f;
    const SFVEC3F half_up_nh = m_up * m_frustum.nh * 0.5f;
    const SFVEC3F half_up_fh = m_up * m_frustum.fh * 0.5f;

    // compute the centers of the near and far planes
    m_frustum.nc = m_pos - m_dir * m_frustum.nearD;
    m_frustum.fc = m_pos - m_dir * m_frustum.farD;

    // compute the 4 corners of the frustum on the near plane
    m_frustum.ntl = m_frustum.nc + half_up_nh - half_right_nw;
    m_frustum.ntr = m_frustum.nc + half_up_nh + half_right_nw;
    m_frustum.nbl = m_frustum.nc - half_up_nh - half_right_nw;
    m_frustum.nbr = m_frustum.nc - half_up_nh + half_right_nw;

    // compute the 4 corners of the frustum on the far plane
    m_frustum.ftl = m_frustum.fc + half_up_fh - half_right_fw;
    m_frustum.ftr = m_frustum.fc + half_up_fh + half_right_fw;
    m_frustum.fbl = m_frustum.fc - half_up_fh - half_right_fw;
    m_frustum.fbr = m_frustum.fc - half_up_fh + half_right_fw;

    if( ( m_windowSize.x > 0 ) && ( m_windowSize.y > 0 ) )
    {
        // Reserve size for precalc values
        m_right_nX.resize( m_windowSize.x + 1 );
        m_up_nY.resize( m_windowSize.y + 1 );

        // Precalc X values for camera -> ray generation
        for( unsigned int x = 0; x < ( (unsigned int) m_windowSize.x + 1 ); ++x )
            m_right_nX[x] = half_right_nw * m_scr_nX[x];

        // Precalc Y values for camera -> ray generation
        for( unsigned int y = 0; y < ( (unsigned int) m_windowSize.y + 1 ); ++y )
            m_up_nY[y] = half_up_nh * m_scr_nY[y];
    }
}


void CAMERA::MakeRay( const SFVEC2I& aWindowPos, SFVEC3F& aOutOrigin,
                       SFVEC3F& aOutDirection ) const
{
    wxASSERT( aWindowPos.x < m_windowSize.x );
    wxASSERT( aWindowPos.y < m_windowSize.y );

    aOutOrigin = m_frustum.nc + m_up_nY[aWindowPos.y] + m_right_nX[aWindowPos.x];

    switch( m_projectionType )
    {
    default:
    case PROJECTION_TYPE::PERSPECTIVE:
        aOutDirection = glm::normalize( aOutOrigin - m_pos );
        break;

    case PROJECTION_TYPE::ORTHO:
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

    aOutOrigin = up_plus_right + m_frustum.nc;

    switch( m_projectionType )
    {
    default:
    case PROJECTION_TYPE::PERSPECTIVE:
        aOutDirection = glm::normalize( aOutOrigin - m_pos );
        break;

    case PROJECTION_TYPE::ORTHO:
        aOutDirection = -m_dir + SFVEC3F( FLT_EPSILON );
        break;
    }
}


void CAMERA::MakeRayAtCurrentMousePosition( SFVEC3F& aOutOrigin, SFVEC3F& aOutDirection ) const
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


void CAMERA::SetViewMatrix( glm::mat4 aViewMatrix )
{
    SetRotationMatrix( aViewMatrix );

    // The look at position in the view frame.
    glm::vec4 lookat = aViewMatrix * glm::vec4( m_lookat_pos, 1.0f );

    wxLogTrace( m_logTrace,
                wxT( "CAMERA::SetViewMatrix   aViewMatrix[3].z =%f, old_zoom=%f, new_zoom=%f, "
                     "m[3].z=%f" ),
                aViewMatrix[3].z, m_zoom, lookat.z / m_camera_pos_init.z, lookat.z );

    m_zoom = lookat.z / m_camera_pos_init.z;

    if( m_zoom > m_maxZoom )
    {
        m_zoom = m_maxZoom;
        aViewMatrix[3].z += -lookat.z + m_maxZoom * m_camera_pos_init.z;
    }
    else if( m_zoom < m_minZoom )
    {
        m_zoom = m_minZoom;
        aViewMatrix[3].z += -lookat.z + m_minZoom * m_camera_pos_init.z;
    }

    m_viewMatrix = std::move( aViewMatrix );
    m_camera_pos = m_viewMatrix
                   * glm::inverse( m_rotationMatrix * m_rotationMatrixAux
                                   * glm::translate( glm::mat4( 1.0f ), -m_lookat_pos ) )[3];
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
    if( ( m_zoom <= m_minZoom && aFactor > 1 ) || ( m_zoom >= m_maxZoom && aFactor < 1 )
        || aFactor == 1 )
    {
        return false;
    }

    float zoom = m_zoom;
    m_zoom /= aFactor;

    if( m_zoom <= m_minZoom && aFactor > 1 )
    {
        aFactor = zoom / m_minZoom;
        m_zoom = m_minZoom;
    }
    else if( m_zoom >= m_maxZoom && aFactor < 1 )
    {
        aFactor = zoom / m_maxZoom;
        m_zoom = m_maxZoom;
    }

    m_camera_pos.z /= aFactor;

    updateViewMatrix();
    rebuildProjection();

    return true;
}


bool CAMERA::Zoom_T1( float aFactor )
{
    if( ( m_zoom <= m_minZoom && aFactor > 1 ) || ( m_zoom >= m_maxZoom && aFactor < 1 )
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


void CAMERA::RotateScreen( float aAngleInRadians )
{
    glm::mat4 matrix = GetRotationMatrix();
    SetRotationMatrix( glm::rotate( matrix, aAngleInRadians, GetDir() ) );
    updateRotationMatrix();
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
