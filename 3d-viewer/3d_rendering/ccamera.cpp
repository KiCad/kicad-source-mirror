/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  ccamera.cpp
 * @brief
 */

#include <cstring>
#include "../common_ogl/openGL_includes.h"
#include "ccamera.h"
#include <wx/log.h>


/**
 *  Trace mask used to enable or disable the trace output of this class.
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_CCAMERA".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 */
const wxChar *CCAMERA::m_logTrace = wxT( "KI_TRACE_CCAMERA" );


CCAMERA::CCAMERA( float aRangeScale )
{
    wxLogTrace( m_logTrace, wxT( "CCAMERA::CCAMERA" ) );

    m_parametersChanged    = true;
    m_projectionType       = PROJECTION_PERSPECTIVE;
    m_projectionMatrix     = glm::mat4( 1.0f );
    m_projectionMatrix_inv = glm::mat4( 1.0f );
    m_rotationMatrix       = glm::mat4( 1.0f );
    m_lastPosition         = wxPoint( 0, 0 );
    m_windowSize           = wxSize( 0, 0 );
    m_zoom                 = 1.0f;
    m_range_scale          = aRangeScale;
    m_camera_pos_init      = SFVEC3F( 0.0f, 0.0f, -aRangeScale );
    m_camera_pos           = m_camera_pos_init;
    m_boardLookAt_pos      = SFVEC3F( 0.0, 0.0, 0.0 );
    updateViewMatrix();
    m_viewMatrix_inverse   = glm::inverse( m_viewMatrix );
    m_scr_nX.clear();
    m_scr_nY.clear();
    memset( &m_frustum, 0, sizeof( m_frustum ) );
}


void CCAMERA::updateViewMatrix()
{
    m_viewMatrix = glm::translate( glm::mat4( 1.0f ), m_camera_pos ) *
                   m_rotationMatrix * glm::translate( glm::mat4( 1.0f ), m_boardLookAt_pos );
}


const glm::mat4 &CCAMERA::GetRotationMatrix() const
{
    return m_rotationMatrix;
}


void CCAMERA::rebuildProjection()
{
    m_frustum.ratio = (float) m_windowSize.x / m_windowSize.y;
    m_frustum.nearD = 0.01f;
    m_frustum.farD = glm::length( m_camera_pos_init ) * 2.0f;                   // Consider that we can render double the lenght, review if that is OK...

    switch( m_projectionType )
    {
    case PROJECTION_PERSPECTIVE:
        // Ratio width / height of the window display
        m_frustum.angle = 45.0f * m_zoom;


        m_projectionMatrix = glm::perspective( m_frustum.angle * ( glm::pi<float>() / 180.0f ),
                                               m_frustum.ratio,
                                               m_frustum.nearD,
                                               m_frustum.farD );

        m_projectionMatrix_inv = glm::inverse( m_projectionMatrix );

        m_frustum.tang = (float)tan( m_frustum.angle * ( glm::pi<float>() / 180.0f ) * 0.5f ) ;

        m_focalLen.x = ( (float)m_windowSize.y / (float)m_windowSize.x ) / m_frustum.tang;
        m_focalLen.y = 1.0f / m_frustum.tang;

        m_frustum.nh = m_frustum.nearD * m_frustum.tang;
        m_frustum.nw = m_frustum.nh * m_frustum.ratio;
        m_frustum.fh = m_frustum.farD * m_frustum.tang;
        m_frustum.fw = m_frustum.fh * m_frustum.ratio;
        break;

    case PROJECTION_ORTHO:
        const float orthoReductionFactor = m_zoom / 400.0f;

        // Initialize Projection Matrix for Ortographic View
        m_projectionMatrix = glm::ortho( -m_windowSize.x * orthoReductionFactor,
                                          m_windowSize.x * orthoReductionFactor,
                                         -m_windowSize.y * orthoReductionFactor,
                                          m_windowSize.y * orthoReductionFactor,
                                          m_frustum.nearD,
                                          m_frustum.farD );

        m_projectionMatrix_inv = glm::inverse( m_projectionMatrix );

        m_frustum.nw = m_windowSize.x * orthoReductionFactor * 2.0f;
        m_frustum.nh = m_windowSize.y * orthoReductionFactor * 2.0f;
        m_frustum.fw = m_frustum.nw;
        m_frustum.fh = m_frustum.nh;

        break;
    }

    m_scr_nX.resize( m_windowSize.x );
    m_scr_nY.resize( m_windowSize.y );

    // Precalc X values for camera -> ray generation
    for( unsigned int x = 0; x < (unsigned int)m_windowSize.x; x++ )
    {
        // Converts 0.0 .. 1.0
        float xNormalizedDeviceCoordinates = ( ( (float)x + 0.5f ) / (m_windowSize.x - 0.0f) );

        // Converts -1.0 .. 1.0
        m_scr_nX[x] = 2.0f * xNormalizedDeviceCoordinates - 1.0f;
    }

    // Precalc Y values for camera -> ray generation
    for( unsigned int y = 0; y < (unsigned int)m_windowSize.y; y++ )
    {
        // Converts 0.0 .. 1.0
        float yNormalizedDeviceCoordinates = ( ( (float)y + 0.5f ) / (m_windowSize.y - 0.0f) );

        // Converts -1.0 .. 1.0
        m_scr_nY[y] = 2.0f * yNormalizedDeviceCoordinates - 1.0f;
    }

    updateFrustum();
}


