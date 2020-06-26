/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  eda_3d_canvas.cpp
 * @brief Implementation of a 3d canvas
 */

#include <GL/glew.h>    // Must be included first
#include <gl_utils.h>
#include <wx/tokenzr.h>

#include "../common_ogl/ogl_utils.h"
#include "eda_3d_canvas.h"
#include <eda_3d_viewer.h>
#include <3d_rendering/3d_render_raytracing/c3d_render_raytracing.h>
#include <3d_rendering/3d_render_ogl_legacy/c3d_render_ogl_legacy.h>
#include <3d_viewer_id.h>
#include <class_board.h>
#include <reporter.h>
#include <gl_context_mgr.h>
#include <profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <bitmaps.h>
#include <menus_helpers.h>
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
const wxChar * EDA_3D_CANVAS::m_logTrace = wxT( "KI_TRACE_EDA_3D_CANVAS" );

const float EDA_3D_CANVAS::m_delta_move_step_factor = 0.7f;


BEGIN_EVENT_TABLE( EDA_3D_CANVAS, wxGLCanvas )
    EVT_PAINT( EDA_3D_CANVAS::OnPaint )

    // mouse events
    EVT_LEFT_DOWN( EDA_3D_CANVAS::OnLeftDown )
    EVT_LEFT_UP( EDA_3D_CANVAS::OnLeftUp )
    EVT_MIDDLE_UP( EDA_3D_CANVAS::OnMiddleUp )
    EVT_MIDDLE_DOWN( EDA_3D_CANVAS::OnMiddleDown)
    EVT_MOUSEWHEEL( EDA_3D_CANVAS::OnMouseWheel )
    EVT_MOTION( EDA_3D_CANVAS::OnMouseMove )

#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    EVT_MAGNIFY( EDA_3D_CANVAS::OnMagnify )
#endif

    // other events
    EVT_ERASE_BACKGROUND( EDA_3D_CANVAS::OnEraseBackground )

    EVT_CLOSE( EDA_3D_CANVAS::OnCloseWindow )
    EVT_SIZE(  EDA_3D_CANVAS::OnResize )
END_EVENT_TABLE()


EDA_3D_CANVAS::EDA_3D_CANVAS( wxWindow* aParent, const int* aAttribList, BOARD* aBoard,
                              BOARD_ADAPTER& aBoardAdapter, CCAMERA& aCamera,
                              S3D_CACHE* a3DCachePointer )
        : HIDPI_GL_CANVAS( aParent, wxID_ANY, aAttribList, wxDefaultPosition, wxDefaultSize,
                           wxFULL_REPAINT_ON_RESIZE ),
          m_eventDispatcher( nullptr ),
          m_parentStatusBar( nullptr ),
          m_parentInfoBar( nullptr ),
          m_glRC( nullptr ),
          m_is_opengl_initialized( false ),
          m_mouse_is_moving( false ),
          m_mouse_was_moved( false ),
          m_camera_is_moving( false ),
          m_render_pivot( false ),
          m_camera_moving_speed( 1.0f ),
          m_strtime_camera_movement( 0 ),
          m_animation_enabled( true ),
          m_moving_speed_multiplier( 3 ),
          m_boardAdapter( aBoardAdapter ),
          m_camera( aCamera ),
          m_3d_render( nullptr ),
          m_opengl_supports_raytracing( false ),
          m_render_raytracing_was_requested( false )
{
    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::EDA_3D_CANVAS" );

    m_editing_timeout_timer.SetOwner( this );
    Connect( m_editing_timeout_timer.GetId(),
             wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Editing ),
             NULL,
             this );

    m_redraw_trigger_timer.SetOwner( this );
    Connect( m_redraw_trigger_timer.GetId(),
             wxEVT_TIMER,
             wxTimerEventHandler( EDA_3D_CANVAS::OnTimerTimeout_Redraw ),
             NULL,
             this );

    m_3d_render_raytracing = new C3D_RENDER_RAYTRACING( m_boardAdapter, m_camera );
    m_3d_render_ogl_legacy = new C3D_RENDER_OGL_LEGACY( m_boardAdapter, m_camera );

    wxASSERT( m_3d_render_raytracing != NULL );
    wxASSERT( m_3d_render_ogl_legacy != NULL );

    auto busy_indicator_factory = []() { return std::make_unique<WX_BUSY_INDICATOR>(); };

    m_3d_render_raytracing->SetBusyIndicatorFactory( busy_indicator_factory );
    m_3d_render_ogl_legacy->SetBusyIndicatorFactory( busy_indicator_factory );

    RenderEngineChanged();

    wxASSERT( aBoard != NULL );
    m_boardAdapter.SetBoard( aBoard );

    m_boardAdapter.SetColorSettings( Pgm().GetSettingsManager().GetColorSettings() );

    wxASSERT( a3DCachePointer != NULL );
    m_boardAdapter.Set3DCacheManager( a3DCachePointer );

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
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
        wxEVT_MAGNIFY,
