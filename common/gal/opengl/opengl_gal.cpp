/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012-2016 Kicad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2016 CERN
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
#include <gal/opengl/utils.h>
#include <gal/definitions.h>
#include <gl_context_mgr.h>

#include <macros.h>

#ifdef __WXDEBUG__
#include <profile.h>
#include <wx/log.h>
#endif /* __WXDEBUG__ */

#include <limits>
#include <functional>
using namespace std::placeholders;


using namespace KIGFX;


// The current font is "Ubuntu Mono" available under Ubuntu Font Licence 1.0
// (see ubuntu-font-licence-1.0.txt for details)
#include "gl_resources.h"
#include "gl_builtin_shaders.h"
using namespace KIGFX::BUILTIN_FONT;

static void InitTesselatorCallbacks( GLUtesselator* aTesselator );
static const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 8, 0 };

wxGLContext* OPENGL_GAL::glMainContext = NULL;
int OPENGL_GAL::instanceCounter = 0;
GLuint OPENGL_GAL::fontTexture = 0;
bool OPENGL_GAL::isBitmapFontLoaded = false;
SHADER* OPENGL_GAL::shader = NULL;


OPENGL_GAL::OPENGL_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
                        wxEvtHandler* aMouseListener, wxEvtHandler* aPaintListener,
                        const wxString& aName ) :
    wxGLCanvas( aParent, wxID_ANY, (int*) glAttributes, wxDefaultPosition, wxDefaultSize,
                wxEXPAND, aName ),
    options( aDisplayOptions ), mouseListener( aMouseListener ), paintListener( aPaintListener )
{
    if( glMainContext == NULL )
    {
        glMainContext = GL_CONTEXT_MANAGER::Get().CreateCtx( this );
        glPrivContext = glMainContext;
        shader = new SHADER();
    }
    else
    {
        glPrivContext = GL_CONTEXT_MANAGER::Get().CreateCtx( this, glMainContext );
    }

    ++instanceCounter;

    // Check if OpenGL requirements are met
    runTest();

    // Make VBOs use shaders
    cachedManager = new VERTEX_MANAGER( true );
    cachedManager->SetShader( *shader );
    nonCachedManager = new VERTEX_MANAGER( false );
    nonCachedManager->SetShader( *shader );
    overlayManager = new VERTEX_MANAGER( false );
    overlayManager->SetShader( *shader );

    compositor = new OPENGL_COMPOSITOR;
    compositor->SetAntialiasingMode( options.gl_antialiasing_mode );

    // Initialize the flags
    isFramebufferInitialized = false;
    isBitmapFontInitialized  = false;
    isGrouping               = false;
    groupCounter             = 0;

#ifdef RETINA_OPENGL_PATCH
    SetViewWantsBestResolution( true );
#endif

    observerLink = options.Subscribe( this );

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
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
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

    currentManager = nonCachedManager;
}


OPENGL_GAL::~OPENGL_GAL()
{
    GL_CONTEXT_MANAGER::Get().LockCtx( glPrivContext, this );

    --instanceCounter;
    glFlush();
    gluDeleteTess( tesselator );
    ClearCache();

    delete compositor;
    delete cachedManager;
    delete nonCachedManager;
    delete overlayManager;

    GL_CONTEXT_MANAGER::Get().UnlockCtx( glPrivContext );

    // If it was the main context, then it will be deleted
    // when the last OpenGL GAL instance is destroyed (a few lines below)
    if( glPrivContext != glMainContext )
        GL_CONTEXT_MANAGER::Get().DestroyCtx( glPrivContext );

    // Are we destroying the last GAL instance?
    if( instanceCounter == 0 )
    {
        GL_CONTEXT_MANAGER::Get().LockCtx( glMainContext, this );

        if( isBitmapFontLoaded )
        {
            glDeleteTextures( 1, &fontTexture );
            isBitmapFontLoaded = false;
        }

        delete shader;

        GL_CONTEXT_MANAGER::Get().UnlockCtx( glMainContext );
        GL_CONTEXT_MANAGER::Get().DestroyCtx( glMainContext );
        glMainContext = NULL;
    }

}

