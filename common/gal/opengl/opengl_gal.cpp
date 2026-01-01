/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <build_version.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/opengl/utils.h>
#include <gal/definitions.h>
#include <gal/opengl/gl_context_mgr.h>
#include <geometry/shape_poly_set.h>
#include <math/vector2wx.h>
#include <bitmap_base.h>
#include <bezier_curves.h>
#include <math/util.h> // for KiROUND
#include <pgm_base.h>
#include <trace_helpers.h>

#include <wx/frame.h>

#include <macros.h>
#include <geometry/geometry_utils.h>
#include <thread_pool.h>

#include <core/profile.h>
#include <trace_helpers.h>

#include <gal/opengl/gl_utils.h>

#include <functional>
#include <limits>
#include <memory>
#include <list>
using namespace std::placeholders;
using namespace KIGFX;

//#define DISABLE_BITMAP_CACHE

// The current font is "Ubuntu Mono" available under Ubuntu Font Licence 1.0
// (see ubuntu-font-licence-1.0.txt for details)
#include "gl_resources.h"
#include <glsl_kicad_frag.h>
#include <glsl_kicad_vert.h>
using namespace KIGFX::BUILTIN_FONT;

static void InitTesselatorCallbacks( GLUtesselator* aTesselator );

// Trace mask for XOR/difference mode debugging
static const wxChar* const traceGalXorMode = wxT( "KICAD_GAL_XOR_MODE" );

static wxGLAttributes getGLAttribs()
{
    wxGLAttributes attribs;
    attribs.RGBA().DoubleBuffer().Depth( 8 ).EndList();

    return attribs;
}

wxGLContext* OPENGL_GAL::m_glMainContext = nullptr;
int          OPENGL_GAL::m_instanceCounter = 0;
GLuint       OPENGL_GAL::g_fontTexture = 0;
bool         OPENGL_GAL::m_isBitmapFontLoaded = false;

namespace KIGFX
{
class GL_BITMAP_CACHE
{
public:
    GL_BITMAP_CACHE() :
            m_cacheSize( 0 )
    {}

    ~GL_BITMAP_CACHE();

    GLuint RequestBitmap( const BITMAP_BASE* aBitmap );

private:
    struct CACHED_BITMAP
    {
        GLuint        id;
        int           w, h;
        size_t        size;
        long long int accessTime;
    };

    GLuint cacheBitmap( const BITMAP_BASE* aBitmap );

    const size_t m_cacheMaxElements = 50;
    const size_t m_cacheMaxSize     = 256 * 1024 * 1024;

    std::map<const KIID, CACHED_BITMAP> m_bitmaps;
    std::list<KIID>                     m_cacheLru;
    size_t                              m_cacheSize;
    std::list<GLuint>                   m_freedTextureIds;
};

}; // namespace KIGFX


GL_BITMAP_CACHE::~GL_BITMAP_CACHE()
{
    for( auto& bitmap : m_bitmaps )
        glDeleteTextures( 1, &bitmap.second.id );
}


GLuint GL_BITMAP_CACHE::RequestBitmap( const BITMAP_BASE* aBitmap )
{
#ifndef DISABLE_BITMAP_CACHE
    auto it = m_bitmaps.find( aBitmap->GetImageID() );

    if( it != m_bitmaps.end() )
    {
        // A bitmap is found in cache bitmap. Ensure the associated texture is still valid.
        if( glIsTexture( it->second.id ) )
        {
            it->second.accessTime = wxGetUTCTimeMillis().GetValue();
            return it->second.id;
        }
        else
        {
            // Delete the invalid bitmap cache and its data
            glDeleteTextures( 1, &it->second.id );
            m_freedTextureIds.emplace_back( it->second.id );

            auto listIt = std::find( m_cacheLru.begin(), m_cacheLru.end(), it->first );

            if( listIt != m_cacheLru.end() )
                m_cacheLru.erase( listIt );

            m_cacheSize -= it->second.size;

            m_bitmaps.erase( it );
        }

        // the cached bitmap is not valid and deleted, it will be recreated.
    }

#endif
    return cacheBitmap( aBitmap );
}


GLuint GL_BITMAP_CACHE::cacheBitmap( const BITMAP_BASE* aBitmap )
{
    CACHED_BITMAP bmp;

    const wxImage* imgPtr = aBitmap->GetOriginalImageData();

    if( !imgPtr )
        return std::numeric_limits< GLuint >::max();

    const wxImage& imgData = *imgPtr;

    bmp.w = imgData.GetSize().x;
    bmp.h = imgData.GetSize().y;

    GLuint textureID;

    if( m_freedTextureIds.empty() )
    {
        glGenTextures( 1, &textureID );
    }
    else
    {
        textureID = m_freedTextureIds.front();
        m_freedTextureIds.pop_front();
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    if( imgData.HasAlpha() || imgData.HasMask() )
    {
        bmp.size = bmp.w * bmp.h * 4;
        auto buf = std::make_unique<uint8_t[]>( bmp.size );

        uint8_t* dstP = buf.get();
        uint8_t* srcP = imgData.GetData();

        long long pxCount = static_cast<long long>( bmp.w ) * bmp.h;

        if( imgData.HasAlpha() )
        {
            uint8_t* srcAlpha = imgData.GetAlpha();

            for( long long px = 0; px < pxCount; px++ )
            {
                memcpy( dstP, srcP, 3 );
                dstP[3] = *srcAlpha;

                srcAlpha += 1;
                srcP += 3;
                dstP += 4;
            }
        }
        else if( imgData.HasMask() )
        {
            uint8_t maskRed = imgData.GetMaskRed();
            uint8_t maskGreen = imgData.GetMaskGreen();
            uint8_t maskBlue = imgData.GetMaskBlue();

            for( long long px = 0; px < pxCount; px++ )
            {
                memcpy( dstP, srcP, 3 );

                if( srcP[0] == maskRed && srcP[1] == maskGreen && srcP[2] == maskBlue )
                    dstP[3] = wxALPHA_TRANSPARENT;
                else
                    dstP[3] = wxALPHA_OPAQUE;

                srcP += 3;
                dstP += 4;
            }
        }

        glBindTexture( GL_TEXTURE_2D, textureID );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bmp.w, bmp.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                      buf.get() );
    }
    else
    {
        bmp.size = bmp.w * bmp.h * 3;

        uint8_t* srcP = imgData.GetData();

        glBindTexture( GL_TEXTURE_2D, textureID );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, bmp.w, bmp.h, 0, GL_RGB, GL_UNSIGNED_BYTE, srcP );
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    long long currentTime = wxGetUTCTimeMillis().GetValue();

    bmp.id = textureID;
    bmp.accessTime = currentTime;

#ifndef DISABLE_BITMAP_CACHE
    if( ( m_cacheLru.size() + 1 > m_cacheMaxElements || m_cacheSize + bmp.size > m_cacheMaxSize )
        && !m_cacheLru.empty() )
    {
        KIID toRemove( 0 );
        auto toRemoveLru = m_cacheLru.end();

        // Remove entries accessed > 1s ago first
        for( const auto& [kiid, cachedBmp] : m_bitmaps )
        {
            const int cacheTimeoutMillis = 1000L;

            if( currentTime - cachedBmp.accessTime > cacheTimeoutMillis )
            {
                toRemove = kiid;
                toRemoveLru = std::find( m_cacheLru.begin(), m_cacheLru.end(), toRemove );
                break;
            }
        }

        // Otherwise, remove the latest entry (it's less likely to be needed soon)
        if( toRemove == niluuid )
        {
            toRemoveLru = m_cacheLru.end();
            toRemoveLru--;

            toRemove = *toRemoveLru;
        }

        CACHED_BITMAP& cachedBitmap = m_bitmaps[toRemove];

        m_cacheSize -= cachedBitmap.size;
        glDeleteTextures( 1, &cachedBitmap.id );
        m_freedTextureIds.emplace_back( cachedBitmap.id );

        m_bitmaps.erase( toRemove );
        m_cacheLru.erase( toRemoveLru );
    }

    m_cacheLru.emplace_back( aBitmap->GetImageID() );
    m_cacheSize += bmp.size;
    m_bitmaps.emplace( aBitmap->GetImageID(), std::move( bmp ) );
#endif

    return textureID;
}


