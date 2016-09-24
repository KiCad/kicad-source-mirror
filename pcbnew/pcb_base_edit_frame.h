/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef BASE_EDIT_FRAME_H
#define BASE_EDIT_FRAME_H

#include <wxBasePcbFrame.h>

class BOARD_ITEM_CONTAINER;

/**
 * Common, abstract interface for edit frames.
 */
class PCB_BASE_EDIT_FRAME : public PCB_BASE_FRAME
{
public:
    PCB_BASE_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                long aStyle, const wxString& aFrameName ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName ),
    m_rotationAngle( 900 ), m_undoRedoBlocked( false )
    {}

    virtual ~PCB_BASE_EDIT_FRAME() {};

    /**
     * Function GetModel()
     * returns the primary data model.
     */
    virtual BOARD_ITEM_CONTAINER* GetModel() const = 0;

    /**
     * Function CreateNewLibrary
     * prompts user for a library path, then creates a new footprint library at that
     * location.  If library exists, user is warned about that, and is given a chance
     * to abort the new creation, and in that case existing library is first deleted.
     *
     * @return wxString - the newly created library path if library was successfully
     *   created, else wxEmptyString because user aborted or error.
     */
    wxString CreateNewLibrary();

    /**
     * Function OnEditItemRequest
     * Install the corresponding dialog editor for the given item
     * @param aDC = the current device context
     * @param aItem = a pointer to the BOARD_ITEM to edit
     */
    virtual void OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem ) = 0;

    // Undo buffer handling

    /**
     * Function SaveCopyInUndoList
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItemToCopy = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    void SaveCopyInUndoList( BOARD_ITEM* aItemToCopy, UNDO_REDO_T aTypeCommand,
                            const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) override;

    /**
     * Function SaveCopyInUndoList
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList, UNDO_REDO_T aTypeCommand,
                            const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) override;
    /**
     * Function RestoreCopyFromRedoList
     *  Redo the last edition:
     *  - Save the current board in Undo list
     *  - Get an old version of the board from Redo list
     *  @return none
     */
    void RestoreCopyFromRedoList( wxCommandEvent& aEvent );

    /**
     * Function RestoreCopyFromUndoList
     *  Undo the last edition:
     *  - Save the current board in Redo list
     *  - Get an old version of the board from Undo list
     *  @return none
     */
    void RestoreCopyFromUndoList( wxCommandEvent& aEvent );

    /**
     * Function PutDataInPreviousState
     * Used in undo or redo command.
     * Put data pointed by List in the previous state, i.e. the state memorized by List
     * @param aList = a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
     * @param aRedoCommand = a bool: true for redo, false for undo
     * @param aRebuildRatsnet = a bool: true to rebuild ratsnest (normal use), false
     * to just retrieve last state (used in abort commands that do not need to
     * rebuild ratsnest)
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList,
                                 bool               aRedoCommand,
                                 bool               aRebuildRatsnet = true );

    /**
     * Function UndoRedoBlocked
     * Checks if the undo and redo operations are currently blocked.
     */
    bool UndoRedoBlocked() const
    {
        return m_undoRedoBlocked;
    }

    /**
     * Function UndoRedoBlock
     * Enables/disable undo and redo operations.
     */
    void UndoRedoBlock( bool aBlock = true )
    {
        m_undoRedoBlocked = aBlock;
    }

    /**
     * Function GetRotationAngle()
     * Returns the angle used for rotate operations.
     */
    int GetRotationAngle() const { return m_rotationAngle; }

    /**
     * Function SetRotationAngle()
     * Sets the angle used for rotate operations.
     */
    void SetRotationAngle( int aRotationAngle );

    bool PostCommandMenuEvent( int evt_type );

    ///> @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    void UseGalCanvas( bool aEnable ) override;

    ///> @copydoc PCB_BASE_FRAME::SetBoard()
    virtual void SetBoard( BOARD* aBoard ) override;

protected:
    /// User defined rotation angle (in tenths of a degree).
    int m_rotationAngle;

    /// Is undo/redo operation currently blocked?
    bool m_undoRedoBlocked;

    /**
     * Function createArray
     * Create an array of the selected item (invokes the dialogue)
     * This function is shared between pcbnew and modedit, as it is virtually
     * the same
     */
    void createArray();

    /**
     * Function duplicateItem
     * Duplicate the specified item
     * This function is shared between pcbnew and modedit, as it is virtually
     * the same
     * @param aItem the item to duplicate
     * @param aIncrement (has meaning only for pads in footprint editor):
     * increment pad name if appropriate
     */
    void duplicateItem( BOARD_ITEM* aItem, bool aIncrement );

    /**
     * Function duplicateItems
     * Find and duplicate the currently selected items
     * @param aIncrement (has meaning only for pads in footprint editor):
     * increment pad name if appropriate
     *
     * @note The implementer should find the selected item (and do processing
     * like finding parents when relevant, and then call
     * duplicateItem(BOARD_ITEM*, bool) above
     */
    virtual void duplicateItems( bool aIncrement ) = 0;
};

#endif
