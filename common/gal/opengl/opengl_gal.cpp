/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Graphics Abstraction Layer (GAL) for OpenGL
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

#include <gal/opengl/opengl_gal.h>
#include <gal/definitions.h>

#include <wx/log.h>
#include <macros.h>
#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

#include <limits>
#include <boost/foreach.hpp>

#ifndef CALLBACK
#define CALLBACK
#endif

using namespace KiGfx;

// Prototypes
void InitTesselatorCallbacks( GLUtesselator* aTesselator );

const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };

OPENGL_GAL::OPENGL_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener,
                        wxEvtHandler* aPaintListener, bool isUseShaders, const wxString& aName ) :
    wxGLCanvas( aParent, wxID_ANY, (int*) glAttributes, wxDefaultPosition, wxDefaultSize,
                wxEXPAND, aName ),
    verticesCircle( &precomputedContainer )
{
    // Create the OpenGL-Context
    glContext       = new wxGLContext( this );
    parentWindow    = aParent;
    mouseListener   = aMouseListener;
    paintListener   = aPaintListener;

    // Set the cursor size
    initCursor( 20 );
    SetCursorColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );

    // Initialize the flags
    isDeleteSavedPixels      = true;
    isGlewInitialized        = false;
    isFrameBufferInitialized = false;
    isUseShader              = isUseShaders;
    isShaderInitialized      = false;
    isGrouping               = false;
    wxSize parentSize        = aParent->GetSize();

    isVboInitialized         = false;
    vboNeedsUpdate           = false;
    currentGroup             = NULL;
    groupCounter             = 0;
    transform                = glm::mat4( 1.0f );   // Identity matrix

    SetSize( parentSize );

    screenSize.x = parentSize.x;
    screenSize.y = parentSize.y;

    // Set grid defaults
    SetGridColor( COLOR4D( 0.3, 0.3, 0.3, 0.3 ) );
    SetCoarseGrid( 10 );
    SetGridLineWidth( 1.0 );

    // Connecting the event handlers.
    Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif

    // Tesselator initialization
    tesselator = gluNewTess();
    InitTesselatorCallbacks( tesselator );
    gluTessProperty( tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    // Compute unit semicircle & circle vertices and store them in a buffer for faster drawing
    computeCircleVbo();
}


OPENGL_GAL::~OPENGL_GAL()
{
    glFlush();

    if( glIsList( displayListSemiCircle ) )
        glDeleteLists( displayListSemiCircle, 1 );
    if( glIsList( displayListCircle ) )
        glDeleteLists( displayListCircle, 1 );

    // Delete the buffers
    if( isFrameBufferInitialized )
    {
        deleteFrameBuffer( &frameBuffer, &depthBuffer, &texture );
        deleteFrameBuffer( &frameBufferBackup, &depthBufferBackup, &textureBackup );
    }

    gluDeleteTess( tesselator );

    if( isVboInitialized )
    {
        ClearCache();
        deleteVertexBufferObjects();
    }

    delete glContext;
}


void OPENGL_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint();
}


void OPENGL_GAL::ResizeScreen( int aWidth, int aHeight )
{
    screenSize = VECTOR2D( aWidth, aHeight );

    // Delete old buffers for resizing
    if( isFrameBufferInitialized )
    {
        deleteFrameBuffer( &frameBuffer, &depthBuffer, &texture );
        deleteFrameBuffer( &frameBufferBackup, &depthBufferBackup, &textureBackup );

        // This flag is used for recreating the buffers
        isFrameBufferInitialized = false;
    }

    wxGLCanvas::SetSize( aWidth, aHeight );
}


void OPENGL_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


void OPENGL_GAL::generateFrameBuffer( GLuint* aFrameBuffer, GLuint* aDepthBuffer,
                                      GLuint* aTexture )
{
    // We need frame buffer objects for drawing the screen contents

    // Generate frame buffer and a depth buffer
    glGenFramebuffersEXT( 1, aFrameBuffer );
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, *aFrameBuffer );

    // Allocate memory for the depth buffer
    // Attach the depth buffer to the frame buffer
    glGenRenderbuffersEXT( 1, aDepthBuffer );
    glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, *aDepthBuffer );

    // Use here a size of 24 bits for the depth buffer, 8 bits for the stencil buffer
    // this is required later for anti-aliasing
    glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_STENCIL_EXT, screenSize.x,
                              screenSize.y );
    glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT,
                                  *aDepthBuffer );
    glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                  GL_RENDERBUFFER_EXT, *aDepthBuffer );

    // Generate the texture for the pixel storage
    // Attach the texture to the frame buffer
    glGenTextures( 1, aTexture );
    glBindTexture( GL_TEXTURE_2D, *aTexture );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, screenSize.x, screenSize.y, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                               *aTexture, 0 );

    // Check the status, exit if the frame buffer can't be created
    GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

    if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
    {
        wxLogError( wxT( "Can't create the frame buffer." ) );
        exit( 1 );
    }

    isFrameBufferInitialized = true;
}


void OPENGL_GAL::deleteFrameBuffer( GLuint* aFrameBuffer, GLuint* aDepthBuffer, GLuint* aTexture )
{
    glDeleteFramebuffers( 1, aFrameBuffer );
    glDeleteRenderbuffers( 1, aDepthBuffer );
    glDeleteTextures( 1, aTexture );
}


void OPENGL_GAL::initFrameBuffers()
{
    generateFrameBuffer( &frameBuffer, &depthBuffer, &texture );
    generateFrameBuffer( &frameBufferBackup, &depthBufferBackup, &textureBackup );
}


void OPENGL_GAL::initVertexBufferObjects()
{
    // Generate buffers for vertices and indices
    glGenBuffers( 1, &vboVertices );
    glGenBuffers( 1, &vboIndices );

    isVboInitialized = true;
}


void OPENGL_GAL::deleteVertexBufferObjects()
{
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glDeleteBuffers( 1, &vboVertices );
    glDeleteBuffers( 1, &vboIndices );

    isVboInitialized = false;
}


void OPENGL_GAL::SaveScreen()
{
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBufferBackup );
    glBindFramebuffer( GL_READ_FRAMEBUFFER, frameBuffer );
    glBlitFramebuffer( 0, 0, screenSize.x, screenSize.y, 0, 0, screenSize.x, screenSize.y,
                       GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                       GL_NEAREST );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer );
}


