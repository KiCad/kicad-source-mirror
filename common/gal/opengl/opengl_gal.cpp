/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 * Copyright (C) 2013-2015 CERN
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
#include <boost/bind.hpp>

using namespace KIGFX;

static void InitTesselatorCallbacks( GLUtesselator* aTesselator );
const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };
wxGLContext* OPENGL_GAL::glContext = NULL;

OPENGL_GAL::OPENGL_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener,
                        wxEvtHandler* aPaintListener, const wxString& aName ) :
    wxGLCanvas( aParent, wxID_ANY, (int*) glAttributes, wxDefaultPosition, wxDefaultSize,
                wxEXPAND, aName ),
    mouseListener( aMouseListener ),
    paintListener( aPaintListener ),
    cachedManager( true ),
    nonCachedManager( false ),
    overlayManager( false )
{
    if( glContext == NULL )
        glContext = new wxGLContext( this );

    // Check if OpenGL requirements are met
    runTest();

    // Make VBOs use shaders
    cachedManager.SetShader( shader );
    nonCachedManager.SetShader( shader );
    overlayManager.SetShader( shader );

    // Initialize the flags
    isFramebufferInitialized = false;
    isGrouping               = false;
    groupCounter             = 0;

#ifdef RETINA_OPENGL_PATCH
    SetViewWantsBestResolution( true );
#endif

    // Connecting the event handlers
    Connect( wxEVT_PAINT,           wxPaintEventHandler( OPENGL_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION,          wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN,       wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP,         wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK,     wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN,     wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP,       wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK,   wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN,      wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP,        wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK,    wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL,      wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#ifdef USE_OSX_MAGNIFY_EVENT
    Connect( wxEVT_MAGNIFY,         wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif
#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW,    wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif

    SetSize( aParent->GetSize() );
    screenSize = VECTOR2I( aParent->GetSize() );

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.8, 0.8, 0.8, 0.1 ) );

    // Tesselator initialization
    tesselator = gluNewTess();
    InitTesselatorCallbacks( tesselator );

    if( tesselator == NULL )
        throw std::runtime_error( "Could not create the tesselator" );

    gluTessProperty( tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    currentManager = &nonCachedManager;
}


OPENGL_GAL::~OPENGL_GAL()
{
    glFlush();

    gluDeleteTess( tesselator );
    ClearCache();
}


void OPENGL_GAL::BeginDrawing()
{
    SetCurrent( *glContext );
    clientDC = new wxClientDC( this );

#ifdef RETINA_OPENGL_PATCH
    const float scaleFactor = GetBackingScaleFactor();
#else
    const float scaleFactor = 1.0f;
#endif

    // Set up the view port
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, (GLsizei) screenSize.x * scaleFactor, (GLsizei) screenSize.y * scaleFactor );

    // Create the screen transformation
    glOrtho( 0, (GLint) screenSize.x, 0, (GLsizei) screenSize.y,
             -depthRange.x, -depthRange.y );

    if( !isFramebufferInitialized )
    {
        // Prepare rendering target buffers
        compositor.Initialize();
        mainBuffer = compositor.CreateBuffer();
        overlayBuffer = compositor.CreateBuffer();

        isFramebufferInitialized = true;
    }

    // Disable 2D Textures
    glDisable( GL_TEXTURE_2D );

    // Enable the depth buffer
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    // Setup blending, required for transparent objects
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

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

    // Unbind buffers - set compositor for direct drawing
    compositor.SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    // Remove all previously stored items
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
    nonCachedManager.EndDrawing();
    cachedManager.EndDrawing();

    // Overlay container is rendered to a different buffer
    compositor.SetBuffer( overlayBuffer );
    overlayManager.EndDrawing();

    // Be sure that the framebuffer is not colorized (happens on specific GPU&drivers combinations)
    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Draw the remaining contents, blit the rendering targets to the screen, swap the buffers
    compositor.DrawBuffer( mainBuffer );
    compositor.DrawBuffer( overlayBuffer );
    blitCursor();

    glFlush();
    SwapBuffers();

    delete clientDC;
}


void OPENGL_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    const VECTOR2D  startEndVector = aEndPoint - aStartPoint;
    double          lineAngle = startEndVector.Angle();

    currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

    drawLineQuad( aStartPoint, aEndPoint );

    // Line caps
    if( lineWidth > 1.0 )
    {
        drawFilledSemiCircle( aStartPoint, lineWidth / 2, lineAngle + M_PI / 2 );
        drawFilledSemiCircle( aEndPoint,   lineWidth / 2, lineAngle - M_PI / 2 );
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
        drawStrokedSemiCircle( VECTOR2D( 0.0, 0.0 ), aWidth / 2, M_PI / 2 );
        drawStrokedSemiCircle( VECTOR2D( lineLength, 0.0 ), aWidth / 2, -M_PI / 2 );

        Restore();
    }
}


