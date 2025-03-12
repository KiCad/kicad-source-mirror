/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef SYMBOL_EDITOR_MOVE_TOOL_H
#define SYMBOL_EDITOR_MOVE_TOOL_H

#include <tools/sch_tool_base.h>
#include <symbol_edit_frame.h>

class SYMBOL_EDIT_FRAME;
class SCH_SELECTION_TOOL;


class SYMBOL_EDITOR_MOVE_TOOL : public SCH_TOOL_BASE<SYMBOL_EDIT_FRAME>
{
public:
    SYMBOL_EDITOR_MOVE_TOOL();
    ~SYMBOL_EDITOR_MOVE_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Run an interactive move of the selected items, or the item under the cursor.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Align selected elements to the grid.
     *
     * @param aEvent current event that activated the tool
     * @return 0
     */
    int AlignElements( const TOOL_EVENT& aEvent );

private:
    bool doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit );

    void moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    bool        m_moveInProgress;

    ///< Last cursor position (needed for getModificationPoint() to avoid changes
    ///< of edit reference point).
    VECTOR2I    m_cursor;
    VECTOR2I    m_anchorPos;
};

#endif // SYMBOL_EDITOR_MOVE_TOOL_H
