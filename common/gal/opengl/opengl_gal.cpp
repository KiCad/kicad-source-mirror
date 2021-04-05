/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012-2021 Kicad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2017 CERN
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

// Apple, in their infinite wisdom, has decided to mark OpenGL as deprecated.
// Luckily we can silence warnings about its deprecation.
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION 1
#endif

#include <gl_utils.h>

#include <advanced_config.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/opengl/utils.h>
#include <gal/definitions.h>
#include <gl_context_mgr.h>
#include <geometry/shape_poly_set.h>
#include <bitmap_base.h>
#include <bezier_curves.h>
#include <math/util.h> // for KiROUND
#include <trace_helpers.h>

#include <wx/frame.h>

#include <macros.h>

#ifdef __WXDEBUG__
#include <profile.h>
#include <wx/log.h>
#endif /* __WXDEBUG__ */

#include <functional>
#include <limits>
#include <memory>
using namespace std::placeholders;
using namespace KIGFX;

// A ugly workaround to avoid serious issues (crashes) when using bitmaps cache
// to speedup redraw.
// issues arise when using bitmaps in page layout, when the page layout containd bitmaps,
// and is common to schematic and board editor,
// and the schematic is a hierarchy and when using cross-probing
// When the cross probing from pcbnew to eeschema switches to a sheet, the bitmaps cache
// becomes broken (in fact the associated texture).
// I hope (JPC) it will be fixed later, but a slighty slower refresh is better than a crash
#define DISABLE_BITMAP_CACHE

// The current font is "Ubuntu Mono" available under Ubuntu Font Licence 1.0
// (see ubuntu-font-licence-1.0.txt for details)
#include "gl_resources.h"
#include "gl_builtin_shaders.h"
using namespace KIGFX::BUILTIN_FONT;

static void      InitTesselatorCallbacks( GLUtesselator* aTesselator );
static const int glAttributes[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 8, 0 };

wxGLContext* OPENGL_GAL::m_glMainContext = NULL;
int          OPENGL_GAL::m_instanceCounter = 0;
GLuint       OPENGL_GAL::g_fontTexture = 0;
bool         OPENGL_GAL::m_isBitmapFontLoaded = false;

namespace KIGFX
{
class GL_BITMAP_CACHE
{
public:
    GL_BITMAP_CACHE() {}

    ~GL_BITMAP_CACHE();

    GLuint RequestBitmap( const BITMAP_BASE* aBitmap );

private:
    struct CACHED_BITMAP
    {
        GLuint id;
        int    w, h;
    };

    GLuint cacheBitmap( const BITMAP_BASE* aBitmap );

    std::map<const BITMAP_BASE*, CACHED_BITMAP> m_bitmaps;
};

}; // namespace KIGFX


GL_BITMAP_CACHE::~GL_BITMAP_CACHE()
{
    for( auto& bitmap : m_bitmaps )
        glDeleteTextures( 1, &bitmap.second.id );
}


GLuint GL_BITMAP_CACHE::RequestBitmap( const BITMAP_BASE* aBitmap )
{
    auto it = m_bitmaps.find( aBitmap );

    if( it != m_bitmaps.end() )
    {
        // A bitmap is found in cache bitmap.
        // Ensure the associated texture is still valid (can be destroyed somewhere)
        if( glIsTexture( it->second.id ) )
            return it->second.id;

        // else if not valid, it will be recreated.
    }

    return cacheBitmap( aBitmap );
}


GLuint GL_BITMAP_CACHE::cacheBitmap( const BITMAP_BASE* aBitmap )
{
    CACHED_BITMAP bmp;

    bmp.w = aBitmap->GetSizePixels().x;
    bmp.h = aBitmap->GetSizePixels().y;

    // The bitmap size needs to be a multiple of 4.
    // This is easiest to achieve by ensuring that each row
    // has a multiple of 4 pixels
    int extra_w = bmp.w % 4;

    if( extra_w )
        extra_w = 4 - extra_w;

    GLuint textureID;
    glGenTextures( 1, &textureID );

    // make_unique initializes this to 0, so extra pixels are transparent
    auto           buf = std::make_unique<uint8_t[]>( ( bmp.w + extra_w ) * bmp.h * 4 );
    const wxImage& imgData = *aBitmap->GetImageData();

    for( int y = 0; y < bmp.h; y++ )
    {
        for( int x = 0; x < bmp.w; x++ )
        {
            uint8_t* p = buf.get() + ( ( bmp.w + extra_w ) * y + x ) * 4;

            p[0] = imgData.GetRed( x, y );
            p[1] = imgData.GetGreen( x, y );
            p[2] = imgData.GetBlue( x, y );

            if( imgData.HasAlpha() )
                p[3] = imgData.GetAlpha( x, y );
            else if( imgData.HasMask() && p[0] == imgData.GetMaskRed()
                     && p[1] == imgData.GetMaskGreen() && p[2] == imgData.GetMaskBlue() )
                p[3] = wxALPHA_TRANSPARENT;
            else
                p[3] = wxALPHA_OPAQUE;
        }
    }

    glBindTexture( GL_TEXTURE_2D, textureID );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bmp.w + extra_w, bmp.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                  buf.get() );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    bmp.id = textureID;

#ifndef DISABLE_BITMAP_CACHE
    m_bitmaps[aBitmap] = bmp;
#endif

    return textureID;
}

OPENGL_GAL::OPENGL_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
                        wxEvtHandler* aMouseListener, wxEvtHandler* aPaintListener,
                        const wxString& aName ) :
        GAL( aDisplayOptions ),
        HIDPI_GL_CANVAS( aParent, wxID_ANY, (int*) glAttributes, wxDefaultPosition, wxDefaultSize,
                         wxEXPAND, aName ),
        m_mouseListener( aMouseListener ),
        m_paintListener( aPaintListener ),
        m_currentManager( nullptr ),
        m_cachedManager( nullptr ),
        m_nonCachedManager( nullptr ),
        m_overlayManager( nullptr ),
        m_mainBuffer( 0 ),
        m_overlayBuffer( 0 ),
        m_isContextLocked( false ),
        m_lockClientCookie( 0 )
{
    if( m_glMainContext == NULL )
    {
        m_glMainContext = GL_CONTEXT_MANAGER::Get().CreateCtx( this );

        m_glPrivContext = m_glMainContext;
    }
    else
    {
        m_glPrivContext = GL_CONTEXT_MANAGER::Get().CreateCtx( this, m_glMainContext );
    }

    m_shader = new SHADER();
    ++m_instanceCounter;

    m_bitmapCache = std::make_unique<GL_BITMAP_CACHE>();

    m_compositor = new OPENGL_COMPOSITOR;
    m_compositor->SetAntialiasingMode( m_options.gl_antialiasing_mode );

    // Initialize the flags
    m_isFramebufferInitialized = false;
    m_isBitmapFontInitialized = false;
    m_isInitialized = false;
    m_isGrouping = false;
    m_groupCounter = 0;

    // Connect the native cursor handler
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( OPENGL_GAL::onSetNativeCursor ), NULL,
             this );

    // Connecting the event handlers
    Connect( wxEVT_PAINT, wxPaintEventHandler( OPENGL_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    Connect( wxEVT_MAGNIFY, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif
#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif

    SetSize( aParent->GetClientSize() );
    m_screenSize = VECTOR2I( GetNativePixelSize() );

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.8, 0.8, 0.8, 0.1 ) );
    SetAxesColor( COLOR4D( BLUE ) );

    // Tesselator initialization
    m_tesselator = gluNewTess();
    InitTesselatorCallbacks( m_tesselator );

    gluTessProperty( m_tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    SetTarget( TARGET_NONCACHED );

    // Avoid unitialized variables:
    ufm_worldPixelSize = 1;
    ufm_screenPixelSize = 1;
    ufm_pixelSizeMultiplier = 1;
}


OPENGL_GAL::~OPENGL_GAL()
{
    GL_CONTEXT_MANAGER::Get().LockCtx( m_glPrivContext, this );

    --m_instanceCounter;
    glFlush();
    gluDeleteTess( m_tesselator );
    ClearCache();

    delete m_compositor;

    if( m_isInitialized )
    {
        delete m_cachedManager;
        delete m_nonCachedManager;
        delete m_overlayManager;
    }

    GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glPrivContext );

    // If it was the main context, then it will be deleted
    // when the last OpenGL GAL instance is destroyed (a few lines below)
    if( m_glPrivContext != m_glMainContext )
        GL_CONTEXT_MANAGER::Get().DestroyCtx( m_glPrivContext );

    delete m_shader;

    // Are we destroying the last GAL instance?
    if( m_instanceCounter == 0 )
    {
        GL_CONTEXT_MANAGER::Get().LockCtx( m_glMainContext, this );

        if( m_isBitmapFontLoaded )
        {
            glDeleteTextures( 1, &g_fontTexture );
            m_isBitmapFontLoaded = false;
        }

        GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glMainContext );
        GL_CONTEXT_MANAGER::Get().DestroyCtx( m_glMainContext );
        m_glMainContext = NULL;
    }
}


