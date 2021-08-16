/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_3D_CANVAS_H
#define EDA_3D_CANVAS_H

#include <atomic>
#include "board_adapter.h"
#include "3d_rendering/3d_render_raytracing/accelerators/accelerator_3d.h"
#include "3d_rendering/render_3d_base.h"
#include "3d_cache/3d_cache.h"
#include <gal/hidpi_gl_canvas.h>
#include <wx/image.h>
#include <wx/timer.h>


class WX_INFOBAR;
class wxStatusBar;
class BOARD;
class RENDER_3D_RAYTRACE;
class RENDER_3D_LEGACY;


/**
 *  Implement a canvas based on a wxGLCanvas
 */
class EDA_3D_CANVAS : public HIDPI_GL_CANVAS
{
public:
    /**
     *  Create a new 3D Canvas with an attribute list.
     *
     *  @param aParent the parent creator of this canvas.
     *  @param aAttribList a list of openGL options created by GetOpenGL_AttributesList.
     *  @param aBoard The board.
     *  @param aSettings the settings options to be used by this canvas.
     */
    EDA_3D_CANVAS( wxWindow* aParent, const int* aAttribList,
                   BOARD_ADAPTER& aSettings, CAMERA& aCamera, S3D_CACHE* a3DCachePointer );

    ~EDA_3D_CANVAS();

    /**
     * Set a dispatcher that processes events and forwards them to tools.
     *
     * #DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the DRAW_PANEL_GAL and parent frame.
     *
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    void SetStatusBar( wxStatusBar* aStatusBar )
    {
        m_parentStatusBar = aStatusBar;
    }

    void SetInfoBar( WX_INFOBAR* aInfoBar )
    {
        m_parentInfoBar = aInfoBar;
    }

    void ReloadRequest( BOARD* aBoard = nullptr, S3D_CACHE* aCachePointer = nullptr );

    /**
     * Query if there is a pending reload request.
     *
     * @return true if it wants to reload, false if there is no reload pending.
     */
    bool IsReloadRequestPending() const
    {
        if( m_3d_render )
            return m_3d_render->IsReloadRequestPending();
        else
            return false;
    }

    /**
     * @return the current render ( a RENDER_3D_RAYTRACE* or a RENDER_3D_LEGACY* render )
     */
    RENDER_3D_BASE* GetCurrentRender() const { return m_3d_render; }

    /**
     * Request to render the current view in Raytracing mode.
     */
    void RenderRaytracingRequest();

    /**
     *  Request a screenshot and output it to the \a aDstImage
     *
     *  @param aDstImage - Screenshot destination image
     */
    void GetScreenshot( wxImage& aDstImage );

    /**
     * Helper function to call view commands.
     *
     * @param aKeycode ascii key commands.
     * @return true if the key code was handled, false if no command found for this code.
     */
    bool SetView3D( int aKeycode );

    /**
     * Enable or disable camera animation when switching to a pre-defined view.
     */
    void SetAnimationEnabled( bool aEnable ) { m_animation_enabled = aEnable; }
    bool GetAnimationEnabled() const { return m_animation_enabled; }

    /**
     * Set the camera animation moving speed multiplier option.
     *
     * @param aMultiplier one of the possible integer options: [1,2,3,4,5].
     */
    void SetMovingSpeedMultiplier( int aMultiplier ) { m_moving_speed_multiplier = aMultiplier; }
    int GetMovingSpeedMultiplier() const { return m_moving_speed_multiplier; }

    int GetProjectionMode() const { return (int) m_camera.GetProjection(); };
    void SetProjectionMode( int aMode ) { m_camera.SetProjection( (PROJECTION_TYPE) aMode ); }

    /**
     * Notify that the render engine was changed.
     */
    void RenderEngineChanged();

    /**
     * Update the status bar with the position information.
     */
    void DisplayStatus();

    /**
     * Schedule a refresh update of the canvas.
     *
     * @param aRedrawImmediately true will request a redraw, false will schedule a redraw
     *                           after a short timeout.
     */
    void Request_refresh( bool aRedrawImmediately = true );

    /**
     * Used to forward events to the canvas from popups, etc.
     */
    void OnEvent( wxEvent& aEvent );

private:
    /**
     * Called by a wxPaintEvent event
     */
    void OnPaint( wxPaintEvent& aEvent );

