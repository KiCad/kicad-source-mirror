/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TOOL_DISPATCHER_H
#define TOOL_DISPATCHER_H

#include <vector>
#include <wx/event.h>
#include <tool/tool_event.h>   // Needed for MD_ constants

class TOOL_MANAGER;
class PCB_BASE_FRAME;
class ACTIONS;
class ACTION_MENU;

namespace KIGFX
{
class VIEW;
}

/**
 * - takes wx events,
 * - fixes all wx quirks (mouse warping, panning, ordering problems, etc)
 * - translates coordinates to world space
 * - low-level input conditioning (drag/click threshold), updating mouse position during
 *   view auto-scroll/pan.
 * - issues TOOL_EVENTS to the tool manager
 */
class TOOL_DISPATCHER : public wxEvtHandler
{
public:
    /**
     * @param aToolMgr: tool manager instance the events will be sent to.
     */
    TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr );

    virtual ~TOOL_DISPATCHER();

    /**
     * Bring the dispatcher to its initial state.
     */
    virtual void ResetState();

    /**
     * Process wxEvents (mostly UI events), translate them to TOOL_EVENTs, and make tools
     * handle those.
     *
     * @param aEvent is the wxWidgets event to be processed.
     */
    virtual void DispatchWxEvent( wxEvent& aEvent );

    /**
     * Map a wxWidgets key event to a TOOL_EVENT.
     */
    std::optional<TOOL_EVENT> GetToolEvent( wxKeyEvent* aKeyEvent, bool* aSpecialKeyFlag );

private:
    /// Handles mouse related events (click, motion, dragging).
    bool handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion );

    /// Returns the instance of VIEW, used by the application.
    KIGFX::VIEW* getView();

    /// Returns the state of key modifiers (Alt, Ctrl and so on) as OR'ed list
    /// of bits (MD_CTRL, MD_ALT ...)
    static int decodeModifiers( const wxKeyboardState* aState );

private:
    /// The time threshold for a mouse button press that distinguishes between a single mouse
    /// click and a beginning of drag event (expressed in milliseconds).
    static const int DragTimeThreshold = 300;

    /// The distance threshold for mouse cursor that distinguishes between a single mouse click
    /// and a beginning of drag event (expressed in screen pixels).
    /// System drag preferences take precedence if available
    static const int DragDistanceThreshold = 8;

    int      m_sysDragMinX;          ///< Minimum distance before drag is activated in the X axis
    int      m_sysDragMinY;          ///< Maximum distance before drag is activated in the Y axis

    VECTOR2D m_lastMousePos;         ///< The last mouse cursor position (in world coordinates).
    VECTOR2D m_lastMousePosScreen;   ///< The last mouse cursor position (in screen coordinates).

    /// State of mouse buttons.
    struct BUTTON_STATE;
    std::vector<BUTTON_STATE*> m_buttons;

    /// Instance of tool manager that cooperates with the dispatcher.
    TOOL_MANAGER* m_toolMgr;
};

#endif  // TOOL_DISPATCHER_H