void OPENGL_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    if( isFillEnabled )
    {
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        /* Draw a triangle that contains the circle, then shade it leaving only the circle.
         *  Parameters given to setShader are indices of the triangle's vertices
         *  (if you want to understand more, check the vertex shader source [shader.vert]).
         *  Shader uses this coordinates to determine if fragments are inside the circle or not.
         *       v2
         *       /\
         *      //\\
         *  v0 /_\/_\ v1
         */
        currentManager->Shader( SHADER_FILLED_CIRCLE, 1.0 );
        currentManager->Vertex( aCenterPoint.x - aRadius * sqrt( 3.0f ),            // v0
                                aCenterPoint.y - aRadius, layerDepth );

        currentManager->Shader( SHADER_FILLED_CIRCLE, 2.0 );
        currentManager->Vertex( aCenterPoint.x + aRadius * sqrt( 3.0f ),             // v1
                                aCenterPoint.y - aRadius, layerDepth );

        currentManager->Shader( SHADER_FILLED_CIRCLE, 3.0 );
        currentManager->Vertex( aCenterPoint.x, aCenterPoint.y + aRadius * 2.0f,    // v2
                                layerDepth );
    }

    if( isStrokeEnabled )
    {
        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        /* Draw a triangle that contains the circle, then shade it leaving only the circle.
         *  Parameters given to setShader are indices of the triangle's vertices
         *  (if you want to understand more, check the vertex shader source [shader.vert]).
         *  and the line width. Shader uses this coordinates to determine if fragments are
         *  inside the circle or not.
         *       v2
         *       /\
         *      //\\
         *  v0 /_\/_\ v1
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


void OPENGL_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                          double aEndAngle )
{
    if( aRadius <= 0 )
        return;

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    Save();
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, layerDepth );

    if( isStrokeEnabled )
    {
        const double alphaIncrement = 2.0 * M_PI / CIRCLE_POINTS;
        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        VECTOR2D p( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
        double alpha;

        for( alpha = aStartAngle + alphaIncrement; alpha <= aEndAngle; alpha += alphaIncrement )
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

    if( isFillEnabled )
    {
        const double alphaIncrement = 2 * M_PI / CIRCLE_POINTS;
        double alpha;
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );
        currentManager->Shader( SHADER_NONE );

        // Triangle fan
        for( alpha = aStartAngle; ( alpha + alphaIncrement ) < aEndAngle; )
        {
            currentManager->Vertex( 0.0, 0.0, 0.0 );
            currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, 0.0 );
            alpha += alphaIncrement;
            currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, 0.0 );
        }

        // The last missing triangle
        const VECTOR2D endPoint( cos( aEndAngle ) * aRadius, sin( aEndAngle ) * aRadius );
        currentManager->Vertex( 0.0, 0.0, 0.0 );
        currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, 0.0 );
        currentManager->Vertex( endPoint.x,    endPoint.y,     0.0 );
    }

    Restore();
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


void OPENGL_GAL::DrawPolyline( std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.empty() )
        return;

    currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    // Start from the second point
    for( ++it; it != aPointList.end(); ++it )
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


void OPENGL_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    // Any non convex polygon needs to be tesselated
    // for this purpose the GLU standard functions are used
    currentManager->Shader( SHADER_NONE );
    currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    TessParams params = { currentManager, tessIntersects };
    gluTessBeginPolygon( tesselator, &params );
    gluTessBeginContour( tesselator );

    boost::shared_array<GLdouble> points( new GLdouble[3 * aPointList.size()] );
    int v = 0;
    for( std::deque<VECTOR2D>::const_iterator it = aPointList.begin(); it != aPointList.end(); ++it )
    {
        points[v]     = it->x;
        points[v + 1] = it->y;
        points[v + 2] = layerDepth;
        gluTessVertex( tesselator, &points[v], &points[v] );
        v += 3;
    }

    gluTessEndContour( tesselator );
    gluTessEndPolygon( tesselator );

    // Free allocated intersecting points
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


void OPENGL_GAL::ResizeScreen( int aWidth, int aHeight )
{
    screenSize = VECTOR2I( aWidth, aHeight );

#ifdef RETINA_OPENGL_PATCH
    const float scaleFactor = GetBackingScaleFactor();
#else
    const float scaleFactor = 1.0f;
#endif

    // Resize framebuffers
    compositor.Resize( aWidth * scaleFactor, aHeight * scaleFactor );
    isFramebufferInitialized = false;

    wxGLCanvas::SetSize( aWidth, aHeight );
}


bool OPENGL_GAL::Show( bool aShow )
{
    bool s = wxGLCanvas::Show( aShow );

    if( aShow )
        wxGLCanvas::Raise();

    return s;
}


void OPENGL_GAL::Flush()
{
    glFlush();
}


void OPENGL_GAL::ClearScreen( const COLOR4D& aColor )
{
    // Clear screen
    glClearColor( aColor.r, aColor.g, aColor.b, aColor.a );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void OPENGL_GAL::Transform( const MATRIX3x3D& aTransformation )
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
    cachedManager.FinishItem();
    isGrouping = false;
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


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    // Frees memory in the container as well
    groups.erase( aGroupNumber );
}


void OPENGL_GAL::ClearCache()
{
    groups.clear();
    cachedManager.Clear();
}


void OPENGL_GAL::SaveScreen()
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void OPENGL_GAL::RestoreScreen()
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void OPENGL_GAL::SetTarget( RENDER_TARGET aTarget )
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

    currentTarget = aTarget;
}


RENDER_TARGET OPENGL_GAL::GetTarget() const
{
    return currentTarget;
}


void OPENGL_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int oldTarget = compositor.GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor.SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor.SetBuffer( overlayBuffer );
        break;
    }

    compositor.ClearBuffer();

    // Restore the previous state
    compositor.SetBuffer( oldTarget );
}


void OPENGL_GAL::DrawCursor( const VECTOR2D& aCursorPosition )
{
    // Now we should only store the position of the mouse cursor
    // The real drawing routines are in blitCursor()
    VECTOR2D screenCursor = worldScreenMatrix * aCursorPosition;

    cursorPosition = screenWorldMatrix * VECTOR2D( screenCursor.x, screenSize.y - screenCursor.y );
}


void OPENGL_GAL::drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    compositor.SetBuffer( mainBuffer );

    // We do not need a very precise comparison here (the lineWidth is set by GAL::DrawGrid())
    if( fabs( lineWidth - 2.0 * gridLineWidth / worldScale ) < 0.1 )
        glLineWidth( 1.0 );
    else
        glLineWidth( 2.0 );

    glColor4d( gridColor.r, gridColor.g, gridColor.b, gridColor.a );

    glBegin( GL_LINES );
    glVertex3d( aStartPoint.x, aStartPoint.y, layerDepth );
    glVertex3d( aEndPoint.x, aEndPoint.y, layerDepth );
    glEnd();

    // Restore the default color, so textures will be drawn properly
    glColor4d( 1.0, 1.0, 1.0, 1.0 );
}


void OPENGL_GAL::drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    /* Helper drawing:                   ____--- v3       ^
     *                           ____---- ...   \          \
     *                   ____----      ...       \   end    \
     *     v1    ____----           ...    ____----          \ width
     *       ----                ...___----        \          \
     *       \             ___...--                 \          v
     *        \    ____----...                ____---- v2
     *         ----     ...           ____----
     *  start   \    ...      ____----
     *           \... ____----
     *            ----
     *            v0
     * dots mark triangles' hypotenuses
     */

    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength     = startEndVector.EuclideanNorm();

    if( lineLength <= 0.0 )
        return;

    double   scale          = 0.5 * lineWidth / lineLength;

    // The perpendicular vector also needs transformations
    glm::vec4 vector = currentManager->GetTransformation() *
                       glm::vec4( -startEndVector.y * scale, startEndVector.x * scale, 0.0, 0.0 );

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
    Save();
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

    /* Draw a triangle that contains the semicircle, then shade it to leave only
     * the semicircle. Parameters given to setShader are indices of the triangle's vertices
     * (if you want to understand more, check the vertex shader source [shader.vert]).
     * Shader uses these coordinates to determine if fragments are inside the semicircle or not.
     *       v2
     *       /\
     *      /__\
     *  v0 //__\\ v1
     */
    currentManager->Shader( SHADER_FILLED_CIRCLE, 4.0f );
    currentManager->Vertex( -aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );     // v0

    currentManager->Shader( SHADER_FILLED_CIRCLE, 5.0f );
    currentManager->Vertex( aRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );      // v1

    currentManager->Shader( SHADER_FILLED_CIRCLE, 6.0f );
    currentManager->Vertex( 0.0f, aRadius * 2.0f, layerDepth );                     // v2

    Restore();
}