OPENGL_GAL::OPENGL_GAL( const KIGFX::VC_SETTINGS& aVcSettings, GAL_DISPLAY_OPTIONS& aDisplayOptions,
                        wxWindow* aParent,
                        wxEvtHandler* aMouseListener, wxEvtHandler* aPaintListener,
                        const wxString& aName ) :
        GAL( aDisplayOptions ),
        HIDPI_GL_CANVAS( aVcSettings, aParent, getGLAttribs(), wxID_ANY, wxDefaultPosition,
                         wxDefaultSize,
                         wxEXPAND, aName ),
        m_mouseListener( aMouseListener ),
        m_paintListener( aPaintListener ),
        m_currentManager( nullptr ),
        m_cachedManager( nullptr ),
        m_nonCachedManager( nullptr ),
        m_overlayManager( nullptr ),
        m_tempManager( nullptr ),
        m_mainBuffer( 0 ),
        m_overlayBuffer( 0 ),
        m_tempBuffer( 0 ),
        m_isContextLocked( false ),
        m_lockClientCookie( 0 )
{
    if( m_glMainContext == nullptr )
    {
        m_glMainContext = Pgm().GetGLContextManager()->CreateCtx( this );

        if( !m_glMainContext )
            throw std::runtime_error( "Could not create the main OpenGL context" );

        m_glPrivContext = m_glMainContext;
    }
    else
    {
        m_glPrivContext = Pgm().GetGLContextManager()->CreateCtx( this, m_glMainContext );

        if( !m_glPrivContext )
            throw std::runtime_error( "Could not create a private OpenGL context" );
    }

    m_shader = new SHADER();
    ++m_instanceCounter;

    m_bitmapCache = std::make_unique<GL_BITMAP_CACHE>();

    m_compositor = new OPENGL_COMPOSITOR;
    m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );

    // Initialize the flags
    m_isFramebufferInitialized = false;
    m_isBitmapFontInitialized = false;
    m_isInitialized = false;
    m_isGrouping = false;
    m_groupCounter = 0;

    // Connect the native cursor handler
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( OPENGL_GAL::onSetNativeCursor ), nullptr,
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
    Connect( wxEVT_AUX1_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DCLICK, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DOWN, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_UP, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DCLICK, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
    Connect( wxEVT_MAGNIFY, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );

#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( OPENGL_GAL::skipMouseEvent ) );
#endif

    Bind( wxEVT_GESTURE_ZOOM, &OPENGL_GAL::skipGestureEvent, this );
    Bind( wxEVT_GESTURE_PAN, &OPENGL_GAL::skipGestureEvent, this );

    SetSize( aParent->GetClientSize() );
    m_screenSize = ToVECTOR2I( GetNativePixelSize() );

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.8, 0.8, 0.8, 0.1 ) );
    SetAxesColor( COLOR4D( BLUE ) );

    // Tesselator initialization
    m_tesselator = gluNewTess();
    InitTesselatorCallbacks( m_tesselator );

    gluTessProperty( m_tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    SetTarget( TARGET_NONCACHED );

    // Avoid uninitialized variables:
    ufm_worldPixelSize = -1;
    ufm_screenPixelSize = -1;
    ufm_pixelSizeMultiplier = -1;
    ufm_antialiasingOffset = -1;
    ufm_minLinePixelWidth = -1;
    ufm_fontTexture = -1;
    ufm_fontTextureWidth = -1;
    m_swapInterval  = 0;
}


OPENGL_GAL::~OPENGL_GAL()
{

    GL_CONTEXT_MANAGER* gl_mgr = Pgm().GetGLContextManager();
    gl_mgr->LockCtx( m_glPrivContext, this );

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
        delete m_tempManager;
    }

    gl_mgr->UnlockCtx( m_glPrivContext );

    // If it was the main context, then it will be deleted
    // when the last OpenGL GAL instance is destroyed (a few lines below)
    if( m_glPrivContext != m_glMainContext )
        gl_mgr->DestroyCtx( m_glPrivContext );

    delete m_shader;

    // Are we destroying the last GAL instance?
    if( m_instanceCounter == 0 )
    {
        gl_mgr->LockCtx( m_glMainContext, this );

        if( m_isBitmapFontLoaded )
        {
            glDeleteTextures( 1, &g_fontTexture );
            m_isBitmapFontLoaded = false;
        }

        gl_mgr->UnlockCtx( m_glMainContext );
        gl_mgr->DestroyCtx( m_glMainContext );
        m_glMainContext = nullptr;
    }
}


wxString OPENGL_GAL::CheckFeatures( GAL_DISPLAY_OPTIONS& aOptions )
{
    wxString retVal = wxEmptyString;

    wxFrame* testFrame = new wxFrame( nullptr, wxID_ANY, wxT( "" ), wxDefaultPosition,
                                      wxSize( 1, 1 ), wxFRAME_TOOL_WINDOW | wxNO_BORDER );

    KIGFX::OPENGL_GAL* opengl_gal = nullptr;

    try
    {
        KIGFX::VC_SETTINGS dummy;
        opengl_gal = new KIGFX::OPENGL_GAL( dummy, aOptions, testFrame );

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
    GAL_CONTEXT_LOCKER lock( this );

    bool refresh = false;

    if( m_options.antialiasing_mode != m_compositor->GetAntialiasingMode() )
    {
        m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );
        m_isFramebufferInitialized = false;
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
    return VECTOR2D( 2.0 / (double) ( m_screenSize.x * sf ), 2.0 /
                     (double) ( m_screenSize.y * sf ) );
}


void OPENGL_GAL::BeginDrawing()
{
#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalRealTime( "OPENGL_GAL::beginDrawing()", true );
#endif /* KICAD_GAL_PROFILE */

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
            m_tempBuffer = m_compositor->CreateBuffer();
        }
        catch( const std::runtime_error& )
        {
            wxLogVerbose( "Could not create a framebuffer for diff mode blending.\n" );
            m_tempBuffer = 0;
        }
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
    m_tempManager->Clear();

    m_cachedManager->BeginDrawing();
    m_nonCachedManager->BeginDrawing();
    m_overlayManager->BeginDrawing();
    m_tempManager->BeginDrawing();

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
    const VECTOR2D& screenPixelSize = getScreenPixelSize();
    m_shader->SetParameter( ufm_screenPixelSize, screenPixelSize );
    double pixelSizeMultiplier = m_compositor->GetAntialiasSupersamplingFactor();
    m_shader->SetParameter( ufm_pixelSizeMultiplier, (float) pixelSizeMultiplier );
    VECTOR2D renderingOffset = m_compositor->GetAntialiasRenderingOffset();
    renderingOffset.x *= screenPixelSize.x;
    renderingOffset.y *= screenPixelSize.y;
    m_shader->SetParameter( ufm_antialiasingOffset, renderingOffset );
    m_shader->SetParameter( ufm_minLinePixelWidth, GetMinLineWidth() );
    m_shader->Deactivate();

    // Something between BeginDrawing and EndDrawing seems to depend on
    // this texture unit being active, but it does not assure it itself.
    glActiveTexture( GL_TEXTURE0 );

    // Unbind buffers - set compositor for direct drawing
    m_compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

#ifdef KICAD_GAL_PROFILE
    totalRealTime.Stop();
    wxLogTrace( traceGalProfile, wxT( "OPENGL_GAL::beginDrawing(): %.1f ms" ),
                totalRealTime.msecs() );
#endif /* KICAD_GAL_PROFILE */
}