#endif
        wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT
    };

    for( wxEventType eventType : events )
        Connect( eventType, wxEventHandler( EDA_3D_CANVAS::OnEvent ), NULL, m_eventDispatcher );
}


EDA_3D_CANVAS::~EDA_3D_CANVAS()
{
    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::~EDA_3D_CANVAS" );

    releaseOpenGL();
}


void EDA_3D_CANVAS::releaseOpenGL()
{
    if( m_glRC )
    {
        GL_CONTEXT_MANAGER::Get().LockCtx( m_glRC, this );

        delete m_3d_render_raytracing;
        m_3d_render_raytracing = NULL;

        delete m_3d_render_ogl_legacy;
        m_3d_render_ogl_legacy = NULL;

        // This is just a copy of a pointer, can safely be set to NULL
        m_3d_render = NULL;

        GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );
        GL_CONTEXT_MANAGER::Get().DestroyCtx( m_glRC );
        m_glRC = NULL;
    }
}


void EDA_3D_CANVAS::OnCloseWindow( wxCloseEvent &event )
{
    releaseOpenGL();

    event.Skip();
}


void EDA_3D_CANVAS::OnResize( wxSizeEvent &event )
{
    this->Request_refresh();
}


bool  EDA_3D_CANVAS::initializeOpenGL()
{
    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::initializeOpenGL" );

    const GLenum err = glewInit();

    if( GLEW_OK != err )
    {
        const wxString msgError = (const char*) glewGetErrorString( err );

        wxLogMessage( msgError );

        return false;
    }
    else
    {
        wxLogTrace( m_logTrace, "EDA_3D_CANVAS::initializeOpenGL Using GLEW version %s",
                    FROM_UTF8( (char*) glewGetString( GLEW_VERSION ) ) );
    }

    wxString version = FROM_UTF8( (char *) glGetString( GL_VERSION ) );

    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::%s OpenGL version string %s.",
                __WXFUNCTION__, version );

    // Extract OpenGL version from string.  This method is used because prior to OpenGL 2,
    // getting the OpenGL major and minor version as integers didn't exist.
    wxString tmp;

    wxStringTokenizer tokenizer( version );

    m_opengl_supports_raytracing = true;

    if( tokenizer.HasMoreTokens() )
    {
        long major = 0;
        long minor = 0;

        tmp = tokenizer.GetNextToken();

        tokenizer.SetString( tmp, wxString( "." ) );

        if( tokenizer.HasMoreTokens() )
            tokenizer.GetNextToken().ToLong( &major );

        if( tokenizer.HasMoreTokens() )
            tokenizer.GetNextToken().ToLong( &minor );

        if( major < 2 || ( (major == 2 ) && (minor < 1) ) )
        {
            wxLogTrace( m_logTrace, "EDA_3D_CANVAS::%s OpenGL ray tracing not supported.",
                        __WXFUNCTION__ );

            if( GetParent() )
            {
                wxCommandEvent evt( wxEVT_MENU, ID_DISABLE_RAY_TRACING );
                GetParent()->ProcessWindowEvent( evt );
            }

            m_opengl_supports_raytracing = false;
        }
    }

    m_is_opengl_initialized = true;

    return true;
}