void OPENGL_GAL::drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius,
                                        double aAngle )
{
    double outerRadius = aRadius + ( lineWidth / 2 );

    Save();
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

    /* Draw a triangle that contains the semicircle, then shade it to leave only
     * the semicircle. Parameters given to setShader are indices of the triangle's vertices
     * (if you want to understand more, check the vertex shader source [shader.vert]), the
     * radius and the line width. Shader uses these coordinates to determine if fragments are
     * inside the semicircle or not.
     *       v2
     *       /\
     *      /__\
     *  v0 //__\\ v1
     */
    currentManager->Shader( SHADER_STROKED_CIRCLE, 4.0f, aRadius, lineWidth );
    currentManager->Vertex( -outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );     // v0

    currentManager->Shader( SHADER_STROKED_CIRCLE, 5.0f, aRadius, lineWidth );
    currentManager->Vertex( outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, layerDepth );      // v1

    currentManager->Shader( SHADER_STROKED_CIRCLE, 6.0f, aRadius, lineWidth );
    currentManager->Vertex( 0.0f, outerRadius * 2.0f, layerDepth );                     // v2

    Restore();
}


void OPENGL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    PostPaint();
}


void OPENGL_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


void OPENGL_GAL::blitCursor()
{
    if( !isCursorEnabled )
        return;

    compositor.SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    VECTOR2D cursorBegin  = cursorPosition - cursorSize / ( 2 * worldScale );
    VECTOR2D cursorEnd    = cursorPosition + cursorSize / ( 2 * worldScale );
    VECTOR2D cursorCenter = ( cursorBegin + cursorEnd ) / 2;

    glDisable( GL_TEXTURE_2D );
    glLineWidth( 1.0 );
    glColor4d( cursorColor.r, cursorColor.g, cursorColor.b, cursorColor.a );

    glBegin( GL_LINES );
    glVertex2d( cursorCenter.x, cursorBegin.y );
    glVertex2d( cursorCenter.x, cursorEnd.y );

    glVertex2d( cursorBegin.x, cursorCenter.y );
    glVertex2d( cursorEnd.x, cursorCenter.y );
    glEnd();
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


bool OPENGL_GAL::runTest()
{
    wxDialog* dialog = new wxDialog( GetParent(), -1, wxT( "opengl test" ),
                                     wxPoint( 50, 50 ), wxSize( 50, 50 ) );
    OPENGL_TEST* test = new OPENGL_TEST( dialog, this );

    dialog->ShowModal();
    bool result = test->IsOk();

    if( !result )
        throw std::runtime_error( test->GetError() );

    return result;
}


OPENGL_GAL::OPENGL_TEST::OPENGL_TEST( wxDialog* aParent, OPENGL_GAL* aGal ) :
    wxGLCanvas( aParent, wxID_ANY, glAttributes, wxDefaultPosition,
                wxDefaultSize, 0, wxT( "GLCanvas" ) ),
    m_parent( aParent ), m_gal( aGal ), m_tested( false ), m_result( false )
{
    Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::OPENGL_TEST::Render ) );
}


