/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SCH_MOVE_TOOL_H
#define KICAD_SCH_MOVE_TOOL_H

#include <tools/ee_tool_base.h>
#include <sch_base_frame.h>


class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


class SCH_MOVE_TOOL : public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_MOVE_TOOL();
    ~SCH_MOVE_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Function Main()
     *
     * Runs an interactive move of the selected items, or the item under the cursor.
     */
    int Main( const TOOL_EVENT& aEvent );

private:
    void moveItem( EDA_ITEM* aItem, VECTOR2I aDelta, bool isDrag );

    ///> Finds additional items for a drag operation.
    ///> Connected items with no wire are included (as there is no wire to adjust for the drag).
    ///> Connected wires are included with any un-connected ends flagged (STARTPOINT or ENDPOINT).
    void getConnectedDragItems( SCH_ITEM* aOriginalItem, wxPoint aPoint, EDA_ITEMS& aList );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    ///> Flag determining if anything is being dragged right now
    bool                  m_moveInProgress;
    bool                  m_isDragOperation;

    ///> Items (such as wires) which were added to the selection for a drag
    EDA_ITEMS             m_dragAdditions;

    ///> Used for chaining commands
    VECTOR2I              m_moveOffset;

    ///> Last cursor position (needed for getModificationPoint() to avoid changes
    ///> of edit reference point).
    VECTOR2I              m_cursor;

    boost::optional<VECTOR2I> m_anchorPos;
};

#endif //KICAD_SCH_MOVE_TOOL_H