wxString OPENGL_GAL::CheckFeatures( GAL_DISPLAY_OPTIONS& aOptions )
{
    wxString retVal = wxEmptyString;

    wxFrame* testFrame = new wxFrame( NULL, wxID_ANY, wxT( "" ), wxDefaultPosition, wxSize( 1, 1 ),
                                      wxFRAME_TOOL_WINDOW | wxNO_BORDER );

    KIGFX::OPENGL_GAL* opengl_gal = nullptr;

    try
    {
        opengl_gal = new KIGFX::OPENGL_GAL( aOptions, testFrame );

        testFrame->Raise();
        testFrame->Show();

        GAL_CONTEXT_LOCKER lock( opengl_gal );
        opengl_gal->init();
    }
    catch( std::runtime_error& err )
    {
        //Test failed
        retVal = wxString( err.what() );
    }

    delete opengl_gal;
    delete testFrame;

    return retVal;
}


void OPENGL_GAL::PostPaint( wxPaintEvent& aEvent )
{
    // posts an event to m_paint_listener to ask for redraw the canvas.
    if( m_paintListener )
        wxPostEvent( m_paintListener, aEvent );
}


bool OPENGL_GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    bool refresh = false;

    if( m_options.gl_antialiasing_mode != m_compositor->GetAntialiasingMode() )
    {
        m_compositor->SetAntialiasingMode( m_options.gl_antialiasing_mode );
        m_isFramebufferInitialized = false;
        refresh = true;
    }

    if( m_options.m_scaleFactor != GetScaleFactor() )
    {
        SetScaleFactor( m_options.m_scaleFactor );
        refresh = true;
    }

    if( super::updatedGalDisplayOptions( aOptions ) || refresh )
    {
        Refresh();
        refresh = true;
    }

    return refresh;
}


double OPENGL_GAL::getWorldPixelSize() const
{
    MATRIX3x3D matrix = GetScreenWorldMatrix();
    return std::min( std::abs( matrix.GetScale().x ), std::abs( matrix.GetScale().y ) );
}


VECTOR2D OPENGL_GAL::getScreenPixelSize() const
{
    double sf = GetScaleFactor();
    return VECTOR2D( 2.0 / (double) ( m_screenSize.x * sf ), 2.0 / (double) ( m_screenSize.y * sf ) );
}


void OPENGL_GAL::beginDrawing()
{
#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime( "OPENGL_GAL::beginDrawing()", true );
#endif /* __WXDEBUG__ */

    wxASSERT_MSG( m_isContextLocked, "GAL_DRAWING_CONTEXT RAII object should have locked context. "
                                     "Calling GAL::beginDrawing() directly is not allowed." );

    wxASSERT_MSG( IsVisible(), "GAL::beginDrawing() must not be entered when GAL is not visible. "
                               "Other drawing routines will expect everything to be initialized "
                               "which will not be the case." );

    if( !m_isInitialized )
        init();

    // Set up the view port
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    // Create the screen transformation (Do the RH-LH conversion here)
    glOrtho( 0, (GLint) m_screenSize.x, (GLsizei) m_screenSize.y, 0,
             -m_depthRange.x, -m_depthRange.y );

    if( !m_isFramebufferInitialized )
    {
        // Prepare rendering target buffers
        m_compositor->Initialize();
        m_mainBuffer = m_compositor->CreateBuffer();
        try
        {
            m_overlayBuffer = m_compositor->CreateBuffer();
        }
        catch( const std::runtime_error& )
        {
            wxLogVerbose( "Could not create a framebuffer for overlays.\n" );
            m_overlayBuffer = 0;
        }
        m_isFramebufferInitialized = true;
    }

    m_compositor->Begin();

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
    matrixData[0] = m_worldScreenMatrix.m_data[0][0];
    matrixData[1] = m_worldScreenMatrix.m_data[1][0];
    matrixData[2] = m_worldScreenMatrix.m_data[2][0];
    matrixData[4] = m_worldScreenMatrix.m_data[0][1];
    matrixData[5] = m_worldScreenMatrix.m_data[1][1];
    matrixData[6] = m_worldScreenMatrix.m_data[2][1];
    matrixData[12] = m_worldScreenMatrix.m_data[0][2];
    matrixData[13] = m_worldScreenMatrix.m_data[1][2];
    matrixData[14] = m_worldScreenMatrix.m_data[2][2];
    glLoadMatrixd( matrixData );

    // Set defaults
    SetFillColor( m_fillColor );
    SetStrokeColor( m_strokeColor );

    // Remove all previously stored items
    m_nonCachedManager->Clear();
    m_overlayManager->Clear();

    m_cachedManager->BeginDrawing();
    m_nonCachedManager->BeginDrawing();
    m_overlayManager->BeginDrawing();

    if( !m_isBitmapFontInitialized )
    {
        // Keep bitmap font texture always bound to the second texturing unit
        const GLint FONT_TEXTURE_UNIT = 2;

        // Either load the font atlas to video memory, or simply bind it to a texture unit
        if( !m_isBitmapFontLoaded )
        {
            glActiveTexture( GL_TEXTURE0 + FONT_TEXTURE_UNIT );
            glGenTextures( 1, &g_fontTexture );
            glBindTexture( GL_TEXTURE_2D, g_fontTexture );
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, font_image.width, font_image.height, 0, GL_RGB,
                          GL_UNSIGNED_BYTE, font_image.pixels );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            checkGlError( "loading bitmap font", __FILE__, __LINE__ );

            glActiveTexture( GL_TEXTURE0 );

            m_isBitmapFontLoaded = true;
        }
        else
        {
            glActiveTexture( GL_TEXTURE0 + FONT_TEXTURE_UNIT );
            glBindTexture( GL_TEXTURE_2D, g_fontTexture );
            glActiveTexture( GL_TEXTURE0 );
        }

        // Set shader parameter
        GLint ufm_fontTexture = m_shader->AddParameter( "fontTexture" );
        GLint ufm_fontTextureWidth = m_shader->AddParameter( "fontTextureWidth" );
        ufm_worldPixelSize = m_shader->AddParameter( "worldPixelSize" );
        ufm_screenPixelSize = m_shader->AddParameter( "screenPixelSize" );
        ufm_pixelSizeMultiplier = m_shader->AddParameter( "pixelSizeMultiplier" );

        m_shader->Use();
        m_shader->SetParameter( ufm_fontTexture, (int) FONT_TEXTURE_UNIT );
        m_shader->SetParameter( ufm_fontTextureWidth, (int) font_image.width );
        m_shader->Deactivate();
        checkGlError( "setting bitmap font sampler as shader parameter", __FILE__, __LINE__ );

        m_isBitmapFontInitialized = true;
    }

    m_shader->Use();
    m_shader->SetParameter( ufm_worldPixelSize,
                            (float) ( getWorldPixelSize() / GetScaleFactor() ) );
    m_shader->SetParameter( ufm_screenPixelSize, getScreenPixelSize() );
    double pixelSizeMultiplier = m_compositor->GetAntialiasSupersamplingFactor();
    m_shader->SetParameter( ufm_pixelSizeMultiplier, (float) pixelSizeMultiplier );
    m_shader->Deactivate();

    // Something betreen BeginDrawing and EndDrawing seems to depend on
    // this texture unit being active, but it does not assure it itself.
    glActiveTexture( GL_TEXTURE0 );

    // Unbind buffers - set compositor for direct drawing
    m_compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( traceGalProfile, wxT( "OPENGL_GAL::beginDrawing(): %.1f ms" ),
                totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}


