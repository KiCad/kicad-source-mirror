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
    m_rotationAngle( 900 )
    {}

    virtual ~PCB_BASE_EDIT_FRAME() {};

    /**
     * Function OnEditItemRequest
     * Install the corresponding dialog editor for the given item
     * @param aDC = the current device context
     * @param aItem = a pointer to the BOARD_ITEM to edit
     */
    virtual void OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem ) = 0;

    /**
     * Function RestoreCopyFromRedoList
     *  Redo the last edition:
     *  - Save the current data in Undo list
     *  - Get an old version of the data from Redo list
     */
    virtual void RestoreCopyFromRedoList( wxCommandEvent& aEvent ) = 0;

    /**
     * Function RestoreCopyFromUndoList
     *  Undo the last edition:
     *  - Save the current board in Redo list
     *  - Get an old version of the data from Undo list
     */
    virtual void RestoreCopyFromUndoList( wxCommandEvent& aEvent ) = 0;

    int GetRotationAngle() const { return m_rotationAngle; }
    void SetRotationAngle( int aRotationAngle );

    bool PostCommandMenuEvent( int evt_type );

protected:
    /// User defined rotation angle (in tenths of a degree).
    int m_rotationAngle;

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
     * @aIncrement increment item reference (module ref, pad number, etc,
     * if appropriate)
     */
    void duplicateItem( BOARD_ITEM* aItem, bool aIncrement );

    /**
     * Function duplicateItems
     * Find and duplicate the currently selected items
     * @param aIncrement increment item reference (module ref, pad number, etc,
     * if appropriate)
     *
     * @note The implementer should find the selected item (and do processing
     * like finding parents when relevant, and then call
     * duplicateItem(BOARD_ITEM*, bool) above
     */
    virtual void duplicateItems( bool aIncrement ) = 0;
};

#endif
