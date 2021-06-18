/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <pcb_base_frame.h>

class APPEARANCE_CONTROLS;
class BOARD_ITEM_CONTAINER;
class PANEL_SELECTION_FILTER;

/**
 * Common, abstract interface for edit frames.
 */
class PCB_BASE_EDIT_FRAME : public PCB_BASE_FRAME
{
public:
    PCB_BASE_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                         const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                         long aStyle, const wxString& aFrameName );

    virtual ~PCB_BASE_EDIT_FRAME();

    bool TryBefore( wxEvent& aEvent ) override;

    void doCloseWindow() override;

    /**
     * If a library name is given, creates a new footprint library in the project folder
     * with the given name. If no library name is given it prompts user for a library path,
     * then creates a new footprint library at that location.
     * If library exists, user is warned about that, and is given a chance
     * to abort the new creation, and in that case existing library is first deleted.
     *
     * @param aProposedName is the initial path and filename shown in the file chooser dialog.
     * @return The newly created library path if library was successfully created, else
     *         wxEmptyString because user aborted or error.
     */
    wxString CreateNewLibrary( const wxString& aLibName = wxEmptyString,
                               const wxString& aProposedName = wxEmptyString );

    wxString CreateNewProjectLibrary( const wxString& aLibName = wxEmptyString,
                                      const wxString& aProposedName = wxEmptyString );

    /**
     * Add an existing library to either the global or project library table.
     *
     * @param aFileName the library to add; a file open dialog will be displayed if empty.
     * @return true if successfully added.
     */
    bool AddLibrary( const wxString& aLibName = wxEmptyString, FP_LIB_TABLE* aTable = nullptr );

    /**
     * Install the corresponding dialog editor for the given item.
     *
     * @param aDC the current device context.
     * @param aItem a pointer to the BOARD_ITEM to edit.
     */
    virtual void OnEditItemRequest( BOARD_ITEM* aItem ) = 0;

    /**
     * Create a new entry in undo list of commands.
     *
     * Add a picker to handle \a  aItemToCopy.
     *
     * @param aItemToCopy the board item modified by the command to undo.
     * @param aTypeCommand command type (see enum UNDO_REDO).
     */
    void SaveCopyInUndoList( EDA_ITEM* aItemToCopy, UNDO_REDO aTypeCommand ) override;

    /**
     * Create a new entry in undo list of commands.
     *
     * Add a list of pickers to handle a list of items.
     *
     * @param aItemsList the list of items modified by the command to undo.
     * @param aCommandType command type (see enum UNDO_REDO).
     */
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList, UNDO_REDO aCommandType ) override;

    /**
     * Redo the last edit:
     *  - Save the current board in Undo list
     *  - Get an old version of the board from Redo list
     */
    void RestoreCopyFromRedoList( wxCommandEvent& aEvent );

    /**
     * Undo the last edit:
     *  - Save the current board in Redo list
     *  - Get an old version of the board from Undo list
     */
    void RestoreCopyFromUndoList( wxCommandEvent& aEvent );

    /**
     * Perform an undo of the last edit **without** logging a corresponding redo.  Used to cancel
     * an in-progress operation.
     */
    void RollbackFromUndo();

    /**
     * Used in undo or redo command.
     *
     * Put data pointed by List in the previous state, i.e. the state memorized by \a aList.
     *
     * @param aList a PICKED_ITEMS_LIST pointer to the list of items to undo/redo.
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList );

    /**
     * Check if the undo and redo operations are currently blocked.
     */
    bool UndoRedoBlocked() const
    {
        return m_undoRedoBlocked;
    }

    /**
     * Enable/disable undo and redo operations.
     */
    void UndoRedoBlock( bool aBlock = true )
    {
        m_undoRedoBlocked = aBlock;
    }

    /**
     * Override this function in the PCB_BASE_EDIT_FRAME to refill the layer widget
     *
     * @param aVisible true if the grid must be shown.
     */
    void SetGridVisibility( bool aVisible ) override;

    void SetObjectVisible( GAL_LAYER_ID aLayer, bool aVisible = true );

    /**
     * Return the angle used for rotate operations.
     */
    int GetRotationAngle() const { return m_rotationAngle; }

    /**
     * Set the angle used for rotate operations.
     */
    void SetRotationAngle( int aRotationAngle );

    void ShowTextPropertiesDialog( BOARD_ITEM* aText );
    void ShowGraphicItemPropertiesDialog( BOARD_ITEM* aItem );

    ///< @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    void ActivateGalCanvas() override;

    ///< @copydoc PCB_BASE_FRAME::SetBoard()
    virtual void SetBoard( BOARD* aBoard ) override;

    COLOR_SETTINGS* GetColorSettings() const override;

    /* full undo redo management : */

    // use EDA_BASE_FRAME::ClearUndoRedoList()
    // use EDA_BASE_FRAME::PushCommandToUndoList( PICKED_ITEMS_LIST* aItem )
    // use EDA_BASE_FRAME::PushCommandToRedoList( PICKED_ITEMS_LIST* aItem )

    /**
     * Free the undo or redo list from List element.
     *
     * Wrappers are deleted.  Data pointed by wrappers are deleted if not in use in schematic
     * i.e. when they are copy of a schematic item or they are no more in use (DELETED).
     * Items are removed from the beginning of the list so this function can be called to
     * remove old commands.
     *
     * @param whichList the #UNDO_REDO_CONTAINER to clear.
     * @param aItemCount the count of items to remove. < 0 for all items.
     */
    void ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount = -1 ) override;

    /**
     * Return the absolute path to the design rules file for the currently-loaded board.
     *
     * @note There is no guarantee that this file actually exists and can be opened!  It only
     *       makes sense from PcbNew but is needed in #PCB_BASE_EDIT_FRAME::SetBoard.
     */
    wxString GetDesignRulesPath();

    APPEARANCE_CONTROLS* GetAppearancePanel() { return m_appearancePanel; }

    /**
     * Notify the frame that the layer Alpha is being changed
     */
    virtual void OnLayerAlphaChanged();

protected:
    /**
     * Prompts a user to select global or project library tables
     *
     * @return Pointer to library table selected or nullptr if none selected/canceled
     */
    FP_LIB_TABLE* selectLibTable( bool aOptional = false );

    /**
     * Create a new library in the given table (presumed to be either the global or project
     * library table).
     */
    wxString createNewLibrary( const wxString& aLibName, const wxString& aProposedName,
                               FP_LIB_TABLE* aTable );

    void handleActivateEvent( wxActivateEvent& aEvent ) override;

    void unitsChangeRefresh() override;

protected:
    int                     m_rotationAngle;        // Rotation step (in tenths of a degree)
    bool                    m_undoRedoBlocked;

    PANEL_SELECTION_FILTER* m_selectionFilterPanel;
    APPEARANCE_CONTROLS*    m_appearancePanel;
};

#endif