void OPENGL_GAL::SetMinLineWidth( float aLineWidth )
{
    GAL::SetMinLineWidth( aLineWidth );

    if( m_shader && ufm_minLinePixelWidth != -1 )
    {
        m_shader->Use();
        m_shader->SetParameter( ufm_minLinePixelWidth, aLineWidth );
        m_shader->Deactivate();
    }
}


void OPENGL_GAL::EndDrawing()
{
    wxASSERT_MSG( m_isContextLocked, "What happened to the context lock?" );

    PROF_TIMER cntTotal( "gl-end-total" );
    PROF_TIMER cntEndCached( "gl-end-cached" );
    PROF_TIMER cntEndNoncached( "gl-end-noncached" );
    PROF_TIMER cntEndOverlay( "gl-end-overlay" );
    PROF_TIMER cntComposite( "gl-composite" );
    PROF_TIMER cntSwap( "gl-swap" );

    cntTotal.Start();

    // Cached & non-cached containers are rendered to the same buffer
    m_compositor->SetBuffer( m_mainBuffer );

    cntEndNoncached.Start();
    m_nonCachedManager->EndDrawing();
    cntEndNoncached.Stop();

    cntEndCached.Start();
    m_cachedManager->EndDrawing();
    cntEndCached.Stop();

    cntEndOverlay.Start();
    // Overlay container is rendered to a different buffer
    if( m_overlayBuffer )
        m_compositor->SetBuffer( m_overlayBuffer );

    m_overlayManager->EndDrawing();
    cntEndOverlay.Stop();

    cntComposite.Start();

    // Be sure that the framebuffer is not colorized (happens on specific GPU&drivers combinations)
    glColor4d( 1.0, 1.0, 1.0, 1.0 );

    // Draw the remaining contents, blit the rendering targets to the screen, swap the buffers
    m_compositor->DrawBuffer( m_mainBuffer );

    if( m_overlayBuffer )
        m_compositor->DrawBuffer( m_overlayBuffer );

    m_compositor->Present();
    blitCursor();

    cntComposite.Stop();

    cntSwap.Start();
    SwapBuffers();
    cntSwap.Stop();

    cntTotal.Stop();

    KI_TRACE( traceGalProfile, "Timing: %s %s %s %s %s %s\n", cntTotal.to_string(),
              cntEndCached.to_string(), cntEndNoncached.to_string(), cntEndOverlay.to_string(),
              cntComposite.to_string(), cntSwap.to_string() );
}


void OPENGL_GAL::LockContext( int aClientCookie )
{
    wxASSERT_MSG( !m_isContextLocked, "Context already locked." );
    m_isContextLocked = true;
    m_lockClientCookie = aClientCookie;

    Pgm().GetGLContextManager()->LockCtx( m_glPrivContext, this );
}


void OPENGL_GAL::UnlockContext( int aClientCookie )
{
    wxASSERT_MSG( m_isContextLocked, "Context not locked.  A GAL_CONTEXT_LOCKER RAII object must "
                                     "be stacked rather than making separate lock/unlock calls." );

    wxASSERT_MSG( m_lockClientCookie == aClientCookie,
                  "Context was locked by a different client. "
                  "Should not be possible with RAII objects." );

    m_isContextLocked = false;

    Pgm().GetGLContextManager()->UnlockCtx( m_glPrivContext );
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
    drawSegment( aStartPoint, aEndPoint, aWidth );
}


void OPENGL_GAL::drawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth,
                              bool aReserve )
{
    VECTOR2D startEndVector = aEndPoint - aStartPoint;
    double   lineLength = startEndVector.EuclideanNorm();

    // Be careful about floating point rounding.  As we draw segments in larger and larger
    // coordinates, the shader (which uses floats) will lose precision and stop drawing small
    // segments.  In this case, we need to draw a circle for the minimal segment.
    // Check if the coordinate differences can be accurately represented as floats
    float startX = static_cast<float>( aStartPoint.x );
    float startY = static_cast<float>( aStartPoint.y );
    float endX = static_cast<float>( aEndPoint.x );
    float endY = static_cast<float>( aEndPoint.y );

    if( startX == endX && startY == endY )
    {
        drawCircle( aStartPoint, aWidth / 2, aReserve );
        return;
    }

    if( m_isFillEnabled || aWidth == 1.0 )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        SetLineWidth( aWidth );
        drawLineQuad( aStartPoint, aEndPoint, aReserve );
    }
    else
    {
        EDA_ANGLE lineAngle( startEndVector );

        // Outlined tracks
        SetLineWidth( 1.0 );
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        Save();

        if( aReserve )
            m_currentManager->Reserve( 6 + 6 + 3 + 3 ); // Two line quads and two semicircles

        m_currentManager->Translate( aStartPoint.x, aStartPoint.y, 0.0 );
        m_currentManager->Rotate( lineAngle.AsRadians(), 0.0f, 0.0f, 1.0f );

        drawLineQuad( VECTOR2D( 0.0, aWidth / 2.0 ), VECTOR2D( lineLength, aWidth / 2.0 ), false );

        drawLineQuad( VECTOR2D( 0.0, -aWidth / 2.0 ), VECTOR2D( lineLength, -aWidth / 2.0 ),
                      false );

        // Draw line caps
        drawStrokedSemiCircle( VECTOR2D( 0.0, 0.0 ), aWidth / 2, M_PI / 2, false );
        drawStrokedSemiCircle( VECTOR2D( lineLength, 0.0 ), aWidth / 2, -M_PI / 2, false );

        Restore();
    }
}


void OPENGL_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    drawCircle( aCenterPoint, aRadius );
}


void OPENGL_GAL::DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                               double aWallWidth )
{
    if( m_isFillEnabled )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

        m_currentManager->Shader( SHADER_HOLE_WALL, 1.0, aHoleRadius, aWallWidth );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_HOLE_WALL, 2.0, aHoleRadius, aWallWidth );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );

        m_currentManager->Shader( SHADER_HOLE_WALL, 3.0, aHoleRadius, aWallWidth );
        m_currentManager->Vertex( aCenterPoint.x, aCenterPoint.y, m_layerDepth );
    }
}


