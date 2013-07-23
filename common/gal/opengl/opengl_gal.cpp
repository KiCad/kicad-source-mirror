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
    cachedManager( true ),
    nonCachedManager( false ),
    overlayManager( false )
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
    isGlewInitialized        = false;
    isFramebufferInitialized = false;
    isUseShader              = isUseShaders;
    isShaderInitialized      = false;
    isGrouping               = false;
    wxSize parentSize        = aParent->GetSize();
    groupCounter             = 0;

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

    // Compute the unit circle vertices and store them in a buffer for faster drawing
    computeCircle();
}


OPENGL_GAL::~OPENGL_GAL()
{
    glFlush();

    gluDeleteTess( tesselator );
    ClearCache();

    delete glContext;
}


void OPENGL_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint();
}


void OPENGL_GAL::ResizeScreen( int aWidth, int aHeight )
{
    screenSize = VECTOR2D( aWidth, aHeight );

    // Resize framebuffers
    compositor.Resize( aWidth, aHeight );
    isFramebufferInitialized = false;

    wxGLCanvas::SetSize( aWidth, aHeight );
}


void OPENGL_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


void OPENGL_GAL::SaveScreen()
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void OPENGL_GAL::RestoreScreen()
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void OPENGL_GAL::SetTarget( RenderTarget aTarget )
{
    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
        currentManager = &cachedManager;
        break;

    case TARGET_NONCACHED:
        currentManager = &nonCachedManager;
        break;

    case TARGET_OVERLAY:
        currentManager = &overlayManager;
        break;
    }
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

    // Framebuffers have to be supported
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

    isGlewInitialized = true;
}


void OPENGL_GAL::BeginDrawing()
{
    SetCurrent( *glContext );

    clientDC = new wxClientDC( this );

    // Initialize GLEW, FBOs & VBOs
    if( !isGlewInitialized )
        initGlew();

    if( !isFramebufferInitialized )
    {
        // Set up the view port
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        glViewport( 0, 0, (GLsizei) screenSize.x, (GLsizei) screenSize.y );

        // Create the screen transformation
        glOrtho( 0, (GLint) screenSize.x, 0, (GLsizei) screenSize.y, -depthRange.x, -depthRange.y );

        // Prepare rendering target buffers
        compositor.Initialize();
        mainBuffer = compositor.GetBuffer();
        overlayBuffer = compositor.GetBuffer();

        isFramebufferInitialized = true;
    }

    // Compile the shaders
    if( !isShaderInitialized && isUseShader )
    {
        if( !shader.LoadBuiltinShader( 0, SHADER_TYPE_VERTEX ) )
            wxLogFatalError( wxT( "Cannot compile vertex shader!" ) );

        if( !shader.LoadBuiltinShader( 1, SHADER_TYPE_FRAGMENT ) )
            wxLogFatalError( wxT( "Cannot compile fragment shader!" ) );

        if( !shader.Link() )
            wxLogFatalError( wxT( "Cannot link the shaders!" ) );

        // Make VBOs use shaders
        cachedManager.SetShader( shader );
        nonCachedManager.SetShader( shader );
        overlayManager.SetShader( shader );

        isShaderInitialized = true;
    }

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

    // Prepare buffers for drawing
    nonCachedManager.Clear();
    overlayManager.Clear();

    cachedManager.BeginDrawing();
    nonCachedManager.BeginDrawing();
    overlayManager.BeginDrawing();
}


void OPENGL_GAL::EndDrawing()
{
    // Cached & non-cached containers are rendered to the same buffer
    compositor.SetBuffer( mainBuffer );
    compositor.ClearBuffer();
    nonCachedManager.EndDrawing();
    cachedManager.EndDrawing();

    // Overlay container is rendered to a different buffer
    compositor.SetBuffer( overlayBuffer );
    compositor.ClearBuffer();
    overlayManager.EndDrawing();

    // Draw the remaining contents, blit the rendering targets to the screen, swap the buffers
    glFlush();
    compositor.DrawBuffer( mainBuffer, -1.0 );
    compositor.DrawBuffer( overlayBuffer, 0.0 );
    SwapBuffers();

    delete clientDC;
}


