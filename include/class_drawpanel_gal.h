/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
 * @file class_drawpanel_gal.h:
 * @brief EDA_DRAW_PANEL_GAL class definition.
 */

#ifndef  PANELGAL_WXSTRUCT_H
#define  PANELGAL_WXSTRUCT_H

#include <wx/wx.h>
#include <wx/window.h>

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

    /// @copydoc wxWindow::Refresh()
    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL );

    /**
     * Function SetEventDispatcher()
     * Sets a dispatcher that processes events and forwards them to tools.
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
    {
        m_eventDispatcher = aEventDispatcher;
    }

protected:
    void onPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onSize( wxSizeEvent& aEvent );
    void onEvent( wxEvent& aEvent );
    void onEnter( wxEvent& aEvent );
    void onRefreshTimer ( wxTimerEvent& aEvent );
    void skipEvent( wxEvent& aEvent );

    static const int MinRefreshPeriod = 17;             ///< 60 FPS.

    wxLongLong               m_lastRefresh;      ///< Last time the panel was refreshed
    bool                     m_pendingRefresh;
    wxTimer                  m_refreshTimer;

    KIGFX::GAL*              m_gal;              ///< Interface for drawing objects on a 2D-surface
    KIGFX::VIEW*             m_view;             ///< Stores view settings (scale, center, etc.)
                                                 ///< and items to be drawn
    KIGFX::PAINTER*          m_painter;          ///< Contains information about how to draw items
                                                 ///< using GAL
    KIGFX::WX_VIEW_CONTROLS* m_viewControls;     ///< Control for VIEW (moving, zooming, etc.)
    GalType                  m_currentGal;       ///< Currently used GAL
    TOOL_DISPATCHER*         m_eventDispatcher;  ///< Processes and forwards events to tools
};

#endif