void EDA_3D_CANVAS::GetScreenshot( wxImage &aDstImage )
{
    OGL_GetScreenshot( aDstImage );
}


void EDA_3D_CANVAS::ReloadRequest( BOARD *aBoard , S3D_CACHE *aCachePointer )
{
    if( aCachePointer != NULL )
        m_boardAdapter.Set3DCacheManager( aCachePointer );

    if( aBoard != NULL )
        m_boardAdapter.SetBoard( aBoard );

    m_boardAdapter.SetColorSettings( Pgm().GetSettingsManager().GetColorSettings() );

    if( m_3d_render )
        m_3d_render->ReloadRequest();
}


void EDA_3D_CANVAS::RenderRaytracingRequest()
{
    m_3d_render = m_3d_render_raytracing;

    if( m_3d_render )
        m_3d_render->ReloadRequest();

    m_render_raytracing_was_requested = true;
    //m_mouse_was_moved = true;

    Request_refresh();
}


void EDA_3D_CANVAS::DisplayStatus()
{
    if( m_parentStatusBar )
    {
        wxString msg;

        msg.Printf( "dx %3.2f", m_camera.GetCameraPos().x );
        m_parentStatusBar->SetStatusText( msg, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::X_POS ) );

        msg.Printf( "dy %3.2f", m_camera.GetCameraPos().y );
        m_parentStatusBar->SetStatusText( msg, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::Y_POS ) );
    }
}


void EDA_3D_CANVAS::OnPaint( wxPaintEvent &event )
{
    // Please have a look at:
    // https://lists.launchpad.net/kicad-developers/msg25149.html
    // wxPaintDC( this );
    // event.Skip( false );

    // SwapBuffer requires the window to be shown before calling
    if( !IsShownOnScreen() )
    {
        wxLogTrace( m_logTrace, "EDA_3D_CANVAS::OnPaint !IsShown" );
        return;
    }

    // Because the board to draw is handled by the parent viewer frame,
    // ensure this parent is still alive. When it is closed before the viewer
    // frame, a paint event can be generated after the parent is closed,
    // therefore with invalid board.
    // This is dependent of the platform.
    // Especially on OSX, but also on Windows, it frequently happens
    if( !GetParent()->GetParent()->IsShown() )
        return; // The parent board editor frame is no more alive

    wxString err_messages;

    // !TODO: implement error reporter
    //WX_STRING_REPORTER errorReporter( &err_messages );
    INFOBAR_REPORTER   warningReporter( m_parentInfoBar );
    STATUSBAR_REPORTER activityReporter(
            m_parentStatusBar, static_cast<int>( EDA_3D_VIEWER_STATUSBAR::STATUS_TEXT ) );

    unsigned strtime = GetRunningMicroSecs();

    // "Makes the OpenGL state that is represented by the OpenGL rendering
    //  context context current, i.e. it will be used by all subsequent OpenGL calls.
    //  This function may only be called when the window is shown on screen"

    // Explicitly create a new rendering context instance for this canvas.
    if( m_glRC == NULL )
        m_glRC = GL_CONTEXT_MANAGER::Get().CreateCtx( this );

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
    // /////////////////////////////////////////////////////////////////////////
    if( !m_is_opengl_initialized )
    {
        if( !initializeOpenGL() )
        {
            GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );

            return;
        }
    }

    // Don't attend to ray trace if OpenGL doesn't support it.
    if( !m_opengl_supports_raytracing )
    {
        m_3d_render = m_3d_render_ogl_legacy;
        m_render_raytracing_was_requested = false;
        m_boardAdapter.RenderEngineSet( RENDER_ENGINE::OPENGL_LEGACY );
    }

    // Check if a raytacing was requested and need to switch to raytracing mode
    if( m_boardAdapter.RenderEngineGet() == RENDER_ENGINE::OPENGL_LEGACY )
    {
        const bool was_camera_changed = m_camera.ParametersChanged();

        // It reverts back to OpenGL mode if it was requested a raytracing
        // render of the current scene. AND the mouse / camera is moving
        if( ( m_mouse_is_moving ||
              m_camera_is_moving ||
              was_camera_changed ||
              windows_size_changed ) &&
            m_render_raytracing_was_requested )
        {
            m_render_raytracing_was_requested = false;
            m_3d_render = m_3d_render_ogl_legacy;
        }
    }

    float curtime_delta_s = 0.0f;

    if( m_camera_is_moving )
    {
        const unsigned curtime_delta = GetRunningMicroSecs() - m_strtime_camera_movement;
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
        m_3d_render->SetCurWindowSize( clientSize );

        requested_redraw = m_3d_render->Redraw( m_mouse_was_moved || m_camera_is_moving,
                                                &activityReporter, &warningReporter );
    }

    if( m_render_pivot )
    {
        const float scale = glm::min( m_camera.ZoomGet(), 1.0f );
        render_pivot( curtime_delta_s, scale * scale );
    }

    // "Swaps the double-buffer of this window, making the back-buffer the
    //  front-buffer and vice versa, so that the output of the previous OpenGL
    //  commands is displayed on the window."
    SwapBuffers();

    GL_CONTEXT_MANAGER::Get().UnlockCtx( m_glRC );

    if( !activityReporter.HasMessage() )
    {
        if( m_mouse_was_moved || m_camera_is_moving )
        {
            // Calculation time in milliseconds
            const double calculation_time = (double)( GetRunningMicroSecs() - strtime) / 1e3;

            activityReporter.Report( wxString::Format( _( "Render time %.0f ms ( %.1f fps)" ),
                                     calculation_time, 1000.0 / calculation_time ) );
        }
    }

    // This will reset the flag of camera parameters changed
    m_camera.ParametersChanged();

    warningReporter.Finalize();

    if( !err_messages.IsEmpty() )
        wxLogMessage( err_messages );

    if( (!m_camera_is_moving) && requested_redraw )
    {
        m_mouse_was_moved = false;
        Request_refresh( false );
    }
}