void CCAMERA::updateFrustum()
{
    // Update matrix and vectors
    m_viewMatrix_inverse = glm::inverse( m_viewMatrix );

    m_right = glm::normalize( SFVEC3F( m_viewMatrix_inverse * glm::vec4( SFVEC3F( 1.0, 0.0, 0.0 ), 0.0 ) ) );
    m_up    = glm::normalize( SFVEC3F( m_viewMatrix_inverse * glm::vec4( SFVEC3F( 0.0, 1.0, 0.0 ), 0.0 ) ) );
    m_dir   = glm::normalize( SFVEC3F( m_viewMatrix_inverse * glm::vec4( SFVEC3F( 0.0, 0.0, 1.0 ), 0.0 ) ) );
    m_pos   = SFVEC3F( m_viewMatrix_inverse * glm::vec4( SFVEC3F( 0.0, 0.0, 0.0 ), 1.0 ) );


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

    // Reserve size for precalc values
    m_right_nX.resize( m_windowSize.x );
    m_up_nY.resize( m_windowSize.y );

    // Precalc X values for camera -> ray generation
    SFVEC3F right_nw = m_right * m_frustum.nw;

    for( unsigned int x = 0; x < (unsigned int)m_windowSize.x; x++ )
        m_right_nX[x] = right_nw * m_scr_nX[x];

    // Precalc Y values for camera -> ray generation
    SFVEC3F up_nh = m_up * m_frustum.nh;

    for( unsigned int y = 0; y < (unsigned int)m_windowSize.y; y++ )
        m_up_nY[y] = m_frustum.nc + (up_nh * m_scr_nY[y]);
}


void CCAMERA::MakeRay( const SFVEC2I &aWindowPos, SFVEC3F &aOutOrigin, SFVEC3F &aOutDirection ) const
{
    aOutOrigin = m_up_nY[aWindowPos.y] +
                 m_right_nX[aWindowPos.x];

    aOutDirection = glm::normalize( aOutOrigin - m_pos );
}


