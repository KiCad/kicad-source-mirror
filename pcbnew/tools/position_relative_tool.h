/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __POSITION_RELATIVE_TOOL_H
#define __POSITION_RELATIVE_TOOL_H

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection_tool.h>

class BOARD_COMMIT;
class BOARD_ITEM;
class PCB_SELECTION_TOOL;
class DIALOG_POSITION_RELATIVE;


/**
 * The interactive edit tool.
 *
 * Allows one to move, rotate, flip and change properties of items selected using the
 * pcbnew.InteractiveSelection tool.
 */

class POSITION_RELATIVE_TOOL : public PCB_TOOL_BASE
{
public:
    POSITION_RELATIVE_TOOL();

    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /**
     * Invoke a dialog box to allow positioning of the item relative to another by an exact amount.
     */
    int PositionRelative( const TOOL_EVENT& aEvent );

    /**
     * Draw a line connecting two points and allow the user to enter what it should be.
     * The move the items to match that.
     */
    int InteractiveOffset( const TOOL_EVENT& aEvent );

    /**
     * Return the position of the selected item(s)
     */
    VECTOR2I GetSelectionAnchorPosition() const { return m_selectionAnchor; }

    /**
     * Position the m_position_relative_selection selection relative to anchor position using
     * the given translation.
     */
    int RelativeItemSelectionMove( const VECTOR2I& anchor, const VECTOR2I& translation );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    DIALOG_POSITION_RELATIVE*     m_dialog;

    PCB_SELECTION_TOOL*           m_selectionTool;
    PCB_SELECTION                 m_selection;
    VECTOR2I                      m_selectionAnchor;
    bool                          m_inInteractivePosition; // Re-entrancy guard

    std::unique_ptr<BOARD_COMMIT> m_commit;
};

#endif
