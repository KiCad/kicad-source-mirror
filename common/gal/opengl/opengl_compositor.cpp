/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file opengl_compositor.cpp
 * @brief Class that handles multitarget rendering (i.e. to different textures/surfaces) and
 * later compositing into a single image (OpenGL flavour).
 */

#include <gal/opengl/opengl_compositor.h>
#include <gal/opengl/utils.h>

#include <gal/color4d.h>

#include <cassert>
#include <memory>
#include <stdexcept>
#include <wx/log.h>
#include <wx/debug.h>

using namespace KIGFX;

OPENGL_COMPOSITOR::OPENGL_COMPOSITOR() :
        m_initialized( false ),
        m_curBuffer( 0 ),
        m_mainFbo( 0 ),
        m_depthBuffer( 0 ),
        m_curFbo( DIRECT_RENDERING ),
        m_currentAntialiasingMode( GAL_ANTIALIASING_MODE::AA_NONE )
{
    m_antialiasing = std::make_unique<ANTIALIASING_NONE>( this );
}


OPENGL_COMPOSITOR::~OPENGL_COMPOSITOR()
{
    if( m_initialized )
    {
        try
        {
            clean();
        }
        catch( const std::runtime_error& exc )
        {
            wxLogError( wxT( "Run time exception `%s` occurred in OPENGL_COMPOSITOR destructor." ),
                        exc.what() );
        }
    }
}


void OPENGL_COMPOSITOR::SetAntialiasingMode( GAL_ANTIALIASING_MODE aMode )
{
    m_currentAntialiasingMode = aMode;

    if( m_initialized )
        clean();
}


GAL_ANTIALIASING_MODE OPENGL_COMPOSITOR::GetAntialiasingMode() const
{
    return m_currentAntialiasingMode;
}


void OPENGL_COMPOSITOR::Initialize()
{
    if( m_initialized )
        return;

    switch( m_currentAntialiasingMode )
    {
    case GAL_ANTIALIASING_MODE::AA_FAST:
        m_antialiasing = std::make_unique<ANTIALIASING_SMAA>( this );
        break;
    case GAL_ANTIALIASING_MODE::AA_HIGHQUALITY:
        m_antialiasing = std::make_unique<ANTIALIASING_SUPERSAMPLING>( this );
        break;
    default:
        m_antialiasing = std::make_unique<ANTIALIASING_NONE>( this );
        break;
    }

    VECTOR2I dims = m_antialiasing->GetInternalBufferSize();
    assert( dims.x != 0 && dims.y != 0 );

    GLint maxBufSize;
    glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE_EXT, &maxBufSize );

    if( dims.x < 0 || dims.y < 0 || dims.x > maxBufSize || dims.y >= maxBufSize )
        throw std::runtime_error( "Requested render buffer size is not supported" );

    // We need framebuffer objects for drawing the screen contents
    // Generate framebuffer and a depth buffer
    glGenFramebuffersEXT( 1, &m_mainFbo );
    checkGlError( "generating framebuffer", __FILE__, __LINE__ );
    bindFb( m_mainFbo );

    // Allocate memory for the depth buffer
    // Attach the depth buffer to the framebuffer
    glGenRenderbuffersEXT( 1, &m_depthBuffer );
    checkGlError( "generating renderbuffer", __FILE__, __LINE__ );
    glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_depthBuffer );
    checkGlError( "binding renderbuffer", __FILE__, __LINE__ );

    glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8, dims.x, dims.y );
    checkGlError( "creating renderbuffer storage", __FILE__, __LINE__ );
    glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER_EXT, m_depthBuffer );
    checkGlError( "attaching renderbuffer", __FILE__, __LINE__ );

    // Unbind the framebuffer, so by default all the rendering goes directly to the display
    bindFb( DIRECT_RENDERING );

    m_initialized = true;

    m_antialiasing->Init();
}


void OPENGL_COMPOSITOR::Resize( unsigned int aWidth, unsigned int aHeight )
{
    if( m_initialized )
        clean();

    m_antialiasing->OnLostBuffers();

    m_width = aWidth;
    m_height = aHeight;
}


unsigned int OPENGL_COMPOSITOR::CreateBuffer()
{
    return m_antialiasing->CreateBuffer();
}


