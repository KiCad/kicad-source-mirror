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

#ifndef __TOOL_DISPATCHER_H
#define __TOOL_DISPATCHER_H

#include <vector>
#include <wx/event.h>
#include <tool/tool_event.h>

class TOOL_MANAGER;
class PCB_BASE_FRAME;

namespace KIGFX
{
class VIEW;
};

/**
 * Class TOOL_DISPATCHER
 *
 * - takes wx events,
 * - fixes all wx quirks (mouse warping, panning, ordering problems, etc)
 * - translates coordinates to world space
 * - low-level input conditioning (drag/click threshold), updating mouse position during view auto-scroll/pan.
 * - issues TOOL_EVENTS to the tool manager
 */

class TOOL_DISPATCHER : public wxEvtHandler
{
public:
    /**
     * Constructor
     *
     * @param aToolMgr: tool manager instance the events will be sent to
     */
    TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr );

    virtual ~TOOL_DISPATCHER();

    /**
     * Function ResetState()
     * Brings the dispatcher to its initial state.
     */
    virtual void ResetState();

    /**
     * Function DispatchWxEvent()
     * Processes wxEvents (mostly UI events), translates them to TOOL_EVENTs, and makes tools
     * handle those.
     * @param aEvent is the wxWidgets event to be processed.
     */
    virtual void DispatchWxEvent( wxEvent& aEvent );

    /**
     * Function DispatchWxCommand()
     * Processes wxCommands (mostly menu related events) and runs appropriate actions (eg. run the
     * specified tool).
     * @param aEvent is the wxCommandEvent to be processed.
     */
    virtual void DispatchWxCommand( wxCommandEvent& aEvent );

private:
    ///> Number of mouse buttons that is handled in events.
    static const int MouseButtonCount = 3;

    ///> The time threshold for a mouse button press that distinguishes between a single mouse
    ///> click and a beginning of drag event (expressed in milliseconds).
    static const int DragTimeThreshold = 300;

    ///> The distance threshold for mouse cursor that disinguishes between a single mouse click
    ///> and a beginning of drag event (expressed in screen pixels).
    static const int DragDistanceThreshold = 8;

    ///> Handles mouse related events (click, motion, dragging).
    bool handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion );

    ///> Saves the state of key modifiers (Alt, Ctrl and so on).
    template <class EventType>
    static int decodeModifiers( const EventType* aState )
    {
        int mods = 0;

        if( aState->ControlDown() )
            mods |= MD_CTRL;

        if( aState->AltDown() )
            mods |= MD_ALT;

        if( aState->ShiftDown() )
            mods |= MD_SHIFT;

        return mods;
    }

    ///> Redraws the status bar and message panel.
    void updateUI();

    ///> Stores all the informations regarding a mouse button state.
    struct BUTTON_STATE;

    ///> The last mouse cursor position (in world coordinates).
    VECTOR2D m_lastMousePos;

    ///> State of mouse buttons.
    std::vector<BUTTON_STATE*> m_buttons;

    ///> Returns the instance of VIEW, used by the application.
    KIGFX::VIEW* getView();

    ///> Instance of tool manager that cooperates with the dispatcher.
    TOOL_MANAGER* m_toolMgr;
};

#endif