void OPENGL_GAL::endDrawing()
{
    wxASSERT_MSG( m_isContextLocked, "What happened to the context lock?" );

#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime( "OPENGL_GAL::endDrawing()", true );
#endif /* __WXDEBUG__ */

    // Cached & non-cached containers are rendered to the same buffer
    m_compositor->SetBuffer( m_mainBuffer );
    m_nonCachedManager->EndDrawing();
    m_cachedManager->EndDrawing();

    // Overlay container is rendered to a different buffer
    if( m_overlayBuffer )
        m_compositor->SetBuffer( m_overlayBuffer );

    m_overlayManager->EndDrawing();

    // Be sure that the framebuffer is not colorized (happens on specific GPU&drivers combinations)
    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Draw the remaining contents, blit the rendering targets to the screen, swap the buffers
    m_compositor->DrawBuffer( m_mainBuffer );

    if( m_overlayBuffer )
        m_compositor->DrawBuffer( m_overlayBuffer );

    m_compositor->Present();
    blitCursor();

    SwapBuffers();

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( traceGalProfile, wxT( "OPENGL_GAL::endDrawing(): %.1f ms" ),
                totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}


void OPENGL_GAL::lockContext( int aClientCookie )
{
    wxASSERT_MSG( !m_isContextLocked, "Context already locked." );
    m_isContextLocked = true;
    m_lockClientCookie = aClientCookie;

    GL_CONTEXT_MANAGER::Get().LockCtx( m_glPrivContext, this );
}


void OPENGL_GAL::unlockContext( int aClientCookie )
{
    wxASSERT_MSG( m_isContextLocked, "Context not locked.  A GAL_CONTEXT_LOCKER RAII object must "
                                     "be stacked rather than making separate lock/unlock calls." );

    wxASSERT_MSG( m_lockClientCookie == aClientCookie, "Context was locked by a different client. "
                                                       "Should not be possible with RAII objects." );

    m_isContextLocked = false;

    GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glPrivContext );
}


void OPENGL_GAL::beginUpdate()
{
    wxASSERT_MSG( m_isContextLocked, "GAL_UPDATE_CONTEXT RAII object should have locked context. "
                                     "Calling this from anywhere else is not allowed." );

    wxASSERT_MSG( IsVisible(), "GAL::beginUpdate() must not be entered when GAL is not visible. "
                               "Other update routines will expect everything to be initialized "
                               "which will not be the case." );

    if( !m_isInitialized )
        init();

    m_cachedManager->Map();
}


void OPENGL_GAL::endUpdate()
{
    if( !m_isInitialized )
        return;

    m_cachedManager->Unmap();
}


void OPENGL_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );

    drawLineQuad( aStartPoint, aEndPoint );
}


void OPENGL_GAL::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength = startEndVector.EuclideanNorm();

    float startx = aStartPoint.x;
    float starty = aStartPoint.y;
    float endx = aStartPoint.x + lineLength;
    float endy = aStartPoint.y + lineLength;

    // Be careful about floating point rounding.  As we draw segments in larger and larger
    // coordinates, the shader (which uses floats) will lose precision and stop drawing small
    // segments.  In this case, we need to draw a circle for the minimal segment.
    if( startx == endx || starty == endy )
    {
        DrawCircle( aStartPoint, aWidth / 2 );
        return;
    }

    if( m_isFillEnabled || aWidth == 1.0 )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        SetLineWidth( aWidth );
        drawLineQuad( aStartPoint, aEndPoint );
    }
    else
    {
        auto lineAngle = startEndVector.Angle();
        // Outlined tracks

        SetLineWidth( 1.0 );
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        Save();

        m_currentManager->Translate( aStartPoint.x, aStartPoint.y, 0.0 );
        m_currentManager->Rotate( lineAngle, 0.0f, 0.0f, 1.0f );

        drawLineQuad( VECTOR2D( 0.0, aWidth / 2.0 ), VECTOR2D( lineLength, aWidth / 2.0 ) );

        drawLineQuad( VECTOR2D( 0.0, -aWidth / 2.0 ), VECTOR2D( lineLength, -aWidth / 2.0 ) );

        // Draw line caps
        drawStrokedSemiCircle( VECTOR2D( 0.0, 0.0 ), aWidth / 2, M_PI / 2 );
        drawStrokedSemiCircle( VECTOR2D( lineLength, 0.0 ), aWidth / 2, -M_PI / 2 );

        Restore();
    }
}


void OPENGL_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    if( m_isFillEnabled )
    {
        m_currentManager->Reserve( 3 );
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        /* Draw a triangle that contains the circle, then shade it leaving only the circle.
         *  Parameters given to Shader() are indices of the triangle's vertices
         *  (if you want to understand more, check the vertex shader source [shader.vert]).
         *  Shader uses this coordinates to determine if fragments are inside the circle or not.
         *  Does the calculations in the vertex shader now (pixel alignment)
         *       v2
         *       /\
         *      //\\
         *  v0 /_\/_\ v1
         */
        m_currentManager->Shader( SHADER_FILLED_CIRCLE, 1.0, aRadius );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_FILLED_CIRCLE, 2.0, aRadius );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_FILLED_CIRCLE, 3.0, aRadius );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );
    }
    if( m_isStrokeEnabled )
    {
        m_currentManager->Reserve( 3 );
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

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
        m_currentManager->Shader( SHADER_STROKED_CIRCLE, 1.0, aRadius, m_lineWidth );
        m_currentManager->Vertex( aCenterPoint.x, // v0
                                  aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_STROKED_CIRCLE, 2.0, aRadius, m_lineWidth );
        m_currentManager->Vertex( aCenterPoint.x, // v1
                                  aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_STROKED_CIRCLE, 3.0, aRadius, m_lineWidth );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, // v2
                                  m_layerDepth );
    }
}


void OPENGL_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                          double aEndAngle )
{
    if( aRadius <= 0 )
        return;

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    const double alphaIncrement = calcAngleStep( aRadius );

    Save();
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );

    if( m_isFillEnabled )
    {
        double alpha;
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );
        m_currentManager->Shader( SHADER_NONE );

        // Triangle fan
        for( alpha = aStartAngle; ( alpha + alphaIncrement ) < aEndAngle; )
        {
            m_currentManager->Reserve( 3 );
            m_currentManager->Vertex( 0.0, 0.0, m_layerDepth );
            m_currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius,
                                      m_layerDepth );
            alpha += alphaIncrement;
            m_currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius,
                                      m_layerDepth );
        }

        // The last missing triangle
        const VECTOR2D endPoint( cos( aEndAngle ) * aRadius, sin( aEndAngle ) * aRadius );

        m_currentManager->Reserve( 3 );
        m_currentManager->Vertex( 0.0, 0.0, m_layerDepth );
        m_currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, m_layerDepth );
        m_currentManager->Vertex( endPoint.x, endPoint.y, m_layerDepth );
    }

    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        VECTOR2D p( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
        double   alpha;

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

    Restore();
}


void OPENGL_GAL::DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                                 double aEndAngle, double aWidth )
{
    if( aRadius <= 0 )
    {
        // Arcs of zero radius are a circle of aWidth diameter
        if( aWidth > 0 )
            DrawCircle( aCenterPoint, aWidth / 2.0 );

        return;
    }

    // Swap the angles, if start angle is greater than end angle
    SWAP( aStartAngle, >, aEndAngle );

    double alphaIncrement = calcAngleStep( aRadius );

    // Refinement: Use a segment count multiple of 2, because we have a control point
    // on the middle of the arc, and the look is better if it is on a segment junction
    // because there is no approx error
    int seg_count = KiROUND( ( aEndAngle - aStartAngle ) / alphaIncrement );

    if( seg_count % 2 != 0 )
        seg_count += 1;

    // Recalculate alphaIncrement with a even integer number of segment
    if( seg_count )
        alphaIncrement = ( aEndAngle - aStartAngle ) / seg_count;

    Save();
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );

    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        double   width = aWidth / 2.0;
        VECTOR2D startPoint( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
        VECTOR2D endPoint( cos( aEndAngle ) * aRadius, sin( aEndAngle ) * aRadius );

        drawStrokedSemiCircle( startPoint, width, aStartAngle + M_PI );
        drawStrokedSemiCircle( endPoint, width, aEndAngle );

        VECTOR2D pOuter( cos( aStartAngle ) * ( aRadius + width ),
                         sin( aStartAngle ) * ( aRadius + width ) );

        VECTOR2D pInner( cos( aStartAngle ) * ( aRadius - width ),
                         sin( aStartAngle ) * ( aRadius - width ) );

        double alpha;

        for( alpha = aStartAngle + alphaIncrement; alpha <= aEndAngle; alpha += alphaIncrement )
        {
            VECTOR2D pNextOuter( cos( alpha ) * ( aRadius + width ),
                                 sin( alpha ) * ( aRadius + width ) );
            VECTOR2D pNextInner( cos( alpha ) * ( aRadius - width ),
                                 sin( alpha ) * ( aRadius - width ) );

            DrawLine( pOuter, pNextOuter );
            DrawLine( pInner, pNextInner );

            pOuter = pNextOuter;
            pInner = pNextInner;
        }

        // Draw the last missing part
        if( alpha != aEndAngle )
        {
            VECTOR2D pLastOuter( cos( aEndAngle ) * ( aRadius + width ),
                                 sin( aEndAngle ) * ( aRadius + width ) );
            VECTOR2D pLastInner( cos( aEndAngle ) * ( aRadius - width ),
                                 sin( aEndAngle ) * ( aRadius - width ) );

            DrawLine( pOuter, pLastOuter );
            DrawLine( pInner, pLastInner );
        }
    }

    if( m_isFillEnabled )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );
        SetLineWidth( aWidth );

        VECTOR2D p( cos( aStartAngle ) * aRadius, sin( aStartAngle ) * aRadius );
        double   alpha;

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

    Restore();
}


