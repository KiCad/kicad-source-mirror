/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file wx_view_controls.h
 * @brief WX_VIEW_CONTROLS class definition.
 */

#ifndef __WX_VIEW_CONTROLS_H
#define __WX_VIEW_CONTROLS_H

#include <wx/wx.h>

#include <view/view_controls.h>

#include <memory>

namespace KIGFX
{

class ZOOM_CONTROLLER;

/**
 * An implementation of class #VIEW_CONTROLS for wxWidgets library.
 */
class WX_VIEW_CONTROLS : public VIEW_CONTROLS, public wxEvtHandler
{
public:
    WX_VIEW_CONTROLS( VIEW* aView, wxScrolledCanvas* aParentPanel );
    virtual ~WX_VIEW_CONTROLS();

    /// Handler functions
    void onWheel( wxMouseEvent& aEvent );
    void onMotion( wxMouseEvent& aEvent );
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    void onMagnify( wxMouseEvent& aEvent );
#endif
    void onButton( wxMouseEvent& aEvent );
    void onEnter( wxMouseEvent& WXUNUSED( aEvent ) );
    void onLeave( wxMouseEvent& WXUNUSED( aEvent ) );
    void onTimer( wxTimerEvent& WXUNUSED( aEvent ) );
    void onScroll( wxScrollWinEvent& aEvent );

    /**
     * Enable or disable mouse cursor grabbing (limits the movement field only to the panel area).
     *
     * @param aEnabled says whether the option should be enabled or disabled.
     */
    void SetGrabMouse( bool aEnabled ) override;

    /**
     * Force the cursor to stay within the drawing panel area.
     *
     * @param aEnabled determines if the cursor should be captured.
     */
    void CaptureCursor( bool aEnabled ) override;

    /**
     * Turn on/off auto panning (this feature is used when there is a tool active (eg. drawing a
     * track) and user moves mouse to the VIEW edge - then the view can be translated or not).
     *
     * @param aEnabled tells if the autopanning should be active.
     */
    void SetAutoPan( bool aEnabled ) override;

    ///< @copydoc VIEW_CONTROLS::GetMousePosition()
    VECTOR2D GetMousePosition( bool aWorldCoordinates = true ) const override;

    using VIEW_CONTROLS::GetCursorPosition;

    ///< @copydoc VIEW_CONTROLS::GetCursorPosition()
    VECTOR2D GetCursorPosition( bool aSnappingEnabled ) const override;

    ///< @copydoc VIEW_CONTROLS::GetRawCursorPosition()
    VECTOR2D GetRawCursorPosition( bool aSnappingEnabled = true ) const override;

    void SetCursorPosition( const VECTOR2D& aPosition, bool warpView,
                            bool aTriggeredByArrows, long aArrowCommand ) override;

    ///< @copydoc VIEW_CONTROLS::SetCrossHairCursorPosition()
    void SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView ) override;

    ///< @copydoc VIEW_CONTROLS::CursorWarp()
    void WarpCursor( const VECTOR2D& aPosition, bool aWorldCoordinates = false,
            bool aWarpView = false ) override;

    ///< @copydoc VIEW_CONTROLS::CenterOnCursor()
    void CenterOnCursor() const override;

    ///< Adjusts the scrollbars position to match the current viewport.
    void UpdateScrollbars();

    void ForceCursorPosition( bool aEnabled,
                              const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) ) override;

    ///< Applies VIEW_CONTROLS settings from the program COMMON_SETTINGS
    void LoadSettings() override;

    ///< Event that forces mouse move event in the dispatcher (eg. used in autopanning, when
    ///< mouse cursor does not move in screen coordinates, but does in world coordinates)
    static const wxEventType EVT_REFRESH_MOUSE;

private:
    ///< Possible states for WX_VIEW_CONTROLS.
    enum STATE
    {
        IDLE = 1,           ///< Nothing is happening.
        DRAG_PANNING,       ///< Panning with mouse button pressed.
        AUTO_PANNING,       ///< Panning on approaching borders of the frame.
        DRAG_ZOOMING,       ///< Zooming with mouse button pressed.
    };

    /**
     * Compute new viewport settings while in autopanning mode.
     *
     * @param aEvent is an event to be processed and decide if autopanning should happen.
     * @return true if it is currently autopanning (ie. autopanning is active and mouse cursor
     *         is in the area that causes autopanning to happen).
     */
    bool handleAutoPanning( const wxMouseEvent& aEvent );

    /**
     * Send an event to refresh mouse position.
     *
     * It is mostly used for notifying the tools that the cursor position in the world
     * coordinates has changed, whereas the screen coordinates remained the same (e.g.
     * frame edge autopanning).
     */
    void refreshMouse();

    /**
     * Get the cursor position in the screen coordinates.
     */
    wxPoint getMouseScreenPosition() const;

    ///< Current state of VIEW_CONTROLS.
    STATE       m_state;

    ///< Panel that is affected by VIEW_CONTROLS.
    wxScrolledCanvas* m_parentPanel;

    ///< Store information about point where dragging has started.
    VECTOR2D    m_dragStartPoint;

    ///< Stores information about the center of viewport when dragging has started.
    VECTOR2D    m_lookStartPoint;

    ///< Current direction of panning (only autopanning mode).
    VECTOR2D    m_panDirection;

    ///< Timer responsible for handling autopanning.
    wxTimer     m_panTimer;

    ///< Ratio used for scaling world coordinates to scrollbar position.
    VECTOR2D    m_scrollScale;

    ///< Current scrollbar position.
    VECTOR2I    m_scrollPos;

    ///< The zoom scale when a drag zoom started.
    double      m_initialZoomScale;

    ///< The mouse position when a drag zoom started.
    VECTOR2D      m_zoomStartPoint;

#ifdef __WXGTK3__
    ///< Last event timestamp used to de-bounce mouse wheel.
    long int    m_lastTimestamp;
#endif

    ///< Current cursor position (world coordinates).
    VECTOR2D    m_cursorPos;

    ///< Flag deciding whether the cursor position should be calculated using the mouse position.
    bool        m_updateCursor;

    ///< A #ZOOM_CONTROLLER that determines zoom steps. This is platform-specific.
    std::unique_ptr<ZOOM_CONTROLLER> m_zoomController;
};
} // namespace KIGFX

#endif