void EDA_3D_CANVAS::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;

    if( m_eventDispatcher )
    {
        m_parent->Connect( wxEVT_TOOL,
                           wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                           NULL, m_eventDispatcher );
    }
    else
    {
        // While loop is used to be sure that all event handlers are removed.
        while( m_parent->Disconnect( wxEVT_TOOL,
                                     wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                                     NULL, m_eventDispatcher ) );
    }
}


void EDA_3D_CANVAS::OnEvent( wxEvent& aEvent )
{
    if( !m_eventDispatcher )
        aEvent.Skip();
    else
        m_eventDispatcher->DispatchWxEvent( aEvent );

    Refresh();
}


void EDA_3D_CANVAS::OnEraseBackground( wxEraseEvent &event )
{
    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::OnEraseBackground" );
    // Do nothing, to avoid flashing.
}


void EDA_3D_CANVAS::OnMouseWheel( wxMouseEvent &event )
{
    bool mouseActivity = false;

    wxLogTrace( m_logTrace, "EDA_3D_CANVAS::OnMouseWheel" );

    if( m_camera_is_moving )
        return;

    float delta_move = m_delta_move_step_factor * m_camera.ZoomGet();

    if( m_boardAdapter.GetFlag( FL_MOUSEWHEEL_PANNING ) )
        delta_move *= (0.01f * event.GetWheelRotation());
    else
        if( event.GetWheelRotation() < 0 )
            delta_move = -delta_move;

    // mousewheel_panning enabled:
    //      wheel           -> pan;
    //      wheel + shift   -> horizontal scrolling;
    //      wheel + ctrl    -> zooming;
    // mousewheel_panning disabled:
    //      wheel + shift   -> vertical scrolling;
    //      wheel + ctrl    -> horizontal scrolling;
    //      wheel           -> zooming.

    if( m_boardAdapter.GetFlag( FL_MOUSEWHEEL_PANNING ) && !event.ControlDown() )
    {
        if( event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL || event.ShiftDown() )
            m_camera.Pan( SFVEC3F( -delta_move, 0.0f, 0.0f ) );
        else
            m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );

        mouseActivity = true;
    }
    else if( event.ShiftDown() && !m_boardAdapter.GetFlag( FL_MOUSEWHEEL_PANNING ) )
    {
        m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );
        mouseActivity = true;
    }
    else if( event.ControlDown() && !m_boardAdapter.GetFlag( FL_MOUSEWHEEL_PANNING ) )
    {
        m_camera.Pan( SFVEC3F( delta_move, 0.0f, 0.0f ) );
        mouseActivity = true;
    }
    else
    {
        mouseActivity = m_camera.Zoom( event.GetWheelRotation() > 0 ? 1.1f : 1/1.1f );
    }

    // If it results on a camera movement
    if( mouseActivity )
    {
        DisplayStatus();
        Request_refresh();

        m_mouse_is_moving = true;
        m_mouse_was_moved = true;

        restart_editingTimeOut_Timer();
    }

    // Update the cursor current mouse position on the camera
    m_camera.SetCurMousePosition( event.GetPosition() );
}