void OPENGL_GAL::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // Compute the diagonal points of the rectangle
    VECTOR2D diagonalPointA( aEndPoint.x, aStartPoint.y );
    VECTOR2D diagonalPointB( aStartPoint.x, aEndPoint.y );

    // Fill the rectangle
    if( m_isFillEnabled )
    {
        m_currentManager->Reserve( 6 );
        m_currentManager->Shader( SHADER_NONE );
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        m_currentManager->Vertex( aStartPoint.x, aStartPoint.y, m_layerDepth );
        m_currentManager->Vertex( diagonalPointA.x, diagonalPointA.y, m_layerDepth );
        m_currentManager->Vertex( aEndPoint.x, aEndPoint.y, m_layerDepth );

        m_currentManager->Vertex( aStartPoint.x, aStartPoint.y, m_layerDepth );
        m_currentManager->Vertex( aEndPoint.x, aEndPoint.y, m_layerDepth );
        m_currentManager->Vertex( diagonalPointB.x, diagonalPointB.y, m_layerDepth );
    }

    // Stroke the outline
    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        std::deque<VECTOR2D> pointList;
        pointList.push_back( aStartPoint );
        pointList.push_back( diagonalPointA );
        pointList.push_back( aEndPoint );
        pointList.push_back( diagonalPointB );
        pointList.push_back( aStartPoint );
        DrawPolyline( pointList );
    }
}


void OPENGL_GAL::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    drawPolyline(
            [&]( int idx )
            {
                return aPointList[idx];
            },
            aPointList.size() );
}


void OPENGL_GAL::DrawPolyline( const VECTOR2D aPointList[], int aListSize )
{
    drawPolyline(
            [&]( int idx )
            {
                return aPointList[idx];
            },
            aListSize );
}


void OPENGL_GAL::DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain )
{
    auto numPoints = aLineChain.PointCount();

    if( aLineChain.IsClosed() )
        numPoints += 1;

    drawPolyline(
            [&]( int idx )
            {
                return aLineChain.CPoint( idx );
            },
            numPoints );
}


void OPENGL_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    wxCHECK( aPointList.size() >= 2, /* void */ );
    auto      points = std::unique_ptr<GLdouble[]>( new GLdouble[3 * aPointList.size()] );
    GLdouble* ptr = points.get();

    for( const VECTOR2D& p : aPointList )
    {
        *ptr++ = p.x;
        *ptr++ = p.y;
        *ptr++ = m_layerDepth;
    }

    drawPolygon( points.get(), aPointList.size() );
}


void OPENGL_GAL::DrawPolygon( const VECTOR2D aPointList[], int aListSize )
{
    wxCHECK( aListSize >= 2, /* void */ );
    auto            points = std::unique_ptr<GLdouble[]>( new GLdouble[3 * aListSize] );
    GLdouble*       target = points.get();
    const VECTOR2D* src = aPointList;

    for( int i = 0; i < aListSize; ++i )
    {
        *target++ = src->x;
        *target++ = src->y;
        *target++ = m_layerDepth;
        ++src;
    }

    drawPolygon( points.get(), aListSize );
}


void OPENGL_GAL::drawTriangulatedPolyset( const SHAPE_POLY_SET& aPolySet )
{
    m_currentManager->Shader( SHADER_NONE );
    m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

    if( m_isFillEnabled )
    {
        for( unsigned int j = 0; j < aPolySet.TriangulatedPolyCount(); ++j )
        {
            auto triPoly = aPolySet.TriangulatedPolygon( j );

            for( size_t i = 0; i < triPoly->GetTriangleCount(); i++ )
            {
                VECTOR2I a, b, c;
                triPoly->GetTriangle( i, a, b, c );
                m_currentManager->Vertex( a.x, a.y, m_layerDepth );
                m_currentManager->Vertex( b.x, b.y, m_layerDepth );
                m_currentManager->Vertex( c.x, c.y, m_layerDepth );
            }
        }
    }

    if( m_isStrokeEnabled )
    {
        for( int j = 0; j < aPolySet.OutlineCount(); ++j )
        {
            const auto& poly = aPolySet.Polygon( j );

            for( const auto& lc : poly )
            {
                DrawPolyline( lc );
            }
        }
    }

    if( ADVANCED_CFG::GetCfg().m_DrawTriangulationOutlines )
    {
        COLOR4D oldStrokeColor = m_strokeColor;
        double  oldLayerDepth = m_layerDepth;

        SetLayerDepth( m_layerDepth - 1 );
        SetStrokeColor( COLOR4D( 0.0, 1.0, 0.2, 1.0 ) );

        for( unsigned int j = 0; j < aPolySet.TriangulatedPolyCount(); ++j )
        {
            auto triPoly = aPolySet.TriangulatedPolygon( j );

            for( size_t i = 0; i < triPoly->GetTriangleCount(); i++ )
            {
                VECTOR2I a, b, c;
                triPoly->GetTriangle( i, a, b, c );
                DrawLine( a, b );
                DrawLine( b, c );
                DrawLine( c, a );
            }
        }

        SetStrokeColor( oldStrokeColor );
        SetLayerDepth( oldLayerDepth );
    }
}


void OPENGL_GAL::DrawPolygon( const SHAPE_POLY_SET& aPolySet )
{
    if( aPolySet.IsTriangulationUpToDate() )
    {
        drawTriangulatedPolyset( aPolySet );
        return;
    }

    for( int j = 0; j < aPolySet.OutlineCount(); ++j )
    {
        const SHAPE_LINE_CHAIN& outline = aPolySet.COutline( j );
        DrawPolygon( outline );
    }
}


void OPENGL_GAL::DrawPolygon( const SHAPE_LINE_CHAIN& aPolygon )
{
    wxCHECK( aPolygon.PointCount() >= 2, /* void */ );

    const int                   pointCount = aPolygon.SegmentCount() + 1;
    std::unique_ptr<GLdouble[]> points( new GLdouble[3 * pointCount] );
    GLdouble*                   ptr = points.get();

    for( int i = 0; i < pointCount; ++i )
    {
        const VECTOR2I& p = aPolygon.CPoint( i );
        *ptr++ = p.x;
        *ptr++ = p.y;
        *ptr++ = m_layerDepth;
    }

    drawPolygon( points.get(), pointCount );
}


void OPENGL_GAL::DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                            const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint,
                            double aFilterValue )
{
    std::vector<VECTOR2D> output;
    std::vector<VECTOR2D> pointCtrl;

    pointCtrl.push_back( aStartPoint );
    pointCtrl.push_back( aControlPointA );
    pointCtrl.push_back( aControlPointB );
    pointCtrl.push_back( aEndPoint );

    BEZIER_POLY converter( pointCtrl );
    converter.GetPoly( output, aFilterValue );

    DrawPolyline( &output[0], output.size() );
}