inline void OPENGL_GAL::drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength     = startEndVector.EuclideanNorm();
    double   scale          = 0.5 * lineWidth / lineLength;

    if( lineLength <= 0.0 )
        return;

    VECTOR2D perpendicularVector( -startEndVector.y * scale, startEndVector.x * scale );

    if( isUseShader )
    {
        glm::vec4 vector( perpendicularVector.x, perpendicularVector.y, 0.0, 0.0 );

        // The perpendicular vector also needs transformations
        vector = currentManager->GetTransformation() * vector;

        // Line width is maintained by the vertex shader
        currentManager->Shader( SHADER_LINE, vector.x, vector.y, lineWidth );
        currentManager->Vertex( aStartPoint.x, aStartPoint.y, layerDepth );    // v0

        currentManager->Shader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        currentManager->Vertex( aStartPoint.x, aStartPoint.y, layerDepth );    // v1

        currentManager->Shader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        currentManager->Vertex( aEndPoint.x, aEndPoint.y, layerDepth );        // v3

        currentManager->Shader( SHADER_LINE, vector.x, vector.y, lineWidth );
        currentManager->Vertex( aStartPoint.x, aStartPoint.y, layerDepth );    // v0

        currentManager->Shader( SHADER_LINE, -vector.x, -vector.y, lineWidth );
        currentManager->Vertex( aEndPoint.x, aEndPoint.y, layerDepth );        // v3

        currentManager->Shader( SHADER_LINE, vector.x, vector.y, lineWidth );
        currentManager->Vertex( aEndPoint.x, aEndPoint.y, layerDepth );        // v2
    }
    else
    {
        // Compute the edge points of the line
        VECTOR2D v0 = aStartPoint + perpendicularVector;
        VECTOR2D v1 = aStartPoint - perpendicularVector;
        VECTOR2D v2 = aEndPoint + perpendicularVector;
        VECTOR2D v3 = aEndPoint - perpendicularVector;

        currentManager->Vertex( v0.x, v0.y, layerDepth );
        currentManager->Vertex( v1.x, v1.y, layerDepth );
        currentManager->Vertex( v3.x, v3.y, layerDepth );

        currentManager->Vertex( v0.x, v0.y, layerDepth );
        currentManager->Vertex( v3.x, v3.y, layerDepth );
        currentManager->Vertex( v2.x, v2.y, layerDepth );
    }
}


void OPENGL_GAL::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineAngle      = startEndVector.Angle();

    if( isFillEnabled )
    {
        // Filled tracks
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

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

        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        Save();

        currentManager->Translate( aStartPoint.x, aStartPoint.y, 0.0 );
        currentManager->Rotate( lineAngle, 0.0f, 0.0f, 1.0f );

        drawLineQuad( VECTOR2D( 0.0,         aWidth / 2.0 ),
                      VECTOR2D( lineLength,  aWidth / 2.0 ) );

        drawLineQuad( VECTOR2D( 0.0,        -aWidth / 2.0 ),
                      VECTOR2D( lineLength, -aWidth / 2.0 ) );

        // Draw line caps
        drawStrokedSemiCircle( VECTOR2D( 0.0, 0.0 ),
                               ( aWidth + lineWidth ) / 2, M_PI / 2 );
        drawStrokedSemiCircle( VECTOR2D( lineLength, 0.0 ),
                               ( aWidth + lineWidth ) / 2, -M_PI / 2 );

        Restore();
    }
}


unsigned int OPENGL_GAL::getNewGroupNumber()
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
        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

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
        currentManager->Shader( SHADER_NONE );
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        currentManager->Vertex( aStartPoint.x, aStartPoint.y, layerDepth );
        currentManager->Vertex( diagonalPointA.x, diagonalPointA.y, layerDepth );
        currentManager->Vertex( aEndPoint.x, aEndPoint.y, layerDepth );

        currentManager->Vertex( aStartPoint.x, aStartPoint.y, layerDepth );
        currentManager->Vertex( aEndPoint.x, aEndPoint.y, layerDepth );
        currentManager->Vertex( diagonalPointB.x, diagonalPointB.y, layerDepth );
    }
}