void OPENGL_GAL::drawCircle( const VECTOR2D& aCenterPoint, double aRadius, bool aReserve )
{
    if( m_isFillEnabled )
    {
        if( aReserve )
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
        if( aReserve )
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


void OPENGL_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                          const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle )
{
    if( aRadius <= 0 )
        return;

    double startAngle = aStartAngle.AsRadians();
    double endAngle = startAngle + aAngle.AsRadians();

    // Normalize arc angles
    normalize( startAngle, endAngle );

    const double alphaIncrement = calcAngleStep( aRadius );

    Save();
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );

    if( m_isFillEnabled )
    {
        double alpha;
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );
        m_currentManager->Shader( SHADER_NONE );

        // Triangle fan
        for( alpha = startAngle; ( alpha + alphaIncrement ) < endAngle; )
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
        const VECTOR2D endPoint( cos( endAngle ) * aRadius, sin( endAngle ) * aRadius );

        m_currentManager->Reserve( 3 );
        m_currentManager->Vertex( 0.0, 0.0, m_layerDepth );
        m_currentManager->Vertex( cos( alpha ) * aRadius, sin( alpha ) * aRadius, m_layerDepth );
        m_currentManager->Vertex( endPoint.x, endPoint.y, m_layerDepth );
    }

    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        VECTOR2D p( cos( startAngle ) * aRadius, sin( startAngle ) * aRadius );
        double   alpha;
        unsigned int lineCount = 0;

        for( alpha = startAngle + alphaIncrement; alpha <= endAngle; alpha += alphaIncrement )
            lineCount++;

        if( alpha != endAngle )
            lineCount++;

        reserveLineQuads( lineCount );

        for( alpha = startAngle + alphaIncrement; alpha <= endAngle; alpha += alphaIncrement )
        {
            VECTOR2D p_next( cos( alpha ) * aRadius, sin( alpha ) * aRadius );
            drawLineQuad( p, p_next, false );

            p = p_next;
        }

        // Draw the last missing part
        if( alpha != endAngle )
        {
            VECTOR2D p_last( cos( endAngle ) * aRadius, sin( endAngle ) * aRadius );
            drawLineQuad( p, p_last, false );
        }
    }

    Restore();
}


void OPENGL_GAL::DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                                 const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                                 double aWidth, double aMaxError )
{
    if( aRadius <= 0 )
    {
        // Arcs of zero radius are a circle of aWidth diameter
        if( aWidth > 0 )
            DrawCircle( aCenterPoint, aWidth / 2.0 );

        return;
    }

    double startAngle = aStartAngle.AsRadians();
    double endAngle = startAngle + aAngle.AsRadians();

    // Swap the angles, if start angle is greater than end angle
    normalize( startAngle, endAngle );

    // Calculate the seg count to approximate the arc with aMaxError or less
    int segCount360 = GetArcToSegmentCount( aRadius, aMaxError, FULL_CIRCLE );
    segCount360 = std::max( SEG_PER_CIRCLE_COUNT, segCount360 );
    double alphaIncrement = 2.0 * M_PI / segCount360;

    // Refinement: Use a segment count multiple of 2, because we have a control point
    // on the middle of the arc, and the look is better if it is on a segment junction
    // because there is no approx error
    int seg_count = KiROUND( ( endAngle - startAngle ) / alphaIncrement );

    if( seg_count % 2 != 0 )
        seg_count += 1;

    // Our shaders have trouble rendering null line quads, so delegate this task to DrawSegment.
    if( seg_count == 0 )
    {
        VECTOR2D p_start( aCenterPoint.x + cos( startAngle ) * aRadius,
                          aCenterPoint.y + sin( startAngle ) * aRadius );

        VECTOR2D p_end( aCenterPoint.x + cos( endAngle ) * aRadius,
                        aCenterPoint.y + sin( endAngle ) * aRadius );

        DrawSegment( p_start, p_end, aWidth );
        return;
    }

    // Recalculate alphaIncrement with a even integer number of segment
    alphaIncrement = ( endAngle - startAngle ) / seg_count;

    Save();
    m_currentManager->Translate( aCenterPoint.x, aCenterPoint.y, 0.0 );

    if( m_isStrokeEnabled )
    {
        m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                                 m_strokeColor.a );

        double   width = aWidth / 2.0;
        VECTOR2D startPoint( cos( startAngle ) * aRadius, sin( startAngle ) * aRadius );
        VECTOR2D endPoint( cos( endAngle ) * aRadius, sin( endAngle ) * aRadius );

        drawStrokedSemiCircle( startPoint, width, startAngle + M_PI );
        drawStrokedSemiCircle( endPoint, width, endAngle );

        VECTOR2D pOuter( cos( startAngle ) * ( aRadius + width ),
                         sin( startAngle ) * ( aRadius + width ) );

        VECTOR2D pInner( cos( startAngle ) * ( aRadius - width ),
                         sin( startAngle ) * ( aRadius - width ) );

        double alpha;

        for( alpha = startAngle + alphaIncrement; alpha <= endAngle; alpha += alphaIncrement )
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
        if( alpha != endAngle )
        {
            VECTOR2D pLastOuter( cos( endAngle ) * ( aRadius + width ),
                                 sin( endAngle ) * ( aRadius + width ) );
            VECTOR2D pLastInner( cos( endAngle ) * ( aRadius - width ),
                                 sin( endAngle ) * ( aRadius - width ) );

            DrawLine( pOuter, pLastOuter );
            DrawLine( pInner, pLastInner );
        }
    }

    if( m_isFillEnabled )
    {
        m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );
        SetLineWidth( aWidth );

        VECTOR2D p( cos( startAngle ) * aRadius, sin( startAngle ) * aRadius );
        double   alpha;

        int lineCount = 0;

        for( alpha = startAngle + alphaIncrement; alpha <= endAngle; alpha += alphaIncrement )
        {
            lineCount++;
        }

        // The last missing part
        if( alpha != endAngle )
        {
            lineCount++;
        }

        reserveLineQuads( lineCount );

        for( alpha = startAngle + alphaIncrement; alpha <= endAngle; alpha += alphaIncrement )
        {
            VECTOR2D p_next( cos( alpha ) * aRadius, sin( alpha ) * aRadius );
            drawLineQuad( p, p_next, false );

            p = p_next;
        }

        // Draw the last missing part
        if( alpha != endAngle )
        {
            VECTOR2D p_last( cos( endAngle ) * aRadius, sin( endAngle ) * aRadius );
            drawLineQuad( p, p_last, false );
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

        // DrawLine (and DrawPolyline )
        // has problem with 0 length lines so enforce minimum
        if( aStartPoint == aEndPoint )
        {
            DrawLine( aStartPoint + VECTOR2D( 1.0, 0.0 ), aEndPoint );
        }
        else
        {
            std::deque<VECTOR2D> pointList;

            pointList.push_back( aStartPoint );
            pointList.push_back( diagonalPointA );
            pointList.push_back( aEndPoint );
            pointList.push_back( diagonalPointB );
            pointList.push_back( aStartPoint );
            DrawPolyline( pointList );
        }
    }
}


void OPENGL_GAL::DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth )
{
    drawSegmentChain(
            [&]( int idx )
            {
                return aPointList[idx];
            },
            aPointList.size(), aWidth );
}