void OPENGL_GAL::RestoreScreen()
{
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer );
    glBindFramebuffer( GL_READ_FRAMEBUFFER, frameBufferBackup );
    glBlitFramebuffer( 0, 0, screenSize.x, screenSize.y, 0, 0, screenSize.x, screenSize.y,
                       GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                       GL_NEAREST );
}


void OPENGL_GAL::initGlew()
{
    // Initialize GLEW library
    GLenum err = glewInit();

    if( GLEW_OK != err )
    {
        wxLogError( wxString::FromUTF8( (char*) glewGetErrorString( err ) ) );
        exit( 1 );
    }
    else
    {
        wxLogDebug( wxString( wxT( "Status: Using GLEW " ) ) +
                    FROM_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
    }

    // Check the OpenGL version (minimum 2.1 is required)
    if( GLEW_VERSION_2_1 )
    {
        wxLogInfo( wxT( "OpenGL Version 2.1 supported." ) );
    }
    else
    {
        wxLogError( wxT( "OpenGL Version 2.1 is not supported!" ) );
        exit( 1 );
    }

    // Frame buffers have to be supported
    if( !GLEW_ARB_framebuffer_object )
    {
        wxLogError( wxT( "Framebuffer objects are not supported!" ) );
        exit( 1 );
    }

    // Vertex buffer have to be supported
    if( !GLEW_ARB_vertex_buffer_object )
    {
        wxLogError( wxT( "Vertex buffer objects are not supported!" ) );
        exit( 1 );
    }

    initVertexBufferObjects();
    computeCircleDisplayLists();

    isGlewInitialized = true;
}


void OPENGL_GAL::BeginDrawing()
{
    SetCurrent( *glContext );

    clientDC = new wxClientDC( this );

    // Initialize GLEW, FBOs & VBOs
    if( !isGlewInitialized )
    {
        initGlew();
    }

    if( !isFrameBufferInitialized )
    {
        initFrameBuffers();
    }

    // Compile the shaders
    if( !isShaderInitialized && isUseShader )
    {
        if( !shader.LoadBuiltinShader( 0, SHADER_TYPE_VERTEX ) )
        {
        	wxLogFatalError( wxT( "Cannot compile vertex shader!" ) );
        }

        if( !shader.LoadBuiltinShader( 1, SHADER_TYPE_FRAGMENT ) )
        {
        	wxLogFatalError( wxT( "Cannot compile fragment shader!" ) );
        }

        if( !shader.Link() )
        {
            wxLogFatalError( wxT( "Cannot link the shaders!" ) );
        }

        shaderAttrib = shader.GetAttribute( "attrShaderParams" );
        if( shaderAttrib == -1 )
        {
            wxLogFatalError( wxT( "Could not get the shader attribute location" ) );
        }

        isShaderInitialized = true;
    }

    // Bind the main frame buffer object - all contents are drawn there
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, frameBuffer );

    // Disable 2D Textures
    glDisable( GL_TEXTURE_2D );

    // Enable the depth buffer
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    // Setup blending, required for transparent objects
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Enable smooth lines
    glEnable( GL_LINE_SMOOTH );

    // Set up the view port
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, (GLsizei) screenSize.x, (GLsizei) screenSize.y );

    // Create the screen transformation
    glOrtho( 0, (GLint) screenSize.x, 0, (GLsizei) screenSize.y, -depthRange.x, -depthRange.y );

    glMatrixMode( GL_MODELVIEW );

    // Set up the world <-> screen transformation
    ComputeWorldScreenMatrix();
    GLdouble matrixData[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    matrixData[0]   = worldScreenMatrix.m_data[0][0];
    matrixData[1]   = worldScreenMatrix.m_data[1][0];
    matrixData[2]   = worldScreenMatrix.m_data[2][0];
    matrixData[4]   = worldScreenMatrix.m_data[0][1];
    matrixData[5]   = worldScreenMatrix.m_data[1][1];
    matrixData[6]   = worldScreenMatrix.m_data[2][1];
    matrixData[12]  = worldScreenMatrix.m_data[0][2];
    matrixData[13]  = worldScreenMatrix.m_data[1][2];
    matrixData[14]  = worldScreenMatrix.m_data[2][2];
    glLoadMatrixd( matrixData );

    // Set defaults
    SetFillColor( fillColor );
    SetStrokeColor( strokeColor );
    isDeleteSavedPixels = true;

    // If any of VBO items is dirty - recache everything
    if( vboNeedsUpdate )
        rebuildVbo();

    // Number of vertices to be drawn
    indicesSize = 0;

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIndices );
    // Discard old buffer, so we can use it again
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vboContainer.GetSize() * VBO_ITEM::IndByteSize,
                  NULL, GL_STREAM_DRAW );

    // Map the GPU memory, so we can store indices that are going to be drawn
    indicesPtr = static_cast<GLuint*>( glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY ) );
    if( indicesPtr == NULL )
    {
        wxLogError( wxT( "Could not map GPU memory" ) );
    }
}


void OPENGL_GAL::blitMainTexture( bool aIsClearFrameBuffer )
{
    // Don't use blending for the final blitting
    glDisable( GL_BLEND );

    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Switch to the main frame buffer and blit the scene
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

    if( aIsClearFrameBuffer )
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    // Enable texturing and bind the main texture
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texture );

    // Draw a full screen quad with the texture
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();

    glBegin( GL_TRIANGLES );
    glTexCoord2i( 0, 1 );
    glVertex3i( -1, -1, 0 );
    glTexCoord2i( 1, 1 );
    glVertex3i( 1, -1, 0 );
    glTexCoord2i( 1, 0 );
    glVertex3i( 1, 1, 0 );

    glTexCoord2i( 0, 1 );
    glVertex3i( -1, -1, 0 );
    glTexCoord2i( 1, 0 );
    glVertex3i( 1, 1, 0 );
    glTexCoord2i( 0, 0 );
    glVertex3i( -1, 1, 0 );
    glEnd();
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
}