void OPENGL_GAL::OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& aDisplayOptions )
{
    if(options.gl_antialiasing_mode != compositor->GetAntialiasingMode())
    {
        compositor->SetAntialiasingMode( options.gl_antialiasing_mode );
        isFramebufferInitialized = false;
        Refresh();
    }
}

void OPENGL_GAL::BeginDrawing()
{
    if( !IsShownOnScreen() )
        return;

#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime( "OPENGL_GAL::BeginDrawing()", true );
#endif /* __WXDEBUG__ */

    GL_CONTEXT_MANAGER::Get().LockCtx( glPrivContext, this );

    // Set up the view port
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    // Create the screen transformation (Do the RH-LH conversion here)
    glOrtho( 0, (GLint) screenSize.x, (GLsizei) screenSize.y, 0, -depthRange.x, -depthRange.y );

    if( !isFramebufferInitialized )
    {
        // Prepare rendering target buffers
        compositor->Initialize();
        mainBuffer = compositor->CreateBuffer();
        overlayBuffer = compositor->CreateBuffer();

        isFramebufferInitialized = true;
    }

    compositor->Begin();

    // Disable 2D Textures
    glDisable( GL_TEXTURE_2D );

    glShadeModel( GL_FLAT );

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

    // Remove all previously stored items
    nonCachedManager->Clear();
    overlayManager->Clear();

    cachedManager->BeginDrawing();
    nonCachedManager->BeginDrawing();
    overlayManager->BeginDrawing();

    if( !isBitmapFontInitialized )
    {
        // Keep bitmap font texture always bound to the second texturing unit
        const GLint FONT_TEXTURE_UNIT = 2;

        // Either load the font atlas to video memory, or simply bind it to a texture unit
        if( !isBitmapFontLoaded )
        {
            glActiveTexture( GL_TEXTURE0 + FONT_TEXTURE_UNIT );
            glGenTextures( 1, &fontTexture );
            glBindTexture( GL_TEXTURE_2D, fontTexture );
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, font_image.width, font_image.height,
                          0, GL_RGB, GL_UNSIGNED_BYTE, font_image.pixels );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            checkGlError( "loading bitmap font" );

            glActiveTexture( GL_TEXTURE0 );

            isBitmapFontLoaded = true;
        }
        else
        {
            glActiveTexture( GL_TEXTURE0 + FONT_TEXTURE_UNIT );
            glBindTexture( GL_TEXTURE_2D, fontTexture );
            glActiveTexture( GL_TEXTURE0 );
        }

        // Set shader parameter
        GLint ufm_fontTexture       = shader->AddParameter( "fontTexture" );
        GLint ufm_fontTextureWidth  = shader->AddParameter( "fontTextureWidth" );
        shader->Use();
        shader->SetParameter( ufm_fontTexture,       (int) FONT_TEXTURE_UNIT  );
        shader->SetParameter( ufm_fontTextureWidth,  (int) font_image.width  );
        shader->Deactivate();
        checkGlError( "setting bitmap font sampler as shader parameter" );

        isBitmapFontInitialized = true;
    }

    // Something betreen BeginDrawing and EndDrawing seems to depend on
    // this texture unit being active, but it does not assure it itself.
    glActiveTexture(GL_TEXTURE0);

    // Unbind buffers - set compositor for direct drawing
    compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( "GAL_PROFILE",
                wxT( "OPENGL_GAL::BeginDrawing(): %.1f ms" ), totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}


