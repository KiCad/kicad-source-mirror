/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2009 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2009 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _CLASS_UNDOREDO_CONTAINER_H
#define _CLASS_UNDOREDO_CONTAINER_H
#include <vector>

#include "base_struct.h"

/**
 * @info Undo Redo considerations:
 * Basically we have 3 cases
 *      New item
 *      Deleted item
 *      Modified item
 * there is also a specfific case in eeschema, when wires are modified
 * If an item is modified, a copy of the "old" item parameters value is held.
 * When an item is deleted or added (new item) the pointer points the item, and there is
 * no other copy.
 *  However, because there are some commands that concern a lot of items
 *  and modify them, but modifications are easy tu undo/redo,
 *  so a copy of old items is not necessary. They are block command
 *  Move block
 *  Rotate block
 *  Mirror or Flip block
 * and they are undo/redo by the same command
 */

/* Type of undo/redo operations
 * each type must be redo/undoed by a specfic operation
 */
enum UndoRedoOpType {
    UR_UNSPECIFIED = 0,     // illegal
    UR_CHANGED,             // params of items have a value changed: undo is made by exchange values with a copy of these values
    UR_NEW,                 // new item, undo by changing in deleted
    UR_DELETED,             // deleted item, undo by changing in deleted
    UR_MOVED,               // moved item, undo by move it
    UR_MIRRORED_X,          // mirrored item, undo by mirror X
    UR_MIRRORED_Y,          // mirrored item, undo by mirror Y
    UR_ROTATED,             // Rotated item (counterclockwise), undo by rotating it
    UR_ROTATED_CLOCKWISE,   // Rotated item (clockwise), undo by rotating it
    UR_FLIPPED,             // flipped (board items only), undo by flipping it
    UR_WIRE_IMAGE,          // Specific to eeschema: handle wires changes
    UR_MODEDIT,             // Specific to the module editor (modedit creates a full copy of the current module when changed)
    UR_LIBEDIT              // Specific to the component editor (libedit creates a full copy of the current component when changed)
};

class ITEM_PICKER
{
public:
    UndoRedoOpType m_UndoRedoStatus;   /* type of operation to undo/redo for this item */
    EDA_ITEM*      m_PickedItem;       /* Pointer on the schematic or board item that is concerned (picked),
                                         *  or in undo redo commands, the copy of an edited item.
                                         */
    KICAD_T        m_PickedItemType;   /* type of schematic or board item that is concerned
                                         */
    int            m_PickerFlags;      /* a copy of m_Flags member. usefull in mode/drag undo/redo commands */
    EDA_ITEM*      m_Link;             /* Pointer on an other item. Used in undo redo command
                                         * used when a duplicate exists i.e. when an item is modified,
                                         * and the copy of initial item exists (the duplicate)
                                         * m_Item points the duplicate (i.e the old copy of an active item)
                                         * and m_Link points the active item in schematic
                                         */

public:
    ITEM_PICKER( EDA_ITEM* aItem = NULL, UndoRedoOpType aUndoRedoStatus = UR_UNSPECIFIED );
};

/* Class PICKED_ITEMS_LIST
 * is a holder to handle information on schematic or board items.
 * The information held is a pointer on each item, and the command made.
 */

class PICKED_ITEMS_LIST
{
public:
    UndoRedoOpType  m_Status;               /* info about operation to undo/redo for this item. can be UR_UNSPECIFIED */
    wxPoint m_TransformPoint;               /* used to undo redo command by the same command:
                                             * we usually need to know the rotate point or the move vector
                                             */
private:
    std::vector <ITEM_PICKER> m_ItemsList;

public:
    PICKED_ITEMS_LIST();
    ~PICKED_ITEMS_LIST();

    /** PushItem
     * push a picker to the top of the list
     * @param aItem = picker to push
     */
    void        PushItem( ITEM_PICKER& aItem );

    /** PopItem
     * @return the picker from the top of the list
     * the picker is removed from the list
     */
    ITEM_PICKER PopItem();

    /**
     * Function ClearItemsList
     * delete only the list of pickers, NOT the picked data itself
     */
    void        ClearItemsList();

    /**
     * Function ClearListAndDeleteItems
     * delete the list of pickers, AND the data pointed
     * by m_PickedItem or m_PickedItemLink, according to the type of undo/redo command recorded
     */
    void        ClearListAndDeleteItems();

    /**
     * Function GetCount
     * @return the count of pickers stored in this list
     */
    unsigned        GetCount() const
    {
        return m_ItemsList.size();
    }