void OPENGL_GAL::EndDrawing()
{
    // TODO Checking if we are using right VBOs, in other case do the binding.
    // Right now there is only one VBO, so there is no problem.

    if( !glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER ) )
    {
        wxLogError( wxT( "Unmapping indices buffer failed" ) );
    }

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    // Bind vertices data buffers
    glBindBuffer( GL_ARRAY_BUFFER, vboVertices );
    glVertexPointer( VBO_ITEM::CoordStride, GL_FLOAT, VBO_ITEM::VertByteSize, 0 );
    glColorPointer( VBO_ITEM::ColorStride, GL_UNSIGNED_BYTE, VBO_ITEM::VertByteSize,
                    (GLvoid*) VBO_ITEM::ColorByteOffset );

    // Shader parameters
    if( isUseShader )
    {
        shader.Use();
        glEnableVertexAttribArray( shaderAttrib );
        glVertexAttribPointer( shaderAttrib, VBO_ITEM::ShaderStride, GL_FLOAT, GL_FALSE,
                               VBO_ITEM::VertByteSize, (GLvoid*) VBO_ITEM::ShaderByteOffset );
    }

    glDrawElements( GL_TRIANGLES, indicesSize, GL_UNSIGNED_INT, (GLvoid*) 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
    if( isUseShader )
    {
        glDisableVertexAttribArray( shaderAttrib );
        shader.Deactivate();
    }

    // Draw the remaining contents, blit the main texture to the screen, swap the buffers
    glFlush();
    blitMainTexture( true );
    SwapBuffers();

    delete clientDC;
}


void OPENGL_GAL::rebuildVbo()
{
#ifdef __WXDEBUG__
    prof_counter totalTime;
    prof_start( &totalTime, false );
#endif /* __WXDEBUG__ */

    GLfloat* data = (GLfloat*) vboContainer.GetAllVertices();

    // Upload vertices coordinates and shader types to GPU memory
    glBindBuffer( GL_ARRAY_BUFFER, vboVertices );
    glBufferData( GL_ARRAY_BUFFER, vboContainer.GetSize() * VBO_ITEM::VertByteSize,
                  data, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Allocate the biggest possible buffer for indices
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIndices );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, vboContainer.GetSize() * VBO_ITEM::IndByteSize,
                  NULL, GL_STREAM_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    vboNeedsUpdate = false;

#ifdef __WXDEBUG__
    prof_end( &totalTime );

    wxLogDebug( wxT( "Rebuilding VBO::%d vertices / %.1f ms" ),
            vboContainer.GetSize(), (double) totalTime.value / 1000.0 );
#endif /* __WXDEBUG__ */
}


inline void OPENGL_GAL::drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength     = startEndVector.EuclideanNorm();
    double   scale          = 0.5 * lineWidth / lineLength;

    if( lineLength <= 0.0 )
        return;

    if( lineWidth * worldScale < 1.0002 && !isGrouping && !isUseShader )
    {
        // Limit the width of the line to a minimum of one pixel
        // this looks best without anti-aliasing
        scale = 0.5001 / worldScale / lineLength;
    }

    VECTOR2D perpendicularVector( -startEndVector.y * scale, startEndVector.x * scale );

    begin( GL_TRIANGLES );

    if( isUseShader && isGrouping )
    {
        glm::vec4 vector( perpendicularVector.x, perpendicularVector.y, 0.0, 0.0 );

        // If transform stack is not empty, then it means that
        // there is a transformation matrix that has to be applied
        if( !transformStack.empty() )
        {
            vector = transform * vector;
        }

        // Line width is maintained by the vertex shader
        setShader( SHADER_LINE, vector.x, vector.y, lineWidth );
        vertex3( aStartPoint.x, aStartPoint.y, layerDepth );    // v0

        setShader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        vertex3( aStartPoint.x, aStartPoint.y, layerDepth );    // v1

        setShader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        vertex3( aEndPoint.x, aEndPoint.y, layerDepth );        // v3

        setShader( SHADER_LINE, vector.x, vector.y, lineWidth );
        vertex3( aStartPoint.x, aStartPoint.y, layerDepth );    // v0

        setShader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        vertex3( aEndPoint.x, aEndPoint.y, layerDepth );        // v3

        setShader( SHADER_LINE, vector.x, vector.y, lineWidth );
        vertex3( aEndPoint.x, aEndPoint.y, layerDepth );        // v2
    }
    else
    {
        // Compute the edge points of the line
        VECTOR2D v0 = aStartPoint + perpendicularVector;
        VECTOR2D v1 = aStartPoint - perpendicularVector;
        VECTOR2D v2 = aEndPoint + perpendicularVector;
        VECTOR2D v3 = aEndPoint - perpendicularVector;

        vertex3( v0.x, v0.y, layerDepth );
        vertex3( v1.x, v1.y, layerDepth );
        vertex3( v3.x, v3.y, layerDepth );

        vertex3( v0.x, v0.y, layerDepth );
        vertex3( v3.x, v3.y, layerDepth );
        vertex3( v2.x, v2.y, layerDepth );
    }

    end();
}


void OPENGL_GAL::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineAngle      = startEndVector.Angle();

    if( isFillEnabled )
    {
        // Filled tracks
        color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        SetLineWidth( aWidth );
        drawLineQuad( aStartPoint, aEndPoint );

        // Draw line caps
        drawFilledSemiCircle( aStartPoint, aWidth / 2, lineAngle + M_PI / 2 );
        drawFilledSemiCircle( aEndPoint,   aWidth / 2, lineAngle - M_PI / 2 );
    }
    else
    {
        // Outlined tracks
        double lineLength = startEndVector.EuclideanNorm();

        color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        Save();

        translate3( aStartPoint.x, aStartPoint.y, 0.0 );
        Rotate( lineAngle );

        drawLineQuad( VECTOR2D( 0.0,         aWidth / 2.0 ),
                      VECTOR2D( lineLength,  aWidth / 2.0 ) );

        drawLineQuad( VECTOR2D( 0.0,        -aWidth / 2.0 ),
                      VECTOR2D( lineLength, -aWidth / 2.0 ) );

        // Draw line caps
        drawStrokedSemiCircle( VECTOR2D( 0.0, 0.0 ),        ( aWidth + lineWidth ) / 2, M_PI / 2 );
        drawStrokedSemiCircle( VECTOR2D( lineLength, 0.0 ), ( aWidth + lineWidth ) / 2, -M_PI / 2 );

        Restore();
    }
}


unsigned int OPENGL_GAL::getGroupNumber()
{
    wxASSERT_MSG( groups.size() < std::numeric_limits<unsigned int>::max(),
            wxT( "There are no free slots to store a group" ) );

    while( groups.find( groupCounter ) != groups.end() )
    {
        groupCounter++;
    }

    return groupCounter++;
}


void OPENGL_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    const VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double lineAngle = startEndVector.Angle();

    drawLineQuad( aStartPoint, aEndPoint );

    // Line caps
    drawFilledSemiCircle( aStartPoint, lineWidth / 2, lineAngle + M_PI / 2 );
    drawFilledSemiCircle( aEndPoint, lineWidth / 2, lineAngle - M_PI / 2 );
}