void OPENGL_GAL::EndDrawing()
{
#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime( "OPENGL_GAL::EndDrawing()", true );
#endif /* __WXDEBUG__ */

    // Cached & non-cached containers are rendered to the same buffer
    compositor->SetBuffer( mainBuffer );
    nonCachedManager->EndDrawing();
    cachedManager->EndDrawing();

    // Overlay container is rendered to a different buffer
    compositor->SetBuffer( overlayBuffer );
    overlayManager->EndDrawing();

    // Be sure that the framebuffer is not colorized (happens on specific GPU&drivers combinations)
    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Draw the remaining contents, blit the rendering targets to the screen, swap the buffers
    compositor->DrawBuffer( mainBuffer );
    compositor->DrawBuffer( overlayBuffer );
    compositor->Present();
    //blitCursor();

    SwapBuffers();
    GL_CONTEXT_MANAGER::Get().UnlockCtx( glPrivContext );

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( "GAL_PROFILE", wxT( "OPENGL_GAL::EndDrawing(): %.1f ms" ), totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}


void OPENGL_GAL::BeginUpdate()
{
    GL_CONTEXT_MANAGER::Get().LockCtx( glPrivContext, this );
    cachedManager->Map();
}


void OPENGL_GAL::EndUpdate()
{
    cachedManager->Unmap();
    GL_CONTEXT_MANAGER::Get().UnlockCtx( glPrivContext );
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
        currentManager->Reserve( 3 );
        currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

        /* Draw a triangle that contains the circle, then shade it leaving only the circle.
         *  Parameters given to Shader() are indices of the triangle's vertices
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
        currentManager->Vertex( aCenterPoint.x + aRadius * sqrt( 3.0f),             // v1
                                aCenterPoint.y - aRadius, layerDepth );

        currentManager->Shader( SHADER_FILLED_CIRCLE, 3.0 );
        currentManager->Vertex( aCenterPoint.x, aCenterPoint.y + aRadius * 2.0f,    // v2
                                layerDepth );
    }

    if( isStrokeEnabled )
    {
        currentManager->Reserve( 3 );
        currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        /* Draw a triangle that contains the circle, then shade it leaving only the circle.
         *  Parameters given to Shader() are indices of the triangle's vertices
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
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );

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
            currentManager->Reserve( 3 );
            currentManager->Vertex( 0.0, 0.0, 0.0 );
            currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, 0.0 );
            alpha += alphaIncrement;
            currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, 0.0 );
        }

        // The last missing triangle
        const VECTOR2D endPoint( cos( aEndAngle ) * aRadius, sin( aEndAngle ) * aRadius );

        currentManager->Reserve( 3 );
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
        currentManager->Reserve( 6 );
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


void OPENGL_GAL::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.size() < 2 )
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


void OPENGL_GAL::DrawPolyline( const VECTOR2D aPointList[], int aListSize )
{
    if( aListSize < 2 )
        return;

    currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

    // Start from the second point
    for( int i = 1; i < aListSize; ++i )
    {
        const VECTOR2D startEndVector = ( aPointList[i] - aPointList[i - 1] );
        double lineAngle = startEndVector.Angle();

        drawLineQuad( aPointList[i - 1], aPointList[i] );

        // There is no need to draw line caps on both ends of polyline's segments
        drawFilledSemiCircle( aPointList[i - 1], lineWidth / 2, lineAngle + M_PI / 2 );
    }

    // ..and now - draw the ending cap
    const VECTOR2D startEndVector = ( aPointList[aListSize - 1] - aPointList[aListSize - 2] );
    double lineAngle = startEndVector.Angle();
    drawFilledSemiCircle( aPointList[aListSize - 1], lineWidth / 2, lineAngle - M_PI / 2 );
}


void OPENGL_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    currentManager->Shader( SHADER_NONE );
    currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    // Any non convex polygon needs to be tesselated
    // for this purpose the GLU standard functions are used
    TessParams params = { currentManager, tessIntersects };
    gluTessBeginPolygon( tesselator, &params );
    gluTessBeginContour( tesselator );

    std::unique_ptr<GLdouble[]> points( new GLdouble[ 3 * aPointList.size() ] );
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


void OPENGL_GAL::DrawPolygon( const VECTOR2D aPointList[], int aListSize )
{
    currentManager->Shader( SHADER_NONE );
    currentManager->Color( fillColor.r, fillColor.g, fillColor.b, fillColor.a );

    // Any non convex polygon needs to be tesselated
    // for this purpose the GLU standard functions are used
    TessParams params = { currentManager, tessIntersects };
    gluTessBeginPolygon( tesselator, &params );
    gluTessBeginContour( tesselator );

    std::unique_ptr<GLdouble[]> points( new GLdouble[3 * aListSize] );
    int v = 0;
    const VECTOR2D* ptr = aPointList;

    for( int i = 0; i < aListSize; ++i )
    {
        points[v]     = ptr->x;
        points[v + 1] = ptr->y;
        points[v + 2] = layerDepth;
        gluTessVertex( tesselator, &points[v], &points[v] );
        ++ptr;
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


void OPENGL_GAL::BitmapText( const wxString& aText, const VECTOR2D& aPosition,
                             double aRotationAngle )
{
    wxASSERT_MSG( !IsTextMirrored(), "No support for mirrored text using bitmap fonts." );

    // Compute text size, so it can be properly justified
    VECTOR2D textSize;
    float commonOffset;
    std::tie( textSize, commonOffset ) = computeBitmapTextSize( aText );

    const double SCALE = GetGlyphSize().y / textSize.y;
    int tildas = 0;
    bool overbar = false;

    int overbarLength = 0;
    double overbarHeight = textSize.y;

    Save();

    currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
    currentManager->Translate( aPosition.x, aPosition.y, layerDepth );
    currentManager->Rotate( aRotationAngle, 0.0f, 0.0f, -1.0f );

    double sx = SCALE * ( globalFlipX ? -1.0 : 1.0 );
    double sy = SCALE * ( globalFlipY ? -1.0 : 1.0 );

    currentManager->Scale( sx, sy, 0 );
    currentManager->Translate( 0, -commonOffset, 0 );

    switch( GetHorizontalJustify() )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        Translate( VECTOR2D( -textSize.x / 2.0, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        //if( !IsTextMirrored() )
            Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        //if( IsTextMirrored() )
            //Translate( VECTOR2D( -textSize.x, 0 ) );
        break;
    }

    switch( GetVerticalJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        Translate( VECTOR2D( 0, -textSize.y ) );
        overbarHeight = -textSize.y / 2.0;
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        Translate( VECTOR2D( 0, -textSize.y / 2.0 ) );
        overbarHeight = 0;
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;
    }

    for( unsigned int ii = 0; ii < aText.length(); ++ii )
    {
        const unsigned int c = aText[ii];

        wxASSERT_MSG( LookupGlyph(c) != nullptr, wxT( "Missing character in bitmap font atlas." ) );
        wxASSERT_MSG( c != '\n' && c != '\r', wxT( "No support for multiline bitmap text yet" ) );

        // Handle overbar
        if( c == '~' )
        {
            overbar = !overbar;
            ++tildas;
            continue;
        }
        else if( tildas > 0 )
        {
            if( tildas % 2 == 1 )
            {
                if( overbar )                   // Overbar begins
                    overbarLength = 0;
                else if( overbarLength > 0 )    // Overbar finishes
                    drawBitmapOverbar( overbarLength, overbarHeight );

                --tildas;
            }

            // Draw tilda characters if there are any remaining
            for( int jj = 0; jj < tildas / 2; ++jj )
                overbarLength += drawBitmapChar( '~' );

            tildas = 0;
        }

        overbarLength += drawBitmapChar( c );
    }

    // Handle the case when overbar is active till the end of the drawn text
    currentManager->Translate( 0, commonOffset, 0 );

    if( overbar )
        drawBitmapOverbar( overbarLength, overbarHeight );

    Restore();
}


void OPENGL_GAL::DrawGrid()
{
    if( !gridVisibility )
        return;

    int gridScreenSizeDense  = KiROUND( gridSize.x * worldScale );
    int gridScreenSizeCoarse = KiROUND( gridSize.x * static_cast<double>( gridTick ) * worldScale );

    // Check if the grid would not be too dense
    if( std::max( gridScreenSizeDense, gridScreenSizeCoarse ) < gridDrawThreshold )
        return;

    SetTarget( TARGET_NONCACHED );
    compositor->SetBuffer( mainBuffer );

    // Draw the grid
    // For the drawing the start points, end points and increments have
    // to be calculated in world coordinates
    VECTOR2D worldStartPoint = screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
    VECTOR2D worldEndPoint = screenWorldMatrix * VECTOR2D( screenSize );

    // Compute grid variables
    int gridStartX = KiROUND( worldStartPoint.x / gridSize.x );
    int gridEndX = KiROUND( worldEndPoint.x / gridSize.x );
    int gridStartY = KiROUND( worldStartPoint.y / gridSize.y );
    int gridEndY = KiROUND( worldEndPoint.y / gridSize.y );

    // Correct the index, else some lines are not correctly painted
    gridStartY -= std::abs( gridOrigin.y / gridSize.y ) + 1;
    gridEndY += std::abs( gridOrigin.y / gridSize.y ) + 1;

    if ( gridStartX <= gridEndX )
    {
        gridStartX -= std::abs( gridOrigin.x / gridSize.x ) + 1;
        gridEndX += std::abs( gridOrigin.x / gridSize.x ) + 1;
    }
    else
    {
        gridStartX += std::abs( gridOrigin.x / gridSize.x ) + 1;
        gridEndX -= std::abs( gridOrigin.x / gridSize.x ) + 1;
    }

    int dirX = gridStartX >= gridEndX ? -1 : 1;
    int dirY = gridStartY >= gridEndY ? -1 : 1;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );

    if( gridStyle == GRID_STYLE_DOTS )
    {
        glEnable( GL_STENCIL_TEST );
        glStencilFunc( GL_ALWAYS, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
        glColor4d( 0.0, 0.0, 0.0, 0.0 );
    }
    else
    {
        glColor4d( gridColor.r, gridColor.g, gridColor.b, 1.0 );
    }

    // Vertical lines
    for( int j = gridStartY; j != gridEndY; j += dirY )
    {
        if( j % gridTick == 0 && gridScreenSizeDense > gridDrawThreshold )
            glLineWidth( 2.0 );
        else
            glLineWidth( 1.0 );

        if( ( j % gridTick == 0 && gridScreenSizeCoarse > gridDrawThreshold )
            || gridScreenSizeDense > gridDrawThreshold )
        {
            glBegin( GL_LINES );
            glVertex2d( gridStartX * gridSize.x, j * gridSize.y + gridOrigin.y );
            glVertex2d( gridEndX * gridSize.x, j * gridSize.y + gridOrigin.y );
            glEnd();
        }
    }

    if( gridStyle == GRID_STYLE_DOTS )
    {
        glStencilFunc( GL_NOTEQUAL, 0, 1 );
        glColor4d( gridColor.r, gridColor.g, gridColor.b, 1.0 );
    }

    // Horizontal lines
    for( int i = gridStartX; i != gridEndX; i += dirX )
    {
        if( i % gridTick == 0 && gridScreenSizeDense > gridDrawThreshold )
            glLineWidth( 2.0 );
        else
            glLineWidth( 1.0 );

        if( ( i % gridTick == 0 && gridScreenSizeCoarse > gridDrawThreshold )
            || gridScreenSizeDense > gridDrawThreshold )
        {
            glBegin( GL_LINES );
            glVertex2d( i * gridSize.x + gridOrigin.x, gridStartY * gridSize.y );
            glVertex2d( i * gridSize.x + gridOrigin.x, gridEndY * gridSize.y );
            glEnd();
        }
    }

    if( gridStyle == GRID_STYLE_DOTS )
        glDisable( GL_STENCIL_TEST );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
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
    compositor->Resize( aWidth * scaleFactor, aHeight * scaleFactor );
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
    compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );
    glClearColor( aColor.r, aColor.g, aColor.b, aColor.a );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
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

    std::shared_ptr<VERTEX_ITEM> newItem = std::make_shared<VERTEX_ITEM>( *cachedManager );
    int groupNumber = getNewGroupNumber();
    groups.insert( std::make_pair( groupNumber, newItem ) );

    return groupNumber;
}


void OPENGL_GAL::EndGroup()
{
    cachedManager->FinishItem();
    isGrouping = false;
}


void OPENGL_GAL::DrawGroup( int aGroupNumber )
{
    cachedManager->DrawItem( *groups[aGroupNumber] );
}


void OPENGL_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    cachedManager->ChangeItemColor( *groups[aGroupNumber], aNewColor );
}


void OPENGL_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    cachedManager->ChangeItemDepth( *groups[aGroupNumber], aDepth );
}


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    // Frees memory in the container as well
    groups.erase( aGroupNumber );
}


void OPENGL_GAL::ClearCache()
{
    groups.clear();
    cachedManager->Clear();
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
        currentManager = cachedManager;
        break;

    case TARGET_NONCACHED:
        currentManager = nonCachedManager;
        break;

    case TARGET_OVERLAY:
        currentManager = overlayManager;
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
    unsigned int oldTarget = compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor->SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor->SetBuffer( overlayBuffer );
        break;
    }

    compositor->ClearBuffer();

    // Restore the previous state
    compositor->SetBuffer( oldTarget );
}


void OPENGL_GAL::DrawCursor( const VECTOR2D& aCursorPosition )
{
    // Now we should only store the position of the mouse cursor
    // The real drawing routines are in blitCursor()
    VECTOR2D screenCursor = worldScreenMatrix * aCursorPosition;

    cursorPosition = screenWorldMatrix * VECTOR2D( screenCursor.x, screenSize.y - screenCursor.y );
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

    currentManager->Reserve( 6 );

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

    currentManager->Reserve( 3 );
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

    /* Draw a triangle that contains the semicircle, then shade it to leave only
     * the semicircle. Parameters given to Shader() are indices of the triangle's vertices
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

    currentManager->Reserve( 3 );
    currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

    /* Draw a triangle that contains the semicircle, then shade it to leave only
     * the semicircle. Parameters given to Shader() are indices of the triangle's vertices
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


int OPENGL_GAL::drawBitmapChar( unsigned long aChar )
{
    const float TEX_X = font_image.width;
    const float TEX_Y = font_image.height;

    const FONT_GLYPH_TYPE* glyph = LookupGlyph(aChar);
    if( !glyph ) return 0;

    const float X = glyph->atlas_x + font_information.smooth_pixels;
    const float Y = glyph->atlas_y + font_information.smooth_pixels;
    const float XOFF =  glyph->minx;

    // adjust for height rounding
    const float round_adjust =   ( glyph->maxy - glyph->miny )
                               - float( glyph->atlas_h - font_information.smooth_pixels * 2 );
    const float top_adjust   = font_information.max_y - glyph->maxy;
    const float YOFF = round_adjust + top_adjust;
    const float W    = glyph->atlas_w  - font_information.smooth_pixels *2;
    const float H    = glyph->atlas_h  - font_information.smooth_pixels *2;
    const float B    = 0;

    currentManager->Reserve( 6 );
    Translate( VECTOR2D( XOFF, YOFF ) );
    /* Glyph:
    * v0    v1
    *   +--+
    *   | /|
    *   |/ |
    *   +--+
    * v2    v3
    */
    currentManager->Shader( SHADER_FONT, X / TEX_X, ( Y + H ) / TEX_Y );
    currentManager->Vertex( -B,      -B, 0 );             // v0

    currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, ( Y + H ) / TEX_Y );
    currentManager->Vertex( W + B,   -B, 0 );             // v1

    currentManager->Shader( SHADER_FONT, X / TEX_X, Y / TEX_Y );
    currentManager->Vertex( -B,   H + B, 0 );             // v2


    currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, ( Y + H ) / TEX_Y );
    currentManager->Vertex( W + B, -B, 0 );               // v1

    currentManager->Shader( SHADER_FONT, X / TEX_X, Y / TEX_Y );
    currentManager->Vertex( -B,  H + B, 0 );              // v2

    currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, Y / TEX_Y );
    currentManager->Vertex( W + B,  H + B, 0 );           // v3

    Translate( VECTOR2D( -XOFF + glyph->advance, -YOFF ) );

    return glyph->advance;
}