    /**
     * The actual function to repaint the canvas.
     *
     * It is usually called by OnPaint() but because it does not use a wxPaintDC it can be
     * called outside a wxPaintEvent
     */
    void DoRePaint();

    void OnEraseBackground( wxEraseEvent& event );

    void OnRefreshRequest( wxEvent& aEvent );

    void OnMouseWheel( wxMouseEvent& event );

#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    void   OnMagnify( wxMouseEvent& event );
#endif

    void OnMouseMove( wxMouseEvent& event );
    void OnLeftDown( wxMouseEvent& event );
    void OnLeftUp( wxMouseEvent& event );
    void OnMiddleUp( wxMouseEvent& event );
    void OnMiddleDown( wxMouseEvent& event );
    void OnTimerTimeout_Editing( wxTimerEvent& event );
    void OnCloseWindow( wxCloseEvent& event );
    void OnResize( wxSizeEvent& event );
    void OnTimerTimeout_Redraw( wxTimerEvent& event );

    DECLARE_EVENT_TABLE()

    /**
     *Stop the editing time so it will not timeout.
     */
    void stop_editingTimeOut_Timer();

    /**
     * Reset the editing timer.
     */
    void restart_editingTimeOut_Timer();

    /**
     * Start a camera movement.
     *
     * @param aMovingSpeed the time speed.
     * @param aRenderPivot if it should display pivot cursor while move.
     */
    void request_start_moving_camera( float aMovingSpeed = 2.0f, bool aRenderPivot = true );

    /**
     * This function hits a ray to the board and start a movement.
     */
    void move_pivot_based_on_cur_mouse_position();

    /**
     * Render the pivot cursor.
     *
     * @param t time between 0.0 and 1.0.
     * @param aScale scale to apply on the cursor.
     */
    void render_pivot( float t, float aScale );

    /**
     * @return true if OpenGL initialization succeeded.
     */
    bool initializeOpenGL();

    /**
     * Free created targets and openGL context.
     */
    void releaseOpenGL();

    RAY getRayAtCurrrentMousePosition();

private:
    TOOL_DISPATCHER*       m_eventDispatcher;
    wxStatusBar*           m_parentStatusBar;         // Parent statusbar to report progress
    WX_INFOBAR*            m_parentInfoBar;

    wxGLContext*           m_glRC;                    // Current OpenGL context
    bool                   m_is_opengl_initialized;
    bool                   m_is_opengl_version_supported;

    wxTimer                m_editing_timeout_timer;   // Expires after some time signaling that
                                                      // the mouse / keyboard movements are over
    wxTimer                m_redraw_trigger_timer;    // Used to schedule a redraw event
    std::atomic_flag       m_is_currently_painting;   // Avoid drawing twice at the same time

    bool                   m_mouse_is_moving;         // Mouse activity is in progress
    bool                   m_mouse_was_moved;
    bool                   m_camera_is_moving;        // Camera animation is ongoing
    bool                   m_render_pivot;            // Render the pivot while camera moving
    float                  m_camera_moving_speed;     // 1.0f will be 1:1
    unsigned               m_strtime_camera_movement; // Ticktime of camera movement start
    bool                   m_animation_enabled;       // Camera animation enabled
    int                    m_moving_speed_multiplier; // Camera animation speed multiplier option

    BOARD_ADAPTER&         m_boardAdapter;            // Pre-computed 3D info and settings
    CAMERA&                m_camera;
    RENDER_3D_BASE*        m_3d_render;
    RENDER_3D_RAYTRACE*    m_3d_render_raytracing;
    RENDER_3D_LEGACY*      m_3d_render_ogl_legacy;

    static const float     m_delta_move_step_factor;  // Step factor to used with cursor on
                                                      // relation to the current zoom

    bool                   m_opengl_supports_raytracing;
    bool                   m_render_raytracing_was_requested;

    ACCELERATOR_3D*        m_accelerator3DShapes;    // used for mouse over searching

    BOARD_ITEM*            m_currentRollOverItem;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_CANVAS".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;
};


#endif // EDA_3D_CANVAS_H