void OPENGL_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    if( isUseShader )
    {
        if( isFillEnabled )
        {
            currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

            /* Draw a triangle that contains the circle, then shade it leaving only the circle.
               Parameters given to setShader are indices of the triangle's vertices
               (if you want to understand more, check the vertex shader source [shader.vert]).
               Shader uses this coordinates to determine if fragments are inside the circle or not.
                    v2
                    /\
                   //\\
               v0 /_\/_\ v1
            */
            currentManager->Shader( SHADER_FILLED_CIRCLE, 1.0 );
            currentManager->Vertex( aCenterPoint.x - aRadius * sqrt( 3.0f ),            // v0
                     aCenterPoint.y - aRadius, layerDepth );

            currentManager->Shader( SHADER_FILLED_CIRCLE, 2.0 );
            currentManager->Vertex( aCenterPoint.x + aRadius* sqrt( 3.0f ),             // v1
                     aCenterPoint.y - aRadius, layerDepth );

            currentManager->Shader( SHADER_FILLED_CIRCLE, 3.0 );
            currentManager->Vertex( aCenterPoint.x, aCenterPoint.y + aRadius * 2.0f,    // v2
                                    layerDepth );
        }

        if( isStrokeEnabled )
        {
            currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            /* Draw a triangle that contains the circle, then shade it leaving only the circle.
               Parameters given to setShader are indices of the triangle's vertices
               (if you want to understand more, check the vertex shader source [shader.vert]).
               and the line width. Shader uses this coordinates to determine if fragments are
               inside the circle or not.
                    v2
                    /\
                   //\\
               v0 /_\/_\ v1
            */
            double outerRadius = aRadius + ( lineWidth / 2 );
            currentManager->Shader( SHADER_STROKED_CIRCLE, 1.0, aRadius, lineWidth );
            currentManager->Vertex( aCenterPoint.x - outerRadius * sqrt( 3.0f ),            // v0
                     aCenterPoint.y - outerRadius, layerDepth );

            currentManager->Shader( SHADER_STROKED_CIRCLE, 2.0, aRadius, lineWidth );
            currentManager->Vertex( aCenterPoint.x + outerRadius * sqrt( 3.0f ),            // v1
                     aCenterPoint.y - outerRadius, layerDepth );

            currentManager->Shader( SHADER_STROKED_CIRCLE, 3.0, aRadius, lineWidth );
            currentManager->Vertex( aCenterPoint.x, aCenterPoint.y + outerRadius * 2.0f,    // v2
                                    layerDepth );
        }
    }
    else
    {
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
                VERTEX* circle = circleContainer.GetAllVertices();
                int     next;

                currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b,
                                       strokeColor.a );

                Save();

                currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );
                currentManager->Scale( aRadius, aRadius, 0.0f );

                for( int i = 0; i < 3 * CIRCLE_POINTS; ++i )
                {
                    // verticesCircle contains precomputed circle points interleaved with vertex
                    // (0,0,0), so filled circles can be drawn as consecutive triangles, ie:
                    // { 0,a,b, 0,c,d, 0,e,f, 0,g,h, ... }
                    // where letters stand for consecutive circle points and 0 for (0,0,0) vertex.

                    // We have to skip all (0,0,0) vertices (every third vertex)
                    if( i % 3 == 0 )
                    {
                        i++;
                        // Depending on the vertex, next circle point
                        // may be stored in the next vertex..
                        next = i + 1;
                    }
                    else
                    {
                        // ..or 2 vertices away (in case it is preceded by (0,0,0) vertex)
                        next = i + 2;
                    }

                    currentManager->Vertex( circle[i].x * innerScale,
                                            circle[i].y * innerScale,
                                            layerDepth );
                    currentManager->Vertex( circle[i].x * outerScale,
                                            circle[i].y * outerScale,
                                            layerDepth );
                    currentManager->Vertex( circle[next].x * innerScale,
                                            circle[next].y * innerScale,
                                            layerDepth );

                    currentManager->Vertex( circle[i].x * outerScale,
                                            circle[i].y * outerScale,
                                            layerDepth );
                    currentManager->Vertex( circle[next].x * outerScale,
                                            circle[next].y * outerScale,
                                            layerDepth );
                    currentManager->Vertex( circle[next].x * innerScale,
                                            circle[next].y * innerScale,
                                            layerDepth );
                }

                Restore();
            }
        }

        // Filled circles are easy to draw by using the stored vertices list
        if( isFillEnabled )
        {
            currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

            Save();
            currentManager->Translate( aCenterPoint.x, aCenterPoint.y, layerDepth );
            currentManager->Scale( aRadius, aRadius, 0.0f );
            currentManager->Vertices( circleContainer.GetAllVertices(), CIRCLE_POINTS * 3 );
            Restore();
        }
    }
}


