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
#include <unordered_map>
#include <unordered_set>


class SCH_EDIT_FRAME;
class SCH_SELECTION_TOOL;
class SCH_LINE;
class SCH_LABEL_BASE;
class SCH_SHEET_PIN;
class SCH_JUNCTION;
class SCH_SELECTION;
class SCH_SHEET;
class SCH_COMMIT;
class SCH_ITEM;
class EE_GRID_HELPER;

enum GRID_HELPER_GRIDS : int;


struct SPECIAL_CASE_LABEL_INFO
{
    SCH_LINE* attachedLine;
    VECTOR2I  originalLabelPos;
};


class SCH_MOVE_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    enum MOVE_MODE
    {
        MOVE,
        DRAG,
        BREAK,
        SLICE
    };

    SCH_MOVE_TOOL();
    ~SCH_MOVE_TOOL() override { }

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
    int AlignToGrid( const TOOL_EVENT& aEvent );

private:
    bool doMoveSelection( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit );

    void moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta );

    ///< Find additional items for a drag operation.
    ///< Connected items with no wire are included (as there is no wire to adjust for the drag).
    ///< Connected wires are included with any un-connected ends flagged (STARTPOINT or ENDPOINT).
    void getConnectedItems( SCH_ITEM* aOriginalItem, const VECTOR2I& aPoint, EDA_ITEMS& aList );
    void getConnectedDragItems( SCH_COMMIT* aCommit, SCH_ITEM* fixed, const VECTOR2I& selected,
                                EDA_ITEMS& aList );

    void orthoLineDrag( SCH_COMMIT* aCommit, SCH_LINE* line, const VECTOR2I& splitDelta,
                        int& xBendCount, int& yBendCount, const EE_GRID_HELPER& grid );

    void moveSelectionToSheet( SCH_SELECTION& aSelection, SCH_SHEET* aTarget, SCH_COMMIT* aCommit );

    ///< Clears the new drag lines and removes them from the screen
    void clearNewDragLines();

    ///< Set up handlers for various events.
    void setTransitions() override;

    ///< Cleanup dangling lines left after a drag
    void trimDanglingLines( SCH_COMMIT* aCommit );

    ///< Break or slice the current selection before initiating a move, if required
    void preprocessBreakOrSliceSelection( SCH_COMMIT* aCommit, const TOOL_EVENT& aEvent );

    // Helper methods for doMoveSelection refactoring
    ///< Check if a move is already in progress and handle state transitions
    bool checkMoveInProgress( const TOOL_EVENT& aEvent, SCH_COMMIT* aCommit, bool aCurrentModeIsDragLike,
                              bool aWasDragging );

    ///< Promote pin selections to parent symbols and request final selection
    SCH_SELECTION& prepareSelection( bool& aUnselect );

    ///< Refresh selection traits (sheet pins, graphic items, etc.)
    void refreshSelectionTraits( const SCH_SELECTION& aSelection, bool& aHasSheetPins,
                                 bool& aHasGraphicItems, bool& aHasNonGraphicItems,
                                 bool& aIsGraphicsOnly );

    ///< Initialize the move/drag operation, setting up flags and connections
    void initializeMoveOperation( const TOOL_EVENT& aEvent, SCH_SELECTION& aSelection, SCH_COMMIT* aCommit,
                                  std::vector<DANGLING_END_ITEM>& aInternalPoints, GRID_HELPER_GRIDS& aSnapLayer );

    ///< Setup items for drag operation, collecting connected items
    void setupItemsForDrag( SCH_SELECTION& aSelection, SCH_COMMIT* aCommit );

    ///< Setup items for move operation, marking dangling ends
    void setupItemsForMove( SCH_SELECTION& aSelection,
                            std::vector<DANGLING_END_ITEM>& aInternalPoints );

    ///< Find the target sheet for dropping items (if any)
    SCH_SHEET* findTargetSheet( const SCH_SELECTION& aSelection, const VECTOR2I& aCursorPos,
                                bool aHasSheetPins, bool aIsGraphicsOnly, bool aCtrlDown );

    ///< Perform the actual move of items by delta, handling split moves and orthogonal dragging
    void performItemMove( SCH_SELECTION& aSelection, const VECTOR2I& aDelta,
                          SCH_COMMIT* aCommit, int& aXBendCount, int& aYBendCount,
                          const EE_GRID_HELPER& aGrid );

    ///< Handle tool action events during the move operation
    bool handleMoveToolActions( const TOOL_EVENT* aEvent, SCH_COMMIT* aCommit,
                                const SCH_SELECTION& aSelection );

    ///< Update stored positions after transformations (rotation, mirroring, etc.) during move
    void updateStoredPositions( const SCH_SELECTION& aSelection );

    ///< Finalize the move operation, updating junctions and cleaning up
    void finalizeMoveOperation( SCH_SELECTION& aSelection, SCH_COMMIT* aCommit, bool aUnselect,
                                const std::vector<DANGLING_END_ITEM>& aInternalPoints );

private:
    ///< Re-entrancy guard
    bool                  m_inMoveTool;

    ///< Flag determining if anything is being dragged right now
    bool                  m_moveInProgress;
    MOVE_MODE             m_mode;

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
    OPT_VECTOR2I          m_breakPos;

    // A map of labels to scaling factors.  Used to scale the movement vector for labels that
    // are attached to wires which have only one end moving.
    std::map<SCH_LABEL_BASE*, SPECIAL_CASE_LABEL_INFO> m_specialCaseLabels;

    // A map of sheet pins to the line-endings (true == start) they're connected to.  Sheet
    // pins are constrained in their movement so their attached lines must be too.
    std::map<SCH_SHEET_PIN*, std::pair<SCH_LINE*, bool>> m_specialCaseSheetPins;
};

#endif //KICAD_SCH_MOVE_TOOL_H