unsigned int OPENGL_COMPOSITOR::CreateBuffer( VECTOR2I aDimensions )
{
    assert( m_initialized );

    int maxBuffers, maxTextureSize;

    // Get the maximum number of buffers
    glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, (GLint*) &maxBuffers );

    if( (int) usedBuffers() >= maxBuffers )
    {
        throw std::runtime_error( "Cannot create more framebuffers. OpenGL rendering backend requires at "
                                  "least 3 framebuffers. You may try to update/change your graphic drivers." );
    }

    glGetIntegerv( GL_MAX_TEXTURE_SIZE, (GLint*) &maxTextureSize );

    if( maxTextureSize < (int) aDimensions.x || maxTextureSize < (int) aDimensions.y )
    {
        throw std::runtime_error( "Requested texture size is not supported. Could not create a buffer." );
    }

    // GL_COLOR_ATTACHMENTn are consecutive integers
    GLuint attachmentPoint = GL_COLOR_ATTACHMENT0 + usedBuffers();
    GLuint textureTarget;

    // Generate the texture for the pixel storage
    glActiveTexture( GL_TEXTURE0 );
    glGenTextures( 1, &textureTarget );
    checkGlError( "generating framebuffer texture target", __FILE__, __LINE__ );
    glBindTexture( GL_TEXTURE_2D, textureTarget );
    checkGlError( "binding framebuffer texture target", __FILE__, __LINE__ );

    // Set texture parameters
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, aDimensions.x, aDimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
    checkGlError( "creating framebuffer texture", __FILE__, __LINE__ );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    // Bind the texture to the specific attachment point, clear and rebind the screen
    bindFb( m_mainFbo );
    glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, attachmentPoint, GL_TEXTURE_2D, textureTarget, 0 );

    // Check the status, exit if the framebuffer can't be created
    GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

    if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
    {
        switch( status )
        {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            throw std::runtime_error( "The framebuffer attachment points are incomplete." );

        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            throw std::runtime_error( "No images attached to the framebuffer." );

        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            throw std::runtime_error( "The framebuffer does not have at least one image attached to it." );

        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            throw std::runtime_error( "The framebuffer read buffer is incomplete." );

        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            throw std::runtime_error( "The combination of internal formats of the attached images violates "
                                      "an implementation-dependent set of restrictions." );

        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
            throw std::runtime_error( "GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers" );

        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT:
            throw std::runtime_error( "Framebuffer incomplete layer targets errors." );

        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            throw std::runtime_error( "Framebuffer attachments have different dimensions" );

        default:
            throw std::runtime_error( "Unknown error occurred when creating the framebuffer." );
        }
    }

    ClearBuffer( COLOR4D::BLACK );

    // Return to direct rendering (we were asked only to create a buffer, not switch to one)
    bindFb( DIRECT_RENDERING );

    // Store the new buffer
    OPENGL_BUFFER buffer = { aDimensions, textureTarget, attachmentPoint };
    m_buffers.push_back( buffer );

    return usedBuffers();
}


GLenum OPENGL_COMPOSITOR::GetBufferTexture( unsigned int aBufferHandle )
{
    wxCHECK( aBufferHandle > 0 && aBufferHandle <= usedBuffers(), 0 );
    return m_buffers[aBufferHandle - 1].textureTarget;
}


void OPENGL_COMPOSITOR::SetBuffer( unsigned int aBufferHandle )
{
    wxCHECK( m_initialized && aBufferHandle <= usedBuffers(), /* void */ );

    // Either unbind the FBO for direct rendering, or bind the one with target textures
    bindFb( aBufferHandle == DIRECT_RENDERING ? DIRECT_RENDERING : m_mainFbo );

    // Switch the target texture
    if( m_curFbo != DIRECT_RENDERING )
    {
        m_curBuffer = aBufferHandle - 1;
        glDrawBuffer( m_buffers[m_curBuffer].attachmentPoint );
        checkGlError( "setting draw buffer", __FILE__, __LINE__ );

        glViewport( 0, 0, m_buffers[m_curBuffer].dimensions.x, m_buffers[m_curBuffer].dimensions.y );
    }
    else
    {
        glViewport( 0, 0, GetScreenSize().x, GetScreenSize().y );
    }
}