void OPENGL_GAL::drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
    if( isFillEnabled )
    {
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
        drawFilledSemiCircle( aCenterPoint, aRadius, aAngle );
    }

    if( isStrokeEnabled )
    {
        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
        drawStrokedSemiCircle( aCenterPoint, aRadius, aAngle );
    }
}


void OPENGL_GAL::drawFilledSemiCircle( const VECTOR2D& aCenterPoint, double aRadius,
                                       double aAngle )
{
    if( isUseShader )
    {
        Save();
        currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
        currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

        /* Draw a triangle that contains the semicircle, then shade it to leave only
         * the semicircle. Parameters given to setShader are indices of the triangle's vertices
           (if you want to understand more, check the vertex shader source [shader.vert]).
           Shader uses this coordinates to determine if fragments are inside the semicircle or not.
                v2
                /\
               /__\
           v0 //__\\ v1
         */
        currentManager->Shader( SHADER_FILLED_CIRCLE, 4.0f );
        currentManager->Vertex( -aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );     // v0

        currentManager->Shader( SHADER_FILLED_CIRCLE, 5.0f );
        currentManager->Vertex( aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );      // v1

        currentManager->Shader( SHADER_FILLED_CIRCLE, 6.0f );
        currentManager->Vertex( 0.0f, aRadius * 2.0f, layerDepth );                     // v2

        Restore();
    }
    else
    {
        Save();
        currentManager->Translate( aCenterPoint.x, aCenterPoint.y, layerDepth );
        currentManager->Scale( aRadius, aRadius, 0.0f );
        currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

        // It is enough just to draw a half of the circle vertices to make a semicircle
        currentManager->Vertices( circleContainer.GetAllVertices(), ( CIRCLE_POINTS * 3 ) / 2 );

        Restore();
    }
}


void OPENGL_GAL::drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius,
                                        double aAngle )
{
    if( isUseShader )
    {
        double outerRadius = aRadius + ( lineWidth / 2 );

        Save();
        currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
        currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

        /* Draw a triangle that contains the semicircle, then shade it to leave only
         * the semicircle. Parameters given to setShader are indices of the triangle's vertices
           (if you want to understand more, check the vertex shader source [shader.vert]), the
           radius and the line width. Shader uses this coordinates to determine if fragments are
           inside the semicircle or not.
                v2
                /\
               /__\
           v0 //__\\ v1
         */
        currentManager->Shader( SHADER_STROKED_CIRCLE, 4.0f, aRadius, lineWidth );
        currentManager->Vertex( -outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );     // v0

        currentManager->Shader( SHADER_STROKED_CIRCLE, 5.0f, aRadius, lineWidth );
        currentManager->Vertex( outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );      // v1

        currentManager->Shader( SHADER_STROKED_CIRCLE, 6.0f, aRadius, lineWidth );
        currentManager->Vertex( 0.0f, outerRadius * 2.0f, layerDepth );                     // v2

        Restore();
    }
    else
    {
        // Compute the factors for the unit circle
        double innerScale = 1.0 - lineWidth / aRadius;

        Save();
        currentManager->Translate( aCenterPoint.x, aCenterPoint.y, layerDepth );
        currentManager->Scale( aRadius, aRadius, 0.0f );
        currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

        // Draw the outline
        VERTEX* circle = circleContainer.GetAllVertices();
        int     next;

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

            currentManager->Vertex( circle[i].x * innerScale,    circle[i].y * innerScale,    0.0 );
            currentManager->Vertex( circle[i].x,                 circle[i].y,                 0.0 );
            currentManager->Vertex( circle[next].x * innerScale, circle[next].y * innerScale, 0.0 );

            currentManager->Vertex( circle[i].x,                 circle[i].y,                 0.0 );
            currentManager->Vertex( circle[next].x,              circle[next].y,              0.0 );
            currentManager->Vertex( circle[next].x * innerScale, circle[next].y * innerScale, 0.0 );
        }

        Restore();
    }
}


