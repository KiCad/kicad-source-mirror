/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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
#include <gal/opengl/shader.h>
#include <gal/definitions.h>

#include <wx/log.h>
#include <macros.h>

#ifndef CALLBACK
#define CALLBACK
#endif

using namespace KiGfx;

// Prototypes
void InitTesselatorCallbacks( GLUtesselator* aTesselator );

// FIXME Checking of attributes

// #if defined(__WXGTK__)
const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };
// #elif defined(__WXMSW__)
// #define glAttributes NULL
// #endif


OPENGL_GAL::OPENGL_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener,
                        wxEvtHandler* aPaintListener, bool isUseShaders, const wxString& aName ) :
    wxGLCanvas( aParent, wxID_ANY, (int*) glAttributes, wxDefaultPosition, wxDefaultSize,
                wxEXPAND, aName )
{
    // Create the OpenGL-Context
    glContext = new wxGLContext( this );
    parentWindow    = aParent;
    mouseListener   = aMouseListener;
    paintListener   = aPaintListener;

    // Set the cursor size
    initCursor( 20 );
    SetCursorColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );

    // Initialize the flags
    isCreated                = false;
    isDeleteSavedPixels      = true;
    isGlewInitialized        = false;
    isFrameBufferInitialized = false;
    isUseShader              = isUseShaders;
    isShaderInitialized      = false;
    isGroupStarted           = false;
    shaderPath               = "../../../gal/opengl/shader/";
    wxSize parentSize        = aParent->GetSize();

    SetSize( parentSize );

    screenSize.x = parentSize.x;
    screenSize.y = parentSize.y;

    currentShader = -1;

    // Set grid defaults
    SetGridColor( COLOR4D( 0.3, 0.3, 0.3, 0.3 ) );
    SetCoarseGrid( 10 );
    SetGridLineWidth( 1.0 );

    // Connecting the event handlers.
    Connect( wxEVT_SIZE, wxSizeEventHandler( OPENGL_GAL::onSize ) );
    Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
}


