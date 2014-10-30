/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 CERN
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
 * @file class_draw_panel_gal.h:
 * @brief EDA_DRAW_PANEL_GAL class definition.
 */

#ifndef  PANELGAL_WXSTRUCT_H
#define  PANELGAL_WXSTRUCT_H

#include <wx/window.h>
#include <wx/timer.h>
#include <layers_id_colors_and_visibility.h>
#include <math/vector2d.h>

class BOARD;
class TOOL_DISPATCHER;

namespace KIGFX
{
class GAL;
class VIEW;
class WX_VIEW_CONTROLS;
class VIEW_CONTROLS;
class PAINTER;
};


class EDA_DRAW_PANEL_GAL : public wxWindow
{
public:
    enum GalType {
        GAL_TYPE_NONE,      ///< Not used
        GAL_TYPE_OPENGL,    ///< OpenGL implementation
        GAL_TYPE_CAIRO,     ///< Cairo implementation
    };

    EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                        const wxSize& aSize, GalType aGalType = GAL_TYPE_OPENGL );
    ~EDA_DRAW_PANEL_GAL();

    /**
     * Function SwitchBackend
     * Switches method of rendering graphics.
     * @param aGalType is a type of rendering engine that you want to use.
     */
    void SwitchBackend( GalType aGalType );

    /**
     * Function GetBackend
     * Returns the type of backend currently used by GAL canvas.
     */
    inline GalType GetBackend() const
    {
        return m_backend;
    }

    /**
     * Function GetGAL()
     * Returns a pointer to the GAL instance used in the panel.
     * @return The instance of GAL.
     */
    KIGFX::GAL* GetGAL() const
    {
        return m_gal;
    }

    /**
     * Function GetView()
     * Returns a pointer to the VIEW instance used in the panel.
     * @return The instance of VIEW.
     */
    KIGFX::VIEW* GetView() const
    {
        return m_view;
    }

    /**
     * Function GetViewControls()
     * Returns a pointer to the VIEW_CONTROLS instance used in the panel.
     * @return The instance of VIEW_CONTROLS.
     */
    KIGFX::VIEW_CONTROLS* GetViewControls() const
    {
        return (KIGFX::VIEW_CONTROLS*)( m_viewControls );
    }

    /**
     * Function GetEnableMousewheelPan
     * Return default mousewheel behavior.
     */
    bool GetEnableMousewheelPan() const
    {
        return m_enableMousewheelPan;
    }

    /**
     * Function SetEnableMousewheelPan
     * Set default mousewheel behavior.
     */
    void SetEnableMousewheelPan( bool aEnable )
    {
        m_enableMousewheelPan = aEnable;
    }

    /// @copydoc wxWindow::Refresh()
    void Refresh( bool aEraseBackground = true, const wxRect* aRect = NULL );

    /**
     * Function ForceRefresh()
     * Forces a redraw.
     */
    void ForceRefresh();

    /**
     * Function SetEventDispatcher()
     * Sets a dispatcher that processes events and forwards them to tools.
     * @param aEventDispatcher is the object that will be used for dispatching events.
     * DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the DRAW_PANEL_GAL and parent frame.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    /**
     * Function StartDrawing()
     * Begins drawing if it was stopped previously.
     */
    void StartDrawing();

    /**
     * Function StopDrawing()
     * Prevents the GAL canvas from further drawing till it is recreated
     * or StartDrawing() is called.
     */
    void StopDrawing();

    /**
     * Function SetHighContrastLayer
     * Takes care of display settings for the given layer to be displayed in high contrast mode.
     */
    virtual void SetHighContrastLayer( LAYER_ID aLayer );

    /**
     * Function SetTopLayer
     * Moves the selected layer to the top, so it is displayed above all others.
     */
    virtual void SetTopLayer( LAYER_ID aLayer );

protected:
    void onPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onSize( wxSizeEvent& aEvent );
    void onEvent( wxEvent& aEvent );
    void onEnter( wxEvent& aEvent );
    void onRefreshTimer( wxTimerEvent& aEvent );

    static const int MinRefreshPeriod = 17;             ///< 60 FPS.

    /// Pointer to the parent window
    wxWindow*               m_parent;

    /// Last timestamp when the panel was refreshed
    wxLongLong               m_lastRefresh;

    /// Is there a redraw event requested?
    bool                     m_pendingRefresh;

    /// True if GAL is currently redrawing the view
    bool                     m_drawing;

    /// Timer responsible for preventing too frequent refresh
    wxTimer                  m_refreshTimer;

    /// Interface for drawing objects on a 2D-surface
    KIGFX::GAL*              m_gal;

    /// Stores view settings (scale, center, etc.) and items to be drawn
    KIGFX::VIEW*             m_view;

    /// Contains information about how to draw items using GAL
    KIGFX::PAINTER*          m_painter;

    /// Control for VIEW (moving, zooming, etc.)
    KIGFX::WX_VIEW_CONTROLS* m_viewControls;

    /// Currently used GAL
    GalType                  m_backend;

    /// Processes and forwards events to tools
    TOOL_DISPATCHER*         m_eventDispatcher;

    /// True to enable mousewheel panning by default.
    bool                     m_enableMousewheelPan;
};

#endif