    /**
     * Function ReversePickersListOrder
     * reverses the order of pickers stored in this list
     * Useful when pop a list from Undo to Redo (and vice-versa)
     * because sometimes undo (or redo) a command needs to keep the
     * order of successive changes.
     * and obviously, undo and redo are in reverse order
     */
    void        ReversePickersListOrder();


    /**
     * Function GetItemWrapper
     * @return the picker of a picked item
     * @param aIdx = index of the picker in the picked list
     * if this picker does not exist, a picker is returned,
     * with its members set to 0 or NULL
     */
    ITEM_PICKER     GetItemWrapper( unsigned int aIdx );

    /**
     * Function GetPickedItem
     * @return a pointer to the picked item
     * @param aIdx = index of the picked item in the picked list
     */
    EDA_ITEM* GetPickedItem( unsigned int aIdx );

    /**
     * Function GetPickedItemLink
     * @return link of the picked item, or null if does not exist
     * @param aIdx = index of the picked item in the picked list
     */
    EDA_ITEM* GetPickedItemLink( unsigned int aIdx );

    /**
     * Function GetPickedItemStatus
     * @return the type of undo/redo opertaion associated to the picked item,
     *   or UR_UNSPECIFIED if does not exist
     * @param aIdx = index of the picked item in the picked list
     */
    UndoRedoOpType  GetPickedItemStatus( unsigned int aIdx );

    /**
     * Function GetPickerFlags
     * return the value of the picker flag
      * @param aIdx = index of the picker in the picked list
     * @return the value stored in the picker, if the picker exists, or 0 if does not exist
     */
    int            GetPickerFlags( unsigned aIdx );

    /**
     * Function SetPickedItem
     * @param aItem = a pointer to the item to pick
     * @param aIdx = index of the picker in the picked list
     * @return true if the pixker exists, or false if does not exist
     */
    bool            SetPickedItem( EDA_ITEM* aItem, unsigned aIdx );

    /**
     * Function SetPickedItem
     * @param aItem = a pointer to the item to pick
     * @param aStatus = the type of undo/redo operation associated to the item to pick
     * @param aIdx = index of the picker in the picked list
     * @return true if the pixker exists, or false if does not exist
     */
    bool            SetPickedItem( EDA_ITEM* aItem, UndoRedoOpType aStatus, unsigned aIdx );

    /**
     * Function SetPickedItemLink
     * Set the link associated to a given picked item
     * @param aLink = the link to the item associated to the picked item
     * @param aIdx = index of the picker in the picked list
     * @return true if the pixker exists, or false if does not exist
     */
    bool            SetPickedItemLink( EDA_ITEM* aLink, unsigned aIdx );

    /**
     * Function SetPickedItemStatus
     * Set the type of undo/redo operation for a given picked item
     * @param aStatus = the type of undo/redo operation associated to the picked item
     * @param aIdx = index of the picker in the picked list
     * @return true if the picker exists, or false if does not exist
     */
    bool            SetPickedItemStatus( UndoRedoOpType aStatus, unsigned aIdx );

    /**
     * Function SetPickerFlags
     * Set the flags of the picker (usually to the picked item m_Flags value)
     * @param aFlags = the value to save in picker
     * @param aIdx = index of the picker in the picked list
     * @return true if the picker exists, or false if does not exist
     */
    bool            SetPickerFlags( int aFlags, unsigned aIdx );

    /**
     * Function RemovePicker
     * remove one entry (one picker) from the list of picked items
     * @param aIdx = index of the picker in the picked list
     * @return true if ok, or false if did not exist
     */
    bool            RemovePicker( unsigned aIdx );

    /**
     * Function CopyList
     * copy all data from aSource
     * Items picked are not copied. just pointer on them are copied
     */
    void            CopyList( const PICKED_ITEMS_LIST& aSource );
};

/**
 * Class UNDO_REDO_CONTAINER
 * is a holder to handle alist of undo (or redo) command.
 * this class handles a list of ITEM_PICKER (each manage one schematic or board item).
 */
class UNDO_REDO_CONTAINER
{
public:
    std::vector <PICKED_ITEMS_LIST*> m_CommandsList;         // the list of possible undo/redo commands

public:

    UNDO_REDO_CONTAINER();
    ~UNDO_REDO_CONTAINER();
    void               PushCommand( PICKED_ITEMS_LIST* aCommand );
    PICKED_ITEMS_LIST* PopCommand();
    void               ClearCommandList();
};


#endif      // _CLASS_UNDOREDO_CONTAINER_H
