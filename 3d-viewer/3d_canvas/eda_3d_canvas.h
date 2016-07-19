/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  eda_3d_canvas.h
 * @brief
 */

#ifndef EDA_3D_CANVAS_H
#define EDA_3D_CANVAS_H


#include "cinfo3d_visu.h"
#include "3d_rendering/c3d_render_base.h"
#include "3d_rendering/3d_render_ogl_legacy/c3d_render_ogl_legacy.h"
#include "3d_rendering/3d_render_raytracing/c3d_render_raytracing.h"
#include "3d_cache/3d_cache.h"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/glcanvas.h>
#include <wx/image.h>
#include <wx/wupdlock.h>
#include <wx/timer.h>
#include <wx/statusbr.h>
#include <wxBasePcbFrame.h>


/**
 *  Class EDA_3D_CANVAS
 *  Implement a canvas based on a wxGLCanvas
 */
class EDA_3D_CANVAS : public wxGLCanvas
{

 public:


    /**
     *  @brief EDA_3D_CANVAS - Creates a new 3D Canvas with a attribute list
     *  @param aParent: the parent creator of this canvas
     *  @param aAttribList: a list of openGL options created by GetOpenGL_AttributesList
     *  @param aBoard: The board
     *  @param aSettings: the settings options to be used by this canvas
     */
    EDA_3D_CANVAS( wxWindow *aParent,
                   const int *aAttribList  = 0,
                   BOARD *aBoard = NULL,
                   CINFO3D_VISU &aSettings = G_null_CINFO3D_VISU,
                   S3D_CACHE *a3DCachePointer = NULL );

    ~EDA_3D_CANVAS();

    void SetStatusBar( wxStatusBar *aStatusBar ) { m_parentStatusBar = aStatusBar; }

    void ReloadRequest( BOARD *aBoard = NULL, S3D_CACHE *aCachePointer = NULL );

    /**
     * @brief IsReloadRequestPending - Query if there is a pending reload request
     * @return true if it wants to reload, false if there is no reload pending
     */
    bool IsReloadRequestPending() const
    {
        if( m_3d_render )
            return m_3d_render->IsReloadRequestPending();
        else
            return false;
    }

    /**
     * @brief RenderRaytracingRequest - Request to render the current view in Raytracing mode
     */
    void RenderRaytracingRequest();

    /**
     *  Request a screenshot and output it to the aDstImage
     *  @param aDstImage - Screenshot destination image
     */
    void GetScreenshot( wxImage &aDstImage );

    /**
     * @brief SetView3D - Helper function to call view commands
     * @param keycode: ascii key commands
     */
    void SetView3D( int keycode );

    /**
     * @brief RenderEngineChanged - Notify that the render engine was changed
     */
    void RenderEngineChanged();

    /**
     * @brief DisplayStatus - Update the status bar with the position information
     */
    void DisplayStatus();

    /**
     * @brief Request_refresh - Schedule a refresh update of the canvas
     * @param aRedrawImmediately - true will request a redraw, false will
     * schedule a redraw, after a short timeout.
     */
    void Request_refresh( bool aRedrawImmediately = true );

 private:

    void OnPaint( wxPaintEvent &event );

    void OnEraseBackground( wxEraseEvent &event );

    void OnMouseWheel( wxMouseEvent &event );

#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    void   OnMagnify( wxMouseEvent& event );
#endif

    void OnMouseMove( wxMouseEvent &event );

    void OnLeftDown( wxMouseEvent &event );

    void OnLeftUp( wxMouseEvent &event );

    void OnMiddleUp( wxMouseEvent &event );

    void OnMiddleDown( wxMouseEvent &event );

    void OnRightClick( wxMouseEvent &event );

    void OnPopUpMenu( wxCommandEvent &event );

    void OnCharHook( wxKeyEvent& event );

    void OnKeyEvent( wxKeyEvent& event );

    void OnTimerTimeout_Editing( wxTimerEvent& event );

    /**
     * @brief OnCloseWindow - called when the frame is closed
     * @param event
     */
    void OnCloseWindow( wxCloseEvent &event );

    void OnTimerTimeout_Redraw( wxTimerEvent& event );

    DECLARE_EVENT_TABLE();

 private:

    /**
     * @brief stop_editingTimeOut_Timer - stop the editing time, so it will not timeout
     */
    void stop_editingTimeOut_Timer();


    /**
     * @brief restart_editingTimeOut_Timer - reset the editing timer
     */
    void restart_editingTimeOut_Timer();


    /**
     * @brief request_start_moving_camera - start a camera movement
     * @param aMovingSpeed: the time speed
     * @param aRenderPivot: if it should display pivot cursor while move
     */
    void request_start_moving_camera( float aMovingSpeed = 2.0f,
                                      bool aRenderPivot = true );


    /**
     * @brief move_pivot_based_on_cur_mouse_position -
     * This function hits a ray to the board and start a moviment
     */
    void move_pivot_based_on_cur_mouse_position();


    /**
     * @brief render_pivot - render the pivot cursor
     * @param t: time between 0.0 and 1.0
     * @param aScale: scale to apply on the cursor
     */
    void render_pivot( float t, float aScale );

    /**
     * @brief initializeOpenGL
     * @return if OpenGL initialization succeed
     */
    bool initializeOpenGL();

    /**
     * @brief releaseOpenGL - free created targets and openGL context
     */
    void releaseOpenGL();

 private:

    /// current OpenGL context
    wxGLContext *m_glRC;

    /// Parent statusbar to report progress
    wxStatusBar *m_parentStatusBar;

    /// Time timeout will expires after some time sinalizing that the mouse /
    /// keyboard movements are over.
    wxTimer m_editing_timeout_timer;

    /// This timer will be used to schedule a redraw event
    wxTimer m_redraw_trigger_timer;

    /// true if mouse activity is on progress
    bool m_mouse_is_moving;

    /// true if there was some type of activity, it will be used to render in
    /// preview mode
    bool m_mouse_was_moved;

    /// true if camera animation is ongoing
    bool m_camera_is_moving;

    /// activated the render of pivot while camera moving
    bool m_render_pivot;

    /// 1.0f will be 1:1
    float m_camera_moving_speed;

    /// Stores the ticktime when the camera star its movement
    unsigned m_strtime_camera_movement;

    /// Stores all pre-computed 3D information and visualization settings to render the board
    CINFO3D_VISU &m_settings;

    /// The current render in used for this canvas
    C3D_RENDER_BASE       *m_3d_render;

    /// Raytracing render class
    C3D_RENDER_RAYTRACING *m_3d_render_raytracing;

    /// OpenGL legacy render class
    C3D_RENDER_OGL_LEGACY *m_3d_render_ogl_legacy;

    /// Flag to store if opengl was initialized already
    bool m_is_opengl_initialized;

    /// Step factor to used with cursor on relation to the current zoom
    static const float m_delta_move_step_factor;

    /// Flags that the user requested the current view to be render with raytracing
    bool m_render_raytracing_was_requested;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_CANVAS".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar *m_logTrace;
};


#endif // EDA_3D_CANVAS_H