void OPENGL_GAL::DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth )
{
    auto numPoints = aLineChain.PointCount();

    if( aLineChain.IsClosed() )
        numPoints += 1;

    drawSegmentChain(
            [&]( int idx )
            {
                return aLineChain.CPoint( idx );
            },
            numPoints, aWidth );
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


void OPENGL_GAL::DrawPolyline( const std::vector<VECTOR2D>& aPointList )
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


void OPENGL_GAL::DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointList )
{
    int lineQuadCount = 0;

    for( const std::vector<VECTOR2D>& points : aPointList )
        lineQuadCount += points.size() - 1;

    reserveLineQuads( lineQuadCount );

    for( const std::vector<VECTOR2D>& points : aPointList )
    {
        drawPolyline(
                [&]( int idx )
                {
                    return points[idx];
                },
                points.size(), false );
    }
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


void OPENGL_GAL::drawTriangulatedPolyset( const SHAPE_POLY_SET& aPolySet,
                                          bool aStrokeTriangulation )
{
    m_currentManager->Shader( SHADER_NONE );
    m_currentManager->Color( m_fillColor.r, m_fillColor.g, m_fillColor.b, m_fillColor.a );

    if( m_isFillEnabled )
    {
        int totalTriangleCount = 0;

        for( unsigned int j = 0; j < aPolySet.TriangulatedPolyCount(); ++j )
        {
            auto triPoly = aPolySet.TriangulatedPolygon( j );

            totalTriangleCount += triPoly->GetTriangleCount();
        }

        m_currentManager->Reserve( 3 * totalTriangleCount );

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
        aStrokeTriangulation = true;
        SetStrokeColor( COLOR4D( 0.0, 1.0, 0.2, 1.0 ) );
    }

    if( aStrokeTriangulation )
    {
        GAL_SCOPED_ATTRS( *this, GAL_SCOPED_ATTRS::STROKE_COLOR | GAL_SCOPED_ATTRS::LAYER_DEPTH );
        SetLayerDepth( m_layerDepth - 1 );

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
    }
}


void OPENGL_GAL::DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool aStrokeTriangulation )
{
    if( aPolySet.IsTriangulationUpToDate() )
    {
        drawTriangulatedPolyset( aPolySet, aStrokeTriangulation );
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
    if( aPolygon.PointCount() < 2 )
        return;

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

    if( output.size() == 1 )
        output.push_back( output.front() );

    DrawPolygon( &output[0], output.size() );
}


void OPENGL_GAL::DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend )
{
    GLfloat alpha = std::clamp( alphaBlend, 0.0, 1.0 );

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

    glDepthFunc( GL_ALWAYS );

    glAlphaFunc( GL_GREATER, 0.01f );
    glEnable( GL_ALPHA_TEST );

    glMatrixMode( GL_TEXTURE );
    glPushMatrix();
    glTranslated( 0.5, 0.5, 0.5 );
    glRotated( aBitmap.Rotation().AsDegrees(), 0, 0, 1 );
    glTranslated( -0.5, -0.5, -0.5 );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glTranslated( trans.x, trans.y, trans.z );

    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_id );

    float texStartX = aBitmap.IsMirroredX() ? 1.0 : 0.0;
    float texEndX   = aBitmap.IsMirroredX() ? 0.0 : 1.0;
    float texStartY = aBitmap.IsMirroredY() ? 1.0 : 0.0;
    float texEndY   = aBitmap.IsMirroredY() ? 0.0 : 1.0;

    glBegin( GL_QUADS );
    glColor4f( 1.0, 1.0, 1.0, alpha );
    glTexCoord2f( texStartX, texStartY );
    glVertex3f( v0.x, v0.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, alpha );
    glTexCoord2f( texEndX,  texStartY);
    glVertex3f( v1.x, v0.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, alpha );
    glTexCoord2f( texEndX, texEndY);
    glVertex3f( v1.x, v1.y, m_layerDepth );
    glColor4f( 1.0, 1.0, 1.0, alpha );
    glTexCoord2f( texStartX, texEndY);
    glVertex3f( v0.x, v1.y, m_layerDepth );
    glEnd();

    glBindTexture( GL_TEXTURE_2D, 0 );

#ifdef DISABLE_BITMAP_CACHE
    glDeleteTextures( 1, &texture_id );
#endif

    glPopMatrix();

    glMatrixMode( GL_TEXTURE );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );

    glDisable( GL_ALPHA_TEST );

    glDepthFunc( GL_LESS );
}