void OPENGL_GAL::DrawPolyline( std::deque<VECTOR2D>& aPointList )
{
    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    // Start from the second point
    for( it++; it != aPointList.end(); it++ )
    {
        const VECTOR2D startEndVector = ( *it - *( it - 1 ) );
        double lineAngle = startEndVector.Angle();

        drawLineQuad( *( it - 1 ), *it );

        // There is no need to draw line caps on both ends of polyline's segments
        drawFilledSemiCircle( *( it - 1 ), lineWidth / 2, lineAngle + M_PI / 2 );
    }

    // ..and now - draw the ending cap
    const VECTOR2D startEndVector = ( *( it - 1 ) - *( it - 2 ) );
    double lineAngle = startEndVector.Angle();
    drawFilledSemiCircle( *( it - 1 ), lineWidth / 2, lineAngle - M_PI / 2 );
}


void OPENGL_GAL::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // Compute the diagonal points of the rectangle
    VECTOR2D diagonalPointA( aEndPoint.x, aStartPoint.y );
    VECTOR2D diagonalPointB( aStartPoint.x, aEndPoint.y );

    // Stroke the outline
    if( isStrokeEnabled )
    {
        color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        std::deque<VECTOR2D> pointList;
        pointList.push_back( aStartPoint );
        pointList.push_back( diagonalPointA );
        pointList.push_back( aEndPoint );
        pointList.push_back( diagonalPointB );
        pointList.push_back( aStartPoint );
        DrawPolyline( pointList );
    }

    // Fill the rectangle
    if( isFillEnabled )
    {
        setShader( SHADER_NONE );
        color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        begin( GL_TRIANGLES );
        vertex3( aStartPoint.x, aStartPoint.y, layerDepth );
        vertex3( diagonalPointA.x, diagonalPointA.y, layerDepth );
        vertex3( aEndPoint.x, aEndPoint.y, layerDepth );

        vertex3( aStartPoint.x, aStartPoint.y, layerDepth );
        vertex3( aEndPoint.x, aEndPoint.y, layerDepth );
        vertex3( diagonalPointB.x, diagonalPointB.y, layerDepth );
        end();
    }
}


void OPENGL_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    if( isUseShader && isGrouping )
    {
        if( isFillEnabled )
        {
            color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

            /* Draw a triangle that contains the circle, then shade it leaving only the circle.
               Parameters given to setShader are relative coordinates of the triangle's vertices.
               Shader uses this coordinates to determine if fragments are inside the circle or not.
                    v2
                    /\
                   //\\
               v0 /_\/_\ v1
            */
            setShader( SHADER_FILLED_CIRCLE, -sqrt( 3.0f ), -1.0f );
            vertex3( aCenterPoint.x - aRadius * sqrt( 3.0f ),                           // v0
                     aCenterPoint.y - aRadius, layerDepth );

            setShader( SHADER_FILLED_CIRCLE, sqrt( 3.0f ), -1.0f );
            vertex3( aCenterPoint.x + aRadius * sqrt( 3.0f ),                           // v1
                     aCenterPoint.y - aRadius, layerDepth );

            setShader( SHADER_FILLED_CIRCLE, 0.0f, 2.0f );
            vertex3( aCenterPoint.x, aCenterPoint.y + aRadius * 2.0f, layerDepth );     // v2
        }

        if( isStrokeEnabled )
        {
            color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            /* Draw a triangle that contains the circle, then shade it leaving only the circle.
               Parameters given to setShader are relative coordinates of the triangle's vertices
               and the line width. Shader uses this coordinates to determine if fragments are inside
               the circle or not. Width parameter has to be passed as a ratio of inner radius
               to outer radius.
                    v2
                    /\
                   //\\
               v0 /_\/_\ v1
            */
            float outerRadius = aRadius + ( lineWidth / 2.0f );
            float innerRadius = aRadius - ( lineWidth / 2.0f );
            float relWidth = innerRadius / outerRadius;

            setShader( SHADER_STROKED_CIRCLE, -sqrt( 3.0f ), -1.0f, relWidth );
            vertex3( aCenterPoint.x - outerRadius * sqrt( 3.0f ),                           // v0
                     aCenterPoint.y - outerRadius, layerDepth );

            setShader( SHADER_STROKED_CIRCLE, sqrt( 3.0f ), -1.0f, relWidth );
            vertex3( aCenterPoint.x + outerRadius * sqrt( 3.0f ),                           // v1
                     aCenterPoint.y - outerRadius, layerDepth );

            setShader( SHADER_STROKED_CIRCLE, 0.0f, 2.0f, relWidth );
            vertex3( aCenterPoint.x,                                                        // v2
                     aCenterPoint.y + outerRadius * 2.0f, layerDepth );
        }

        return;
    }

    if( isStrokeEnabled )
    {
        // Compute the factors for the unit circle
        double outerScale = lineWidth / aRadius / 2;
        double innerScale = -outerScale;
        outerScale += 1.0;
        innerScale += 1.0;

        if( innerScale < outerScale )
        {
            // Draw the outline
            VBO_VERTEX* circle = verticesCircle.GetVertices();
            int next;

            color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            Save();

            translate3( aCenterPoint.x, aCenterPoint.y, 0.0 );
            Scale( VECTOR2D( aRadius, aRadius ) );

            begin( GL_TRIANGLES );

            for( int i = 0; i < 3 * CIRCLE_POINTS; ++i )
            {
                // verticesCircle contains precomputed circle points interleaved with vertex
                // (0,0,0), so filled circles can be drawn as consecutive triangles, ie:
                // { 0,a,b, 0,c,d, 0,e,f, 0,g,h, ... }
                // where letters stand for consecutive circle points and 0 for (0,0,0) vertex.

                // We have to skip all (0,0,0) vertices (every third vertex)
                if( i % 3 == 0)
                {
                    i++;
                    // Depending on the vertex, next circle point may be stored in the next vertex..
                    next = i + 1;
                }
                else
                {
                    // ..or 2 vertices away (in case it is preceded by (0,0,0) vertex)
                    next = i + 2;
                }

                vertex3( circle[i].x * innerScale, circle[i].y * innerScale, layerDepth );
                vertex3( circle[i].x * outerScale, circle[i].y * outerScale, layerDepth );
                vertex3( circle[next].x * innerScale, circle[next].y * innerScale, layerDepth );

                vertex3( circle[i].x * outerScale, circle[i].y * outerScale, layerDepth );
                vertex3( circle[next].x * outerScale, circle[next].y * outerScale, layerDepth );
                vertex3( circle[next].x * innerScale, circle[next].y * innerScale, layerDepth );
            }

            end();

            Restore();
        }
    }

    // Filled circles are easy to draw by using the stored display list, scaling and translating
    if( isFillEnabled )
    {
        color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        Save();
        translate3( aCenterPoint.x, aCenterPoint.y, layerDepth );
        Scale( VECTOR2D( aRadius, aRadius ) );

        if( isGrouping )
        {
            currentGroup->PushVertices( verticesCircle.GetVertices(), CIRCLE_POINTS * 3 );
        }
        else
        {
            glCallList( displayListCircle );
        }

        Restore();
    }
}