void OPENGL_GAL::drawBitmapOverbar( double aLength, double aHeight )
{
    // To draw an overbar, simply draw an overbar
    const FONT_GLYPH_TYPE* glyph = LookupGlyph( '_' );
    const float H = glyph->maxy - glyph->miny;

    Save();

    Translate( VECTOR2D( -aLength, -aHeight-1.5*H ) );

    currentManager->Reserve( 6 );
    currentManager->Color( strokeColor.r, strokeColor.g, strokeColor.b, 1 );

    currentManager->Shader( 0 );

    currentManager->Vertex( 0, 0, 0 );          // v0
    currentManager->Vertex( aLength, 0, 0 );    // v1
    currentManager->Vertex( 0, H, 0 );          // v2

    currentManager->Vertex( aLength, 0, 0 );    // v1
    currentManager->Vertex( 0, H, 0 );          // v2
    currentManager->Vertex( aLength, H, 0 );    // v3

    Restore();
}

std::pair<VECTOR2D, float> OPENGL_GAL::computeBitmapTextSize( const wxString& aText ) const
{
    VECTOR2D textSize( 0, 0 );
    float commonOffset = std::numeric_limits<float>::max();
    bool wasTilda = false;

    for( unsigned int i = 0; i < aText.length(); ++i )
    {
        // Remove overbar control characters
        if( aText[i] == '~' )
        {
            if( !wasTilda )
            {
                // Only double tildas are counted as characters, so skip it as it might
                // be an overbar control character
                wasTilda = true;
                continue;
            }
            else
            {
                // Double tilda detected, reset the state and process as a normal character
                wasTilda = false;
            }
        }

        const FONT_GLYPH_TYPE* glyph = LookupGlyph(aText[i]);
        if( glyph ) {
            textSize.x  += glyph->advance;
            textSize.y   = std::max<float>( textSize.y, font_information.max_y - glyph->miny );
            commonOffset = std::min<float>( font_information.max_y - glyph->maxy, commonOffset );
        }
    }

    textSize.y -= commonOffset;

    return std::make_pair( textSize, commonOffset );
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

    compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

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
    static bool tested = false;
    static bool testResult = false;
    std::string errorMessage = "OpenGL test failed";

    if( !tested )
    {
        wxDialog dlgtest( GetParent(), -1, wxT( "opengl test" ), wxPoint( 50, 50 ),
                        wxDLG_UNIT( GetParent(), wxSize( 50, 50 ) ) );
        OPENGL_TEST* test = new OPENGL_TEST( &dlgtest, this, glPrivContext );

        dlgtest.Raise();         // on Linux, on some windows managers (Unity for instance) this is needed to actually show the dialog
        dlgtest.ShowModal();
        testResult = test->IsOk();

        if( !testResult )
            errorMessage = test->GetError();
    }

    if( !testResult )
        throw std::runtime_error( errorMessage );

    return testResult;
}