void OPENGL_GAL::BitmapText( const wxString& aText, const VECTOR2I& aPosition,
                             const EDA_ANGLE& aAngle )
{
    // Fallback to generic impl (which uses the stroke font) on cases we don't handle
    if( IsTextMirrored()
            || aText.Contains( wxT( "^{" ) )
            || aText.Contains( wxT( "_{" ) )
            || aText.Contains( wxT( "\n" ) ) )
    {
        return GAL::BitmapText( aText, aPosition, aAngle );
    }

    const UTF8   text( aText );
    VECTOR2D     textSize;
    float        commonOffset;
    std::tie( textSize, commonOffset ) = computeBitmapTextSize( text );

    const double SCALE = 1.4 * GetGlyphSize().y / textSize.y;
    double       overbarHeight = textSize.y;

    Save();

    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );
    m_currentManager->Translate( aPosition.x, aPosition.y, m_layerDepth );
    m_currentManager->Rotate( aAngle.AsRadians(), 0.0f, 0.0f, -1.0f );

    double sx = SCALE * ( m_globalFlipX ? -1.0 : 1.0 );
    double sy = SCALE * ( m_globalFlipY ? -1.0 : 1.0 );

    m_currentManager->Scale( sx, sy, 0 );
    m_currentManager->Translate( 0, -commonOffset, 0 );

    switch( GetHorizontalJustify() )
    {
    case GR_TEXT_H_ALIGN_CENTER:
        Translate( VECTOR2D( -textSize.x / 2.0, 0 ) );
        break;

    case GR_TEXT_H_ALIGN_RIGHT:
        //if( !IsTextMirrored() )
        Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    case GR_TEXT_H_ALIGN_LEFT:
        //if( IsTextMirrored() )
        //Translate( VECTOR2D( -textSize.x, 0 ) );
        break;

    case GR_TEXT_H_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    switch( GetVerticalJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP:
        break;

    case GR_TEXT_V_ALIGN_CENTER:
        Translate( VECTOR2D( 0, -textSize.y / 2.0 ) );
        overbarHeight = 0;
        break;

    case GR_TEXT_V_ALIGN_BOTTOM:
        Translate( VECTOR2D( 0, -textSize.y ) );
        overbarHeight = -textSize.y / 2.0;
        break;

    case GR_TEXT_V_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    int overbarLength = 0;
    int overbarDepth = -1;
    int braceNesting = 0;

    auto iterateString =
            [&]( const std::function<void( int aOverbarLength, int aOverbarHeight )>& overbarFn,
                 const std::function<int( unsigned long aChar )>& bitmapCharFn )
            {
                for( UTF8::uni_iter chIt = text.ubegin(), end = text.uend(); chIt < end; ++chIt )
                {
                    wxASSERT_MSG( *chIt != '\n' && *chIt != '\r',
                                  "No support for multiline bitmap text yet" );

                    if( *chIt == '~' && overbarDepth == -1 )
                    {
                        UTF8::uni_iter lookahead = chIt;

                        if( ++lookahead != end && *lookahead == '{' )
                        {
                            chIt = lookahead;
                            overbarDepth = braceNesting;
                            braceNesting++;
                            continue;
                        }
                    }
                    else if( *chIt == '{' )
                    {
                        braceNesting++;
                    }
                    else if( *chIt == '}' )
                    {
                        if( braceNesting > 0 )
                            braceNesting--;

                        if( braceNesting == overbarDepth )
                        {
                            overbarFn( overbarLength, overbarHeight );
                            overbarLength = 0;

                            overbarDepth = -1;
                            continue;
                        }
                    }

                    if( overbarDepth != -1 )
                        overbarLength += bitmapCharFn( *chIt );
                    else
                        bitmapCharFn( *chIt );
                }
            };

    // First, calculate the amount of characters and overbars to reserve

    int charsCount = 0;
    int overbarsCount = 0;

    iterateString(
            [&overbarsCount]( int aOverbarLength, int aOverbarHeight )
            {
                overbarsCount++;
            },
            [&charsCount]( unsigned long aChar ) -> int
            {
                if( aChar != ' ' )
                    charsCount++;

                return 0;
            } );

    m_currentManager->Reserve( 6 * charsCount + 6 * overbarsCount );

    // Now reset the state and actually draw the characters and overbars
    overbarLength = 0;
    overbarDepth = -1;
    braceNesting = 0;

    iterateString(
            [&]( int aOverbarLength, int aOverbarHeight )
            {
                drawBitmapOverbar( aOverbarLength, aOverbarHeight, false );
            },
            [&]( unsigned long aChar ) -> int
            {
                return drawBitmapChar( aChar, false );
            } );

    // Handle the case when overbar is active till the end of the drawn text
    m_currentManager->Translate( 0, commonOffset, 0 );

    if( overbarDepth != -1 && overbarLength > 0 )
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

    VECTOR2D gridScreenSize = GetVisibleGridSize();

    // Compute grid starting and ending indexes to draw grid points on the
    // visible screen area
    // Note: later any point coordinate will be offset by m_gridOrigin
    int gridStartX = KiROUND( ( worldStartPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridEndX = KiROUND( ( worldEndPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridStartY = KiROUND( ( worldStartPoint.y - m_gridOrigin.y ) / gridScreenSize.y );
    int gridEndY = KiROUND( ( worldEndPoint.y - m_gridOrigin.y ) / gridScreenSize.y );

    // Ensure start coordinate < end coordinate
    normalize( gridStartX, gridEndX );
    normalize( gridStartY, gridEndY );

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
    m_currentManager->Scale( aScale.x, aScale.y, 1.0f );
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
    auto group = m_groups.find( aGroupNumber );

    if( group != m_groups.end() )
        m_cachedManager->DrawItem( *group->second );
}


void OPENGL_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    auto group = m_groups.find( aGroupNumber );

    if( group != m_groups.end() )
        m_cachedManager->ChangeItemColor( *group->second, aNewColor );
}


void OPENGL_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    auto group = m_groups.find( aGroupNumber );

    if( group != m_groups.end() )
        m_cachedManager->ChangeItemDepth( *group->second, aDepth );
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
    case TARGET_TEMP:      m_currentManager = m_tempManager;      break;
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

    case TARGET_TEMP:
        if( m_tempBuffer )
            m_compositor->SetBuffer( m_tempBuffer );
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
    case TARGET_TEMP:      return ( m_tempBuffer != 0 );
    }
}


void OPENGL_GAL::StartDiffLayer()
{
    wxLogTrace( traceGalXorMode, wxT( "OPENGL_GAL::StartDiffLayer() called" ) );
    wxLogTrace( traceGalXorMode, wxT( "StartDiffLayer(): m_tempBuffer=%u" ), m_tempBuffer );

    m_currentManager->EndDrawing();

    if( m_tempBuffer )
    {
        wxLogTrace( traceGalXorMode, wxT( "StartDiffLayer(): setting target to TARGET_TEMP" ) );
        SetTarget( TARGET_TEMP );
        ClearTarget( TARGET_TEMP );

        // ClearTarget restores the previous compositor buffer, so we need to explicitly
        // set the compositor to render to m_tempBuffer for the layer drawing
        m_compositor->SetBuffer( m_tempBuffer );
        wxLogTrace( traceGalXorMode, wxT( "StartDiffLayer(): TARGET_TEMP set and cleared, compositor buffer=%u" ),
                    m_tempBuffer );
    }
    else
    {
        wxLogTrace( traceGalXorMode, wxT( "StartDiffLayer(): WARNING - no temp buffer!" ) );
    }
}


void OPENGL_GAL::EndDiffLayer()
{
    wxLogTrace( traceGalXorMode, wxT( "OPENGL_GAL::EndDiffLayer() called" ) );
    wxLogTrace( traceGalXorMode, wxT( "EndDiffLayer(): m_tempBuffer=%u, m_mainBuffer=%u" ),
                m_tempBuffer, m_mainBuffer );

    if( m_tempBuffer )
    {
        wxLogTrace( traceGalXorMode, wxT( "EndDiffLayer(): using temp buffer path" ) );

        // End drawing to the temp buffer
        m_currentManager->EndDrawing();

        wxLogTrace( traceGalXorMode, wxT( "EndDiffLayer(): calling DrawBufferDifference" ) );

        // Use difference compositing for true XOR/difference mode:
        // - Where only one layer has content: shows that layer's color
        // - Where both layers overlap with identical content: cancels out (black)
        // - Where layers overlap with different content: shows the absolute difference
        m_compositor->DrawBufferDifference( m_tempBuffer, m_mainBuffer );

        wxLogTrace( traceGalXorMode, wxT( "EndDiffLayer(): DrawBufferDifference returned" ) );
    }
    else
    {
        wxLogTrace( traceGalXorMode, wxT( "EndDiffLayer(): NO temp buffer, using fallback path" ) );

        // Fall back to imperfect alpha blending on single buffer
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        m_currentManager->EndDrawing();
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }

    wxLogTrace( traceGalXorMode, wxT( "OPENGL_GAL::EndDiffLayer() complete" ) );
}


bool OPENGL_GAL::SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI )
{
    // Store the current cursor type and get the wx cursor for it
    if( !GAL::SetNativeCursorStyle( aCursor, aHiDPI ) )
        return false;

    m_currentwxCursor = CURSOR_STORE::GetCursor( m_currentNativeCursor, aHiDPI );

#if wxCHECK_VERSION( 3, 3, 0 )
    wxWindow::SetCursorBundle( m_currentwxCursor );
#else
    wxWindow::SetCursor( m_currentwxCursor );
#endif

    return true;
}


void OPENGL_GAL::onSetNativeCursor( wxSetCursorEvent& aEvent )
{
#if wxCHECK_VERSION( 3, 3, 0 )
    aEvent.SetCursor( m_currentwxCursor.GetCursorFor( this ) );
#else
    aEvent.SetCursor( m_currentwxCursor );
#endif
}


void OPENGL_GAL::DrawCursor( const VECTOR2D& aCursorPosition )
{
    // Now we should only store the position of the mouse cursor
    // The real drawing routines are in blitCursor()
    //VECTOR2D screenCursor = m_worldScreenMatrix * aCursorPosition;
    //m_cursorPosition = m_screenWorldMatrix * VECTOR2D( screenCursor.x, screenCursor.y );
    m_cursorPosition = aCursorPosition;
}


void OPENGL_GAL::drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                               const bool aReserve )
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

    if( aReserve )
        reserveLineQuads( 1 );

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


void OPENGL_GAL::reserveLineQuads( const int aLineCount )
{
    m_currentManager->Reserve( 6 * aLineCount );
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


void OPENGL_GAL::drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle,
                                        bool aReserve )
{
    double outerRadius = aRadius + ( m_lineWidth / 2 );

    Save();

    if( aReserve )
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


void OPENGL_GAL::drawPolyline( const std::function<VECTOR2D( int )>& aPointGetter, int aPointCount,
                               bool aReserve )
{
    wxCHECK( aPointCount > 0, /* return */ );

    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );

    if( aPointCount == 1 )
    {
        drawLineQuad( aPointGetter( 0 ), aPointGetter( 0 ), aReserve );
        return;
    }

    if( aReserve )
    {
        reserveLineQuads( aPointCount - 1 );
    }

    for( int i = 1; i < aPointCount; ++i )
    {
        auto start = aPointGetter( i - 1 );
        auto end = aPointGetter( i );

        drawLineQuad( start, end, false );
    }
}


