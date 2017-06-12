/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/pcb_tool.h>
#include "selection_tool.h"
#include "dialogs/dialog_position_relative.h"
#include "dialogs/dialog_position_relative_base.h"

class BOARD_COMMIT;
class BOARD_ITEM;
class SELECTION_TOOL;
class DIALOG_POSITION_RELATIVE;

/**
 * Class POSITION_RELATIVE_TOOL
 *
 * The interactive edit tool. Allows to move, rotate, flip and change properties of items selected
 * using the pcbnew.InteractiveSelection tool.
 */

class POSITION_RELATIVE_TOOL : public PCB_TOOL
{
public:
    POSITION_RELATIVE_TOOL();

    void Reset( RESET_REASON aReason ) override;

    /// @copydoc POSITION_RELATIVE_TOOL::Init()
    bool Init() override;

    /**
     * Function PositionRelative()
     *
     * Invokes a dialog box to allow positioning of the item relative to another by an exact amount.
     */
    int PositionRelative( const TOOL_EVENT& aEvent );

    /**
     * Function SelectPositionRelativeItem()
     *
     * Invokes the picker tool to select the item to which the previos selection will be placed relative to.
     */
    int SelectPositionRelativeItem( const TOOL_EVENT& aEvent  );

    /**
     * Function RelativeItemSelectionMove()
     *
     * Positions the m_position_relative_selection selection relative to anchorpostion using the given translation and rotation.
     * Rotation is around the center of the selection.
     */
    int RelativeItemSelectionMove( wxPoint anchorposition, wxPoint translation, double rotation );

    /**
     * Function GetAnchorItem()
     *
     * Gets the last selected anchor item.
     */
    BOARD_ITEM* GetAnchorItem()
    {
        return m_anchor_item;
    }

    /**
     * Function UpdateAnchor()
     *
     * Selects the item to be used as the reference for relative move operation.
     */
    void UpdateAnchor( BOARD_ITEM* aItem );

    ///> Sets up handlers for various events.
    void SetTransitions() override;

private:
    DIALOG_POSITION_RELATIVE* m_position_relative_dialog;

    ///> Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    std::unique_ptr<BOARD_COMMIT> m_commit;

    ///> Last anchor item selected by Position Relative To function.
    BOARD_ITEM* m_anchor_item;

    ///> Translation for position relative function.
    wxPoint m_position_relative_translation;

    ///> Anchor position for positive relative function.
    wxPoint m_anchor_position;

    ///> Rotation  for positive relative function.
    double m_position_relative_rotation;

    ///> Selected that will be moved by Position relative function.
    SELECTION m_position_relative_selection;
};

#endif
