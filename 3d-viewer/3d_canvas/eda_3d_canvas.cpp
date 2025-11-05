/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
#include <gal/opengl/gl_utils.h>
#include <wx/tokenzr.h>

#include "../common_ogl/ogl_utils.h"
#include "eda_3d_canvas.h"
#include <eda_3d_viewer_frame.h>
#include <3d_rendering/raytracing/render_3d_raytrace_gl.h>
#include <3d_rendering/opengl/render_3d_opengl.h>
#include <3d_viewer_id.h>
#include <advanced_config.h>
#include <build_version.h>
#include <board.h>
#include <pad.h>
#include <pcb_field.h>
#include <reporter.h>
#include <gal/opengl/gl_context_mgr.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <bitmaps.h>
#include <kiway_holder.h>
#include <kiway.h>
#include <macros.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/tool_dispatcher.h>
#include <string_utils.h>
#include <mail_type.h>
#include <kiway_express.h>
#include <fmt/format.h>

#include <widgets/wx_busy_indicator.h>


/**
 * Flag to enable 3D canvas debug tracing.
 *
 * Use "KI_TRACE_EDA_3D_CANVAS" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* EDA_3D_CANVAS::m_logTrace = wxT( "KI_TRACE_EDA_3D_CANVAS" );


// A custom event, used to call DoRePaint during an idle time
wxDEFINE_EVENT( wxEVT_REFRESH_CUSTOM_COMMAND, wxEvent );


BEGIN_EVENT_TABLE( EDA_3D_CANVAS, HIDPI_GL_3D_CANVAS )
    EVT_PAINT( EDA_3D_CANVAS::OnPaint )

    // mouse events
    EVT_LEFT_DOWN( EDA_3D_CANVAS::OnLeftDown )
    EVT_LEFT_UP( EDA_3D_CANVAS::OnLeftUp )
    EVT_MIDDLE_UP( EDA_3D_CANVAS::OnMiddleUp )
    EVT_MIDDLE_DOWN( EDA_3D_CANVAS::OnMiddleDown)
    EVT_RIGHT_DOWN( EDA_3D_CANVAS::OnRightDown )
    EVT_RIGHT_UP( EDA_3D_CANVAS::OnRightUp )
    EVT_MOUSEWHEEL( EDA_3D_CANVAS::OnMouseWheel )
    EVT_MOTION( EDA_3D_CANVAS::OnMouseMove )
    EVT_MAGNIFY( EDA_3D_CANVAS::OnMagnify )

    // touch gesture events
    EVT_GESTURE_ZOOM( wxID_ANY, EDA_3D_CANVAS::OnZoomGesture )
    EVT_GESTURE_PAN( wxID_ANY, EDA_3D_CANVAS::OnPanGesture )
    EVT_GESTURE_ROTATE( wxID_ANY, EDA_3D_CANVAS::OnRotateGesture )

    // other events
    EVT_ERASE_BACKGROUND( EDA_3D_CANVAS::OnEraseBackground )
    EVT_CUSTOM(wxEVT_REFRESH_CUSTOM_COMMAND, ID_CUSTOM_EVENT_1, EDA_3D_CANVAS::OnRefreshRequest )

    EVT_CLOSE( EDA_3D_CANVAS::OnCloseWindow )
    EVT_SIZE(  EDA_3D_CANVAS::OnResize )
END_EVENT_TABLE()


EDA_3D_CANVAS::EDA_3D_CANVAS( wxWindow* aParent, const wxGLAttributes& aGLAttribs,
                              BOARD_ADAPTER& aBoardAdapter, CAMERA& aCamera,
                              S3D_CACHE* a3DCachePointer ) :
    HIDPI_GL_3D_CANVAS( EDA_DRAW_PANEL_GAL::GetVcSettings(), aCamera, aParent, aGLAttribs,
                EDA_3D_CANVAS_ID, wxDefaultPosition,
                wxDefaultSize, wxFULL_REPAINT_ON_RESIZE ),
    m_editing_timeout_timer( this, wxID_HIGHEST + 1 ),
    m_redraw_trigger_timer( this, wxID_HIGHEST + 2 ),
    m_boardAdapter( aBoardAdapter )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::EDA_3D_CANVAS" ) );

    m_editing_timeout_timer.SetOwner( this );
    Connect( m_editing_timeout_timer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Editing ), nullptr, this );

    m_redraw_trigger_timer.SetOwner( this );
    Connect( m_redraw_trigger_timer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Redraw ), nullptr, this );

    m_is_currently_painting.clear();

    m_3d_render_raytracing = new RENDER_3D_RAYTRACE_GL( this, m_boardAdapter, m_camera );
    m_3d_render_opengl = new RENDER_3D_OPENGL( this, m_boardAdapter, m_camera );

    wxASSERT( m_3d_render_raytracing != nullptr );
    wxASSERT( m_3d_render_opengl != nullptr );

    auto busy_indicator_factory =
            []()
            {
                return std::make_unique<WX_BUSY_INDICATOR>();
            };

    m_3d_render_raytracing->SetBusyIndicatorFactory( busy_indicator_factory );
    m_3d_render_opengl->SetBusyIndicatorFactory( busy_indicator_factory );

    // We always start with the opengl engine (raytracing is avoided due to very
    // long calculation time)
    m_3d_render = m_3d_render_opengl;

    m_boardAdapter.ReloadColorSettings();

    wxASSERT( a3DCachePointer != nullptr );
    m_boardAdapter.Set3dCacheManager( a3DCachePointer );

#if defined( __WXMSW__ )
    EnableTouchEvents( wxTOUCH_ZOOM_GESTURE | wxTOUCH_ROTATE_GESTURE | wxTOUCH_PAN_GESTURES );
#elif defined( __WXGTK__ )
    EnableTouchEvents( wxTOUCH_ZOOM_GESTURE | wxTOUCH_ROTATE_GESTURE );
#endif

    const wxEventType events[] =
    {
        // Binding both EVT_CHAR and EVT_CHAR_HOOK ensures that all key events,
        // especially special key like arrow keys, are handled by the GAL event dispatcher,
        // and not sent to GUI without filtering, because they have a default action (scroll)
        // that must not be called.
        wxEVT_LEFT_UP, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_DCLICK,
        wxEVT_MOTION, wxEVT_MOUSEWHEEL, wxEVT_CHAR, wxEVT_CHAR_HOOK,
        wxEVT_MAGNIFY,
        wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT
    };

    for( wxEventType eventType : events )
        Connect( eventType, wxEventHandler( EDA_3D_CANVAS::OnEvent ), nullptr, m_eventDispatcher );
}


EDA_3D_CANVAS::~EDA_3D_CANVAS()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::~EDA_3D_CANVAS" ) );

    delete m_accelerator3DShapes;
    m_accelerator3DShapes = nullptr;

    releaseOpenGL();
}


void EDA_3D_CANVAS::releaseOpenGL()
{
    if( m_glRC )
    {
        GL_CONTEXT_MANAGER* gl_mgr = Pgm().GetGLContextManager();
        gl_mgr->LockCtx( m_glRC, this );

        delete m_3d_render_raytracing;
        m_3d_render_raytracing = nullptr;

        delete m_3d_render_opengl;
        m_3d_render_opengl = nullptr;

        // This is just a copy of a pointer, can safely be set to NULL.
        m_3d_render = nullptr;

        gl_mgr->UnlockCtx( m_glRC );
        gl_mgr->DestroyCtx( m_glRC );
        m_glRC = nullptr;
    }
}


void EDA_3D_CANVAS::OnCloseWindow( wxCloseEvent& event )
{
    releaseOpenGL();

    event.Skip();
}


void EDA_3D_CANVAS::OnResize( wxSizeEvent& event )
{
    Request_refresh();
}


bool  EDA_3D_CANVAS::initializeOpenGL()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::initializeOpenGL" ) );

    const GLenum err = glewInit();

    if( GLEW_OK != err )
    {
        const wxString msgError = (const char*) glewGetErrorString( err );

        wxLogMessage( msgError );

        return false;
    }
    else
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::initializeOpenGL Using GLEW version %s" ),
                    From_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
    }

    SetOpenGLInfo( (const char*) glGetString( GL_VENDOR ), (const char*) glGetString( GL_RENDERER ),
                   (const char*) glGetString( GL_VERSION ) );

    wxString version = From_UTF8( (char *) glGetString( GL_VERSION ) );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::%s OpenGL version string %s." ),
                __WXFUNCTION__, version );

    // Extract OpenGL version from string.  This method is used because prior to OpenGL 2,
    // getting the OpenGL major and minor version as integers didn't exist.
    wxString tmp;

    wxStringTokenizer tokenizer( version, " \t\r\n" );

    if( tokenizer.HasMoreTokens() )
    {
        long major = 0;
        long minor = 0;

        tmp = tokenizer.GetNextToken();

        tokenizer.SetString( tmp, wxString( wxT( "." ) ) );

        if( tokenizer.HasMoreTokens() )
            tokenizer.GetNextToken().ToLong( &major );

        if( tokenizer.HasMoreTokens() )
            tokenizer.GetNextToken().ToLong( &minor );

        if( major < 2 || ( ( major == 2 ) && ( minor < 1 ) ) )
        {
            wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::%s OpenGL ray tracing not supported." ),
                        __WXFUNCTION__ );

            if( GetParent() )
            {
                wxCommandEvent evt( wxEVT_MENU, ID_DISABLE_RAY_TRACING );
                GetParent()->ProcessWindowEvent( evt );
            }

            m_opengl_supports_raytracing = false;
        }

        if( ( major == 1 ) && ( minor < 5 ) )
        {
            wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::%s OpenGL not supported." ),
                        __WXFUNCTION__ );

            m_is_opengl_version_supported = false;
        }
    }

    GL_UTILS::SetSwapInterval( -1 );
    m_is_opengl_initialized = true;

    return true;
}


void EDA_3D_CANVAS::GetScreenshot( wxImage& aDstImage )
{
    OglGetScreenshot( aDstImage );
}


void EDA_3D_CANVAS::ReloadRequest( BOARD* aBoard , S3D_CACHE* aCachePointer )
{
    if( aCachePointer != nullptr )
        m_boardAdapter.Set3dCacheManager( aCachePointer );

    if( aBoard != nullptr )
        m_boardAdapter.SetBoard( aBoard );

    m_boardAdapter.ReloadColorSettings();

    if( m_3d_render )
        m_3d_render->ReloadRequest();
}


void EDA_3D_CANVAS::RenderRaytracingRequest()
{
    m_3d_render = m_3d_render_raytracing;

    if( m_3d_render )
        m_3d_render->ReloadRequest();

    m_render_raytracing_was_requested = true;

    Request_refresh();
}


void EDA_3D_CANVAS::DisplayStatus()
{
    if( m_parentStatusBar )
    {
        wxString msg;

        msg.Printf( wxT( "dx %3.2f" ), m_camera.GetCameraPos().x );
        m_parentStatusBar->SetStatusText( msg, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::X_POS ) );

        msg.Printf( wxT( "dy %3.2f" ), m_camera.GetCameraPos().y );
        m_parentStatusBar->SetStatusText( msg, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::Y_POS ) );

        msg.Printf( wxT( "zoom %3.2f" ), 1 / m_camera.GetZoom() );
        m_parentStatusBar->SetStatusText( msg,
                                          static_cast<int>( EDA_3D_VIEWER_STATUSBAR::ZOOM_LEVEL ) );
    }
}


void EDA_3D_CANVAS::OnPaint( wxPaintEvent& aEvent )
{
    // Please have a look at: https://lists.launchpad.net/kicad-developers/msg25149.html
    DoRePaint();
}


void EDA_3D_CANVAS::DoRePaint()
{
    if( m_is_currently_painting.test_and_set() )
        return;

    // SwapBuffer requires the window to be shown before calling
    if( !IsShownOnScreen() )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::DoRePaint !IsShown" ) );
        m_is_currently_painting.clear();
        return;
    }

    // Because the board to draw is handled by the parent viewer frame,
    // ensure this parent is still alive. When it is closed before the viewer
    // frame, a paint event can be generated after the parent is closed,
    // therefore with invalid board.
    // This is dependent of the platform.
    // Especially on OSX, but also on Windows, it frequently happens
    if( !GetParent()->GetParent()->IsShownOnScreen() )
        return; // The parent board editor frame is no more alive

    wxString            err_messages;
    INFOBAR_REPORTER    warningReporter( m_parentInfoBar );
    STATUSBAR_REPORTER  activityReporter( m_parentStatusBar, EDA_3D_VIEWER_STATUSBAR::ACTIVITY );
    int64_t             start_time = GetRunningMicroSecs();
    GL_CONTEXT_MANAGER* gl_mgr = Pgm().GetGLContextManager();

    // "Makes the OpenGL state that is represented by the OpenGL rendering
    //  context context current, i.e. it will be used by all subsequent OpenGL calls.
    //  This function may only be called when the window is shown on screen"

    // Explicitly create a new rendering context instance for this canvas.
    if( m_glRC == nullptr )
        m_glRC = gl_mgr->CreateCtx( this );

    // CreateCtx could and does fail per sentry crash events, lets be graceful
    if( m_glRC == nullptr )
    {
        warningReporter.Report( _( "OpenGL context creation error" ), RPT_SEVERITY_ERROR );
        warningReporter.Finalize();
        m_is_currently_painting.clear();
        return;
    }

    gl_mgr->LockCtx( m_glRC, this );

    // Set the OpenGL viewport according to the client size of this canvas.
    // This is done here rather than in a wxSizeEvent handler because our
    // OpenGL rendering context (and thus viewport setting) is used with
    // multiple canvases: If we updated the viewport in the wxSizeEvent
    // handler, changing the size of one canvas causes a viewport setting that
    // is wrong when next another canvas is repainted.
    wxSize clientSize = GetNativePixelSize();

    const bool windows_size_changed = m_camera.SetCurWindowSize( clientSize );

    // Initialize openGL if need
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
        {
            gl_mgr->UnlockCtx( m_glRC );
            m_is_currently_painting.clear();

            return;
        }

        if( !m_is_opengl_version_supported )
        {
            warningReporter.Report( _( "Your OpenGL version is not supported. Minimum required "
                                       "is 1.5." ), RPT_SEVERITY_ERROR );

            warningReporter.Finalize();
        }
    }

    if( !m_is_opengl_version_supported )
    {
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );

        SwapBuffers();

        gl_mgr->UnlockCtx( m_glRC );
        m_is_currently_painting.clear();

        return;
    }

    // Don't attempt to ray trace if OpenGL doesn't support it.
    if( !m_opengl_supports_raytracing )
    {
        m_3d_render = m_3d_render_opengl;
        m_render_raytracing_was_requested = false;
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
    }

    // Check if a raytracing was requested and need to switch to raytracing mode
    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
    {
        const bool was_camera_changed = m_camera.ParametersChanged();

        // It reverts back to OpenGL mode if it was requested a raytracing
        // render of the current scene. AND the mouse / camera is moving
        if( ( m_mouse_is_moving || m_camera_is_moving || was_camera_changed
            || windows_size_changed )
          && m_render_raytracing_was_requested )
        {
            m_render_raytracing_was_requested = false;
            m_3d_render = m_3d_render_opengl;
        }
    }

    float curtime_delta_s = 0.0f;

    if( m_camera_is_moving )
    {
    const int64_t curtime_delta = GetRunningMicroSecs() - m_strtime_camera_movement;
    // Convert microseconds to seconds as float and apply speed multiplier
    curtime_delta_s = static_cast<float>( static_cast<double>( curtime_delta ) / 1e6 )
              * m_camera_moving_speed;
        m_camera.Interpolate( curtime_delta_s );

        if( curtime_delta_s > 1.0f )
        {
            m_render_pivot = false;
            m_camera_is_moving = false;
            m_mouse_was_moved = true;

            restart_editingTimeOut_Timer();
            DisplayStatus();
        }
        else
        {
            Request_refresh();
        }
    }

    // It will return true if the render request a new redraw
    bool requested_redraw = false;

    if( m_3d_render )
    {
        try
        {
            m_3d_render->SetCurWindowSize( clientSize );

            bool reloadRaytracingForCalculations = false;

            if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL
                    && m_3d_render_opengl->IsReloadRequestPending() )
            {
                reloadRaytracingForCalculations = true;
            }

            requested_redraw = m_3d_render->Redraw( m_mouse_was_moved || m_camera_is_moving,
                                                    &activityReporter, &warningReporter );

            // Raytracer renderer is responsible for some features also used by the OpenGL
            // renderer.
            // FIXME: Presumably because raytracing renderer reload is called only after current
            // renderer redraw, the old zoom value stays for a single frame. This is ugly, but only
            // cosmetic, so I'm not fixing that for now: I don't know how to do this without
            // reloading twice (maybe it's not too bad of an idea?) or doing a complicated
            // refactor.
            if( reloadRaytracingForCalculations )
                m_3d_render_raytracing->Reload( nullptr, nullptr, true );
        }
        catch( std::runtime_error& )
        {
            m_is_opengl_version_supported = false;
            m_opengl_supports_raytracing  = false;
            m_is_opengl_initialized       = false;
            gl_mgr->UnlockCtx( m_glRC );
            m_is_currently_painting.clear();
            return;
        }
    }

    if( m_render_pivot )
    {
        const float scale = glm::min( m_camera.GetZoom(), 1.0f );
        render_pivot( curtime_delta_s, scale );
    }

    // This will only be enabled by the 3d mouse plugin, so we can leave
    // it as a simple if statement
    if( m_render3dmousePivot )
    {
        const float scale = glm::min( m_camera.GetZoom(), 1.0f );
        render3dmousePivot( scale );
    }

    // "Swaps the double-buffer of this window, making the back-buffer the
    //  front-buffer and vice versa, so that the output of the previous OpenGL
    //  commands is displayed on the window."
    SwapBuffers();

    gl_mgr->UnlockCtx( m_glRC );

    if( m_mouse_was_moved || m_camera_is_moving )
    {
        // Calculation time in milliseconds
        const double calculation_time = (double)( GetRunningMicroSecs() - start_time ) / 1e3;

        activityReporter.Report( wxString::Format( _( "Last render time %.0f ms" ),
                                 calculation_time ) );
    }

    // This will reset the flag of camera parameters changed
    m_camera.ParametersChanged();

    warningReporter.Finalize();

    if( !err_messages.IsEmpty() )
        wxLogMessage( err_messages );

    if( ( !m_camera_is_moving ) && requested_redraw )
    {
        m_mouse_was_moved = false;
        Request_refresh( false );
    }

    static constexpr std::array<VIEW3D_TYPE, static_cast<int>( SPHERES_GIZMO::GizmoSphereSelection::Count )>
            viewTable = { VIEW3D_TYPE::VIEW3D_RIGHT, VIEW3D_TYPE::VIEW3D_LEFT, VIEW3D_TYPE::VIEW3D_BACK,
                          VIEW3D_TYPE::VIEW3D_FRONT, VIEW3D_TYPE::VIEW3D_TOP,  VIEW3D_TYPE::VIEW3D_BOTTOM };

    SPHERES_GIZMO::GizmoSphereSelection selectedGizmoSphere = m_3d_render_opengl->getSelectedGizmoSphere();
    int                                 index = static_cast<int>( selectedGizmoSphere );
    if( index >= 0 && index < static_cast<int>( viewTable.size() ) )
    {
        SetView3D( viewTable[index] );
    }

    m_3d_render_opengl->resetSelectedGizmoSphere();

    m_is_currently_painting.clear();
}


void EDA_3D_CANVAS::RenderToFrameBuffer( unsigned char* buffer, int width, int height )
{
    if( m_is_currently_painting.test_and_set() )
        return;

    // Validate input parameters
    if( !buffer || width <= 0 || height <= 0 )
    {
        m_is_currently_painting.clear();
        return;
    }

    // Because the board to draw is handled by the parent viewer frame,
    // ensure this parent is still alive
    if( !GetParent() || !GetParent()->GetParent() || !GetParent()->GetParent()->IsShownOnScreen() )
    {
        m_is_currently_painting.clear();
        return;
    }

    wxString            err_messages;
    int64_t             start_time = GetRunningMicroSecs();
    GL_CONTEXT_MANAGER* gl_mgr = Pgm().GetGLContextManager();

    // Create OpenGL context if needed
    if( m_glRC == nullptr )
        m_glRC = gl_mgr->CreateCtx( this );

    if( m_glRC == nullptr )
    {
        wxLogError( _( "OpenGL context creation error" ) );
        m_is_currently_painting.clear();
        return;
    }

    gl_mgr->LockCtx( m_glRC, this );

    // Set up framebuffer objects for off-screen rendering
    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthBuffer = 0;
    GLint  oldFramebuffer = 0;
    GLint  oldViewport[4];

    // Save current state
    glGetIntegerv( GL_FRAMEBUFFER_BINDING, &oldFramebuffer );
    glGetIntegerv( GL_VIEWPORT, oldViewport );

    // Create and bind framebuffer
    glGenFramebuffers( 1, &framebuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer );

    // Create color texture attachment
    glGenTextures( 1, &colorTexture );
    glBindTexture( GL_TEXTURE_2D, colorTexture );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0 );

    // Create depth renderbuffer attachment
    glGenRenderbuffers( 1, &depthBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, depthBuffer );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );

    auto resetState = std::unique_ptr<void, std::function<void(void*)>>(
        reinterpret_cast<void*>(1),
        [&](void*) {
            glBindFramebuffer( GL_FRAMEBUFFER, oldFramebuffer );
            glViewport( oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3] );
            glDeleteFramebuffers( 1, &framebuffer );
            glDeleteTextures( 1, &colorTexture );
            glDeleteRenderbuffers( 1, &depthBuffer );
            gl_mgr->UnlockCtx( m_glRC );
            m_is_currently_painting.clear();
        }
    );

    // Check framebuffer completeness
    GLenum framebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER );

    if( framebufferStatus != GL_FRAMEBUFFER_COMPLETE )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::RenderToFrameBuffer Framebuffer incomplete: 0x%04X" ),
                    framebufferStatus );

        return;
    }

    // Set viewport for off-screen rendering
    glViewport( 0, 0, width, height );

    // Set window size for camera and rendering
    wxSize     clientSize( width, height );
    const bool windows_size_changed = m_camera.SetCurWindowSize( clientSize );

    // Initialize OpenGL if needed
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
        {
            wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::RenderToFrameBuffer OpenGL initialization failed." ) );
            return;
        }

        if( !m_is_opengl_version_supported )
        {
            wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::RenderToFrameBuffer OpenGL version not supported." ) );
        }
    }

    if( !m_is_opengl_version_supported )
    {
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );

        // Read black screen to buffer
        glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
        return;
    }

    // Handle raytracing/OpenGL renderer selection
    if( !m_opengl_supports_raytracing )
    {
        m_3d_render = m_3d_render_opengl;
        m_render_raytracing_was_requested = false;
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
    }

    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
    {
        const bool was_camera_changed = m_camera.ParametersChanged();

        if( ( m_mouse_is_moving || m_camera_is_moving || was_camera_changed || windows_size_changed )
            && m_render_raytracing_was_requested )
        {
            m_render_raytracing_was_requested = false;
            m_3d_render = m_3d_render_opengl;
        }
    }

    // Handle camera animation (simplified for off-screen rendering)
    float curtime_delta_s = 0.0f;
    if( m_camera_is_moving )
    {
        const int64_t curtime_delta = GetRunningMicroSecs() - m_strtime_camera_movement;
        curtime_delta_s = static_cast<float>( static_cast<double>( curtime_delta ) / 1e6 )
                          * m_camera_moving_speed;
        m_camera.Interpolate( curtime_delta_s );

        if( curtime_delta_s > 1.0f )
        {
            m_render_pivot = false;
            m_camera_is_moving = false;
            m_mouse_was_moved = true;
        }
    }

    // Perform the actual rendering
    bool requested_redraw = false;
    if( m_3d_render )
    {
        try
        {
            m_3d_render->SetCurWindowSize( clientSize );

            bool reloadRaytracingForCalculations = false;
            if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL
                && m_3d_render_opengl->IsReloadRequestPending() )
            {
                reloadRaytracingForCalculations = true;
            }

            requested_redraw = m_3d_render->Redraw( false, nullptr, nullptr );

            if( reloadRaytracingForCalculations )
                m_3d_render_raytracing->Reload( nullptr, nullptr, true );
        }
        catch( std::runtime_error& )
        {
            m_is_opengl_version_supported = false;
            m_opengl_supports_raytracing = false;
            m_is_opengl_initialized = false;
            return;
        }
    }

    // Read pixels from framebuffer to the provided buffer
    // Note: This reads RGB format. Adjust format as needed.
    glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

    // Check for OpenGL errors
    GLenum error = glGetError();
    if( error != GL_NO_ERROR )
    {
        wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::RenderToFrameBuffer OpenGL error: 0x%04X" ), error );
        err_messages += wxString::Format( _( "OpenGL error during off-screen rendering: 0x%04X\n" ), error );
    }

    // Reset camera parameters changed flag
    m_camera.ParametersChanged();

    if( !err_messages.IsEmpty() )
        wxLogMessage( err_messages );
}


void EDA_3D_CANVAS::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;
}


void EDA_3D_CANVAS::OnEvent( wxEvent& aEvent )
{
    if( !m_eventDispatcher )
        aEvent.Skip();
    else
        m_eventDispatcher->DispatchWxEvent( aEvent );

    Refresh();
}


void EDA_3D_CANVAS::OnEraseBackground( wxEraseEvent& event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::OnEraseBackground" ) );
    // Do nothing, to avoid flashing.
}


void EDA_3D_CANVAS::OnMouseWheel( wxMouseEvent& event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::OnMouseWheel" ) );

    OnMouseWheelCamera( event, m_boardAdapter.m_MousewheelPanning );

    if( m_mouse_was_moved )
    {
        DisplayStatus();
        Request_refresh();
        restart_editingTimeOut_Timer();
    }
}


void EDA_3D_CANVAS::OnMagnify( wxMouseEvent& event )
{
    SetFocus();

    if( m_camera_is_moving )
        return;

    //m_is_moving_mouse = true;
    restart_editingTimeOut_Timer();

    float magnification = ( event.GetMagnification() + 1.0f );

    m_camera.Zoom( magnification );

    DisplayStatus();
    Request_refresh();
}


void EDA_3D_CANVAS::OnZoomGesture( wxZoomGestureEvent& aEvent )
{
    SetFocus();

    if( aEvent.IsGestureStart() )
    {
        m_gestureLastZoomFactor = 1.0;
        m_camera.SetCurMousePosition( aEvent.GetPosition() );
    }

    if( m_camera_is_moving )
        return;

    restart_editingTimeOut_Timer();

    m_camera.Pan( aEvent.GetPosition() );
    m_camera.SetCurMousePosition( aEvent.GetPosition() );

    m_camera.Zoom( static_cast<float>( aEvent.GetZoomFactor() / m_gestureLastZoomFactor ) );

    m_gestureLastZoomFactor = aEvent.GetZoomFactor();

    DisplayStatus();
    Request_refresh();
}


void EDA_3D_CANVAS::OnPanGesture( wxPanGestureEvent& aEvent )
{
    SetFocus();

    if( aEvent.IsGestureStart() )
        m_camera.SetCurMousePosition( aEvent.GetPosition() );

    if( m_camera_is_moving )
        return;

    m_camera.Pan( aEvent.GetPosition() );
    m_camera.SetCurMousePosition( aEvent.GetPosition() );

    DisplayStatus();
    Request_refresh();
}


void EDA_3D_CANVAS::OnRotateGesture( wxRotateGestureEvent& aEvent )
{
    SetFocus();

    if( aEvent.IsGestureStart() )
    {
        m_gestureLastAngle = 0;
        m_camera.SetCurMousePosition( aEvent.GetPosition() );

        // We don't want to process the first angle
        return;
    }

    if( m_camera_is_moving )
        return;

    m_camera.RotateScreen( static_cast<float>( m_gestureLastAngle - aEvent.GetRotationAngle() ) );
    m_gestureLastAngle = aEvent.GetRotationAngle();

    DisplayStatus();
    Request_refresh();
}


void EDA_3D_CANVAS::OnMouseMove( wxMouseEvent& event )
{
    if( m_3d_render && m_3d_render->IsReloadRequestPending() )
        return; // Prevents using invalid m_3d_render_raytracing data

    if( m_camera_is_moving )
        return;

    OnMouseMoveCamera( event );

    if( m_mouse_was_moved )
    {
        DisplayStatus();
        Request_refresh();
        // *Do not* reactivate the timer here during the mouse move command:
        // OnMiddleUp() will do it at the end of mouse drag/move command
    }

    if( !event.Dragging() && m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
    {
        STATUSBAR_REPORTER reporter( m_parentStatusBar, EDA_3D_VIEWER_STATUSBAR::HOVERED_ITEM );
        RAY                mouseRay = getRayAtCurrentMousePosition();
        BOARD_ITEM*        rollOverItem = m_3d_render_raytracing->IntersectBoardItem( mouseRay );

        auto printNetInfo =
                []( BOARD_CONNECTED_ITEM* aItem )
                {
                    return wxString::Format( _( "Net %s\tNet class %s" ), aItem->GetNet()->GetNetname(),
                                             aItem->GetNet()->GetNetClass()->GetHumanReadableName() );
                };

        if( rollOverItem )
        {
            wxString msg;

            if( rollOverItem != m_currentRollOverItem )
            {
                m_3d_render_opengl->SetCurrentRollOverItem( rollOverItem );
                m_currentRollOverItem = rollOverItem;

                Request_refresh();
            }

            switch( rollOverItem->Type() )
            {
            case PCB_PAD_T:
            {
                PAD* pad = static_cast<PAD*>( rollOverItem );

                if( !pad->GetNumber().IsEmpty() )
                    msg += wxString::Format( _( "Pad %s\t" ), pad->GetNumber() );

                if( pad->IsOnCopperLayer() )
                    msg += printNetInfo( pad );

                break;
            }

            case PCB_FOOTPRINT_T:
            {
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( rollOverItem );
                msg += footprint->GetReference() + wxT( "  " ) + footprint->GetValue();
                break;
            }

            case PCB_TRACE_T:
            case PCB_VIA_T:
            case PCB_ARC_T:
            {
                PCB_TRACK* track = static_cast<PCB_TRACK*>( rollOverItem );
                msg += printNetInfo( track );
                break;
            }

            case PCB_ZONE_T:
            {
                ZONE* zone = static_cast<ZONE*>( rollOverItem );

                if( !zone->GetZoneName().IsEmpty() )
                {
                    if( zone->GetIsRuleArea() )
                        msg += wxString::Format( _( "Rule area %s\t" ), zone->GetZoneName() );
                    else
                        msg += wxString::Format( _( "Zone %s\t" ), zone->GetZoneName() );
                }

                if( zone->IsOnCopperLayer() )
                    msg += printNetInfo( zone );

                break;
            }

            default:
                break;
            }

            reporter.Report( msg );
        }
        else
        {
            if( m_currentRollOverItem
                    && m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
            {
                m_3d_render_opengl->SetCurrentRollOverItem( nullptr );
                Request_refresh();

                reporter.Report( wxEmptyString );
            }

            m_currentRollOverItem = nullptr;
        }
    }
}


void EDA_3D_CANVAS::OnLeftDown( wxMouseEvent& event )
{
    SetFocus();
    stop_editingTimeOut_Timer();

    // Ensure m_camera.m_lastPosition (current mouse position) is up to date for
    // future drag events (can be not the case when left clicking after
    // opening a context menu)
    OnMouseMoveCamera( event );

    if( !event.Dragging() && ( m_3d_render_raytracing != nullptr ) )
    {
        RAY mouseRay = getRayAtCurrentMousePosition();

        BOARD_ITEM* intersectedBoardItem = m_3d_render_raytracing->IntersectBoardItem( mouseRay );

        if( intersectedBoardItem )
        {
            FOOTPRINT* footprint = nullptr;

            switch( intersectedBoardItem->Type() )
            {
            case PCB_FOOTPRINT_T:
                footprint = static_cast<FOOTPRINT*>( intersectedBoardItem );
                break;

            case PCB_PAD_T:
                footprint = static_cast<PAD*>( intersectedBoardItem )->GetParentFootprint();
                break;

            case PCB_FIELD_T:
                footprint = static_cast<PCB_FIELD*>( intersectedBoardItem )->GetParentFootprint();
                break;

            default:
                break;
            }

            if( footprint )
            {
                // We send a message (by ExpressMail) to the board and schematic editor, but only
                // if the manager of this canvas is a EDA_3D_VIEWER_FRAME, because only this
                // kind of frame has ExpressMail stuff
                EDA_3D_VIEWER_FRAME* frame = dynamic_cast<EDA_3D_VIEWER_FRAME*>( GetParent() );

                if( frame )
                {
                    std::string command = fmt::format( "$SELECT: 0,F{}",
                                        EscapeString( footprint->GetReference(), CTX_IPC ).ToStdString() );

                    frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_SELECTION, command, frame );
                    frame->Kiway().ExpressMail( FRAME_SCH, MAIL_SELECTION, command, frame );
                }
            }
        }
    }
}


void EDA_3D_CANVAS::OnLeftUp( wxMouseEvent& event )
{
    if( m_camera_is_moving )
        return;

    if( m_mouse_is_moving )
    {
        m_mouse_is_moving = false;
        restart_editingTimeOut_Timer();
    }

    wxSize logicalSize = GetClientSize();
    int    logicalW = logicalSize.GetWidth();
    int    logicalH = logicalSize.GetHeight();

    int gizmo_x = 0, gizmo_y = 0, gizmo_width = 0, gizmo_height = 0;
    std::tie( gizmo_x, gizmo_y, gizmo_width, gizmo_height ) = m_3d_render_opengl->getGizmoViewport();

    float scaleX = static_cast<float>( static_cast<double>( gizmo_width ) / static_cast<double>( logicalW ) );
    float scaleY = static_cast<float>( static_cast<double>( gizmo_height ) / static_cast<double>( logicalH ) );

    int scaledMouseX = static_cast<int>( static_cast<float>( event.GetX() ) * scaleX );
    int scaledMouseY = static_cast<int>( static_cast<float>( logicalH - event.GetY() ) * scaleY );

    m_3d_render_opengl->handleGizmoMouseInput( scaledMouseX, scaledMouseY );
    Refresh();
}


void EDA_3D_CANVAS::OnRightDown( wxMouseEvent& event )
{
    SetFocus();
    stop_editingTimeOut_Timer();

    // Ensure m_camera.m_lastPosition is up to date for future drag events.
    OnMouseMoveCamera( event );
}


void EDA_3D_CANVAS::OnRightUp( wxMouseEvent& event )
{
    if( m_camera_is_moving )
        return;

    if( m_mouse_is_moving )
    {
        m_mouse_is_moving = false;
        restart_editingTimeOut_Timer();
    }
}


void EDA_3D_CANVAS::OnMiddleDown( wxMouseEvent& event )
{
    SetFocus();
    stop_editingTimeOut_Timer();
}


void EDA_3D_CANVAS::OnMiddleUp( wxMouseEvent& event )
{
    if( m_camera_is_moving )
        return;

    if( m_mouse_is_moving )
    {
        m_mouse_is_moving = false;
        restart_editingTimeOut_Timer();
    }
    else
    {
        move_pivot_based_on_cur_mouse_position();
    }
}


void EDA_3D_CANVAS::OnTimerTimeout_Editing( wxTimerEvent& aEvent )
{
    if( aEvent.GetId() != m_editing_timeout_timer.GetId() )
    {
        aEvent.Skip();
        return;
    }

    m_mouse_is_moving = false;
    m_mouse_was_moved = false;

    Request_refresh();
}


void EDA_3D_CANVAS::stop_editingTimeOut_Timer()
{
    m_editing_timeout_timer.Stop();
}


void EDA_3D_CANVAS::restart_editingTimeOut_Timer()
{
    if( m_3d_render )
        m_editing_timeout_timer.Start( m_3d_render->GetWaitForEditingTimeOut(), wxTIMER_ONE_SHOT );
}


void EDA_3D_CANVAS::OnTimerTimeout_Redraw( wxTimerEvent& aEvent )
{
    if( aEvent.GetId() != m_redraw_trigger_timer.GetId() )
    {
        aEvent.Skip();
        return;
    }

    Request_refresh( true );
}


void EDA_3D_CANVAS::OnRefreshRequest( wxEvent& aEvent )
{
    Refresh();
}


void EDA_3D_CANVAS::Request_refresh( bool aRedrawImmediately )
{
    if( aRedrawImmediately )
    {
        // Just calling Refresh() does not work always
        // Using an event to call DoRepaint ensure the repaint code will be executed,
        // and PostEvent will take priority to other events like mouse movements, keys, etc.
        // and is executed during the next idle time
        wxCommandEvent redrawEvent( wxEVT_REFRESH_CUSTOM_COMMAND, ID_CUSTOM_EVENT_1 );
        wxPostEvent( this, redrawEvent );
    }
    else
    {
        // Schedule a timed redraw
        m_redraw_trigger_timer.Start( 10 , wxTIMER_ONE_SHOT );
    }
}


void EDA_3D_CANVAS::request_start_moving_camera( float aMovingSpeed, bool aRenderPivot )
{
    wxASSERT( aMovingSpeed > FLT_EPSILON );

    // Fast forward the animation if the animation is disabled
    if( !m_animation_enabled )
    {
        m_camera.Interpolate( 1.0f );
        DisplayStatus();
        Request_refresh();
        return;
    }

    // Map speed multiplier option to actual multiplier value
    // [1,2,3,4,5] -> [0.25, 0.5, 1, 2, 4]
    aMovingSpeed *= static_cast<float>( ( 1 << m_moving_speed_multiplier ) ) / 8.0f;

    m_render_pivot = aRenderPivot;
    m_camera_moving_speed = aMovingSpeed;

    stop_editingTimeOut_Timer();

    DisplayStatus();
    Request_refresh();

    m_camera_is_moving = true;

    m_strtime_camera_movement = GetRunningMicroSecs();
}


void EDA_3D_CANVAS::move_pivot_based_on_cur_mouse_position()
{
    RAY mouseRay = getRayAtCurrentMousePosition();

    float hit_t = 0.0f;

    // Test it with the board bounding box
    if( m_boardAdapter.GetBBox().Intersect( mouseRay, &hit_t ) )
    {
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.SetLookAtPos_T1( mouseRay.at( hit_t ) );
        m_camera.ResetXYpos_T1();

        request_start_moving_camera();
    }
}


bool EDA_3D_CANVAS::SetView3D( VIEW3D_TYPE aRequestedView )
{
    if( m_camera_is_moving )
        return false;

    const float delta_move = m_delta_move_step_factor * m_camera.GetZoom();
    const float arrow_moving_time_speed = 8.0f;

    switch( aRequestedView )
    {
    case VIEW3D_TYPE::VIEW3D_PIVOT_CENTER:
        move_pivot_based_on_cur_mouse_position();
        return true;

    case VIEW3D_TYPE::VIEW3D_PAN_LEFT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( -delta_move, 0.0f, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case VIEW3D_TYPE::VIEW3D_PAN_RIGHT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( +delta_move, 0.0f, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case VIEW3D_TYPE::VIEW3D_PAN_UP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( 0.0f, +delta_move, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case VIEW3D_TYPE::VIEW3D_PAN_DOWN:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( 0.0f, -delta_move, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case VIEW3D_TYPE::VIEW3D_FIT_SCREEN:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        request_start_moving_camera( glm::min( glm::max( m_camera.GetZoom(), 1 / 1.26f ), 1.26f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_ZOOM_IN:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();

        if( m_camera.Zoom_T1( 1.26f ) )   // 3 steps per doubling
            request_start_moving_camera( 3.0f );

        return true;

    case VIEW3D_TYPE::VIEW3D_ZOOM_OUT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();

        if( m_camera.Zoom_T1( 1/1.26f ) ) // 3 steps per halving
            request_start_moving_camera( 3.0f );

        return true;

    case VIEW3D_TYPE::VIEW3D_RIGHT:
    case VIEW3D_TYPE::VIEW3D_LEFT:
    case VIEW3D_TYPE::VIEW3D_FRONT:
    case VIEW3D_TYPE::VIEW3D_BACK:
    case VIEW3D_TYPE::VIEW3D_FLIP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.ViewCommand_T1( aRequestedView );
        request_start_moving_camera();
        return true;

    case VIEW3D_TYPE::VIEW3D_TOP:
    case VIEW3D_TYPE::VIEW3D_BOTTOM:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.ViewCommand_T1( aRequestedView );
        request_start_moving_camera( glm::min( glm::max( m_camera.GetZoom(), 0.5f ), 1.125f ) );
        return true;

    default:
        return false;
    }
}


void EDA_3D_CANVAS::RenderEngineChanged()
{
    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        switch( cfg->m_Render.engine )
        {
        case RENDER_ENGINE::OPENGL:     m_3d_render = m_3d_render_opengl;     break;
        case RENDER_ENGINE::RAYTRACING: m_3d_render = m_3d_render_raytracing; break;
        default:                        m_3d_render = nullptr;                break;
        }
    }

    if( m_3d_render )
        m_3d_render->ReloadRequest();

    m_mouse_was_moved = false;

    Request_refresh();
}


RAY EDA_3D_CANVAS::getRayAtCurrentMousePosition()
{
    SFVEC3F rayOrigin;
    SFVEC3F rayDir;

    // Generate a ray origin and direction based on current mouser position and camera
    m_camera.MakeRayAtCurrentMousePosition( rayOrigin, rayDir );

    RAY mouseRay;
    mouseRay.Init( rayOrigin, rayDir );

    return mouseRay;
}