void OPENGL_GAL::drawSegmentChain( const std::function<VECTOR2D( int )>& aPointGetter,
                                   int aPointCount, double aWidth, bool aReserve )
{
    wxCHECK( aPointCount >= 2, /* return */ );

    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );

    int vertices = 0;

    for( int i = 1; i < aPointCount; ++i )
    {
        auto start = aPointGetter( i - 1 );
        auto end = aPointGetter( i );

        float startx = start.x;
        float starty = start.y;
        float endx = end.x;
        float endy = end.y;

        // Be careful about floating point rounding.  As we draw segments in larger and larger
        // coordinates, the shader (which uses floats) will lose precision and stop drawing small
        // segments.  In this case, we need to draw a circle for the minimal segment.
        // Check if the coordinate differences can be accurately represented as floats

        if( startx == endx && starty == endy )
        {
            vertices += 3; // One circle
            continue;
        }

        if( m_isFillEnabled || aWidth == 1.0 )
        {
            vertices += 6; // One line
        }
        else
        {
            vertices += 6 + 6 + 3 + 3; // Two lines and two half-circles
        }
    }

    m_currentManager->Reserve( vertices );

    for( int i = 1; i < aPointCount; ++i )
    {
        auto start = aPointGetter( i - 1 );
        auto end = aPointGetter( i );

        drawSegment( start, end, aWidth, false );
    }
}


int OPENGL_GAL::drawBitmapChar( unsigned long aChar, bool aReserve )
{
    const float TEX_X = font_image.width;
    const float TEX_Y = font_image.height;

    // handle space
    if( aChar == ' ' )
    {
        const FONT_GLYPH_TYPE* g = LookupGlyph( 'x' );
        wxCHECK( g, 0 );

        // Match stroke font as well as possible
        double spaceWidth = g->advance * 0.74;

        Translate( VECTOR2D( spaceWidth, 0 ) );
        return KiROUND( spaceWidth );
    }

    const FONT_GLYPH_TYPE* glyph = LookupGlyph( aChar );

    // If the glyph is not found (happens for many esoteric unicode chars)
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

    if( aReserve )
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


void OPENGL_GAL::drawBitmapOverbar( double aLength, double aHeight, bool aReserve )
{
    // To draw an overbar, simply draw an overbar
    const FONT_GLYPH_TYPE* glyph = LookupGlyph( '_' );
    wxCHECK( glyph, /* void */ );

    const float H = glyph->maxy - glyph->miny;

    Save();

    Translate( VECTOR2D( -aLength, -aHeight ) );

    if( aReserve )
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

    VECTOR2D textSize( 0, 0 );
    float    commonOffset = std::numeric_limits<float>::max();
    float    charHeight = font_information.max_y - defaultGlyph->miny;
    int      overbarDepth = -1;
    int braceNesting = 0;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        if( *chIt == '~' && overbarDepth == -1 )
        {
            UTF8::uni_iter lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                overbarDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '{' )
        {
            braceNesting++;
        }
        else if( *chIt == '}' )
        {
            if( braceNesting > 0 )
                braceNesting--;

            if( braceNesting == overbarDepth )
            {
                overbarDepth = -1;
                continue;
            }
        }

        const FONT_GLYPH_TYPE* glyph = LookupGlyph( *chIt );

        if( !glyph                            // Not coded in font
            || *chIt == '-' || *chIt == '_' ) // Strange size of these 2 chars
        {
            glyph = defaultGlyph;
        }

        if( glyph )
            textSize.x += glyph->advance;
    }

    textSize.y = std::max<float>( textSize.y, charHeight );
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


void OPENGL_GAL::skipGestureEvent( wxGestureEvent& aEvent )
{
    // Post the gesture event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void OPENGL_GAL::blitCursor()
{
    if( !IsCursorEnabled() )
        return;

    m_compositor->SetBuffer( OPENGL_COMPOSITOR::DIRECT_RENDERING );

    VECTOR2D cursorBegin;
    VECTOR2D cursorEnd;
    VECTOR2D cursorCenter = m_cursorPosition;

    if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_CROSS )
    {
        cursorBegin = m_screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
        cursorEnd = m_screenWorldMatrix * VECTOR2D( m_screenSize );
    }
    else if( m_crossHairMode == CROSS_HAIR_MODE::SMALL_CROSS )
    {
        const int cursorSize = 80;
        cursorBegin = m_cursorPosition - cursorSize / ( 2 * m_worldScale );
        cursorEnd = m_cursorPosition + cursorSize / ( 2 * m_worldScale );
    }

    const COLOR4D color = getCursorColor();

    GLboolean depthTestEnabled = glIsEnabled( GL_DEPTH_TEST );
    glDisable( GL_DEPTH_TEST );

    glActiveTexture( GL_TEXTURE0 );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glLineWidth( 1.0 );
    glColor4d( color.r, color.g, color.b, color.a );

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glTranslated( 0, 0, -0.5 );

    glBegin( GL_LINES );

    if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL )
    {
        // Calculate screen bounds in world coordinates
        VECTOR2D screenTopLeft = m_screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
        VECTOR2D screenBottomRight = m_screenWorldMatrix * VECTOR2D( m_screenSize );

        // For 45-degree lines passing through cursor position
        // Line equation: y = x + (cy - cx) for positive slope
        // Line equation: y = -x + (cy + cx) for negative slope
        double cx = m_cursorPosition.x;
        double cy = m_cursorPosition.y;

        // Calculate intersections for positive slope diagonal (y = x + offset)
        double offset1 = cy - cx;
        VECTOR2D pos_start( screenTopLeft.x, screenTopLeft.x + offset1 );
        VECTOR2D pos_end( screenBottomRight.x, screenBottomRight.x + offset1 );

        // Draw positive slope diagonal
        glVertex2d( pos_start.x, pos_start.y );
        glVertex2d( pos_end.x, pos_end.y );

        // Calculate intersections for negative slope diagonal (y = -x + offset)
        double offset2 = cy + cx;
        VECTOR2D neg_start( screenTopLeft.x, offset2 - screenTopLeft.x );
        VECTOR2D neg_end( screenBottomRight.x, offset2 - screenBottomRight.x );

        // Draw negative slope diagonal
        glVertex2d( neg_start.x, neg_start.y );
        glVertex2d( neg_end.x, neg_end.y );
    }
    else
    {
        glVertex2d( cursorCenter.x, cursorBegin.y );
        glVertex2d( cursorCenter.x, cursorEnd.y );

        glVertex2d( cursorBegin.x, cursorCenter.y );
        glVertex2d( cursorEnd.x, cursorCenter.y );
    }

    glEnd();

    glPopMatrix();

    if( depthTestEnabled )
        glEnable( GL_DEPTH_TEST );
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
#ifndef KICAD_USE_EGL
    wxASSERT( IsShownOnScreen() );
#endif // KICAD_USE_EGL

    wxASSERT_MSG( m_isContextLocked, "This should only be called from within a locked context." );

    // Check correct initialization from the constructor
    if( m_tesselator == nullptr )
        throw std::runtime_error( "Could not create the tesselator" );
    GLenum err = glewInit();

#ifdef KICAD_USE_EGL
    // TODO: better way to check when EGL is ready (init fails at "getString(GL_VERSION)")
    for( int i = 0; i < 10; i++ )
    {
        if( GLEW_OK == err )
            break;

        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
        err = glewInit();
    }

#endif // KICAD_USE_EGL

    SetOpenGLInfo( (const char*) glGetString( GL_VENDOR ), (const char*) glGetString( GL_RENDERER ),
                   (const char*) glGetString( GL_VERSION ) );

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
                                             BUILTIN_SHADERS::glsl_kicad_vert ) )
    {
        throw std::runtime_error( "Cannot compile vertex shader!" );
    }

    if( !m_shader->IsLinked()
        && !m_shader->LoadShaderFromStrings( SHADER_TYPE_FRAGMENT,
                                             BUILTIN_SHADERS::glsl_kicad_frag ) )
    {
        throw std::runtime_error( "Cannot compile fragment shader!" );
    }

    if( !m_shader->IsLinked() && !m_shader->Link() )
        throw std::runtime_error( "Cannot link the shaders!" );

    // Set up shader parameters after linking
    setupShaderParameters();

    // Check if video card supports textures big enough to fit the font atlas
    int maxTextureSize;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );

    if( maxTextureSize < (int) font_image.width || maxTextureSize < (int) font_image.height )
    {
        // TODO implement software texture scaling
        // for bitmap fonts and use a higher resolution texture?
        throw std::runtime_error( "Requested texture size is not supported" );
    }

    m_swapInterval = GL_UTILS::SetSwapInterval( -1 );

    m_cachedManager = new VERTEX_MANAGER( true );
    m_nonCachedManager = new VERTEX_MANAGER( false );
    m_overlayManager = new VERTEX_MANAGER( false );
    m_tempManager = new VERTEX_MANAGER( false );

    // Make VBOs use shaders
    m_cachedManager->SetShader( *m_shader );
    m_nonCachedManager->SetShader( *m_shader );
    m_overlayManager->SetShader( *m_shader );
    m_tempManager->SetShader( *m_shader );

    m_isInitialized = true;
}