#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
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
#endif


void EDA_3D_CANVAS::OnMouseMove( wxMouseEvent &event )
{
    //wxLogTrace( m_logTrace, wxT( "EDA_3D_CANVAS::OnMouseMove" ) );

    if( m_camera_is_moving )
        return;

    m_camera.SetCurWindowSize( GetNativePixelSize() );

    if( event.Dragging() )
    {
        if( event.LeftIsDown() )            // Drag
            m_camera.Drag( event.GetPosition() );
        else if( event.MiddleIsDown() )     // Pan
            m_camera.Pan( event.GetPosition() );

        m_mouse_is_moving = true;
        m_mouse_was_moved = true;

        // orientation has changed, redraw mesh
        DisplayStatus();
        Request_refresh();
    }

    const wxPoint eventPosition = event.GetPosition();
    m_camera.SetCurMousePosition( eventPosition );
}


void EDA_3D_CANVAS::OnLeftDown( wxMouseEvent &event )
{
    SetFocus();
    stop_editingTimeOut_Timer();
}


void EDA_3D_CANVAS::OnLeftUp( wxMouseEvent &event )
{
    if( m_camera_is_moving )
        return;

    if( m_mouse_is_moving )
    {
        m_mouse_is_moving = false;
        restart_editingTimeOut_Timer();
    }
}


void EDA_3D_CANVAS::OnMiddleDown( wxMouseEvent &event )
{
    SetFocus();
    stop_editingTimeOut_Timer();
}


void EDA_3D_CANVAS::OnMiddleUp( wxMouseEvent &event )
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


void EDA_3D_CANVAS::OnTimerTimeout_Editing( wxTimerEvent &event )
{
    (void)event;

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
    m_editing_timeout_timer.Start( m_3d_render->GetWaitForEditingTimeOut() , wxTIMER_ONE_SHOT );
}