void OPENGL_GAL::drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
        if( isFillEnabled )
        {
            color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
            drawFilledSemiCircle( aCenterPoint, aRadius, aAngle );
        }

        if( isStrokeEnabled )
        {
            color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
            drawStrokedSemiCircle( aCenterPoint, aRadius, aAngle );
        }
}


void OPENGL_GAL::drawFilledSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
    if( isUseShader && isGrouping )
    {
        Save();
        Translate( aCenterPoint );
        Rotate( aAngle );

        /* Draw a triangle that contains the semicircle, then shade it to leave only the semicircle.
                       Parameters given to setShader are relative coordinates of the triangle's vertices.
                       Shader uses this coordinates to determine if fragments are inside the semicircle or not.
                            v2
                            /\
                           /__\
                       v0 //__\\ v1
         */
        setShader( SHADER_FILLED_CIRCLE, -3.0f / sqrt( 3.0f ), 0.0f );
        vertex3( -aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );                // v0

        setShader( SHADER_FILLED_CIRCLE, 3.0f / sqrt( 3.0f ), 0.0f );
        vertex3( aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );                 // v1

        setShader( SHADER_FILLED_CIRCLE, 0.0f, 2.0f );
        vertex3( 0.0f, aRadius * 2.0f, layerDepth );                                // v2

        Restore();
    }
    else
    {
        Save();
        translate3( aCenterPoint.x, aCenterPoint.y, layerDepth );
        Scale( VECTOR2D( aRadius, aRadius ) );
        Rotate( aAngle );

        if( isGrouping )
        {
            // It is enough just to push just a half of the circle vertices to make a semicircle
            currentGroup->PushVertices( verticesCircle.GetVertices(), CIRCLE_POINTS / 2 * 3 );
        }
        else
        {
            glCallList( displayListSemiCircle );
        }

        Restore();
    }
}


void OPENGL_GAL::drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
    if( isUseShader && isGrouping )
    {
        Save();
        Translate( aCenterPoint );
        Rotate( aAngle );

        /* Draw a triangle that contains the semicircle, then shade it to leave only the semicircle.
                   Parameters given to setShader are relative coordinates of the triangle's vertices
                   and the line width. Shader uses this coordinates to determine if fragments are inside
                   the semicircle or not. Width parameter has to be passed as a ratio of inner radius
                   to outer radius.
                        v2
                        /\
                       /__\
                   v0 //__\\ v1
         */
        float outerRadius = aRadius;
        float innerRadius = aRadius - lineWidth;
        float relWidth = innerRadius / outerRadius;

        setShader( SHADER_STROKED_CIRCLE, -3.0f / sqrt( 3.0f ), 0.0f, relWidth );
        vertex3( -aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );                // v0

        setShader( SHADER_STROKED_CIRCLE, 3.0f / sqrt( 3.0f ), 0.0f, relWidth );
        vertex3( aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );                 // v1

        setShader( SHADER_STROKED_CIRCLE, 0.0f, 2.0f, relWidth );
        vertex3( 0.0f, aRadius * 2.0f, layerDepth );                                // v2

        Restore();
    }
    else
    {
        // Compute the factors for the unit circle
        double innerScale = 1.0 - lineWidth / aRadius;

        Save();
        translate3( aCenterPoint.x, aCenterPoint.y, layerDepth );
        Scale( VECTOR2D( aRadius, aRadius ) );
        Rotate( aAngle );

        // Draw the outline
        VBO_VERTEX* circle = verticesCircle.GetVertices();
        int next;

        begin( GL_TRIANGLES );

        for( int i = 0; i < ( 3 * CIRCLE_POINTS ) / 2; ++i )
        {
            // verticesCircle contains precomputed circle points interleaved with vertex
            // (0,0,0), so filled circles can be drawn as consecutive triangles, ie:
            // { 0,a,b, 0,c,d, 0,e,f, 0,g,h, ... }
            // where letters stand for consecutive circle points and 0 for (0,0,0) vertex.

            // We have to skip all (0,0,0) vertices (every third vertex)
            if( i % 3 == 0 )
            {
                i++;
                // Depending on the vertex, next circle point may be stored in the next vertex..
                next = i + 1;
            }
            else
            {
                // ..or 2 vertices away (in case it is preceded by (0,0,0) vertex)
                next = i + 2;
            }

            vertex3( circle[i].x * innerScale,      circle[i].y * innerScale,       0.0 );
            vertex3( circle[i].x,                   circle[i].y,                    0.0 );
            vertex3( circle[next].x * innerScale,   circle[next].y * innerScale,    0.0 );

            vertex3( circle[i].x,                   circle[i].y,                    0.0 );
            vertex3( circle[next].x,                circle[next].y,                 0.0 );
            vertex3( circle[next].x * innerScale,   circle[next].y * innerScale,    0.0 );
        }

        end();

        Restore();
    }
}


