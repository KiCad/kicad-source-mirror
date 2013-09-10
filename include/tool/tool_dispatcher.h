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

#include <tool/tool_event.h>

#include <wx/event.h>
#include <wx/kbdstate.h>

class TOOL_MANAGER;
class PCB_BASE_FRAME;

namespace KiGfx {
	class VIEW;
};

/** 
 * Class TOOL_DISPATCHER
 * 
 * - takes wx events,
 * - fixes all wx quirks (mouse warping, etc)
 * - translates coordinates to world space
 * - low-level input conditioning (drag/click threshold), updating mouse position during view auto-scroll/pan.
 * - issues TOOL_EVENTS to the manager
 */

class TOOL_DISPATCHER
{
public:
    /**
     * Constructor
     *
     * @param aToolMgr: tool manager instance the events will be sent to
     * @param aEditFrame: the frame wx events come from
     */
    TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr, PCB_BASE_FRAME* aEditFrame );
    virtual ~TOOL_DISPATCHER();

    virtual void ResetState();
    virtual void DispatchWxEvent( wxEvent& aEvent );
    virtual void DispatchWxCommand( wxCommandEvent& aEvent );

private:
    static const int MouseButtonCount = 3;
    static const int DragTimeThreshold = 300;
    static const int DragDistanceThreshold = 8;

    bool handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion );
    bool handlePopupMenu( wxEvent& aEvent );

    wxPoint getCurrentMousePos();

    int decodeModifiers( const wxKeyboardState* aState ) const;

    struct ButtonState;
    VECTOR2D m_lastMousePos;
    std::vector<ButtonState*> m_buttons;

    KiGfx::VIEW* getView();
    TOOL_MANAGER* m_toolMgr;
    PCB_BASE_FRAME* m_editFrame;
};

#endif
