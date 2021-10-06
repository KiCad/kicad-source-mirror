/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#ifndef INCLUDE_TOOL_SELECTION_TOOL_H_
#define INCLUDE_TOOL_SELECTION_TOOL_H_

#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <wx/timer.h>

class SELECTION_TOOL : public wxEvtHandler
{
public:
    SELECTION_TOOL();
    ~SELECTION_TOOL(){};


protected:
    /**
     * Set the configuration of m_additive, m_subtractive, m_exclusive_or, m_skip_heuristics
     * from the state of modifier keys SHIFT, CTRL, ALT and depending on the OS
     */
    void setModifiersState( bool aShiftState, bool aCtrlState, bool aAltState );

    bool            m_additive;          // Items should be added to sel (instead of replacing)
    bool            m_subtractive;       // Items should be removed from sel
    bool            m_exclusive_or;      // Items' selection state should be toggled
    bool            m_multiple;          // Multiple selection mode is active
    bool            m_skip_heuristics;   // Show disambuguation menu for all items under the
                                         // cursor rather than trying to narrow them down first
                                         // using heuristics
    bool            m_highlight_modifier;// select highlight net on left click
    bool            m_drag_additive;     // Add multiple items to selection
    bool            m_drag_subtractive;  // Remove multiple from selection

    bool            m_canceledMenu;      // Sets to true if the disambiguation menu was cancelled

    wxTimer         m_disambiguateTimer; // Timer to show the disambiguate menu

    VECTOR2I        m_originalCursor;    // Location of original cursor when starting click
};

#endif /* INCLUDE_TOOL_SELECTION_TOOL_H_ */