// FIXME Optimize
void OPENGL_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                          double aEndAngle )
{
    if( aRadius <= 0 )
    {
        return;
    }

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    VECTOR2D startPoint( cos( aStartAngle ), sin( aStartAngle ) );
    VECTOR2D endPoint( cos( aEndAngle ), sin( aEndAngle ) );
    VECTOR2D startEndPoint = startPoint + endPoint;
    VECTOR2D middlePoint   = 0.5 * startEndPoint;

    Save();
    translate3( aCenterPoint.x, aCenterPoint.y, layerDepth );

    if( isStrokeEnabled )
    {
        if( isUseShader )
        {
            double alphaIncrement = 2.0 * M_PI / CIRCLE_POINTS;
            color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            VECTOR2D p( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
            double alpha;
            for( alpha = aStartAngle + alphaIncrement; alpha < aEndAngle; alpha += alphaIncrement )
            {
                VECTOR2D p_next( cos( alpha ) * aRadius, sin( alpha ) * aRadius );
                DrawLine( p, p_next );

                p = p_next;
            }

            // Draw the last missing part
            if( alpha != aEndAngle )
            {
                VECTOR2D p_last( cos( aEndAngle ) * aRadius, sin( aEndAngle ) * aRadius );
                DrawLine( p, p_last );
            }
        }
        else
        {
            Scale( VECTOR2D( aRadius, aRadius ) );

            double outerScale = lineWidth / aRadius / 2;
            double innerScale = -outerScale;

            outerScale += 1.0;
            innerScale += 1.0;

            double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;
            color4( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            begin( GL_TRIANGLES );

            for( double alpha = aStartAngle; alpha < aEndAngle; )
            {
                double v0[] = { cos( alpha ) * innerScale, sin( alpha ) * innerScale };
                double v1[] = { cos( alpha ) * outerScale, sin( alpha ) * outerScale };

                alpha += alphaIncrement;

                if( alpha > aEndAngle )
                    alpha = aEndAngle;

                double v2[] = { cos( alpha ) * innerScale, sin( alpha ) * innerScale };
                double v3[] = { cos( alpha ) * outerScale, sin( alpha ) * outerScale };

                vertex3( v0[0], v0[1], 0.0 );
                vertex3( v1[0], v1[1], 0.0 );
                vertex3( v2[0], v2[1], 0.0 );

                vertex3( v1[0], v1[1], 0.0 );
                vertex3( v3[0], v3[1], 0.0 );
                vertex3( v2[0], v2[1], 0.0 );
            }

            end();

            // Draw line caps
            drawFilledSemiCircle( startPoint, lineWidth / aRadius / 2.0, aStartAngle + M_PI );
            drawFilledSemiCircle( endPoint, lineWidth / aRadius / 2.0, aEndAngle );
        }
    }

    if( isFillEnabled )
    {
        double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;
        double alpha;
        color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        begin( GL_TRIANGLES );

        for( alpha = aStartAngle; ( alpha + alphaIncrement ) < aEndAngle; )
        {
            vertex3( middlePoint.x, middlePoint.y,  0.0 );
            vertex3( cos( alpha ),  sin( alpha ),   0.0 );
            alpha += alphaIncrement;
            vertex3( cos( alpha ),  sin( alpha ),   0.0 );
        }

        vertex3( middlePoint.x, middlePoint.y,  0.0 );
        vertex3( cos( alpha ),  sin( alpha ),   0.0 );
        vertex3( endPoint.x,    endPoint.y,     0.0 );
        end();
    }

    Restore();
}


struct OGLPOINT
{
    OGLPOINT() :
        x( 0.0 ), y( 0.0 ), z( 0.0 )
    {}

    OGLPOINT( const char* fastest )
    {
        // do nothing for fastest speed, and keep inline
    }

    OGLPOINT( const VECTOR2D& aPoint ) :
        x( aPoint.x ), y( aPoint.y ), z( 0.0 )
    {}

    OGLPOINT& operator=( const VECTOR2D& aPoint )
    {
        x = aPoint.x;
        y = aPoint.y;
        z = 0.0;
        return *this;
    }

    GLdouble x;
    GLdouble y;
    GLdouble z;
};


void OPENGL_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    // Any non convex polygon needs to be tesselated
    // for this purpose the GLU standard functions are used

    setShader( SHADER_NONE );

    typedef std::vector<OGLPOINT> OGLPOINTS;

    // Do only one heap allocation, can do because we know size in advance.
    // std::vector is then fastest
    OGLPOINTS vertexList( aPointList.size(), OGLPOINT( "fastest" ) );

    glNormal3d( 0.0, 0.0, 1.0 );
    color4( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    glShadeModel( GL_FLAT );

    TessParams params = { currentGroup, tessIntersects };
    gluTessBeginPolygon( tesselator, &params );
    gluTessBeginContour( tesselator );

    // use operator=( const POINTS& )
    copy( aPointList.begin(), aPointList.end(), vertexList.begin() );

    for( OGLPOINTS::iterator it = vertexList.begin(); it != vertexList.end(); it++ )
    {
        it->z = layerDepth;
        gluTessVertex( tesselator, &it->x, &it->x );
    }

    gluTessEndContour( tesselator );
    gluTessEndPolygon( tesselator );

    // Free allocated intersecting points
    std::vector<GLdouble*>::iterator it, it_end;
    for( it = tessIntersects.begin(), it_end = tessIntersects.end(); it < it_end; ++it )
    {
        delete[] *it;
    }
    tessIntersects.clear();

    // vertexList destroyed here
}


void OPENGL_GAL::DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                            const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint )
{
    // FIXME The drawing quality needs to be improved
    // FIXME Perhaps choose a quad/triangle strip instead?
    // FIXME Brute force method, use a better (recursive?) algorithm

    std::deque<VECTOR2D> pointList;

    double t  = 0.0;
    double dt = 1.0 / (double) CURVE_POINTS;

    for( int i = 0; i <= CURVE_POINTS; i++ )
    {
        double omt  = 1.0 - t;
        double omt2 = omt * omt;
        double omt3 = omt * omt2;
        double t2   = t * t;
        double t3   = t * t2;

        VECTOR2D vertex = omt3 * aStartPoint + 3.0 * t * omt2 * aControlPointA
                          + 3.0 * t2 * omt * aControlPointB + t3 * aEndPoint;

        pointList.push_back( vertex );

        t += dt;
    }

    DrawPolyline( pointList );
}


void OPENGL_GAL::SetStrokeColor( const COLOR4D& aColor )
{
    isSetAttributes = true;
    strokeColor     = aColor;

    // This is the default drawing color
    color4( aColor.r, aColor.g, aColor.b, aColor.a );
}


void OPENGL_GAL::SetFillColor( const COLOR4D& aColor )
{
    isSetAttributes = true;
    fillColor = aColor;
}


void OPENGL_GAL::SetBackgroundColor( const COLOR4D& aColor )
{
    isSetAttributes = true;
    backgroundColor = aColor;
}


void OPENGL_GAL::SetLineWidth( double aLineWidth )
{
    isSetAttributes = true;
    lineWidth = aLineWidth;
}


void OPENGL_GAL::ClearScreen()
{
    // Clear screen
    glClearColor( backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}


void OPENGL_GAL::Transform( MATRIX3x3D aTransformation )
{
    GLdouble matrixData[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    matrixData[0]   = aTransformation.m_data[0][0];
    matrixData[1]   = aTransformation.m_data[1][0];
    matrixData[2]   = aTransformation.m_data[2][0];
    matrixData[4]   = aTransformation.m_data[0][1];
    matrixData[5]   = aTransformation.m_data[1][1];
    matrixData[6]   = aTransformation.m_data[2][1];
    matrixData[12]  = aTransformation.m_data[0][2];
    matrixData[13]  = aTransformation.m_data[1][2];
    matrixData[14]  = aTransformation.m_data[2][2];

    glMultMatrixd( matrixData );
}


void OPENGL_GAL::Rotate( double aAngle )
{
    if( isGrouping )
    {
        transform = glm::rotate( transform, (float) aAngle, glm::vec3( 0, 0, 1 ) );
    }
    else
    {
        glRotated( aAngle * ( 360 / ( 2 * M_PI ) ), 0, 0, 1 );
    }
}


void OPENGL_GAL::Translate( const VECTOR2D& aVector )
{
    if( isGrouping )
    {
        transform = glm::translate( transform, glm::vec3( aVector.x, aVector.y, 0 ) );
    }
    else
    {
        glTranslated( aVector.x, aVector.y, 0 );
    }
}


void OPENGL_GAL::Scale( const VECTOR2D& aScale )
{
    if( isGrouping )
    {
        transform = glm::scale( transform, glm::vec3( aScale.x, aScale.y, 0 ) );
    }
    else
    {
        glScaled( aScale.x, aScale.y, 0 );
    }
}


void OPENGL_GAL::Flush()
{
    glFlush();
}


void OPENGL_GAL::Save()
{
    if( isGrouping )
    {
        transformStack.push( transform );
        vboContainer.SetTransformMatrix( &transform );
    }
    else
    {
        glPushMatrix();
    }
}


void OPENGL_GAL::Restore()
{
    if( isGrouping )
    {
        transform = transformStack.top();
        transformStack.pop();

        if( transformStack.empty() )
        {
            // Disable transforming, as the selected matrix is identity
            vboContainer.SetTransformMatrix( NULL );
        }
    }
    else
    {
        glPopMatrix();
    }
}


int OPENGL_GAL::BeginGroup()
{
    isGrouping = true;

    // There is a new group that is not in VBO yet
    vboNeedsUpdate = true;

    // Save the pointer for caching the current item
    currentGroup = new VBO_ITEM( &vboContainer );
    int groupNumber = getGroupNumber();
    groups.insert( std::make_pair( groupNumber, currentGroup ) );

    return groupNumber;
}


void OPENGL_GAL::EndGroup()
{
    currentGroup->Finish();

    isGrouping = false;
}


void OPENGL_GAL::ClearCache()
{
    BOOST_FOREACH( GroupsMap::value_type it, groups )
    {
        delete it.second;
    }

    groups.clear();
}


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    delete groups[aGroupNumber];
    groups.erase( aGroupNumber );

    vboNeedsUpdate = true;
}


void OPENGL_GAL::DrawGroup( int aGroupNumber )
{
    int size = groups[aGroupNumber]->GetSize();
    int offset = groups[aGroupNumber]->GetOffset();

    // Copy indices of items that should be drawn to GPU memory
    for( int i = offset; i < offset + size; *indicesPtr++ = i++ );

    indicesSize += size;
}


void OPENGL_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    groups[aGroupNumber]->ChangeColor( aNewColor );
    vboNeedsUpdate = true;
}


void OPENGL_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    groups[aGroupNumber]->ChangeDepth( aDepth );
    vboNeedsUpdate = true;
}