void OPENGL_COMPOSITOR::ClearBuffer( const COLOR4D& aColor )
{
    wxCHECK( m_initialized, /* void */ );

    glClearColor( aColor.r, aColor.g, aColor.b, m_curFbo == DIRECT_RENDERING ? 1.0f : 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}


VECTOR2I OPENGL_COMPOSITOR::GetScreenSize() const
{
    typedef VECTOR2I::coord_type coord_t;
    wxASSERT( m_width <= static_cast<unsigned int>( std::numeric_limits<coord_t>::max() ) );
    wxASSERT( m_height <= static_cast<unsigned int>( std::numeric_limits<coord_t>::max() ) );

    return { static_cast<coord_t>( m_width ), static_cast<coord_t>( m_height ) };
}


void OPENGL_COMPOSITOR::Begin()
{
    m_antialiasing->Begin();
}


void OPENGL_COMPOSITOR::DrawBuffer( unsigned int aBufferHandle )
{
    m_antialiasing->DrawBuffer( aBufferHandle );
}


void OPENGL_COMPOSITOR::DrawBuffer( unsigned int aSourceHandle, unsigned int aDestHandle )
{
    wxCHECK( m_initialized && aSourceHandle != 0 && aSourceHandle <= usedBuffers(), /* void */ );
    wxCHECK( aDestHandle <= usedBuffers(), /* void */ );

    // Switch to the destination buffer and blit the scene
    SetBuffer( aDestHandle );

    // Depth test has to be disabled to make transparency working
    glDisable( GL_DEPTH_TEST );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    // Enable texturing and bind the main texture
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, m_buffers[aSourceHandle - 1].textureTarget );

    // Draw a full screen quad with the texture
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();

    glBegin( GL_TRIANGLES );
    glTexCoord2f( 0.0f, 1.0f );
    glVertex2f( -1.0f, 1.0f );
    glTexCoord2f( 0.0f, 0.0f );
    glVertex2f( -1.0f, -1.0f );
    glTexCoord2f( 1.0f, 1.0f );
    glVertex2f( 1.0f, 1.0f );

    glTexCoord2f( 1.0f, 1.0f );
    glVertex2f( 1.0f, 1.0f );
    glTexCoord2f( 0.0f, 0.0f );
    glVertex2f( -1.0f, -1.0f );
    glTexCoord2f( 1.0f, 0.0f );
    glVertex2f( 1.0f, -1.0f );
    glEnd();

    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
}


void OPENGL_COMPOSITOR::Present()
{
    m_antialiasing->Present();
}


void OPENGL_COMPOSITOR::bindFb( unsigned int aFb )
{
    // Currently there are only 2 valid FBOs
    wxASSERT( aFb == DIRECT_RENDERING || aFb == m_mainFbo );

    if( m_curFbo != aFb )
    {
        glBindFramebufferEXT( GL_FRAMEBUFFER, aFb );
        checkGlError( "switching framebuffer", __FILE__, __LINE__ );
        m_curFbo = aFb;
    }
}


void OPENGL_COMPOSITOR::clean()
{
    wxCHECK( m_initialized, /* void */ );

    bindFb( DIRECT_RENDERING );

    for( const OPENGL_BUFFER& buffer : m_buffers )
        glDeleteTextures( 1, &buffer.textureTarget );

    m_buffers.clear();

    if( glDeleteFramebuffersEXT )
        glDeleteFramebuffersEXT( 1, &m_mainFbo );

    if( glDeleteRenderbuffersEXT )
        glDeleteRenderbuffersEXT( 1, &m_depthBuffer );

    m_initialized = false;
}


int OPENGL_COMPOSITOR::GetAntialiasSupersamplingFactor() const
{
    switch ( m_currentAntialiasingMode )
    {
    case GAL_ANTIALIASING_MODE::AA_HIGHQUALITY: return 2;
    default:                                      return 1;
    }
}

VECTOR2D OPENGL_COMPOSITOR::GetAntialiasRenderingOffset() const
{
    switch( m_currentAntialiasingMode )
    {
    case GAL_ANTIALIASING_MODE::AA_HIGHQUALITY: return VECTOR2D( 0.5, -0.5 );
    default:                                      return VECTOR2D( 0, 0 );
    }
}
