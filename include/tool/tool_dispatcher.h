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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TOOL_DISPATCHER_H
#define TOOL_DISPATCHER_H

#include <vector>
#include <wx/event.h>
#include <wx/longlong.h>
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

    /// The maximum gap (ms) between two events of the same key for them to count as one OS
    /// auto-repeat burst rather than two deliberate key presses.
    static const int AutoRepeatWindowMs = 250;

    /**
     * Pure decision used to discard backlogged OS key auto-repeat events.
     *
     * A repeat event is stale (and must be dropped) only when it belongs to the same burst as
     * the previously processed event (same key seen less than AutoRepeatWindowMs ago) and the
     * key is no longer physically held. The leading edge of every press is kept, so a quick tap
     * whose key has already bounced up still runs once.
     *
     * @param aKeyCode    key code of the current event.
     * @param aNowMs      current time in milliseconds.
     * @param aKeyIsDown  whether the key is currently physically pressed.
     * @param aLastKey    in/out key code of the previously processed event; updated to aKeyCode.
     * @param aLastTimeMs in/out time of the previously processed event; updated to aNowMs.
     * @return true when the event is a stale auto-repeat that should be dropped.
     */
    static bool ShouldDropAutoRepeat( int aKeyCode, wxLongLong aNowMs, bool aKeyIsDown,
                                      int& aLastKey, wxLongLong& aLastTimeMs );

    /**
     * Pure decision on whether a cursor offset from a drag origin has crossed the drag threshold.
     *
     * Used both to detect an in-press drag and to tell a fast click-drag apart from a genuine
     * double-click. A move beyond the threshold on either axis counts as a drag.
     *
     * @param aOffset  screen-space cursor movement from the drag origin.
     * @param aDragMinX drag activation threshold on the X axis in screen pixels.
     * @param aDragMinY drag activation threshold on the Y axis in screen pixels.
     * @return true when the movement exceeds the threshold on either axis.
     */
    static bool IsPastDragThreshold( const VECTOR2D& aOffset, int aDragMinX, int aDragMinY );

private:
    /// Handles mouse related events (click, motion, dragging).
    bool handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion );

    /// Processes any pending mouse clicks that have been physically completed but not yet
    /// dispatched. This ensures clicks are processed before cancel events when both happen
    /// in quick succession.
    void flushPendingClicks();

    /// Decide whether a keyboard event must be dropped because it is a backlogged OS key
    /// auto-repeat event delivered after the key was physically released.
    ///
    /// On Linux a hotkey whose action is slower than the OS auto-repeat interval lets repeat
    /// events accumulate in the wx event queue; once the key is released the queue keeps
    /// draining and the action keeps running. The first event of a burst always runs (so a
    /// quick tap still works), but later repeats only run while the key is still held down.
    bool isStaleAutoRepeat( const wxKeyEvent& aKeyEvent );

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

    /// Key code of the key that was last processed, or 0 when no key is armed. Used together
    /// with m_lastKeyTime to drop stale auto-repeat events that arrive after the key was
    /// released while still letting the leading edge of every fresh press through.
    int       m_lastKeyCode;

    /// Local time (ms) at which m_lastKeyCode was last processed.
    wxLongLong m_lastKeyTime;

    /// Instance of tool manager that cooperates with the dispatcher.
    TOOL_MANAGER* m_toolMgr;
};

#endif  // TOOL_DISPATCHER_H