void OPENGL_GAL::computeCircleVbo()
{
    // Compute the circle points for a given number of segments
    // Insert in a display list and a vector
    const VBO_VERTEX v0 = { 0.0f, 0.0f, 0.0f };

    for( int i = 0; i < CIRCLE_POINTS; i++ )
    {
        const VBO_VERTEX v1 = {
            cos( 2.0 * M_PI / CIRCLE_POINTS * i ),          // x
            sin( 2.0 * M_PI / CIRCLE_POINTS * i ),          // y
            0.0f                                            // z
        };
        const VBO_VERTEX v2 = {
            cos( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ),  // x
            sin( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ),  // y
            0.0f                                            // z
        };

        verticesCircle.PushVertex( &v0 );
        verticesCircle.PushVertex( &v1 );
        verticesCircle.PushVertex( &v2 );
    }

    verticesCircle.Finish();
}


void OPENGL_GAL::computeCircleDisplayLists()
{
    // Circle display list
    displayListCircle = glGenLists( 1 );
    glNewList( displayListCircle, GL_COMPILE );

    glBegin( GL_TRIANGLES );
    for( int i = 0; i < CIRCLE_POINTS; ++i )
    {
        glVertex2d( 0.0, 0.0 );
        glVertex2d( cos( 2.0 * M_PI / CIRCLE_POINTS * i ),
                    sin( 2.0 * M_PI / CIRCLE_POINTS * i ) );
        glVertex2d( cos( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ),
                    sin( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ) );
    }
    glEnd();
    glEndList();

    // Semicircle display list
    displayListSemiCircle = glGenLists( 1 );
    glNewList( displayListSemiCircle, GL_COMPILE );

    glBegin( GL_TRIANGLES );
    for( int i = 0; i < CIRCLE_POINTS / 2; ++i )
    {
        glVertex2d( 0.0, 0.0 );
        glVertex2d( cos( 2.0 * M_PI / CIRCLE_POINTS * i ),
                    sin( 2.0 * M_PI / CIRCLE_POINTS * i ) );
        glVertex2d( cos( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ),
                    sin( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) ) );
    }
    glEnd();
    glEndList();
}


void OPENGL_GAL::ComputeWorldScreenMatrix()
{
    ComputeWorldScale();

    worldScreenMatrix.SetIdentity();

    MATRIX3x3D translation;
    translation.SetIdentity();
    translation.SetTranslation( 0.5 * screenSize );

    MATRIX3x3D scale;
    scale.SetIdentity();
    scale.SetScale( VECTOR2D( worldScale, worldScale ) );

    MATRIX3x3D flip;
    flip.SetIdentity();
    flip.SetScale( VECTOR2D( 1.0, 1.0 ) );

    MATRIX3x3D lookat;
    lookat.SetIdentity();
    lookat.SetTranslation( -lookAtPoint );

    worldScreenMatrix = translation * flip * scale * lookat * worldScreenMatrix;
}


// -------------------------------------
// Callback functions for the tesselator
// -------------------------------------

// Compare Redbook Chapter 11