void OPENGL_GAL::DrawBitmap( const BITMAP_BASE& aBitmap )
{
    // We have to calculate the pixel size in users units to draw the image.
    // m_worldUnitLength is a factor used for converting IU to inches
    double scale = 1.0 / ( aBitmap.GetPPI() * m_worldUnitLength );
    double w = (double) aBitmap.GetSizePixels().x * scale;
    double h = (double) aBitmap.GetSizePixels().y * scale;

    auto xform = m_currentManager->GetTransformation();

    glm::vec4 v0 = xform * glm::vec4( -w / 2, -h / 2, 0.0, 0.0 );
    glm::vec4 v1 = xform * glm::vec4( w / 2, h / 2, 0.0, 0.0 );
    glm::vec4 trans = xform[3];

    auto texture_id = m_bitmapCache->RequestBitmap( &aBitmap );

    if( !glIsTexture( texture_id ) ) // ensure the bitmap texture is still valid
        return;

    auto oldTarget = GetTarget();

    glPushMatrix();
    glTranslated( trans.x, trans.y, trans.z );

    SetTarget( TARGET_NONCACHED );
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_id );

    glBegin( GL_QUADS );
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glTexCoord2f( 0.0, 0.0 );
    glVertex3f( v0.x, v0.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glTexCoord2f( 1.0, 0.0 );
    glVertex3f( v1.x, v0.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glTexCoord2f( 1.0, 1.0 );
    glVertex3f( v1.x, v1.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glTexCoord2f( 0.0, 1.0 );
    glVertex3f( v0.x, v1.y, m_layerDepth );
    glEnd();

    SetTarget( oldTarget );
    glBindTexture( GL_TEXTURE_2D, 0 );

#ifdef DISABLE_BITMAP_CACHE
    glDeleteTextures( 1, &texture_id );
#endif

    glPopMatrix();
}


void OPENGL_GAL::BitmapText( const wxString& aText, const VECTOR2D& aPosition,
                             double aRotationAngle )
{
    // Fallback to generic impl (which uses the stroke font) on cases we don't handle
    if( IsTextMirrored() || aText.Contains( wxT( "^{" ) ) || aText.Contains( wxT( "_{" ) ) )
        return GAL::BitmapText( aText, aPosition, aRotationAngle );

    const UTF8 text( aText );
    // Compute text size, so it can be properly justified
    VECTOR2D textSize;
    float    commonOffset;
    std::tie( textSize, commonOffset ) = computeBitmapTextSize( text );

    const double SCALE = 1.4 * GetGlyphSize().y / textSize.y;
    bool         overbar = false;

    int    overbarLength = 0;
    double overbarHeight = textSize.y;

    Save();

    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );
    m_currentManager->Translate( aPosition.x, aPosition.y, m_layerDepth );
    m_currentManager->Rotate( aRotationAngle, 0.0f, 0.0f, -1.0f );

    double sx = SCALE * ( m_globalFlipX ? -1.0 : 1.0 );
    double sy = SCALE * ( m_globalFlipY ? -1.0 : 1.0 );

    m_currentManager->Scale( sx, sy, 0 );
    m_currentManager->Translate( 0, -commonOffset, 0 );

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

    int i = 0;

    for( UTF8::uni_iter chIt = text.ubegin(), end = text.uend(); chIt < end; ++chIt )
    {
        unsigned int c = *chIt;
        wxASSERT_MSG( c != '\n' && c != '\r', wxT( "No support for multiline bitmap text yet" ) );

        bool wasOverbar = overbar;

        if( c == '~' )
        {
            if( ++chIt == end )
                break;

            c = *chIt;

            if( c == '~' )
            {
                // double ~ is really a ~ so go ahead and process the second one

                // so what's a triple ~?  It could be a real ~ followed by an overbar, or
                // it could be an overbar followed by a real ~.  The old algorithm did the
                // former so we will too....
            }
            else
            {
                overbar = !overbar;
            }
        }
        else if( c == ' ' || c == '}' || c == ')' )
        {
            overbar = false;
        }

        if( wasOverbar && !overbar )
        {
            drawBitmapOverbar( overbarLength, overbarHeight );
            overbarLength = 0;
        }

        if( overbar )
            overbarLength += drawBitmapChar( c );
        else
            drawBitmapChar( c );

        ++i;
    }

    // Handle the case when overbar is active till the end of the drawn text
    m_currentManager->Translate( 0, commonOffset, 0 );

    if( overbar && overbarLength > 0 )
        drawBitmapOverbar( overbarLength, overbarHeight );

    Restore();
}


void OPENGL_GAL::DrawGrid()
{
    SetTarget( TARGET_NONCACHED );
    m_compositor->SetBuffer( m_mainBuffer );

    m_nonCachedManager->EnableDepthTest( false );

    // sub-pixel lines all render the same
    float minorLineWidth = std::fmax( 1.0f,
                                      m_gridLineWidth ) * getWorldPixelSize() / GetScaleFactor();
    float majorLineWidth = minorLineWidth * 2.0f;

    // Draw the axis and grid
    // For the drawing the start points, end points and increments have
    // to be calculated in world coordinates
    VECTOR2D worldStartPoint = m_screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
    VECTOR2D worldEndPoint = m_screenWorldMatrix * VECTOR2D( m_screenSize );

    // Draw axes if desired
    if( m_axesEnabled )
    {
        SetLineWidth( minorLineWidth );
        SetStrokeColor( m_axesColor );

        DrawLine( VECTOR2D( worldStartPoint.x, 0 ), VECTOR2D( worldEndPoint.x, 0 ) );
        DrawLine( VECTOR2D( 0, worldStartPoint.y ), VECTOR2D( 0, worldEndPoint.y ) );
    }

    // force flush
    m_nonCachedManager->EndDrawing();

    if( !m_gridVisibility || m_gridSize.x == 0 || m_gridSize.y == 0 )
        return;

    VECTOR2D gridScreenSize( m_gridSize );

    double gridThreshold = computeMinGridSpacing() / m_worldScale;

    if( m_gridStyle == GRID_STYLE::SMALL_CROSS )
        gridThreshold *= 2.0;

    // If we cannot display the grid density, scale down by a tick size and
    // try again.  Eventually, we get some representation of the grid
    while( std::min( gridScreenSize.x, gridScreenSize.y ) <= gridThreshold )
    {
        gridScreenSize = gridScreenSize * static_cast<double>( m_gridTick );
    }

    // Compute grid starting and ending indexes to draw grid points on the
    // visible screen area
    // Note: later any point coordinate will be offsetted by m_gridOrigin
    int gridStartX = KiROUND( ( worldStartPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridEndX = KiROUND( ( worldEndPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridStartY = KiROUND( ( worldStartPoint.y - m_gridOrigin.y ) / gridScreenSize.y );
    int gridEndY = KiROUND( ( worldEndPoint.y - m_gridOrigin.y ) / gridScreenSize.y );

    // Ensure start coordinate > end coordinate
    SWAP( gridStartX, >, gridEndX );
    SWAP( gridStartY, >, gridEndY );

    // Ensure the grid fills the screen
    --gridStartX;
    ++gridEndX;
    --gridStartY;
    ++gridEndY;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );

    if( m_gridStyle == GRID_STYLE::DOTS )
    {
        glEnable( GL_STENCIL_TEST );
        glStencilFunc( GL_ALWAYS, 1, 1 );
        glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
        glColor4d( 0.0, 0.0, 0.0, 0.0 );
        SetStrokeColor( COLOR4D( 0.0, 0.0, 0.0, 0.0 ) );
    }
    else
    {
        glColor4d( m_gridColor.r, m_gridColor.g, m_gridColor.b, m_gridColor.a );
        SetStrokeColor( m_gridColor );
    }

    if( m_gridStyle == GRID_STYLE::SMALL_CROSS )
    {
        // Vertical positions
        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            bool         tickY = ( j % m_gridTick == 0 );
            const double posY = j * gridScreenSize.y + m_gridOrigin.y;

            // Horizontal positions
            for( int i = gridStartX; i <= gridEndX; i++ )
            {
                bool tickX = ( i % m_gridTick == 0 );
                SetLineWidth( ( ( tickX && tickY ) ? majorLineWidth : minorLineWidth ) );
                auto lineLen = 2.0 * GetLineWidth();
                auto posX = i * gridScreenSize.x + m_gridOrigin.x;

                DrawLine( VECTOR2D( posX - lineLen, posY ), VECTOR2D( posX + lineLen, posY ) );
                DrawLine( VECTOR2D( posX, posY - lineLen ), VECTOR2D( posX, posY + lineLen ) );
            }
        }

        m_nonCachedManager->EndDrawing();
    }
    else
    {
        // Vertical lines
        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            const double y = j * gridScreenSize.y + m_gridOrigin.y;

            // If axes are drawn, skip the lines that would cover them
            if( m_axesEnabled && y == 0.0 )
                continue;

            SetLineWidth( ( j % m_gridTick == 0 ) ? majorLineWidth : minorLineWidth );
            VECTOR2D a( gridStartX * gridScreenSize.x + m_gridOrigin.x, y );
            VECTOR2D b( gridEndX * gridScreenSize.x + m_gridOrigin.x, y );

            DrawLine( a, b );
        }

        m_nonCachedManager->EndDrawing();

        if( m_gridStyle == GRID_STYLE::DOTS )
        {
            glStencilFunc( GL_NOTEQUAL, 0, 1 );
            glColor4d( m_gridColor.r, m_gridColor.g, m_gridColor.b, m_gridColor.a );
            SetStrokeColor( m_gridColor );
        }

        // Horizontal lines
        for( int i = gridStartX; i <= gridEndX; i++ )
        {
            const double x = i * gridScreenSize.x + m_gridOrigin.x;

            // If axes are drawn, skip the lines that would cover them
            if( m_axesEnabled && x == 0.0 )
                continue;

            SetLineWidth( ( i % m_gridTick == 0 ) ? majorLineWidth : minorLineWidth );
            VECTOR2D a( x, gridStartY * gridScreenSize.y + m_gridOrigin.y );
            VECTOR2D b( x, gridEndY * gridScreenSize.y + m_gridOrigin.y );
            DrawLine( a, b );
        }

        m_nonCachedManager->EndDrawing();

        if( m_gridStyle == GRID_STYLE::DOTS )
            glDisable( GL_STENCIL_TEST );
    }

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
}


void OPENGL_GAL::ResizeScreen( int aWidth, int aHeight )
{
    m_screenSize = VECTOR2I( aWidth, aHeight );

    // Resize framebuffers
    const float scaleFactor = GetScaleFactor();
    m_compositor->Resize( aWidth * scaleFactor, aHeight * scaleFactor );
    m_isFramebufferInitialized = false;

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


void OPENGL_GAL::ClearScreen()
{
    // Clear screen
    m_compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    // NOTE: Black used here instead of m_clearColor; it will be composited later
    glClearColor( 0, 0, 0, 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}


void OPENGL_GAL::Transform( const MATRIX3x3D& aTransformation )
{
    GLdouble matrixData[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    matrixData[0] = aTransformation.m_data[0][0];
    matrixData[1] = aTransformation.m_data[1][0];
    matrixData[2] = aTransformation.m_data[2][0];
    matrixData[4] = aTransformation.m_data[0][1];
    matrixData[5] = aTransformation.m_data[1][1];
    matrixData[6] = aTransformation.m_data[2][1];
    matrixData[12] = aTransformation.m_data[0][2];
    matrixData[13] = aTransformation.m_data[1][2];
    matrixData[14] = aTransformation.m_data[2][2];

    glMultMatrixd( matrixData );
}


void OPENGL_GAL::Rotate( double aAngle )
{
    m_currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );
}


void OPENGL_GAL::Translate( const VECTOR2D& aVector )
{
    m_currentManager->Translate( aVector.x, aVector.y, 0.0f );
}


void OPENGL_GAL::Scale( const VECTOR2D& aScale )
{
    m_currentManager->Scale( aScale.x, aScale.y, 0.0f );
}


void OPENGL_GAL::Save()
{
    m_currentManager->PushMatrix();
}


void OPENGL_GAL::Restore()
{
    m_currentManager->PopMatrix();
}


int OPENGL_GAL::BeginGroup()
{
    m_isGrouping = true;

    std::shared_ptr<VERTEX_ITEM> newItem = std::make_shared<VERTEX_ITEM>( *m_cachedManager );
    int                          groupNumber = getNewGroupNumber();
    m_groups.insert( std::make_pair( groupNumber, newItem ) );

    return groupNumber;
}


void OPENGL_GAL::EndGroup()
{
    m_cachedManager->FinishItem();
    m_isGrouping = false;
}


void OPENGL_GAL::DrawGroup( int aGroupNumber )
{
    if( m_groups[aGroupNumber] )
        m_cachedManager->DrawItem( *m_groups[aGroupNumber] );
}


void OPENGL_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    if( m_groups[aGroupNumber] )
        m_cachedManager->ChangeItemColor( *m_groups[aGroupNumber], aNewColor );
}


void OPENGL_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    if( m_groups[aGroupNumber] )
        m_cachedManager->ChangeItemDepth( *m_groups[aGroupNumber], aDepth );
}


void OPENGL_GAL::DeleteGroup( int aGroupNumber )
{
    // Frees memory in the container as well
    m_groups.erase( aGroupNumber );
}


void OPENGL_GAL::ClearCache()
{
    m_bitmapCache = std::make_unique<GL_BITMAP_CACHE>();

    m_groups.clear();

    if( m_isInitialized )
        m_cachedManager->Clear();
}


void OPENGL_GAL::SetTarget( RENDER_TARGET aTarget )
{
    switch( aTarget )
    {
    default:
    case TARGET_CACHED:    m_currentManager = m_cachedManager;    break;
    case TARGET_NONCACHED: m_currentManager = m_nonCachedManager; break;
    case TARGET_OVERLAY:   m_currentManager = m_overlayManager;   break;
    }

    m_currentTarget = aTarget;
}


RENDER_TARGET OPENGL_GAL::GetTarget() const
{
    return m_currentTarget;
}


void OPENGL_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int oldTarget = m_compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        m_compositor->SetBuffer( m_mainBuffer );
        break;

    case TARGET_OVERLAY:
        if( m_overlayBuffer )
            m_compositor->SetBuffer( m_overlayBuffer );
        break;
    }

    if( aTarget != TARGET_OVERLAY )
        m_compositor->ClearBuffer( m_clearColor );
    else if( m_overlayBuffer )
        m_compositor->ClearBuffer( COLOR4D::BLACK );

    // Restore the previous state
    m_compositor->SetBuffer( oldTarget );
}