void EDA_3D_CANVAS::OnTimerTimeout_Redraw( wxTimerEvent &event )
{
    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_3D_CANVAS::Request_refresh( bool aRedrawImmediately )
{
    if( aRedrawImmediately )
    {
        // On some systems, just calling Refresh does not work always
        // (Issue experienced on Win7 MSYS2)
        //Refresh();
        //Update();

        // Using PostEvent will take priority to other events like mouse movements, keys, etc.
        wxPaintEvent redrawEvent;
        wxPostEvent( this, redrawEvent );

        // This behaves the same
        // wxQueueEvent( this,
                        // From wxWidget documentation: "The heap-allocated and
                        // non-NULL event to queue, the function takes ownership of it."
        //               new wxPaintEvent()
        //               );
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

    // Map speed multipler option to actual multiplier value
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
    SFVEC3F rayOrigin;
    SFVEC3F rayDir;

    // Generate a ray origin and direction based on current mouser position and camera
    m_camera.MakeRayAtCurrrentMousePosition( rayOrigin, rayDir );

    RAY mouseRay;
    mouseRay.Init( rayOrigin, rayDir );

    float hit_t;

    // Test it with the board bounding box
    if( m_boardAdapter.GetBBox3DU().Intersect( mouseRay, &hit_t ) )
    {
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.SetLookAtPos_T1( mouseRay.at( hit_t ) );
        m_camera.ResetXYpos_T1();

        request_start_moving_camera();
    }
}


bool EDA_3D_CANVAS::SetView3D( int aKeycode )
{
    if( m_camera_is_moving )
        return false;

    const float delta_move = m_delta_move_step_factor * m_camera.ZoomGet();
    const float arrow_moving_time_speed = 8.0f;
    bool handled = false;

    switch( aKeycode )
    {
    case WXK_SPACE:
        move_pivot_based_on_cur_mouse_position();
        return true;

    case WXK_LEFT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( -delta_move, 0.0f, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case WXK_RIGHT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( +delta_move, 0.0f, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case WXK_UP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( 0.0f, +delta_move, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case WXK_DOWN:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Pan_T1( SFVEC3F( 0.0f, -delta_move, 0.0f ) );
        request_start_moving_camera( arrow_moving_time_speed, false );
        return true;

    case WXK_HOME:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        request_start_moving_camera( glm::min( glm::max( m_camera.ZoomGet(), 1/1.26f ), 1.26f ) );
        return true;

    case WXK_END:
        break;

    case WXK_TAB:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::EASING_IN_OUT );
        m_camera.SetT0_and_T1_current_T();
        m_camera.RotateZ_T1( glm::radians( 45.0f ) );
        request_start_moving_camera();
        handled = true;
        break;

    case WXK_F1:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();

        if( m_camera.Zoom_T1( 1.26f ) )   // 3 steps per doubling
            request_start_moving_camera( 3.0f );

        return true;

    case WXK_F2:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();

        if( m_camera.Zoom_T1( 1/1.26f ) ) // 3 steps per halving
            request_start_moving_camera( 3.0f );

        return true;

    case ID_VIEW3D_RESET:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        request_start_moving_camera( glm::min( glm::max( m_camera.ZoomGet(), 0.5f ), 1.125f ) );
        return true;

    case ID_VIEW3D_RIGHT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateZ_T1( glm::radians( -90.0f ) );
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case ID_VIEW3D_LEFT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateZ_T1( glm::radians(  90.0f ) );
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case ID_VIEW3D_FRONT:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateX_T1( glm::radians( -90.0f ) );
        request_start_moving_camera();
        return true;

    case ID_VIEW3D_BACK:
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

    case ID_VIEW3D_TOP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        request_start_moving_camera( glm::min( glm::max( m_camera.ZoomGet(), 0.5f ), 1.125f ) );
        return true;

    case ID_VIEW3D_BOTTOM:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.Reset_T1();
        m_camera.RotateY_T1( glm::radians( 179.999f ) );    // Rotation = 180 - epsilon
        request_start_moving_camera( glm::min( glm::max( m_camera.ZoomGet(), 0.5f ), 1.125f ) );
        return true;

    case ID_VIEW3D_FLIP:
        m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::BEZIER );
        m_camera.SetT0_and_T1_current_T();
        m_camera.RotateY_T1( glm::radians( 179.999f ) );
        request_start_moving_camera();
        return true;

    default:
        return false;
    }

    m_mouse_was_moved = true;

    restart_editingTimeOut_Timer();

    DisplayStatus();
    Request_refresh();

    return handled;
}


void EDA_3D_CANVAS::RenderEngineChanged()
{
    switch( m_boardAdapter.RenderEngineGet() )
    {
    case RENDER_ENGINE::OPENGL_LEGACY: m_3d_render = m_3d_render_ogl_legacy; break;
    case RENDER_ENGINE::RAYTRACING:    m_3d_render = m_3d_render_raytracing; break;
    default:                           m_3d_render = NULL;                   break;
    }

    if( m_3d_render )
        m_3d_render->ReloadRequest();

    m_mouse_was_moved = false;

    Request_refresh();
}