void OPENGL_GAL::setupShaderParameters()
{
    // Initialize shader uniform parameter locations
    ufm_fontTexture = m_shader->AddParameter( "u_fontTexture" );
    ufm_fontTextureWidth = m_shader->AddParameter( "u_fontTextureWidth" );
    ufm_worldPixelSize = m_shader->AddParameter( "u_worldPixelSize" );
    ufm_screenPixelSize = m_shader->AddParameter( "u_screenPixelSize" );
    ufm_pixelSizeMultiplier = m_shader->AddParameter( "u_pixelSizeMultiplier" );
    ufm_antialiasingOffset = m_shader->AddParameter( "u_antialiasingOffset" );
    ufm_minLinePixelWidth = m_shader->AddParameter( "u_minLinePixelWidth" );
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
    // Note, we use the default_delete for an array because macOS
    // decides to bundle an ancient libc++ that mismatches the C++17 support of clang
    param->intersectPoints.emplace_back( vertex, std::default_delete<GLdouble[]>() );

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


inline double round_to_half_pixel( double f, double r )
{
    return ( ceil( f / r ) - 0.5 ) * r;
}


void OPENGL_GAL::ComputeWorldScreenMatrix()
{
    computeWorldScale();
    auto pixelSize = m_worldScale;

    // we need -m_lookAtPoint == -k * pixelSize + 0.5 * pixelSize for OpenGL
    // meaning m_lookAtPoint = (k-0.5)*pixelSize with integer k
    m_lookAtPoint.x = round_to_half_pixel( m_lookAtPoint.x, pixelSize );
    m_lookAtPoint.y = round_to_half_pixel( m_lookAtPoint.y, pixelSize );

    GAL::ComputeWorldScreenMatrix();
}


void OPENGL_GAL::DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth, int aTotal )
{
    if( aGlyph.IsStroke() )
    {
        const auto& strokeGlyph = static_cast<const KIFONT::STROKE_GLYPH&>( aGlyph );

        DrawPolylines( strokeGlyph );
    }
    else if( aGlyph.IsOutline() )
    {
        const auto& outlineGlyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( aGlyph );

        m_currentManager->Shader( SHADER_NONE );
        m_currentManager->Color( m_fillColor );

        outlineGlyph.Triangulate(
                [&]( const VECTOR2D& aPt1, const VECTOR2D& aPt2, const VECTOR2D& aPt3 )
                {
                    m_currentManager->Reserve( 3 );

                    m_currentManager->Vertex( aPt1.x, aPt1.y, m_layerDepth );
                    m_currentManager->Vertex( aPt2.x, aPt2.y, m_layerDepth );
                    m_currentManager->Vertex( aPt3.x, aPt3.y, m_layerDepth );
                } );
    }
}


void OPENGL_GAL::DrawGlyphs( const std::vector<std::unique_ptr<KIFONT::GLYPH>>& aGlyphs )
{
    if( aGlyphs.empty() )
        return;

    bool allGlyphsAreStroke = true;
    bool allGlyphsAreOutline = true;

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
    {
        if( !glyph->IsStroke() )
        {
            allGlyphsAreStroke = false;
            break;
        }
    }

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
    {
        if( !glyph->IsOutline() )
        {
            allGlyphsAreOutline = false;
            break;
        }
    }

    if( allGlyphsAreStroke )
    {
        // Optimized path for stroke fonts that pre-reserves line quads.
        int lineQuadCount = 0;

        for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
        {
            const auto& strokeGlyph = static_cast<const KIFONT::STROKE_GLYPH&>( *glyph );

            for( const std::vector<VECTOR2D>& points : strokeGlyph )
                lineQuadCount += points.size() - 1;
        }

        reserveLineQuads( lineQuadCount );

        for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
        {
            const auto& strokeGlyph = static_cast<const KIFONT::STROKE_GLYPH&>( *glyph );

            for( const std::vector<VECTOR2D>& points : strokeGlyph )
            {
                drawPolyline(
                        [&]( int idx )
                        {
                            return points[idx];
                        },
                        points.size(), false );
            }
        }

        return;
    }
    else if( allGlyphsAreOutline )
    {
        // Optimized path for outline fonts that pre-reserves glyph triangles.
        int triangleCount = 0;

        for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
        {
            const auto& outlineGlyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( *glyph );

            for( unsigned int i = 0; i < outlineGlyph.TriangulatedPolyCount(); i++ )
            {
                const SHAPE_POLY_SET::TRIANGULATED_POLYGON* polygon =
                        outlineGlyph.TriangulatedPolygon( i );

                triangleCount += polygon->GetTriangleCount();
            }
        }

        m_currentManager->Shader( SHADER_NONE );
        m_currentManager->Color( m_fillColor );

        m_currentManager->Reserve( 3 * triangleCount );

        for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
        {
            const auto& outlineGlyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( *glyph );

            for( unsigned int i = 0; i < outlineGlyph.TriangulatedPolyCount(); i++ )
            {
                const SHAPE_POLY_SET::TRIANGULATED_POLYGON* polygon =
                        outlineGlyph.TriangulatedPolygon( i );

                for( size_t j = 0; j < polygon->GetTriangleCount(); j++ )
                {
                    VECTOR2I a, b, c;
                    polygon->GetTriangle( j, a, b, c );

                    m_currentManager->Vertex( a.x, a.y, m_layerDepth );
                    m_currentManager->Vertex( b.x, b.y, m_layerDepth );
                    m_currentManager->Vertex( c.x, c.y, m_layerDepth );
                }
            }
        }
    }
    else
    {
        // Regular path
        for( size_t i = 0; i < aGlyphs.size(); i++ )
            DrawGlyph( *aGlyphs[i], i, aGlyphs.size() );
    }
}