bool OPENGL_GAL::HasTarget( RENDER_TARGET aTarget )
{
    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED: return true;
    case TARGET_OVERLAY:   return ( m_overlayBuffer != 0 );
    }
}


bool OPENGL_GAL::SetNativeCursorStyle( KICURSOR aCursor )
{
    // Store the current cursor type and get the wxCursor for it
    if( !GAL::SetNativeCursorStyle( aCursor ) )
        return false;

    m_currentwxCursor = CURSOR_STORE::GetCursor( m_currentNativeCursor );

    // Update the cursor in the wx control
    HIDPI_GL_CANVAS::SetCursor( m_currentwxCursor );

    return true;
}


void OPENGL_GAL::onSetNativeCursor( wxSetCursorEvent& aEvent )
{
    aEvent.SetCursor( m_currentwxCursor );
}


void OPENGL_GAL::DrawCursor( const VECTOR2D& aCursorPosition )
{
    // Now we should only store the position of the mouse cursor
    // The real drawing routines are in blitCursor()
    //VECTOR2D screenCursor = m_worldScreenMatrix * aCursorPosition;
    //m_cursorPosition = m_screenWorldMatrix * VECTOR2D( screenCursor.x, screenCursor.y );
    m_cursorPosition = aCursorPosition;
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

    auto v1 = m_currentManager->GetTransformation()
              * glm::vec4( aStartPoint.x, aStartPoint.y, 0.0, 0.0 );
    auto v2 = m_currentManager->GetTransformation()
              * glm::vec4( aEndPoint.x, aEndPoint.y, 0.0, 0.0 );

    VECTOR2D vs( v2.x - v1.x, v2.y - v1.y );

    m_currentManager->Reserve( 6 );

    // Line width is maintained by the vertex shader
    m_currentManager->Shader( SHADER_LINE_A, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aStartPoint, m_layerDepth );

    m_currentManager->Shader( SHADER_LINE_B, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aStartPoint, m_layerDepth );

    m_currentManager->Shader( SHADER_LINE_C, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aEndPoint, m_layerDepth );

    m_currentManager->Shader( SHADER_LINE_D, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aEndPoint, m_layerDepth );

    m_currentManager->Shader( SHADER_LINE_E, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aEndPoint, m_layerDepth );

    m_currentManager->Shader( SHADER_LINE_F, m_lineWidth, vs.x, vs.y );
    m_currentManager->Vertex( aStartPoint, m_layerDepth );
}


void OPENGL_GAL::drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
    if( m_isFillEnabled )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );
        drawFilledSemiCircle( aCenterPoint, aRadius, aAngle );
    }

    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );
        drawStrokedSemiCircle( aCenterPoint, aRadius, aAngle );
    }
}