void CALLBACK VertexCallback( GLvoid* aVertexPtr, void* aData )
{
    GLdouble* vertex = static_cast<GLdouble*>( aVertexPtr );
    OPENGL_GAL::TessParams* param = static_cast<OPENGL_GAL::TessParams*>( aData );
    VBO_ITEM* vboItem = param->vboItem;

    if( vboItem )
    {
        VBO_VERTEX newVertex = { vertex[0], vertex[1], vertex[2] };
        vboItem->PushVertex( &newVertex );
    }
    else
    {
        glVertex3dv( vertex );
    }
}


void CALLBACK CombineCallback( GLdouble coords[3],
                               GLdouble* vertex_data[4],
                               GLfloat weight[4], GLdouble** dataOut, void* aData )
{
    GLdouble* vertex = new GLdouble[3];
    OPENGL_GAL::TessParams* param = static_cast<OPENGL_GAL::TessParams*>( aData );

    // Save the pointer so we can delete it later
    param->intersectPoints.push_back( vertex );

    memcpy( vertex, coords, 3 * sizeof(GLdouble) );

    *dataOut = vertex;
}


void CALLBACK EdgeCallback(void)
{
    // This callback is needed to force GLU tesselator to use triangles only
    return;
}


void CALLBACK BeginCallback( GLenum aWhich, void* aData )
{
    if( !aData )
        glBegin( aWhich );
}


void CALLBACK EndCallback( void* aData )
{
    if( !aData )
        glEnd();
}


void CALLBACK ErrorCallback( GLenum aErrorCode )
{
    const GLubyte* estring;

    estring = gluErrorString( aErrorCode );
    wxLogError( wxT( "Tessellation Error: %s" ), (char*) estring );
}


void InitTesselatorCallbacks( GLUtesselator* aTesselator )
{
    gluTessCallback( aTesselator, GLU_TESS_VERTEX_DATA,  ( void (CALLBACK*)() )VertexCallback );
    gluTessCallback( aTesselator, GLU_TESS_COMBINE_DATA, ( void (CALLBACK*)() )CombineCallback );
    gluTessCallback( aTesselator, GLU_TESS_EDGE_FLAG,    ( void (CALLBACK*)() )EdgeCallback );
    gluTessCallback( aTesselator, GLU_TESS_BEGIN_DATA,   ( void (CALLBACK*)() )BeginCallback );
    gluTessCallback( aTesselator, GLU_TESS_END_DATA,     ( void (CALLBACK*)() )EndCallback );
    gluTessCallback( aTesselator, GLU_TESS_ERROR_DATA,   ( void (CALLBACK*)() )ErrorCallback );
}


// ---------------
// Cursor handling
// ---------------


void OPENGL_GAL::initCursor( int aCursorSize )
{
    cursorSize = aCursorSize;
}


VECTOR2D OPENGL_GAL::ComputeCursorToWorld( const VECTOR2D& aCursorPosition )
{
    VECTOR2D cursorPosition = aCursorPosition;
    cursorPosition.y = screenSize.y - aCursorPosition.y;
    MATRIX3x3D inverseMatrix = worldScreenMatrix.Inverse();
    VECTOR2D   cursorPositionWorld = inverseMatrix * cursorPosition;

    return cursorPositionWorld;
}


void OPENGL_GAL::DrawCursor( VECTOR2D aCursorPosition )
{
    SetCurrent( *glContext );

    // Draw the cursor on the surface
    VECTOR2D cursorPositionWorld = ComputeCursorToWorld( aCursorPosition );

    cursorPositionWorld.x = round( cursorPositionWorld.x / gridSize.x ) * gridSize.x;
    cursorPositionWorld.y = round( cursorPositionWorld.y / gridSize.y ) * gridSize.y;

    aCursorPosition = worldScreenMatrix * cursorPositionWorld;

    // Switch to the main frame buffer and blit the scene
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glLoadIdentity();

    blitMainTexture( false );

    glDisable( GL_TEXTURE_2D );
    glColor4d( cursorColor.r, cursorColor.g, cursorColor.b, cursorColor.a );

    glBegin( GL_TRIANGLES );

    glVertex3f( (int) ( aCursorPosition.x - cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y ), depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x + cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y ), depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x + cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y + 1 ), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x - cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y ), depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x - cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y + 1), depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x + cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y + 1 ), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x ),
                (int) ( aCursorPosition.y - cursorSize / 2 ) + 1, depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x  ),
                (int) ( aCursorPosition.y + cursorSize / 2 ) + 1, depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x  ) + 1,
                (int) ( aCursorPosition.y + cursorSize / 2 ) + 1, depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x ),
                (int) ( aCursorPosition.y - cursorSize / 2 ) + 1, depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x ) + 1,
                (int) ( aCursorPosition.y - cursorSize / 2 ) + 1, depthRange.x );
    glVertex3f( (int) ( aCursorPosition.x  ) + 1,
                (int) ( aCursorPosition.y + cursorSize / 2 ) + 1, depthRange.x );
    glEnd();

    // Blit the current screen contents
    SwapBuffers();
}


void OPENGL_GAL::DrawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // We check, if we got a horizontal or a vertical grid line and compute the offset
    VECTOR2D perpendicularVector;

    if( aStartPoint.x == aEndPoint.x )
    {
        // Vertical grid line
        perpendicularVector = VECTOR2D( 0.5 * lineWidth, 0 );
    }
    else
    {
        // Horizontal grid line
        perpendicularVector = VECTOR2D( 0, 0.5 * lineWidth );
    }

    // Now we compute the edge points of the quad
    VECTOR2D point1 = aStartPoint + perpendicularVector;
    VECTOR2D point2 = aStartPoint - perpendicularVector;
    VECTOR2D point3 = aEndPoint + perpendicularVector;
    VECTOR2D point4 = aEndPoint - perpendicularVector;

    // Set color
    glColor4d( gridColor.r, gridColor.g, gridColor.b, gridColor.a );

    // Draw the quad for the grid line
    glBegin( GL_TRIANGLES );
    double gridDepth = depthRange.y * 0.75;
    glVertex3d( point1.x, point1.y, gridDepth );
    glVertex3d( point2.x, point2.y, gridDepth );
    glVertex3d( point4.x, point4.y, gridDepth );

    glVertex3d( point1.x, point1.y, gridDepth );
    glVertex3d( point4.x, point4.y, gridDepth );
    glVertex3d( point3.x, point3.y, gridDepth );
    glEnd();
}


bool OPENGL_GAL::Show( bool aShow )
{
    bool s = wxGLCanvas::Show( aShow );

    if( aShow )
        wxGLCanvas::Raise();

    return s;
}
