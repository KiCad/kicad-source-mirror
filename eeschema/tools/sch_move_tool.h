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

#ifndef KICAD_SCH_MOVE_TOOL_H
#define KICAD_SCH_MOVE_TOOL_H

#include <tools/sch_tool_base.h>
#include <sch_base_frame.h>


class SCH_EDIT_FRAME;
class SCH_SELECTION_TOOL;
class SCH_LINE;
class SCH_LABEL_BASE;
class SCH_SHEET_PIN;
class SCH_JUNCTION;


struct SPECIAL_CASE_LABEL_INFO
{
    SCH_LINE* attachedLine;
    VECTOR2I  originalLabelPos;
};


class SCH_MOVE_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_MOVE_TOOL();
    ~SCH_MOVE_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

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
    int AlignToGrid( const TOOL_EVENT& aEvent );

private:
    bool doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit, bool aIsSlice );

    void moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta );

    ///< Find additional items for a drag operation.
    ///< Connected items with no wire are included (as there is no wire to adjust for the drag).
    ///< Connected wires are included with any un-connected ends flagged (STARTPOINT or ENDPOINT).
    void getConnectedItems( SCH_ITEM* aOriginalItem, const VECTOR2I& aPoint, EDA_ITEMS& aList );
    void getConnectedDragItems( SCH_COMMIT* aCommit, SCH_ITEM* fixed, const VECTOR2I& selected,
                                EDA_ITEMS& aList );

    void orthoLineDrag( SCH_COMMIT* aCommit, SCH_LINE* line, const VECTOR2I& splitDelta,
                        int& xBendCount, int& yBendCount, const EE_GRID_HELPER& grid );

    ///< Clears the new drag lines and removes them from the screen
    void clearNewDragLines();

    ///< Set up handlers for various events.
    void setTransitions() override;

    ///< Cleanup dangling lines left after a drag
    void trimDanglingLines( SCH_COMMIT* aCommit );

private:
    ///< Re-entrancy guard
    bool                  m_inMoveTool;

    ///< Flag determining if anything is being dragged right now
    bool                  m_moveInProgress;
    bool                  m_isDrag;

    ///< Items (such as wires) which were added to the selection for a drag
    std::vector<KIID>                   m_dragAdditions;
    ///< Cache of the line's original connections before dragging started
    std::map<SCH_LINE*, EDA_ITEMS>      m_lineConnectionCache;
    ///< Lines added at bend points dynamically during the move
    std::unordered_set<SCH_LINE*>       m_newDragLines;
    ///< Lines changed by drag algorithm that weren't selected
    std::unordered_set<SCH_LINE*>       m_changedDragLines;
    ///< Junctions that were hidden during the move
    std::vector<SCH_JUNCTION*>           m_hiddenJunctions;

    VECTOR2I              m_moveOffset;

    ///< Last cursor position (needed for getModificationPoint() to avoid changes
    ///< of edit reference point).
    VECTOR2I              m_cursor;

    OPT_VECTOR2I          m_anchorPos;

    // A map of labels to scaling factors.  Used to scale the movement vector for labels that
    // are attached to wires which have only one end moving.
    std::map<SCH_LABEL_BASE*, SPECIAL_CASE_LABEL_INFO> m_specialCaseLabels;

    // A map of sheet pins to the line-endings (true == start) they're connected to.  Sheet
    // pins are constrained in their movement so their attached lines must be too.
    std::map<SCH_SHEET_PIN*, std::pair<SCH_LINE*, bool>> m_specialCaseSheetPins;
};

#endif //KICAD_SCH_MOVE_TOOL_H