void OPENGL_GAL::drawFilledSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle )
{
    Save();

    m_currentManager->Reserve( 3 );
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    m_currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

    /* Draw a triangle that contains the semicircle, then shade it to leave only
     * the semicircle. Parameters given to Shader() are indices of the triangle's vertices
     * (if you want to understand more, check the vertex shader source [shader.vert]).
     * Shader uses these coordinates to determine if fragments are inside the semicircle or not.
     *       v2
     *       /\
     *      /__\
     *  v0 //__\\ v1
     */
    m_currentManager->Shader( SHADER_FILLED_CIRCLE, 4.0f );
    m_currentManager->Vertex( -aRadius * 3.0f / sqrt( 3.0f ), 0.0f, m_layerDepth ); // v0

    m_currentManager->Shader( SHADER_FILLED_CIRCLE, 5.0f );
    m_currentManager->Vertex( aRadius * 3.0f / sqrt( 3.0f ), 0.0f, m_layerDepth ); // v1

    m_currentManager->Shader( SHADER_FILLED_CIRCLE, 6.0f );
    m_currentManager->Vertex( 0.0f, aRadius * 2.0f, m_layerDepth ); // v2

    Restore();
}


void OPENGL_GAL::drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius,
                                        double aAngle )
{
    double outerRadius = aRadius + ( m_lineWidth / 2 );

    Save();

    m_currentManager->Reserve( 3 );
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0f );
    m_currentManager->Rotate( aAngle, 0.0f, 0.0f, 1.0f );

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
    m_currentManager->Shader( SHADER_STROKED_CIRCLE, 4.0f, aRadius, m_lineWidth );
    m_currentManager->Vertex( -outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, m_layerDepth ); // v0

    m_currentManager->Shader( SHADER_STROKED_CIRCLE, 5.0f, aRadius, m_lineWidth );
    m_currentManager->Vertex( outerRadius * 3.0f / sqrt( 3.0f ), 0.0f, m_layerDepth ); // v1

    m_currentManager->Shader( SHADER_STROKED_CIRCLE, 6.0f, aRadius, m_lineWidth );
    m_currentManager->Vertex( 0.0f, outerRadius * 2.0f, m_layerDepth ); // v2

    Restore();
}


void OPENGL_GAL::drawPolygon( GLdouble* aPoints, int aPointCount )
{
    if( m_isFillEnabled )
    {
        m_currentManager->Shader( SHADER_NONE );
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        // Any non convex polygon needs to be tesselated
        // for this purpose the GLU standard functions are used
        TessParams params = { m_currentManager, m_tessIntersects };
        gluTessBeginPolygon( m_tesselator, &params );
        gluTessBeginContour( m_tesselator );

        GLdouble* point = aPoints;

        for( int i = 0; i < aPointCount; ++i )
        {
            gluTessVertex( m_tesselator, point, point );
            point += 3; // 3 coordinates
        }

        gluTessEndContour( m_tesselator );
        gluTessEndPolygon( m_tesselator );

        // Free allocated intersecting points
        m_tessIntersects.clear();
    }

    if( m_isStrokeEnabled )
    {
        drawPolyline(
                [&]( int idx )
                {
                    return VECTOR2D( aPoints[idx * 3], aPoints[idx * 3 + 1] );
                },
                aPointCount );
    }
}


void OPENGL_GAL::drawPolyline( const std::function<VECTOR2D( int )>& aPointGetter, int aPointCount )
{
    wxCHECK( aPointCount >= 2, /* return */ );

    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );
    int i;

    for( i = 1; i < aPointCount; ++i )
    {
        auto start = aPointGetter( i - 1 );
        auto end = aPointGetter( i );

        drawLineQuad( start, end );
    }
}


int OPENGL_GAL::drawBitmapChar( unsigned long aChar )
{
    const float TEX_X = font_image.width;
    const float TEX_Y = font_image.height;

    // handle space
    if( aChar == ' ' )
    {
        const FONT_GLYPH_TYPE* g = LookupGlyph( 'x' );
        wxASSERT( g );

        if( !g ) // Should not happen.
            return 0;

        Translate( VECTOR2D( g->advance, 0 ) );
        return g->advance;
    }

    const FONT_GLYPH_TYPE* glyph = LookupGlyph( aChar );

    // If the glyph is not found (happens for many esotheric unicode chars)
    // shows a '?' instead.
    if( !glyph )
        glyph = LookupGlyph( '?' );

    if( !glyph ) // Should not happen.
        return 0;

    const float X = glyph->atlas_x + font_information.smooth_pixels;
    const float Y = glyph->atlas_y + font_information.smooth_pixels;
    const float XOFF = glyph->minx;

    // adjust for height rounding
    const float round_adjust = ( glyph->maxy - glyph->miny )
                               - float( glyph->atlas_h - font_information.smooth_pixels * 2 );
    const float top_adjust = font_information.max_y - glyph->maxy;
    const float YOFF = round_adjust + top_adjust;
    const float W = glyph->atlas_w - font_information.smooth_pixels * 2;
    const float H = glyph->atlas_h - font_information.smooth_pixels * 2;
    const float B = 0;

    m_currentManager->Reserve( 6 );
    Translate( VECTOR2D( XOFF, YOFF ) );

    /* Glyph:
     * v0    v1
     *   +--+
     *   | /|
     *   |/ |
     *   +--+
     * v2    v3
     */
    m_currentManager->Shader( SHADER_FONT, X / TEX_X, ( Y + H ) / TEX_Y );
    m_currentManager->Vertex( -B, -B, 0 ); // v0

    m_currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, ( Y + H ) / TEX_Y );
    m_currentManager->Vertex( W + B, -B, 0 ); // v1

    m_currentManager->Shader( SHADER_FONT, X / TEX_X, Y / TEX_Y );
    m_currentManager->Vertex( -B, H + B, 0 ); // v2


    m_currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, ( Y + H ) / TEX_Y );
    m_currentManager->Vertex( W + B, -B, 0 ); // v1

    m_currentManager->Shader( SHADER_FONT, X / TEX_X, Y / TEX_Y );
    m_currentManager->Vertex( -B, H + B, 0 ); // v2

    m_currentManager->Shader( SHADER_FONT, ( X + W ) / TEX_X, Y / TEX_Y );
    m_currentManager->Vertex( W + B, H + B, 0 ); // v3

    Translate( VECTOR2D( -XOFF + glyph->advance, -YOFF ) );

    return glyph->advance;
}


void OPENGL_GAL::drawBitmapOverbar( double aLength, double aHeight )
{
    // To draw an overbar, simply draw an overbar
    const FONT_GLYPH_TYPE* glyph = LookupGlyph( '_' );
    wxCHECK( glyph, /* void */ );

    const float H = glyph->maxy - glyph->miny;

    Save();

    Translate( VECTOR2D( -aLength, -aHeight - 1.5 * H ) );

    m_currentManager->Reserve( 6 );
    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );

    m_currentManager->Shader( 0 );

    m_currentManager->Vertex( 0, 0, 0 );       // v0
    m_currentManager->Vertex( aLength, 0, 0 ); // v1
    m_currentManager->Vertex( 0, H, 0 );       // v2

    m_currentManager->Vertex( aLength, 0, 0 ); // v1
    m_currentManager->Vertex( 0, H, 0 );       // v2
    m_currentManager->Vertex( aLength, H, 0 ); // v3

    Restore();
}


std::pair<VECTOR2D, float> OPENGL_GAL::computeBitmapTextSize( const UTF8& aText ) const
{
    static const FONT_GLYPH_TYPE* defaultGlyph = LookupGlyph( '(' ); // for strange chars
    static const FONT_GLYPH_TYPE* lineGlyph = LookupGlyph( '_' );    // for overbar thickness

    VECTOR2D textSize( 0, 0 );
    float    commonOffset = std::numeric_limits<float>::max();
    bool     in_overbar = false;
    float    char_height = font_information.max_y - defaultGlyph->miny;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        if( *chIt == '~' )
        {
            if( ++chIt == end )
                break;

            if( *chIt == '~' )
            {
                // double ~ is really a ~ so go ahead and process the second one

                // so what's a triple ~?  It could be a real ~ followed by an overbar, or
                // it could be an overbar followed by a real ~.  The old algorithm did the
                // former so we will too....
            }
            else
            {
                // single ~ toggles overbar
                in_overbar = !in_overbar;
            }
        }
        else if( in_overbar && ( *chIt == ' ' || *chIt == '}' || *chIt == ')' ) )
        {
            in_overbar = false;
        }

        const FONT_GLYPH_TYPE* glyph = LookupGlyph( *chIt );

        if( !glyph                            // Not coded in font
            || *chIt == '-' || *chIt == '_' ) // Strange size of these 2 chars
        {
            glyph = defaultGlyph;
        }

        if( glyph )
        {
            textSize.x += glyph->advance;

            if( in_overbar )
            {
                const float H = lineGlyph->maxy - lineGlyph->miny;
                textSize.y = std::max<float>( textSize.y, char_height + 1.5 * H );
            }
        }
    }

    textSize.y = std::max<float>( textSize.y, char_height );
    commonOffset = std::min<float>( font_information.max_y - defaultGlyph->maxy, commonOffset );
    textSize.y -= commonOffset;

    return std::make_pair( textSize, commonOffset );
}