// FIXME Optimize
void OPENGL_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                          double aEndAngle )
{
    if( aRadius <= 0 )
        return;

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    VECTOR2D startPoint( cos( aStartAngle ), sin( aStartAngle ) );
    VECTOR2D endPoint( cos( aEndAngle ), sin( aEndAngle ) );
    VECTOR2D startEndPoint = startPoint + endPoint;
    VECTOR2D middlePoint   = 0.5 * startEndPoint;

    Save();
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, layerDepth );

    if( isStrokeEnabled )
    {
        if( isUseShader )
        {
            double alphaIncrement = 2.0 * M_PI / CIRCLE_POINTS;
            currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            VECTOR2D p( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
            double   alpha;
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

            double alphaIncrement = 2.0 * M_PI / CIRCLE_POINTS;
            currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

            for( double alpha = aStartAngle; alpha < aEndAngle; )
            {
                double v0[] = { cos( alpha ) * innerScale, sin( alpha ) * innerScale };
                double v1[] = { cos( alpha ) * outerScale, sin( alpha ) * outerScale };

                alpha += alphaIncrement;

                if( alpha > aEndAngle )
                    alpha = aEndAngle;

                double v2[] = { cos( alpha ) * innerScale, sin( alpha ) * innerScale };
                double v3[] = { cos( alpha ) * outerScale, sin( alpha ) * outerScale };

                currentManager->Vertex( v0[0], v0[1], 0.0 );
                currentManager->Vertex( v1[0], v1[1], 0.0 );
                currentManager->Vertex( v2[0], v2[1], 0.0 );

                currentManager->Vertex( v1[0], v1[1], 0.0 );
                currentManager->Vertex( v3[0], v3[1], 0.0 );
                currentManager->Vertex( v2[0], v2[1], 0.0 );
            }

            // Draw line caps
            drawFilledSemiCircle( startPoint, lineWidth / aRadius / 2.0, aStartAngle + M_PI );
            drawFilledSemiCircle( endPoint, lineWidth / aRadius / 2.0, aEndAngle );
        }
    }

    if( isFillEnabled )
    {
        double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;
        double alpha;
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        for( alpha = aStartAngle; ( alpha + alphaIncrement ) < aEndAngle; )
        {
            currentManager->Vertex( middlePoint.x, middlePoint.y,  0.0 );
            currentManager->Vertex( cos( alpha ),  sin( alpha ),   0.0 );
            alpha += alphaIncrement;
            currentManager->Vertex( cos( alpha ),  sin( alpha ),   0.0 );
        }

        currentManager->Vertex( middlePoint.x, middlePoint.y,  0.0 );
        currentManager->Vertex( cos( alpha ),  sin( alpha ),   0.0 );
        currentManager->Vertex( endPoint.x,    endPoint.y,     0.0 );
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
    currentManager->Shader( SHADER_NONE );

    typedef std::vector<OGLPOINT> OGLPOINTS;

    // Do only one heap allocation, can do because we know size in advance.
    // std::vector is then fastest
    OGLPOINTS vertexList( aPointList.size(), OGLPOINT( "fastest" ) );

    glNormal3d( 0.0, 0.0, 1.0 );
    currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    glShadeModel( GL_FLAT );

    TessParams params = { currentManager, tessIntersects };
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
    currentManager->Color( aColor.r, aColor.g, aColor.b, aColor.a );
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
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
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
    currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );
}