void OPENGL_GAL::OPENGL_TEST::Render( wxPaintEvent& WXUNUSED( aEvent ) )
{
    if( !m_tested )
    {
        m_result = true;    // Assume everything is fine, until proven otherwise

        // One test is enough - close the testing dialog when the test is finished
        Disconnect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::OPENGL_TEST::Render ) );
        CallAfter( boost::bind( &wxDialog::EndModal, m_parent, wxID_NONE ) );

        SetCurrent( *OPENGL_GAL::glContext );
        GLenum err = glewInit();

        if( GLEW_OK != err )
        {
            error( (const char*) glewGetErrorString( err ) );
            return;
        }
        else
        {
            wxLogDebug( wxString( wxT( "Status: Using GLEW " ) ) +
                        FROM_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
        }

        // Check the OpenGL version (minimum 2.1 is required)
        if( GLEW_VERSION_2_1 )
        {
            wxLogInfo( wxT( "OpenGL 2.1 supported." ) );
        }
        else
        {
            error( "OpenGL 2.1 or higher is required!" );
            return;
        }

        // Framebuffers have to be supported
        if( !GLEW_EXT_framebuffer_object )
        {
            error( "Framebuffer objects are not supported!" );
            return;
        }

        // Vertex buffer has to be supported
        if( !GLEW_ARB_vertex_buffer_object )
        {
            error( "Vertex buffer objects are not supported!" );
            return;
        }

        // Prepare shaders
        if( !m_gal->shader.LoadBuiltinShader( 0, SHADER_TYPE_VERTEX ) )
        {
            error( "Cannot compile vertex shader!" );
            return;
        }

        if( !m_gal->shader.LoadBuiltinShader( 1, SHADER_TYPE_FRAGMENT ) )
        {
            error( "Cannot compile fragment shader!" );
            return;
        }

        if( !m_gal->shader.Link() )
        {
            error( "Cannot link the shaders!" );
            return;
        }

        m_tested = true;
    }
}


