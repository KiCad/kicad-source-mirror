/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <3d_rendering/raytracing/render_3d_raytrace.h>
#include <3d_rendering/opengl/render_3d_opengl.h>
#include <3d_viewer_id.h>
#include <advanced_config.h>
#include <build_version.h>
#include <board.h>
#include <reporter.h>
#include <gal/opengl/gl_context_mgr.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <bitmaps.h>
#include <macros.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/tool_dispatcher.h>

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
    EVT_MOUSEWHEEL( EDA_3D_CANVAS::OnMouseWheel )
    EVT_MOTION( EDA_3D_CANVAS::OnMouseMove )
    EVT_MAGNIFY( EDA_3D_CANVAS::OnMagnify )

    // other events
    EVT_ERASE_BACKGROUND( EDA_3D_CANVAS::OnEraseBackground )
    EVT_CUSTOM(wxEVT_REFRESH_CUSTOM_COMMAND, ID_CUSTOM_EVENT_1, EDA_3D_CANVAS::OnRefreshRequest )

    EVT_CLOSE( EDA_3D_CANVAS::OnCloseWindow )
    EVT_SIZE(  EDA_3D_CANVAS::OnResize )
END_EVENT_TABLE()


EDA_3D_CANVAS::EDA_3D_CANVAS( wxWindow* aParent, const int* aAttribList,
                              BOARD_ADAPTER& aBoardAdapter, CAMERA& aCamera,
                              S3D_CACHE* a3DCachePointer ) :
        HIDPI_GL_3D_CANVAS( EDA_DRAW_PANEL_GAL::GetVcSettings(), aCamera, aParent, wxID_ANY,
                            aAttribList, wxDefaultPosition,
                            wxDefaultSize, wxFULL_REPAINT_ON_RESIZE ),
        m_eventDispatcher( nullptr ),
        m_parentStatusBar( nullptr ),
        m_parentInfoBar( nullptr ),
        m_glRC( nullptr ),
        m_is_opengl_initialized( false ),
        m_is_opengl_version_supported( true ),
        m_editing_timeout_timer( this, wxID_HIGHEST + 1 ),
        m_redraw_trigger_timer( this, wxID_HIGHEST + 2 ),
        m_render_pivot( false ),
        m_camera_moving_speed( 1.0f ),
        m_strtime_camera_movement( 0 ),
        m_animation_enabled( true ),
        m_moving_speed_multiplier( 3 ),
        m_boardAdapter( aBoardAdapter ),
        m_3d_render( nullptr ),
        m_opengl_supports_raytracing( true ),
        m_render_raytracing_was_requested( false ),
        m_accelerator3DShapes( nullptr ),
        m_currentRollOverItem( nullptr )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::EDA_3D_CANVAS" ) );

    m_editing_timeout_timer.SetOwner( this );
    Connect( m_editing_timeout_timer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Editing ), nullptr, this );

    m_redraw_trigger_timer.SetOwner( this );
    Connect( m_redraw_trigger_timer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Redraw ), nullptr, this );

    m_is_currently_painting.clear();

    m_3d_render_raytracing = new RENDER_3D_RAYTRACE( this, m_boardAdapter, m_camera );
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
        GL_CONTEXT_MANAGER::Get().LockCtx( m_glRC, this );

        delete m_3d_render_raytracing;
        m_3d_render_raytracing = nullptr;

        delete m_3d_render_opengl;
        m_3d_render_opengl = nullptr;

        // This is just a copy of a pointer, can safely be set to NULL.
        m_3d_render = nullptr;

        GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );
        GL_CONTEXT_MANAGER::Get().DestroyCtx( m_glRC );
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

    wxStringTokenizer tokenizer( version );

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
        m_parentStatusBar->SetStatusText( msg, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::ZOOM_LEVEL ) );
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

    wxString           err_messages;
    INFOBAR_REPORTER   warningReporter( m_parentInfoBar );
    STATUSBAR_REPORTER activityReporter( m_parentStatusBar, EDA_3D_VIEWER_STATUSBAR::ACTIVITY );
    int64_t            start_time = GetRunningMicroSecs();

    // "Makes the OpenGL state that is represented by the OpenGL rendering
    //  context context current, i.e. it will be used by all subsequent OpenGL calls.
    //  This function may only be called when the window is shown on screen"

    // Explicitly create a new rendering context instance for this canvas.
    if( m_glRC == nullptr )
        m_glRC = GL_CONTEXT_MANAGER::Get().CreateCtx( this );

    // CreateCtx could and does fail per sentry crash events, lets be graceful
    if( m_glRC == nullptr )
    {
        warningReporter.Report( _( "OpenGL context creation error" ), RPT_SEVERITY_ERROR );
        warningReporter.Finalize();
        m_is_currently_painting.clear();
        return;
    }

    GL_CONTEXT_MANAGER::Get().LockCtx( m_glRC, this );

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
            GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );
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

        GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );
        m_is_currently_painting.clear();

        return;
    }

    // Don't attend to ray trace if OpenGL doesn't support it.
    if( !m_opengl_supports_raytracing )
    {
        m_3d_render = m_3d_render_opengl;
        m_render_raytracing_was_requested = false;
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
    }

    // Check if a raytacing was requested and need to switch to raytracing mode
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
        curtime_delta_s = (curtime_delta / 1e6) * m_camera_moving_speed;
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
            GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );
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

    GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );

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

    m_is_currently_painting.clear();
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
                    return wxString::Format( _( "Net %s\tNet class %s" ),
                                             aItem->GetNet()->GetNetname(),
                                             aItem->GetNet()->GetNetClass()->GetName() );
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
                msg += footprint->GetReference();
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

        BOARD_ITEM *intersectedBoardItem = m_3d_render_raytracing->IntersectBoardItem( mouseRay );

        // !TODO: send a selection item to pcbnew, eg: via kiway?
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
    aMovingSpeed *= ( 1 << m_moving_speed_multiplier ) / 8.0f;

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

    float hit_t;

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
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateZ_T1( glm::radians( -90.0f ) );
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case VIEW3D_TYPE::VIEW3D_LEFT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateZ_T1( glm::radians(  90.0f ) );
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case VIEW3D_TYPE::VIEW3D_FRONT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case VIEW3D_TYPE::VIEW3D_BACK:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateX_T1( glm::radians( -90.0f ) );

        // The rotation angle should be 180.
        // We use 179.999 (180 - epsilon) to avoid a full 360 deg rotation when
        // using 180 deg if the previous rotated position was already 180 deg
        m_camera.RotateZ_T1( glm::radians( 179.999f ) );
        request_start_moving_camera();
        return true;

    case VIEW3D_TYPE::VIEW3D_TOP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        request_start_moving_camera( glm::min( glm::max( m_camera.GetZoom(), 0.5f ), 1.125f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_BOTTOM:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateY_T1( glm::radians( 179.999f ) );    // Rotation = 180 - epsilon
        request_start_moving_camera( glm::min( glm::max( m_camera.GetZoom(), 0.5f ), 1.125f ) );
        return true;

    case VIEW3D_TYPE::VIEW3D_FLIP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.RotateY_T1( glm::radians( 179.999f ) );
        request_start_moving_camera();
        return true;

    default:
        return false;
    }
}


void EDA_3D_CANVAS::RenderEngineChanged()
{
    SETTINGS_MANAGER&       mgr = Pgm().GetSettingsManager();
    EDA_3D_VIEWER_SETTINGS* cfg = mgr.GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    switch( cfg->m_Render.engine )
    {
    case RENDER_ENGINE::OPENGL:     m_3d_render = m_3d_render_opengl;     break;
    case RENDER_ENGINE::RAYTRACING: m_3d_render = m_3d_render_raytracing; break;
    default:                        m_3d_render = nullptr;                break;
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
