/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include <gal/opengl/kiglew.h>    // Must be included first

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

#include "render_3d_raytrace_gl.h"
#include "../common_ogl/ogl_utils.h"
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <wx/log.h>


RENDER_3D_RAYTRACE_GL::RENDER_3D_RAYTRACE_GL( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter,
                                              CAMERA& aCamera ) :
    RENDER_3D_RAYTRACE_BASE( aAdapter, aCamera )
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_RAYTRACE_GL::RENDER_3D_RAYTRACE_GL" ) );

    m_openglSupportsVertexBufferObjects = false;
    m_pboId       = GL_NONE;
    m_pboDataSize = 0;
}


RENDER_3D_RAYTRACE_GL::~RENDER_3D_RAYTRACE_GL()
{
    deletePbo();
}


void RENDER_3D_RAYTRACE_GL::deletePbo()
{
    // Delete PBO if it was created
    if( m_openglSupportsVertexBufferObjects )
    {
        if( glIsBufferARB( m_pboId ) )
            glDeleteBuffers( 1, &m_pboId );

        m_pboId = GL_NONE;
    }
}


void RENDER_3D_RAYTRACE_GL::SetCurWindowSize( const wxSize& aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;
        glViewport( 0, 0, m_windowSize.x, m_windowSize.y );

        initPbo();
    }
}


bool RENDER_3D_RAYTRACE_GL::Redraw( bool aIsMoving, REPORTER* aStatusReporter,
                                    REPORTER* aWarningReporter )
{
    bool requestRedraw = false;

    // Initialize openGL if need
    if( !m_canvasInitialized )
    {
        m_canvasInitialized = true;

        //aIsMoving = true;
        requestRedraw = true;

        // It will assign the first time the windows size, so it will now
        // revert to preview mode the first time the Redraw is called
        m_oldWindowsSize = m_windowSize;
        initializeBlockPositions();
    }

    std::unique_ptr<BUSY_INDICATOR> busy = CreateBusyIndicator();

    // Reload board if it was requested
    if( m_reloadRequested )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Loading..." ) );

        //aIsMoving = true;
        requestRedraw = true;
        Reload( aStatusReporter, aWarningReporter, false );
    }


    // Recalculate constants if windows size was changed
    if( m_windowSize != m_oldWindowsSize )
    {
        m_oldWindowsSize = m_windowSize;
        aIsMoving = true;
        requestRedraw = true;

        initializeBlockPositions();
    }

    // Clear buffers
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glClearStencil( 0x00 );
    glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    // 4-byte pixel alignment
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    glDisable( GL_STENCIL_TEST );
    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );
    glDisable( GL_MULTISAMPLE );

    const bool was_camera_changed = m_camera.ParametersChanged();

    if( requestRedraw || aIsMoving || was_camera_changed )
        m_renderState = RT_RENDER_STATE_MAX; // Set to an invalid state,
                                             // so it will restart again latter

    // This will only render if need, otherwise it will redraw the PBO on the screen again
    if( aIsMoving || was_camera_changed )
    {
        // Set head light (camera view light) with the opposite direction of the camera
        if( m_cameraLight )
            m_cameraLight->SetDirection( -m_camera.GetDir() );

        OglDrawBackground( premultiplyAlpha( m_boardAdapter.m_BgColorTop ),
                           premultiplyAlpha( m_boardAdapter.m_BgColorBot ) );

        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        // Get the PBO pixel pointer to write the data
        uint8_t* ptrPBO = (uint8_t *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                     GL_WRITE_ONLY_ARB );

        if( ptrPBO )
        {
            renderPreview( ptrPBO );

            // release pointer to mapping buffer, this initialize the coping to PBO
            glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }
    else
    {
        // Bind PBO
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );

        if( m_renderState != RT_RENDER_STATE_FINISH )
        {
            // Get the PBO pixel pointer to write the data
            uint8_t* ptrPBO = (uint8_t *)glMapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB,
                                                         GL_WRITE_ONLY_ARB );

            if( ptrPBO )
            {
                render( ptrPBO, aStatusReporter );

                if( m_renderState != RT_RENDER_STATE_FINISH )
                    requestRedraw = true;

                // release pointer to mapping buffer, this initialize the coping to PBO
                glUnmapBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB );
            }
        }

        if( m_renderState == RT_RENDER_STATE_FINISH )
        {
            glClear( GL_COLOR_BUFFER_BIT );
        }

        glWindowPos2i( m_xoffset, m_yoffset );
    }

    // This way it will blend the progress rendering with the last buffer. eg:
    // if it was called after a openGL.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_ALPHA_TEST );
    glDrawPixels( m_realBufferSize.x, m_realBufferSize.y, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

    return requestRedraw;
}


void RENDER_3D_RAYTRACE_GL::initPbo()
{
    if( GLEW_ARB_pixel_buffer_object )
    {
        m_openglSupportsVertexBufferObjects = true;

        // Try to delete vbo if it was already initialized
        deletePbo();

        // Learn about Pixel buffer objects at:
        // http://www.songho.ca/opengl/gl_pbo.html
        // http://web.eecs.umich.edu/~sugih/courses/eecs487/lectures/25-PBO+Mipmapping.pdf
        // "create 2 pixel buffer objects, you need to delete them when program exits.
        // glBufferDataARB with NULL pointer reserves only memory space."

        // This sets the number of RGBA pixels
        m_pboDataSize =  m_realBufferSize.x * m_realBufferSize.y * 4;

        glGenBuffersARB( 1, &m_pboId );
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboId );
        glBufferDataARB( GL_PIXEL_UNPACK_BUFFER_ARB, m_pboDataSize, 0, GL_STREAM_DRAW_ARB );
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );

        wxLogTrace( m_logTrace,
                    wxT( "RENDER_3D_RAYTRACE_GL:: GLEW_ARB_pixel_buffer_object is supported" ) );
    }
}