void CCAMERA::GLdebug_Lines()
{
    SFVEC3F ntl = m_frustum.ntl;
    SFVEC3F ntr = m_frustum.ntr;
    SFVEC3F nbl = m_frustum.nbl;
    SFVEC3F nbr = m_frustum.nbr;

    SFVEC3F ftl = m_frustum.ftl;
    SFVEC3F ftr = m_frustum.ftr;
    SFVEC3F fbl = m_frustum.fbl;
    SFVEC3F fbr = m_frustum.fbr;

    glColor4f( 1.0, 1.0, 1.0, 0.7 );

    glBegin(GL_LINE_LOOP);
    //near plane
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(nbl.x,nbl.y,nbl.z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    //far plane
        glVertex3f(ftr.x,ftr.y,ftr.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    //bottom plane
        glVertex3f(nbl.x,nbl.y,nbl.z);
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    //top plane
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);
        glVertex3f(ftr.x,ftr.y,ftr.z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    //left plane
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(nbl.x,nbl.y,nbl.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    // right plane
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(ftr.x,ftr.y,ftr.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);
    glEnd();
}


void CCAMERA::GLdebug_Vectors()
{
    SFVEC3F right = m_pos + m_right * m_frustum.nearD;
    SFVEC3F up    = m_pos + m_up    * m_frustum.nearD;
    SFVEC3F dir   = m_pos - m_dir   * m_frustum.nearD;

    glColor4f( 1.0, 0.0, 0.0, 1.0 );
    glBegin( GL_LINES );
    glVertex3fv( &m_pos.x );
    glVertex3fv( &right.x );
    glEnd();

    glColor4f( 0.0, 1.0, 0.0, 1.0 );
    glBegin( GL_LINES );
    glVertex3fv( &m_pos.x );
    glVertex3fv( &up.x );
    glEnd();

    glColor4f( 0.0, 0.0, 1.0, 1.0 );
    glBegin( GL_LINES );
    glVertex3fv( &m_pos.x );
    glVertex3fv( &dir.x );
    glEnd();
}


void CCAMERA::GLdebug_Planes()
{
    SFVEC3F ntl = m_frustum.ntl;
    SFVEC3F ntr = m_frustum.ntr;
    SFVEC3F nbl = m_frustum.nbl;
    SFVEC3F nbr = m_frustum.nbr;

    SFVEC3F ftl = m_frustum.ftl;
    SFVEC3F ftr = m_frustum.ftr;
    SFVEC3F fbl = m_frustum.fbl;
    SFVEC3F fbr = m_frustum.fbr;

    // Initialize alpha blending function.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glBegin(GL_QUADS);

    //near plane
        glColor4f( 0.0, 0.0, 0.5, 0.6 );
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(nbl.x,nbl.y,nbl.z);

    //far plane
        glColor4f( 0.0, 0.0, 0.5, 0.6 );
        glVertex3f(ftr.x,ftr.y,ftr.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);

    //bottom plane
        glColor4f( 0.0, 0.5, 0.0, 0.6 );
        glVertex3f(nbl.x,nbl.y,nbl.z);
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);

    //top plane
        glColor4f( 0.0, 0.5, 0.0, 0.6 );
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);
        glVertex3f(ftr.x,ftr.y,ftr.z);

    //left plane
        glColor4f( 0.5, 0.0, 0.0, 0.6 );
        glVertex3f(ntl.x,ntl.y,ntl.z);
        glVertex3f(nbl.x,nbl.y,nbl.z);
        glVertex3f(fbl.x,fbl.y,fbl.z);
        glVertex3f(ftl.x,ftl.y,ftl.z);

    // right plane
        glColor4f( 0.5, 0.0, 0.0, 0.6 );
        glVertex3f(nbr.x,nbr.y,nbr.z);
        glVertex3f(ntr.x,ntr.y,ntr.z);
        glVertex3f(ftr.x,ftr.y,ftr.z);
        glVertex3f(fbr.x,fbr.y,fbr.z);

    glEnd();

    glDisable( GL_BLEND );
/*
    glColor3f( 0.0, 0.0, 1.0 );
    OGL_draw_arrow( m_pos,
                    m_frustum.nc,
                    0.1);*/


}


const glm::mat4 &CCAMERA::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}


const glm::mat4 &CCAMERA::GetProjectionMatrixInv() const
{
    return m_projectionMatrix_inv;
}


const glm::mat4 &CCAMERA::GetViewMatrix() const
{
    return m_viewMatrix;
}


const glm::mat4 &CCAMERA::GetViewMatrix_Inv() const
{
    return m_viewMatrix_inverse;
}


void CCAMERA::SetCurMousePosition( const wxPoint &aNewMousePosition )
{
    m_lastPosition = aNewMousePosition;
}


void CCAMERA::SetProjection( PROJECTION_TYPE aProjectionType )
{
    if( m_projectionType != aProjectionType )
    {
        m_projectionType = aProjectionType;
        rebuildProjection();
    }
}


void CCAMERA::SetCurWindowSize( const wxSize &aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        rebuildProjection();
    }
}


void CCAMERA::ZoomReset()
{
    m_zoom = 1.0f;

    m_camera_pos.z = m_camera_pos_init.z;

    updateViewMatrix();
    rebuildProjection();
}


void CCAMERA::ZoomIn( float aFactor )
{
    float old_zoom = m_zoom;

    m_zoom /= aFactor;

    if( m_zoom <= 0.05f )
        m_zoom = 0.05f;

    m_camera_pos.z = m_zoom * m_camera_pos_init.z;

    if( old_zoom != m_zoom )
    {
        updateViewMatrix();
        rebuildProjection();
    }
}


void CCAMERA::ZoomOut( float aFactor )
{
    float old_zoom = m_zoom;

    m_zoom *= aFactor;

    if( m_zoom >= 1.5f )
        m_zoom = 1.5f;

    m_camera_pos.z = m_zoom * m_camera_pos_init.z;

    if( old_zoom != m_zoom )
    {
        updateViewMatrix();
        rebuildProjection();
    }
}


float CCAMERA::ZoomGet() const
{
    return m_zoom;
}


bool CCAMERA::ParametersChanged()
{
    bool parametersChanged = m_parametersChanged;

    m_parametersChanged = false;

    return parametersChanged;
}
