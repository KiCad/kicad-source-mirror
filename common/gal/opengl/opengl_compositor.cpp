/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * or you may search the http:O//www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file opengl_compositor.cpp
 * @brief Class that handles multitarget rendering (ie. to different textures/surfaces) and
 * later compositing into a single image (OpenGL flavour).
 */

#include <gal/opengl/opengl_compositor.h>
#include <wx/log.h>

using namespace KiGfx;

OPENGL_COMPOSITOR::OPENGL_COMPOSITOR() :
    m_initialized( false ), m_current( 0 )
{
}


OPENGL_COMPOSITOR::~OPENGL_COMPOSITOR()
{
    if( m_initialized )
        clean();
}


void OPENGL_COMPOSITOR::Initialize()
{
    if( m_initialized )
        return;

    // Get the maximum number of buffers
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, (GLint*) &m_maxBuffers );

    // We need framebuffer objects for drawing the screen contents
    // Generate framebuffer and a depth buffer
    glGenFramebuffers( 1, &m_framebuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer );
    m_currentFbo = m_framebuffer;

    // Allocate memory for the depth buffer
    // Attach the depth buffer to the framebuffer
    glGenRenderbuffers( 1, &m_depthBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, m_depthBuffer );

    // Use here a size of 24 bits for the depth buffer, 8 bits for the stencil buffer
    // this is required later for anti-aliasing
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_STENCIL, m_width, m_height );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, m_depthBuffer );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                               GL_RENDERBUFFER, m_depthBuffer );

    // Unbind the framebuffer, so by default all the rendering goes directly to the display
    glBindFramebuffer( GL_FRAMEBUFFER, DIRECT_RENDERING );
    m_currentFbo = 0;

    m_initialized = true;
}


void OPENGL_COMPOSITOR::Resize( unsigned int aWidth, unsigned int aHeight )
{
    if( m_initialized )
        clean();

    m_width  = aWidth;
    m_height = aHeight;
}


unsigned int OPENGL_COMPOSITOR::CreateBuffer()
{
    wxASSERT( m_initialized );

    //if( usedBuffers() >= m_maxBuffers )
    {
        wxLogError( wxT( "Cannot create more framebuffers. OpenGL rendering backend requires at"
                         "least 3 framebuffers. You may try to update/change "
                         "your graphic drivers." ) );
        return 0; // Unfortunately we have no more free buffers left
    }

    // GL_COLOR_ATTACHMENTn are consecutive integers
    GLuint attachmentPoint = GL_COLOR_ATTACHMENT0 + usedBuffers();
    GLuint textureTarget;

    // Generate the texture for the pixel storage
    glGenTextures( 1, &textureTarget );
    glBindTexture( GL_TEXTURE_2D, textureTarget );

    // Set texture parameters
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    // Bind the texture to the specific attachment point, clear and rebind the screen
    glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer );
    m_currentFbo = m_framebuffer;
    glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_2D, textureTarget, 0 );

    // Check the status, exit if the framebuffer can't be created
    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );

    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        switch( status )
        {
        case GL_FRAMEBUFFER_UNDEFINED:
            wxLogFatalError( wxT( "Target is the default framebuffer, "
                                  "but the default framebuffer does not exist." ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            wxLogFatalError( wxT( "Cannot create the framebuffer." ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            wxLogFatalError( wxT( "The framebuffer attachment points are incomplete." ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            wxLogFatalError( wxT( "The framebuffer does not have at least "
                                   "one image attached to it." ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            wxLogFatalError( wxT( "The framebuffer read buffer is incomplete." ) );
            break;

        case GL_FRAMEBUFFER_UNSUPPORTED:
            wxLogFatalError( wxT( "The combination of internal formats of the attached images "
                                  "violates an implementation-dependent set of restrictions." ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            wxLogFatalError( wxT( "GL_RENDERBUFFER_SAMPLES is not the same "
                                  "for all attached renderbuffers" ) );
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            wxLogFatalError( wxT( "Framebuffer incomplete layer targets errors." ) );
            break;

        default:
            wxLogFatalError( wxT( "Cannot create the framebuffer." ) );
            break;
        }

        return 0;
    }

    ClearBuffer();

    // Return to direct rendering (we were asked only to create a buffer, not switch to one)
    glBindFramebuffer( GL_FRAMEBUFFER, DIRECT_RENDERING );
    m_currentFbo = DIRECT_RENDERING;

    // Store the new buffer
    OPENGL_BUFFER buffer = { textureTarget, attachmentPoint };
    m_buffers.push_back( buffer );

    return usedBuffers();
}


void OPENGL_COMPOSITOR::SetBuffer( unsigned int aBufferHandle )
{
    if( aBufferHandle > usedBuffers() )
        return;

    // Change the rendering destination to the selected attachment point
    if( aBufferHandle == DIRECT_RENDERING )
    {
        glBindFramebuffer( GL_FRAMEBUFFER, DIRECT_RENDERING );
        m_currentFbo = DIRECT_RENDERING;
    }
    else if( m_currentFbo != m_framebuffer )
    {
        glBindFramebuffer( GL_FRAMEBUFFER, m_framebuffer );
        m_currentFbo = m_framebuffer;
    }

    if( m_currentFbo != DIRECT_RENDERING )
    {
        m_current = aBufferHandle - 1;
        glDrawBuffer( m_buffers[m_current].attachmentPoint );
    }
}


void OPENGL_COMPOSITOR::ClearBuffer()
{
    wxASSERT( m_initialized );

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void OPENGL_COMPOSITOR::DrawBuffer( unsigned int aBufferHandle )
{
    wxASSERT( m_initialized );
    if( aBufferHandle == 0 || aBufferHandle > usedBuffers() )
    {
        wxLogError( wxT( "Wrong framebuffer handle" ) );
        return;
    }

    // Switch to the main framebuffer and blit the scene
    glBindFramebuffer( GL_FRAMEBUFFER, DIRECT_RENDERING );
    m_currentFbo = DIRECT_RENDERING;

    // Depth test has to be disabled to make transparency working
    glDisable( GL_DEPTH_TEST );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    // Enable texturing and bind the main texture
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, m_buffers[aBufferHandle - 1].textureTarget );

    // Draw a full screen quad with the texture
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();

    glBegin( GL_TRIANGLES );
    glTexCoord2f( 0.0f,  1.0f );
    glVertex2f(  -1.0f, -1.0f );
    glTexCoord2f( 1.0f,  1.0f );
    glVertex2f(   1.0f, -1.0f );
    glTexCoord2f( 1.0f,  0.0f );
    glVertex2f(   1.0f,  1.0f );

    glTexCoord2f( 0.0f,  1.0f );
    glVertex2f(  -1.0f, -1.0f );
    glTexCoord2f( 1.0f,  0.0f );
    glVertex2f(   1.0f,  1.0f );
    glTexCoord2f( 0.0f,  0.0f );
    glVertex2f(  -1.0f,  1.0f );
    glEnd();

    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
}


void OPENGL_COMPOSITOR::clean()
{
    wxASSERT( m_initialized );

    glDeleteFramebuffers( 1, &m_framebuffer );
    glDeleteRenderbuffers( 1, &m_depthBuffer );

    OPENGL_BUFFERS::const_iterator it;
    for( it = m_buffers.begin(); it != m_buffers.end(); ++it )
    {
        glDeleteTextures( 1, &it->textureTarget );
    }

    m_buffers.clear();

    m_initialized = false;
}


GLuint OPENGL_COMPOSITOR::m_currentFbo = DIRECT_RENDERING;
