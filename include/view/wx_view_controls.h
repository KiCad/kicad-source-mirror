/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
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
 * @file wx_view_controls.h
 * @brief WX_VIEW_CONTROLS class definition.
 */

#ifndef __WX_VIEW_CONTROLS_H
#define __WX_VIEW_CONTROLS_H

#include <wx/wx.h>
#include <wx/event.h>

#include <view/view_controls.h>

class EDA_DRAW_PANEL_GAL;
class TOOL_DISPATCHER;

namespace KiGfx
{
/**
 * Class WX_VIEW_CONTROLS
 * is a specific implementation of class VIEW_CONTROLS for wxWidgets library.
 */
class WX_VIEW_CONTROLS : public VIEW_CONTROLS, public wxEvtHandler
{
public:

    WX_VIEW_CONTROLS( VIEW* aView, wxWindow* aParentPanel );
    ~WX_VIEW_CONTROLS() {};

    /// Handler functions
    void    onWheel( wxMouseEvent& aEvent );
    void    onMotion( wxMouseEvent& aEvent );
    void    onButton( wxMouseEvent& aEvent );
    void    onEnter( wxMouseEvent& aEvent );
    void    onTimer( wxTimerEvent& aEvent );

    /**
     * Function SetGrabMouse()
     * Enables/disables mouse cursor grabbing (limits the movement field only to the panel area).
     *
     * @param aEnabled says whether the option should enabled or disabled.
     */
    void    SetGrabMouse( bool aEnabled );

    /**
     * Function SetAutoPan()
     * Enables/disables autopanning (panning when mouse cursor reaches the panel border).
     *
     * @param aEnabled says whether the option should enabled or disabled.
     */
    void    SetAutoPan( bool aEnabled )
    {
        m_autoPanEnabled = true;
    }

private:
    enum State {
        IDLE = 1,
        DRAG_PANNING,
        AUTO_PANNING,
    };

    void    handleAutoPanning( wxMouseEvent& aEvent );

    /// Current state of VIEW_CONTROLS
    State       m_state;

    /// Options for WX_VIEW_CONTROLS
    bool        m_autoPanEnabled;
    bool        m_needRedraw;
    bool        m_grabMouse;

    /// Distance from cursor to VIEW edge when panning is active.
    float       m_autoPanMargin;
    /// How fast is panning when in auto mode.
    float       m_autoPanSpeed;
    /// TODO
    float       m_autoPanAcceleration;

    /// Panel that is affected by VIEW_CONTROLS
    wxWindow*   m_parentPanel;

    /// Stores information about point where event started.
    VECTOR2D    m_dragStartPoint;
    VECTOR2D    m_lookStartPoint;
    VECTOR2D    m_panDirection;

    /// Used for determining time intervals between events.
    wxLongLong  m_timeStamp;
    wxTimer     m_panTimer;
};
} // namespace KiGfx

#endif