void OPENGL_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint( aEvent );
}


void OPENGL_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void OPENGL_GAL::blitCursor()
{
    if( !IsCursorEnabled() )
        return;

    m_compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    const int cursorSize = m_fullscreenCursor ? 8000 : 80;

    VECTOR2D cursorBegin = m_cursorPosition - cursorSize / ( 2 * m_worldScale );
    VECTOR2D cursorEnd = m_cursorPosition + cursorSize / ( 2 * m_worldScale );
    VECTOR2D cursorCenter = ( cursorBegin + cursorEnd ) / 2;

    const COLOR4D cColor = getCursorColor();
    const COLOR4D color( cColor.r * cColor.a, cColor.g * cColor.a, cColor.b * cColor.a, 1.0 );

    glActiveTexture( GL_TEXTURE0 );
    glDisable( GL_TEXTURE_2D );
    glLineWidth( 1.0 );
    glColor4d( color.r, color.g, color.b, color.a );

    glBegin( GL_LINES );
    glVertex2d( cursorCenter.x, cursorBegin.y );
    glVertex2d( cursorCenter.x, cursorEnd.y );

    glVertex2d( cursorBegin.x, cursorCenter.y );
    glVertex2d( cursorEnd.x, cursorCenter.y );
    glEnd();
}


unsigned int OPENGL_GAL::getNewGroupNumber()
{
    wxASSERT_MSG( m_groups.size() < std::numeric_limits<unsigned int>::max(),
                  wxT( "There are no free slots to store a group" ) );

    while( m_groups.find( m_groupCounter ) != m_groups.end() )
        m_groupCounter++;

    return m_groupCounter++;
}


void OPENGL_GAL::init()
{
    wxASSERT( IsShownOnScreen() );

    wxASSERT_MSG( m_isContextLocked, "This should only be called from within a locked context." );

// IsDisplayAttr() handles WX_GL_{MAJOR,MINOR}_VERSION correctly only in 3.0.4
// starting with 3.1.0 one should use wxGLContext::IsOk() (done by GL_CONTEXT_MANAGER)
#if wxCHECK_VERSION( 3, 0, 3 ) and !wxCHECK_VERSION( 3, 1, 0 )
    const int attr[] = { WX_GL_MAJOR_VERSION, 2, WX_GL_MINOR_VERSION, 1, 0 };

    if( !IsDisplaySupported( attr ) )
        throw std::runtime_error( "OpenGL 2.1 or higher is required!" );
#endif /* wxCHECK_VERSION( 3, 0, 3 ) */

    // Check correct initialization from the constructor
    if( !m_glMainContext )
        throw std::runtime_error( "Could not create the main OpenGL context" );

    if( !m_glPrivContext )
        throw std::runtime_error( "Could not create a private OpenGL context" );

    if( m_tesselator == NULL )
        throw std::runtime_error( "Could not create the m_tesselator" );
    // End initialzation checks

    GLenum err = glewInit();

    if( GLEW_OK != err )
        throw std::runtime_error( (const char*) glewGetErrorString( err ) );

    // Check the OpenGL version (minimum 2.1 is required)
    if( !GLEW_VERSION_2_1 )
        throw std::runtime_error( "OpenGL 2.1 or higher is required!" );

#if defined( __LINUX__ ) // calling enableGlDebug crashes opengl on some OS (OSX and some Windows)
#ifdef DEBUG
    if( GLEW_ARB_debug_output )
        enableGlDebug( true );
#endif
#endif

    // Framebuffers have to be supported
    if( !GLEW_EXT_framebuffer_object )
        throw std::runtime_error( "Framebuffer objects are not supported!" );

    // Vertex buffer has to be supported
    if( !GLEW_ARB_vertex_buffer_object )
        throw std::runtime_error( "Vertex buffer objects are not supported!" );

    // Prepare shaders
    if( !m_shader->IsLinked()
        && !m_shader->LoadShaderFromStrings( SHADER_TYPE_VERTEX,
                                             BUILTIN_SHADERS::kicad_vertex_shader ) )
    {
        throw std::runtime_error( "Cannot compile vertex shader!" );
    }

    if( !m_shader->IsLinked()
        && !m_shader->LoadShaderFromStrings( SHADER_TYPE_FRAGMENT,
                                             BUILTIN_SHADERS::kicad_fragment_shader ) )
    {
        throw std::runtime_error( "Cannot compile fragment shader!" );
    }

    if( !m_shader->IsLinked() && !m_shader->Link() )
        throw std::runtime_error( "Cannot link the shaders!" );

    // Check if video card supports textures big enough to fit the font atlas
    int maxTextureSize;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );

    if( maxTextureSize < (int) font_image.width || maxTextureSize < (int) font_image.height )
    {
        // TODO implement software texture scaling
        // for bitmap fonts and use a higher resolution texture?
        throw std::runtime_error( "Requested texture size is not supported" );
    }

    GL_UTILS::SetSwapInterval( -1 );

    m_cachedManager = new VERTEX_MANAGER( true );
    m_nonCachedManager = new VERTEX_MANAGER( false );
    m_overlayManager = new VERTEX_MANAGER( false );

    // Make VBOs use shaders
    m_cachedManager->SetShader( *m_shader );
    m_nonCachedManager->SetShader( *m_shader );
    m_overlayManager->SetShader( *m_shader );

    m_isInitialized = true;
}


// Callback functions for the tesselator.  Compare Redbook Chapter 11.
void CALLBACK VertexCallback( GLvoid* aVertexPtr, void* aData )
{
    GLdouble*               vertex = static_cast<GLdouble*>( aVertexPtr );
    OPENGL_GAL::TessParams* param = static_cast<OPENGL_GAL::TessParams*>( aData );
    VERTEX_MANAGER*         vboManager = param->vboManager;

    assert( vboManager );
    vboManager->Vertex( vertex[0], vertex[1], vertex[2] );
}


void CALLBACK CombineCallback( GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4],
                               GLdouble** dataOut, void* aData )
{
    GLdouble*               vertex = new GLdouble[3];
    OPENGL_GAL::TessParams* param = static_cast<OPENGL_GAL::TessParams*>( aData );

    // Save the pointer so we can delete it later
    param->intersectPoints.emplace_back( vertex );

    memcpy( vertex, coords, 3 * sizeof( GLdouble ) );

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
    gluTessCallback( aTesselator, GLU_TESS_VERTEX_DATA, (void( CALLBACK* )()) VertexCallback );
    gluTessCallback( aTesselator, GLU_TESS_COMBINE_DATA, (void( CALLBACK* )()) CombineCallback );
    gluTessCallback( aTesselator, GLU_TESS_EDGE_FLAG, (void( CALLBACK* )()) EdgeCallback );
    gluTessCallback( aTesselator, GLU_TESS_ERROR, (void( CALLBACK* )()) ErrorCallback );
}

void OPENGL_GAL::EnableDepthTest( bool aEnabled )
{
    m_cachedManager->EnableDepthTest( aEnabled );
    m_nonCachedManager->EnableDepthTest( aEnabled );
    m_overlayManager->EnableDepthTest( aEnabled );
}


static double roundr( double f, double r )
{
    return floor( f / r + 0.5 ) * r;
}


void OPENGL_GAL::ComputeWorldScreenMatrix()
{
    auto pixelSize = m_worldScale;

    m_lookAtPoint.x = roundr( m_lookAtPoint.x, pixelSize );
    m_lookAtPoint.y = roundr( m_lookAtPoint.y, pixelSize );

    GAL::ComputeWorldScreenMatrix();
}