void OPENGL_GAL::OPENGL_TEST::error(const std::string& aError )
{
    m_result = false;
    m_tested = true;
    m_error = aError;
}

// ------------------------------------- // Callback functions for the tesselator // ------------------------------------- // Compare Redbook Chapter 11
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
    param->intersectPoints.push_back( boost::shared_array<GLdouble>( vertex ) );

    memcpy( vertex, coords, 3 * sizeof(GLdouble) );

    *dataOut = vertex;
}


void CALLBACK EdgeCallback( GLboolean aEdgeFlag )
{
    // This callback is needed to force GLU tesselator to use triangles only
}


void CALLBACK ErrorCallback( GLenum aErrorCode )
{
    //throw std::runtime_error( std::string( "Tessellation error: " ) +
                              //std::string( (const char*) gluErrorString( aErrorCode ) );
}


static void InitTesselatorCallbacks( GLUtesselator* aTesselator )
{
    gluTessCallback( aTesselator, GLU_TESS_VERTEX_DATA,  ( void (CALLBACK*)() )VertexCallback );
    gluTessCallback( aTesselator, GLU_TESS_COMBINE_DATA, ( void (CALLBACK*)() )CombineCallback );
    gluTessCallback( aTesselator, GLU_TESS_EDGE_FLAG,    ( void (CALLBACK*)() )EdgeCallback );
    gluTessCallback( aTesselator, GLU_TESS_ERROR,        ( void (CALLBACK*)() )ErrorCallback );
}