OPENGL_GAL::~OPENGL_GAL()
{
    glFlush();

    // Delete the stored display lists
    for( std::deque<GLuint>::iterator group = displayListsGroup.begin();
         group != displayListsGroup.end(); group++ )
    {
        glDeleteLists( *group, 1 );
    }

    // Delete the buffers
    if( isFrameBufferInitialized )
    {
        deleteFrameBuffer( &frameBuffer, &depthBuffer, &texture );
        deleteFrameBuffer( &frameBufferBackup, &depthBufferBackup, &textureBackup );
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


void OPENGL_GAL::onSize( wxSizeEvent& aEvent )
{
    ResizeScreen( aEvent.GetSize().x, aEvent.GetSize().y );
    PostPaint();
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
                    FROM_UTF8( (char*) glewGetString( GLEW_VERSION ) )  );
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

    // Compute the unit circles, used for speed up of the circle drawing
    computeUnitCircle();
    computeUnitSemiCircle();
    computeUnitArcs();

    isGlewInitialized = true;
}


void OPENGL_GAL::BeginDrawing()
{
    SetCurrent( *glContext );

    clientDC = new wxClientDC( this );

    // Initialize GLEW & FBOs
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
        std::string shaderNames[SHADER_NUMBER] = { std::string( "round" ) };

        for( int i = 0; i < 1; i++ )
        {
            shaderList.push_back( SHADER() );

            shaderList[i].AddSource( shaderPath + std::string( "/" ) + shaderNames[i] +
                                     std::string( ".frag" ), SHADER_TYPE_FRAGMENT );
            shaderList[i].AddSource( shaderPath + std::string( "/" ) + shaderNames[i] +
                                     std::string( ".vert" ), SHADER_TYPE_VERTEX );

            shaderList[i].Link();
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
}


void OPENGL_GAL::blitMainTexture( bool aIsClearFrameBuffer )
{
    selectShader( -1 );
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
    glBegin( GL_QUADS );
    glTexCoord2i( 0, 1 );
    glVertex3i( -1, -1, 0 );
    glTexCoord2i( 1, 1 );
    glVertex3i( 1, -1, 0 );
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
    // Draw the remaining contents, blit the main texture to the screen, swap the buffers
    glFlush();
    blitMainTexture( true );
    SwapBuffers();

    delete clientDC;
}


inline void OPENGL_GAL::selectShader( int aIndex )
{
    if( currentShader != aIndex )
    {
        if( currentShader >= 0 )
            shaderList[currentShader].Deactivate();

        if( aIndex >= 0 )
            shaderList[aIndex].Use();

        currentShader = aIndex;
    }
}


void OPENGL_GAL::drawRoundedSegment( VECTOR2D aStartPoint, VECTOR2D aEndPoint,
                                     double aWidth, bool aStroke, bool aGlBegin )
{
    VECTOR2D l     = (aEndPoint - aStartPoint);
    double   lnorm = l.EuclideanNorm();
    double   aspect;

    if( l.x == 0 && l.y == 0 )
    {
        l = VECTOR2D( aWidth / 2.0, 0.0 );
        aspect = 0.0;
    }
    else
    {
        l = l.Resize( aWidth / 2.0 );
        aspect = lnorm / (lnorm + aWidth);
    }

    VECTOR2D p = l.Perpendicular();
    VECTOR2D corners[4] = { aStartPoint - l - p, aEndPoint + l - p,
                            aEndPoint + l + p, aStartPoint - l + p };

    if( aStroke )
    {
        glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
    }
    else
    {
        glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
    }

    selectShader( 0 );

    if( aGlBegin )
        glBegin( GL_QUADS );

    glNormal3d( aspect, 0, 0 );
    glTexCoord2f( 0.0, 0.0 );
    glVertex3d( corners[0].x, corners[0].y, layerDepth );
    glNormal3d( aspect, 0, 0 );
    glTexCoord2f( 1.0, 0.0 );
    glVertex3d( corners[1].x, corners[1].y, layerDepth );
    glNormal3d( aspect, 0, 0 );
    glTexCoord2f( 1.0, 1.0 );
    glVertex3d( corners[2].x, corners[2].y, layerDepth );
    glNormal3d( aspect, 0, 0 );
    glTexCoord2f( 0.0, 1.0 );
    glVertex3d( corners[3].x, corners[3].y, layerDepth );

    if( aGlBegin )
        glEnd();
}


inline void OPENGL_GAL::drawLineQuad( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength     = startEndVector.EuclideanNorm();

    // Limit the width of the line to a minimum of one pixel
    // this looks best without anti-aliasing
    // XXX Should be improved later.
    double scale     = 0.5 * lineWidth / lineLength;
    double scale1pix = 0.5001 / worldScale / lineLength;
    if( lineWidth * worldScale < 1.0002 && !isGroupStarted )
    {
        scale = scale1pix;
    }

    VECTOR2D perpendicularVector( -startEndVector.y * scale, startEndVector.x * scale );

    // Compute the edge points of the line
    VECTOR2D point1 = aStartPoint + perpendicularVector;
    VECTOR2D point2 = aStartPoint - perpendicularVector;
    VECTOR2D point3 = aEndPoint + perpendicularVector;
    VECTOR2D point4 = aEndPoint - perpendicularVector;

    glBegin( GL_QUADS );
    glVertex3d( point1.x, point1.y, layerDepth );
    glVertex3d( point2.x, point2.y, layerDepth );
    glVertex3d( point4.x, point4.y, layerDepth );
    glVertex3d( point3.x, point3.y, layerDepth );
    glEnd();
}


inline void OPENGL_GAL::drawLineCap( VECTOR2D aStartPoint, VECTOR2D aEndPoint, double aDepthOffset )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength     = startEndVector.EuclideanNorm();
    double   lineAngle      = atan2( startEndVector.y, startEndVector.x );

    switch( lineCap )
    {
    case LINE_CAP_BUTT:
        // TODO
        break;

    case LINE_CAP_ROUND:
        // Add a semicircle at the line end
        drawSemiCircle( aStartPoint, lineWidth / 2, lineAngle + M_PI / 2, aDepthOffset );
        break;

    case LINE_CAP_SQUARED:
        VECTOR2D offset;
        offset = startEndVector * ( lineWidth / lineLength / 2.0 );
        aStartPoint = aStartPoint - offset;
        aEndPoint = aEndPoint + offset;
        break;
    }
}


void OPENGL_GAL::DrawLine( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    if( isUseShader )
    {
        drawRoundedSegment( aStartPoint, aEndPoint, lineWidth, true, true );
    }
    else
    {
        VECTOR2D startEndVector = aEndPoint - aStartPoint;
        double   lineLength     = startEndVector.EuclideanNorm();
        if( lineLength > 0.0 )
        {
            glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            drawLineCap( aStartPoint, aEndPoint, layerDepth );
            drawLineCap( aEndPoint, aStartPoint, layerDepth );
            drawLineQuad( aStartPoint, aEndPoint );
        }
    }
}


void OPENGL_GAL::DrawPolyline( std::deque<VECTOR2D>& aPointList )
{
    LineCap savedLineCap = lineCap;

    bool isFirstPoint = true;
    bool isFirstLine = true;
    VECTOR2D startEndVector;
    VECTOR2D lastStartEndVector;
    VECTOR2D lastPoint;

    unsigned int i = 0;

    // Draw for each segment a line
    for( std::deque<VECTOR2D>::const_iterator it = aPointList.begin(); it != aPointList.end(); it++ )
    {
        // First point
        if( it == aPointList.begin() )
        {
            isFirstPoint = false;
            lastPoint    = *it;
        }
        else
        {
            VECTOR2D actualPoint = *it;
            startEndVector = actualPoint - lastPoint;

            if( isFirstLine )
            {
                drawLineCap( lastPoint, actualPoint, layerDepth );
                isFirstLine = false;
            }
            else
            {
                // Compute some variables for the joints
                double lineLengthA = lastStartEndVector.EuclideanNorm();
                double scale = 0.5 * lineWidth / lineLengthA;
                VECTOR2D perpendicularVector1( -lastStartEndVector.y * scale,
                                               lastStartEndVector.x * scale );
                double lineLengthB = startEndVector.EuclideanNorm();
                scale = 0.5 * lineWidth / lineLengthB;
                VECTOR2D perpendicularVector2( -startEndVector.y * scale,
                                               startEndVector.x * scale );

                switch( lineJoin )
                {
                case LINE_JOIN_ROUND:
                {
                    // Insert a triangle fan at the line joint
                    // Compute the start and end angle for the triangle fan
                    double angle1 = startEndVector.Angle();
                    double angle2 = lastStartEndVector.Angle();
                    double angleDiff = angle1 - angle2;
                    // Determines the side of the triangle fan
                    double adjust = angleDiff < 0 ? -0.5 * lineWidth : 0.5 * lineWidth;

                    // Angle correction for some special cases
                    if( angleDiff < -M_PI )
                    {
                        if( angle1 < 0 )
                        {
                            angle1 += 2 * M_PI;
                        }
                        if( angle2 < 0 )
                        {
                            angle2 += 2 * M_PI;
                        }
                        adjust = -adjust;
                    }
                    else if( angleDiff > M_PI )
                    {
                        if( angle1 > 0 )
                        {
                            angle1 -= 2 * M_PI;

                        }
                        if( angle2 > 0 )
                        {
                            angle2 -= 2 * M_PI;
                        }
                        adjust = -adjust;
                    }

                    // Now draw the fan
                    glBegin( GL_TRIANGLE_FAN );
                    glVertex3d( lastPoint.x, lastPoint.y, layerDepth );

                    SWAP( angle1, >, angle2 );

                    glVertex3d( lastPoint.x + adjust * sin( angle1 ),
                                lastPoint.y - adjust * cos( angle1 ), layerDepth );
                    for( double a = angle1 + M_PI / 32; a < angle2; a += M_PI / 32 )
                    {
                        glVertex3d( lastPoint.x + adjust * sin( a ),
                                    lastPoint.y - adjust * cos( a ), layerDepth );
                    }
                    glVertex3d( lastPoint.x + adjust * sin( angle2 ),
                                lastPoint.y - adjust * cos( angle2 ), layerDepth );

                    glEnd();
                    break;
                }
                case LINE_JOIN_BEVEL:
                {
                    // We compute the edge points of the line segments at the joint
                    VECTOR2D edgePoint1;
                    VECTOR2D edgePoint2;
                    // Determine the correct side
                    if( lastStartEndVector.x * startEndVector.y
                        - lastStartEndVector.y * startEndVector.x
                        < 0 )
                    {
                        edgePoint1 = lastPoint + perpendicularVector1;
                        edgePoint2 = lastPoint + perpendicularVector2;
                    }
                    else
                    {
                        edgePoint1 = lastPoint - perpendicularVector1;
                        edgePoint2 = lastPoint - perpendicularVector2;
                    }

                    // Insert a triangle at the joint to close the gap
                    glBegin( GL_TRIANGLES );
                    glVertex3d( edgePoint1.x, edgePoint1.y, layerDepth );
                    glVertex3d( edgePoint2.x, edgePoint2.y, layerDepth );
                    glVertex3d( lastPoint.x, lastPoint.y, layerDepth );
                    glEnd();

                    break;
                }
                case LINE_JOIN_MITER:
                {
                    // Compute points of the outer edges
                    VECTOR2D point1 = lastPoint - perpendicularVector1;
                    VECTOR2D point3 = lastPoint - perpendicularVector2;
                    if( lastStartEndVector.x * startEndVector.y
                        - lastStartEndVector.y * startEndVector.x
                        < 0 )
                    {
                        point1 = lastPoint + perpendicularVector1;
                        point3 = lastPoint + perpendicularVector2;

                    }

                    VECTOR2D point2 = point1 - lastStartEndVector;
                    VECTOR2D point4 = point3 + startEndVector;

                    // Now compute the intersection point of the edges
                    double c1 = point1.Cross( point2 );
                    double c2 = point3.Cross( point4 );
                    double quot = startEndVector.Cross( lastStartEndVector );

                    VECTOR2D miterPoint( -c1 * startEndVector.x - c2 * lastStartEndVector.x,
                                        -c1 * startEndVector.y - c2 * lastStartEndVector.y );

                    miterPoint = ( 1 / quot ) * miterPoint;

                    // Check if the point is outside the limit
                    if( ( lastPoint - miterPoint ).EuclideanNorm() > 2 * lineWidth )
                    {
                        // if it's outside cut the edge and insert three triangles
                        double limit = MITER_LIMIT * lineWidth;
                        VECTOR2D mp1 = point1 + ( limit / lineLengthA ) * lastStartEndVector;
                        VECTOR2D mp2 = point3 - ( limit / lineLengthB ) * startEndVector;
                        glBegin( GL_TRIANGLE_FAN );
                        glVertex3d( lastPoint.x, lastPoint.y, layerDepth );
                        glVertex3d( point1.x, point1.y, layerDepth );
                        glVertex3d( mp1.x, mp1.y, layerDepth );
                        glVertex3d( mp2.x, mp2.y, layerDepth );
                        glVertex3d( point3.x, point3.y, layerDepth );
                        glEnd();
                    }
                    else
                    {
                        // Insert two triangles for the mitered edge
                        glBegin( GL_TRIANGLE_FAN );
                        glVertex3d( lastPoint.x, lastPoint.y, layerDepth );
                        glVertex3d( point1.x, point1.y, layerDepth );
                        glVertex3d( miterPoint.x, miterPoint.y, layerDepth );
                        glVertex3d( point3.x, point3.y, layerDepth );
                        glEnd();
                    }
                    break;
                }
                }
            }

            if( it == aPointList.end() - 1 )
            {
                drawLineCap( actualPoint, lastPoint, layerDepth );
            }

            drawLineQuad( lastPoint, *it );
            lastPoint = *it;
            lastStartEndVector = startEndVector;
        }

        i++;
    }

    lineCap = savedLineCap;
}


void OPENGL_GAL::DrawRectangle( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    // Compute the diagonal points of the rectangle
    VECTOR2D diagonalPointA( aEndPoint.x, aStartPoint.y );
    VECTOR2D diagonalPointB( aStartPoint.x, aEndPoint.y );

    if( isUseShader )
    {
        if( isFillEnabled )
        {
            selectShader( 0 );
            glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
            glBegin( GL_QUADS );
            glNormal3d( 1.0, 0, 0);
            glTexCoord2f(0.0, 0.0);
            glVertex3d( aStartPoint.x, aStartPoint.y, layerDepth );
            glNormal3d( 1.0, 0, 0);
            glTexCoord2f(1.0, 0.0);
            glVertex3d( diagonalPointA.x, diagonalPointA.y, layerDepth );
            glNormal3d( 1.0, 0, 0);
            glTexCoord2f(1.0, 1.0);
            glVertex3d( aEndPoint.x, aEndPoint.y, layerDepth );
            glNormal3d( 1.0, 0, 0);
            glTexCoord2f(0.0, 1.0);
            glVertex3d( diagonalPointB.x, diagonalPointB.y, layerDepth );
            glEnd();
        }
        if(isStrokeEnabled)
        {
            glBegin(GL_QUADS);

            drawRoundedSegment(aStartPoint, diagonalPointA, lineWidth, true, false );
            drawRoundedSegment(aEndPoint, diagonalPointA, lineWidth, true, false );
            drawRoundedSegment(aStartPoint, diagonalPointB, lineWidth, true, false );
            drawRoundedSegment(aEndPoint, diagonalPointB, lineWidth, true, false );

            glEnd();
        }

        return;
    }

    selectShader( -1 );

    // Stroke the outline
    if( isStrokeEnabled )
    {
        glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
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
        glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
        glBegin( GL_QUADS );
        glVertex3d( aStartPoint.x, aStartPoint.y, layerDepth );
        glVertex3d( diagonalPointA.x, diagonalPointA.y, layerDepth );
        glVertex3d( aEndPoint.x, aEndPoint.y, layerDepth );
        glVertex3d( diagonalPointB.x, diagonalPointB.y, layerDepth );
        glEnd();
    }

    // Restore the stroke color
    glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
}


void OPENGL_GAL::DrawCircle( VECTOR2D aCenterPoint, double aRadius )
{
    // We need a minimum radius, else simply don't draw the circle
    if( aRadius <= 0.0 )
    {
        return;
    }

    if( isUseShader )
    {
        drawRoundedSegment( aCenterPoint, aCenterPoint, aRadius * 2.0, false, true );
        return;
    }

    // Draw the middle of the circle (not anti-aliased)
    // Compute the factors for the unit circle
    double outerScale = lineWidth / aRadius / 2;
    double innerScale = -outerScale;
    outerScale += 1.0;
    innerScale += 1.0;

    if( isUseShader )
    {
        innerScale *= 1.0 / cos( M_PI / CIRCLE_POINTS );
    }

    if( isStrokeEnabled )
    {
        if( innerScale < outerScale )
        {
            // Draw the outline
            glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            glPushMatrix();

            glTranslated( aCenterPoint.x, aCenterPoint.y, 0.0 );
            glScaled( aRadius, aRadius, 1.0 );

            glBegin( GL_QUAD_STRIP );
            for( std::deque<VECTOR2D>::const_iterator it = unitCirclePoints.begin();
                    it != unitCirclePoints.end(); it++ )
            {
                glVertex3d( it->x * innerScale, it->y * innerScale, layerDepth );
                glVertex3d( it->x * outerScale, it->y * outerScale, layerDepth );
            }
            glEnd();

            glPopMatrix();
        }
    }

    // Filled circles are easy to draw by using the stored display list, scaling and translating
    if( isFillEnabled )
    {
        glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        glPushMatrix();

        glTranslated( aCenterPoint.x, aCenterPoint.y, layerDepth );
        glScaled( aRadius, aRadius, 1.0 );

        glCallList( displayListCircle );

        glPopMatrix();
    }
}


// This method is used for round line caps
void OPENGL_GAL::drawSemiCircle( VECTOR2D aCenterPoint, double aRadius, double aAngle,
                                 double aDepthOffset )
{
    // XXX Depth seems to be buggy
    glPushMatrix();
    glTranslated( aCenterPoint.x, aCenterPoint.y, aDepthOffset );
    glScaled( aRadius, aRadius, 1.0 );
    glRotated( aAngle * 360.0 / ( 2 * M_PI ), 0, 0, 1 );

    glCallList( displayListSemiCircle );

    glPopMatrix();
}


// FIXME Optimize
void OPENGL_GAL::DrawArc( VECTOR2D aCenterPoint, double aRadius, double aStartAngle,
                          double aEndAngle )
{
    if( aRadius <= 0 )
    {
        return;
    }

    double outerScale = lineWidth / aRadius / 2;
    double innerScale = -outerScale;

    outerScale += 1.0;
    innerScale += 1.0;

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    VECTOR2D startPoint( cos( aStartAngle ), sin( aStartAngle ) );
    VECTOR2D endPoint( cos( aEndAngle ), sin( aEndAngle ) );
    VECTOR2D startEndPoint = startPoint + endPoint;
    VECTOR2D middlePoint = 0.5 * startEndPoint;


    glPushMatrix();
    glTranslated( aCenterPoint.x, aCenterPoint.y, layerDepth );
    glScaled( aRadius, aRadius, 1.0 );

    if( isStrokeEnabled )
    {
        if( isUseShader )
        {
            int n_points_s = (int) ( aRadius * worldScale );
            int n_points_a = (int) ( ( aEndAngle - aStartAngle ) / (double) ( 2.0 * M_PI / CIRCLE_POINTS ));

            if( n_points_s < 4 )
                n_points_s = 4;

            int n_points = std::min( n_points_s, n_points_a );

            if( n_points > CIRCLE_POINTS )
                n_points = CIRCLE_POINTS;

            double alphaIncrement = ( aEndAngle - aStartAngle ) / n_points;

            double cosI = cos( alphaIncrement );
            double sinI = sin( alphaIncrement );

            VECTOR2D p( cos( aStartAngle ), sin( aStartAngle ) );

            glBegin( GL_QUADS );
            for( int i = 0; i < n_points; i++ )
            {
                VECTOR2D p_next( p.x * cosI - p.y * sinI, p.x * sinI + p.y * cosI );

                drawRoundedSegment( p, p_next, lineWidth / aRadius, true, false );
                p = p_next;
            }
            glEnd();
        }
        else
        {
            glColor4d( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            glBegin( GL_QUAD_STRIP );

            double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;

            for( double alpha = aStartAngle; alpha < aEndAngle; alpha += alphaIncrement )
            {
                glVertex2d( cos( alpha ) * innerScale, sin( alpha ) * innerScale );
                glVertex2d( cos( alpha ) * outerScale, sin( alpha ) * outerScale );
            }

            glVertex2d( cos( aEndAngle ) * innerScale, sin( aEndAngle ) * innerScale );
            glVertex2d( cos( aEndAngle ) * outerScale, sin( aEndAngle ) * outerScale );

            glEnd();

            if( lineCap == LINE_CAP_ROUND )
            {
                drawSemiCircle( startPoint, lineWidth / aRadius / 2, aStartAngle + M_PI, 0 );
                drawSemiCircle( endPoint, lineWidth / aRadius / 2, aEndAngle, 0 );
            }
        }
    }

    if( isFillEnabled )
    {
        glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        glBegin( GL_TRIANGLE_FAN );

        glVertex2d( middlePoint.x, middlePoint.y );

        double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;

        for( double alpha = aStartAngle; alpha < aEndAngle; alpha += alphaIncrement )
        {
            glVertex2d( cos( alpha ), sin( alpha ) );
        }

        glVertex2d( endPoint.x, endPoint.y );

        glEnd();
    }

    glPopMatrix();
}


struct OGLPOINT
{
    OGLPOINT() :
        x( 0.0 ), y( 0.0 ), z( 0.0 )
    {
    }

    OGLPOINT( const char* fastest )
    {
        // do nothing for fastest speed, and keep inline
    }

    OGLPOINT( const VECTOR2D& aPoint ) :
        x( aPoint.x ), y( aPoint.y ), z( 0.0 )
    {
    }

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

    GLUtesselator* tesselator = gluNewTess();

    typedef std::vector<OGLPOINT> OGLPOINTS;

    // Do only one heap allocation, can do because we know size in advance.
    // std::vector is then fastest
    OGLPOINTS vertexList( aPointList.size(), OGLPOINT( "fastest" ) );

    InitTesselatorCallbacks( tesselator );

    gluTessProperty( tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    glNormal3d( 0.0, 0.0, 1.0 );
    glColor4d( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    glShadeModel( GL_FLAT );
    gluTessBeginPolygon( tesselator, NULL );
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

    gluDeleteTess( tesselator );

    // vertexList destroyed here
}


void OPENGL_GAL::DrawCurve( VECTOR2D aStartPoint, VECTOR2D aControlPointA,
                            VECTOR2D aControlPointB, VECTOR2D aEndPoint )
{
    // FIXME The drawing quality needs to be improved
    // FIXME Perhaps choose a quad/triangle strip instead?
    // FIXME Brute force method, use a better (recursive?) algorithm

    std::deque<VECTOR2D> pointList;

    double t  = 0.0;
    double dt = 1.0 / (double)CURVE_POINTS;

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


void OPENGL_GAL::SetStrokeColor( COLOR4D aColor )
{
    if( strokeColor != aColor )
    {
        isSetAttributes = true;
        strokeColor     = aColor;

        // This is the default drawing color
        glColor4d( aColor.r, aColor.g, aColor.b, aColor.a );
    }
}


void OPENGL_GAL::SetFillColor( COLOR4D aColor )
{
    if( fillColor != aColor )
    {
        isSetAttributes = true;
        fillColor = aColor;
    }
}


void OPENGL_GAL::SetBackgroundColor( COLOR4D aColor )
{
    if( backgroundColor != aColor )
    {
        isSetAttributes = true;
        backgroundColor = aColor;
    }
}


void OPENGL_GAL::SetLineWidth( double aLineWidth )
{
    if( lineWidth != aLineWidth )
    {
        isSetAttributes = true;
        lineWidth = aLineWidth;
    }
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
    glRotated( aAngle * ( 360 / ( 2 * M_PI ) ), 0, 0, 1 );
}


void OPENGL_GAL::Translate( VECTOR2D aVector )
{
    glTranslated( aVector.x, aVector.y, 0 );
}


void OPENGL_GAL::Scale( VECTOR2D aScale )
{
    // TODO: Check method
    glScaled( aScale.x, aScale.y, 0 );
}


void OPENGL_GAL::Flush()
{
    glFlush();
}


void OPENGL_GAL::Save()
{
    glPushMatrix();
}


void OPENGL_GAL::Restore()
{
    glPopMatrix();
}


// TODO Error handling
int OPENGL_GAL::BeginGroup()
{
    isGroupStarted = true;
    GLint displayList = glGenLists( 1 );
    glNewList( displayList, GL_COMPILE );
    displayListsGroup.push_back( displayList );

    return (int) displayList;
}


void OPENGL_GAL::EndGroup()
{
    isGroupStarted = false;
    glEndList();
    glCallList( displayListsGroup.back() );
}


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    std::deque<GLuint>::iterator it = displayListsGroup.begin();
    std::advance( it, aGroupNumber );

    displayListsGroup.erase( it );

    glDeleteLists( (GLint) aGroupNumber, 1 );
}


void OPENGL_GAL::DrawGroup( int aGroupNumber )
{
    glCallList( (GLint) aGroupNumber );
}


void OPENGL_GAL::computeUnitArcs()
{
    displayListsArcs = glGenLists( CIRCLE_POINTS + 1 );

    // Create an individual display list for each arc in with an angle [0 .. 2pi]
    for( int j = 0; j < CIRCLE_POINTS + 1; j++ )
    {
        glNewList( displayListsArcs + j, GL_COMPILE );

        for( int i = 0; i < j; i++ )
        {
            glVertex2d( cos( 2 * M_PI / CIRCLE_POINTS * i ), sin( 2 * M_PI / CIRCLE_POINTS * i ) );
        }

        glEndList();
    }
}


void OPENGL_GAL::computeUnitCircle()
{
    displayListCircle = glGenLists( 1 );
    glNewList( displayListCircle, GL_COMPILE );

    glBegin( GL_TRIANGLE_FAN );
    glVertex2d( 0, 0 );
    // Compute the circle points for a given number of segments
    // Insert in a display list and a vector
    for( int i = 0; i < CIRCLE_POINTS + 1; i++ )
    {
        double valueX = cos( 2.0 * M_PI / CIRCLE_POINTS * i );
        double valueY = sin( 2.0 * M_PI / CIRCLE_POINTS * i );
        glVertex2d( valueX, valueY );
        unitCirclePoints.push_back( VECTOR2D( valueX, valueY ) );
    }
    glEnd();

    glEndList();
}


void OPENGL_GAL::computeUnitSemiCircle()
{
    displayListSemiCircle = glGenLists( 1 );
    glNewList( displayListSemiCircle, GL_COMPILE );

    glBegin( GL_TRIANGLE_FAN );
    glVertex2d( 0, 0 );
    for( int i = 0; i < CIRCLE_POINTS / 2 + 1; i++ )
    {
        glVertex2d( cos( 2.0 * M_PI / CIRCLE_POINTS * i ), sin( 2.0 * M_PI / CIRCLE_POINTS * i ) );
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


void CALLBACK VertexCallback( GLvoid* aVertexPtr )
{
    GLdouble* vertex = (GLdouble*) aVertexPtr;

    glVertex3dv( vertex );
}


void CALLBACK CombineCallback( GLdouble coords[3],
                               GLdouble* vertex_data[4],
                               GLfloat weight[4], GLdouble** dataOut )
{
   GLdouble* vertex = new GLdouble[3];

   memcpy( vertex, coords, 3 * sizeof(GLdouble) );

   *dataOut = vertex;
}


void CALLBACK BeginCallback( GLenum aWhich )
{
    glBegin( aWhich );
}


void CALLBACK EndCallback()
{
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
    gluTessCallback( aTesselator, GLU_TESS_VERTEX,  ( void (CALLBACK*)() )VertexCallback );
    gluTessCallback( aTesselator, GLU_TESS_COMBINE, ( void (CALLBACK*)() )CombineCallback );
    gluTessCallback( aTesselator, GLU_TESS_BEGIN,   ( void (CALLBACK*)() )BeginCallback );
    gluTessCallback( aTesselator, GLU_TESS_END,     ( void (CALLBACK*)() )EndCallback );
    gluTessCallback( aTesselator, GLU_TESS_ERROR,   ( void (CALLBACK*)() )ErrorCallback );
}


// ---------------
// Cursor handling
// ---------------


void OPENGL_GAL::initCursor( int aCursorSize )
{
    cursorSize = aCursorSize;
}


VECTOR2D OPENGL_GAL::ComputeCursorToWorld( VECTOR2D aCursorPosition )
{
    aCursorPosition.y = screenSize.y - aCursorPosition.y;
    MATRIX3x3D inverseMatrix = worldScreenMatrix.Inverse();
    VECTOR2D    cursorPositionWorld = inverseMatrix * aCursorPosition;

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

    glBegin( GL_QUADS );

    glVertex3f( (int) ( aCursorPosition.x - cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y ), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x + cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y ), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x + cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y + 1 ), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x - cursorSize / 2 ) + 1,
                (int) ( aCursorPosition.y + 1), depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x ),
                (int) ( aCursorPosition.y - cursorSize / 2 ) + 1, depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x  ),
                (int) ( aCursorPosition.y + cursorSize / 2 ) + 1, depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x  ) + 1,
                (int) ( aCursorPosition.y + cursorSize / 2 ) + 1, depthRange.x );

    glVertex3f( (int) ( aCursorPosition.x ) + 1,
                (int) ( aCursorPosition.y - cursorSize / 2 ) + 1, depthRange.x );
    glEnd();

    // Blit the current screen contents
    SwapBuffers();
}


void OPENGL_GAL::DrawGridLine( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    // We check, if we got a horizontal or a vertical grid line and compute the offset
    VECTOR2D perpendicularVector;

    if( aStartPoint.x == aEndPoint.x )
    {
        perpendicularVector = VECTOR2D( 0.5 * lineWidth, 0 );
    }
    else
    {
        perpendicularVector = VECTOR2D( 0, 0.5 * lineWidth );
    }

    // Now we compute the edge points of the quad
    VECTOR2D point1 = aStartPoint + perpendicularVector;
    VECTOR2D point2 = aStartPoint - perpendicularVector;
    VECTOR2D point3 = aEndPoint + perpendicularVector;
    VECTOR2D point4 = aEndPoint - perpendicularVector;

    if( isUseShader )
        selectShader( -1 );

    // Set color
    glColor4d( gridColor.r, gridColor.g, gridColor.b, gridColor.a );

    // Draw the quad for the grid line
    glBegin( GL_QUADS );
    double gridDepth = depthRange.y * 0.75;
    glVertex3d( point1.x, point1.y, gridDepth );
    glVertex3d( point2.x, point2.y, gridDepth );
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
