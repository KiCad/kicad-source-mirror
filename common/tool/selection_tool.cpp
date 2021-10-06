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

#include <tool/selection_tool.h>


SELECTION_TOOL::SELECTION_TOOL() :
    m_additive( false ),
    m_subtractive( false ),
    m_exclusive_or( false ),
    m_multiple( false ),
    m_skip_heuristics( false ),
    m_highlight_modifier( false ),
    m_drag_additive( false ),
    m_drag_subtractive( false ),
    m_canceledMenu( false )
{
}

void SELECTION_TOOL::setModifiersState( bool aShiftState, bool aCtrlState, bool aAltState )
{
    // Set the configuration of m_additive, m_subtractive, m_exclusive_or from the state of
    // modifier keys SHIFT and CTRL

    // ALT key cannot be used on MSW because of a conflict with the system menu

    m_subtractive        = aCtrlState && aShiftState;
    m_additive           = !aCtrlState && aShiftState;
    m_exclusive_or       = false;
    m_highlight_modifier = aCtrlState && !aShiftState;

    // Drag is more forgiving and allows either Ctrl+Drag or Shift+Drag to add to the selection
    // Note, however that we cannot provide disambiguation at the same time as the box selection
    m_drag_additive      = ( aCtrlState || aShiftState ) && !aAltState;
    m_drag_subtractive   = aCtrlState && aShiftState && !aAltState;

    // While the ALT key has some conflicts under MSW (and some flavors of Linux WMs), it remains
    // useful for users who only use tap-click rather than holding the button.  We disable it for
    // windows because it flashes the disambiguation menu without showing data
#ifndef __WINDOWS__
    m_skip_heuristics = aAltState;
#else
    m_skip_heuristics = false;
#endif

}