OPENGL_GAL::OPENGL_TEST::OPENGL_TEST( wxDialog* aParent, OPENGL_GAL* aGal, wxGLContext* aContext ) :
    wxGLCanvas( aParent, wxID_ANY, glAttributes, wxDefaultPosition,
                wxDefaultSize, 0, wxT( "GLCanvas" ) ),
    m_parent( aParent ), m_gal( aGal ), m_context( aContext ), m_tested( false ), m_result( false )
{
    m_timeoutTimer.SetOwner( this );
    Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::OPENGL_TEST::Render ) );
    Connect( wxEVT_TIMER, wxTimerEventHandler( OPENGL_GAL::OPENGL_TEST::OnTimeout ) );
    m_parent->Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::OPENGL_TEST::OnDialogPaint ), NULL, this );
}


void OPENGL_GAL::OPENGL_TEST::Render( wxPaintEvent& WXUNUSED( aEvent ) )
{
    if( !m_tested )
    {
        if( !IsShownOnScreen() )
            return;

        m_timeoutTimer.Stop();
        m_result = true;    // Assume everything is fine, until proven otherwise

        // One test is enough - close the testing dialog when the test is finished
        Disconnect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::OPENGL_TEST::Render ) );
        CallAfter( std::bind( &wxDialog::EndModal, m_parent, wxID_NONE ) );

        GL_CONTEXT_MANAGER::Get().LockCtx( m_context, this );
        GLenum err = glewInit();

        if( GLEW_OK != err )
            error( (const char*) glewGetErrorString( err ) );

        // Check the OpenGL version (minimum 2.1 is required)
        else if( !GLEW_VERSION_2_1 )
            error( "OpenGL 2.1 or higher is required!" );

        // Framebuffers have to be supported
        else if( !GLEW_EXT_framebuffer_object )
            error( "Framebuffer objects are not supported!" );

        // Vertex buffer has to be supported
        else if( !GLEW_ARB_vertex_buffer_object )
            error( "Vertex buffer objects are not supported!" );

        // Prepare shaders
        else if( !m_gal->shader->IsLinked() && !m_gal->shader->LoadShaderFromStrings( SHADER_TYPE_VERTEX, BUILTIN_SHADERS::kicad_vertex_shader ) )
            error( "Cannot compile vertex shader!" );

        else if( !m_gal->shader->IsLinked() && !m_gal->shader->LoadShaderFromStrings(SHADER_TYPE_FRAGMENT, BUILTIN_SHADERS::kicad_fragment_shader  ) )
            error( "Cannot compile fragment shader!" );

        else if( !m_gal->shader->IsLinked() && !m_gal->shader->Link() )
            error( "Cannot link the shaders!" );

        // Check if video card supports textures big enough to fit font atlas
        int maxTextureSize;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );

        if( maxTextureSize < (int) font_image.width || maxTextureSize < (int)font_image.height )
        {
            // TODO implement software texture scaling
            // for bitmap fonts and use a higher resolution texture?
            error( "Requested texture size is not supported" );
        }

        GL_CONTEXT_MANAGER::Get().UnlockCtx( m_context );
        m_tested = true;
    }
}


void OPENGL_GAL::OPENGL_TEST::OnTimeout( wxTimerEvent& aEvent )
{
    error( "Could not create OpenGL canvas" );
    m_parent->EndModal( wxID_NONE );
}


void OPENGL_GAL::OPENGL_TEST::OnDialogPaint( wxPaintEvent& aEvent )
{
    // GL canvas may never appear on the screen (e.g. due to missing GL extensions), and the test
    // will not be run. Therefore give at most a second to perform the test, otherwise we conclude
    // it has failed.
    // Also, wxWidgets OnShow event is triggered before a window is shown, therefore here we use
    // OnPaint event, which is executed when a window is actually visible.
    m_timeoutTimer.StartOnce( 1000 );
}


void OPENGL_GAL::OPENGL_TEST::error( const std::string& aError )
{
    m_timeoutTimer.Stop();
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

    assert( vboManager );
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