void OPENGL_GAL::Translate( const VECTOR2D& aVector )
{
    currentManager->Translate( aVector.x, aVector.y, 0.0f );
}


void OPENGL_GAL::Scale( const VECTOR2D& aScale )
{
    currentManager->Scale( aScale.x, aScale.y, 0.0f );
}


void OPENGL_GAL::Flush()
{
    glFlush();
}


void OPENGL_GAL::Save()
{
    currentManager->PushMatrix();
}


void OPENGL_GAL::Restore()
{
    currentManager->PopMatrix();
}


int OPENGL_GAL::BeginGroup()
{
    isGrouping = true;

    boost::shared_ptr<VERTEX_ITEM> newItem( new VERTEX_ITEM( cachedManager ) );
    int groupNumber = getNewGroupNumber();
    groups.insert( std::make_pair( groupNumber, newItem ) );

    return groupNumber;
}


void OPENGL_GAL::EndGroup()
{
    isGrouping = false;
}


void OPENGL_GAL::ClearCache()
{
    groups.clear();
    cachedManager.Clear();
}


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    groups.erase( aGroupNumber );
}


void OPENGL_GAL::DrawGroup( int aGroupNumber )
{
    cachedManager.DrawItem( *groups[aGroupNumber] );
}


void OPENGL_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    cachedManager.ChangeItemColor( *groups[aGroupNumber], aNewColor );
}


void OPENGL_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    cachedManager.ChangeItemDepth( *groups[aGroupNumber], aDepth );
}


void OPENGL_GAL::computeCircle()
{
    VERTEX* vertex = circleContainer.Allocate( CIRCLE_POINTS );

    // Compute the circle points for a given number of segments
    for( int i = 0; i < CIRCLE_POINTS; ++i )
    {
        vertex->x = 0.0f;
        vertex->y = 0.0f;
        vertex->z = 0.0f;
        vertex++;

        vertex->x = cos( 2.0 * M_PI / CIRCLE_POINTS * i );
        vertex->y = sin( 2.0 * M_PI / CIRCLE_POINTS * i );
        vertex->z = 0.0f;
        vertex++;

        vertex->x = cos( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) );
        vertex->y = sin( 2.0 * M_PI / CIRCLE_POINTS * ( i + 1 ) );
        vertex->z = 0.0f;
        vertex++;
    }
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
    VERTEX_MANAGER* vboManager = param->vboManager;

    if( vboManager )
        vboManager->Vertex( vertex[0], vertex[1], vertex[2] );
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
    wxLogWarning( wxT( "Not tested ") );

    SetCurrent( *glContext );

    // Draw the cursor on the surface
    VECTOR2D cursorPositionWorld = ComputeCursorToWorld( aCursorPosition );

    cursorPositionWorld.x = round( cursorPositionWorld.x / gridSize.x ) * gridSize.x;
    cursorPositionWorld.y = round( cursorPositionWorld.y / gridSize.y ) * gridSize.y;

    aCursorPosition = worldScreenMatrix * cursorPositionWorld;

    // Switch to the main framebuffer and blit the scene
    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glLoadIdentity();

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
    currentManager->Color( gridColor.r, gridColor.g, gridColor.b, gridColor.a );

    currentManager->Shader( SHADER_NONE );

    // Draw the quad for the grid line
    double gridDepth = depthRange.y * 0.75;
    currentManager->Vertex( point1.x, point1.y, gridDepth );
    currentManager->Vertex( point2.x, point2.y, gridDepth );
    currentManager->Vertex( point4.x, point4.y, gridDepth );

    currentManager->Vertex( point1.x, point1.y, gridDepth );
    currentManager->Vertex( point4.x, point4.y, gridDepth );
    currentManager->Vertex( point3.x, point3.y, gridDepth );
}


bool OPENGL_GAL::Show( bool aShow )
{
    bool s = wxGLCanvas::Show( aShow );

    if( aShow )
        wxGLCanvas::Raise();

    return s;
}
