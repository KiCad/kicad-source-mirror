/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2009 KiCad Developers, see change_log.txt for contributors.
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

#include <base_struct.h>


class PICKED_ITEMS_LIST;


/**
 * Undo Redo considerations:
 * Basically we have 3 cases
 *      New item
 *      Deleted item
 *      Modified item
 * there is also a specific case in Eeschema, when wires are modified
 * If an item is modified, a copy of the "old" item parameters value is held.
 * When an item is deleted or added (new item) the pointer points the item, and there is
 * no other copy.
 */


/* Type of undo/redo operations
 * each type must be redo/undone by a specific operation
 */
enum UNDO_REDO_T {
    UR_UNSPECIFIED = 0,     // illegal
    UR_CHANGED,             // params of items have a value changed: undo is made by exchange
                            // values with a copy of these values
    UR_NEW,                 // new item, undo by changing in deleted
    UR_DELETED,             // deleted item, undo by changing in deleted
    UR_MOVED,               // moved item, undo by move it
    UR_MIRRORED_X,          // mirrored item, undo by mirror X
    UR_MIRRORED_Y,          // mirrored item, undo by mirror Y
    UR_ROTATED,             // Rotated item (counterclockwise), undo by rotating it
    UR_ROTATED_CLOCKWISE,   // Rotated item (clockwise), undo by rotating it
    UR_FLIPPED,             // flipped (board items only), undo by flipping it
    UR_LIBEDIT,             // Specific to the component editor (libedit creates a full copy
                            // of the current component when changed)
    UR_LIB_RENAME,          // As UR_LIBEDIT, but old copy should be removed from library
    UR_EXCHANGE_T,          // Use for changing the schematic text type where swapping
                            // data structure is insufficient to restore the change.
    UR_DRILLORIGIN,         // origin changed (like UR_CHANGED, contains the origin and a copy)
    UR_GRIDORIGIN,          // origin changed (like UR_CHANGED, contains the origin and a copy)
    UR_PAGESETTINGS         // page settings or title block changes
};


class ITEM_PICKER
{
private:
    STATUS_FLAGS   m_pickerFlags;      /* a copy of m_Flags member. useful in mode/drag
                                        * undo/redo commands */
    UNDO_REDO_T    m_undoRedoStatus;   /* type of operation to undo/redo for this item */
    EDA_ITEM*      m_pickedItem;       /* Pointer on the schematic or board item that is concerned
                                        * (picked), or in undo redo commands, the copy of an
                                        * edited item. */
    KICAD_T        m_pickedItemType;   /* type of schematic or board item that is concerned */

    EDA_ITEM*      m_link;             /* Pointer on another item. Used in undo redo command
                                        * used when a duplicate exists i.e. when an item is
                                        * modified, and the copy of initial item exists (the
                                        * duplicate) m_Item points the duplicate (i.e the old
                                        * copy of an active item) and m_Link points the active
                                        * item in schematic */

public:
    ITEM_PICKER( EDA_ITEM* aItem = NULL, UNDO_REDO_T aUndoRedoStatus = UR_UNSPECIFIED );

    EDA_ITEM* GetItem() const { return m_pickedItem; }

    void SetItem( EDA_ITEM* aItem )
    {
        m_pickedItem = aItem;
        m_pickedItemType = aItem ? aItem->Type() : TYPE_NOT_INIT;
    }

    KICAD_T GetItemType() const { return m_pickedItemType; }

    void SetStatus( UNDO_REDO_T aStatus ) { m_undoRedoStatus = aStatus; }

    UNDO_REDO_T GetStatus() const { return m_undoRedoStatus; }

    void SetFlags( STATUS_FLAGS aFlags ) { m_pickerFlags = aFlags; }

    STATUS_FLAGS GetFlags() const { return m_pickerFlags; }

    void SetLink( EDA_ITEM* aItem ) { m_link = aItem; }

    EDA_ITEM* GetLink() const { return m_link; }
};


/**
 * Class PICKED_ITEMS_LIST
 * is a holder to handle information on schematic or board items.
 * The information held is a pointer on each item, and the command made.
 */
class PICKED_ITEMS_LIST
{
public:
    UNDO_REDO_T m_Status;      /* info about operation to undo/redo for this item. can be
                                   * UR_UNSPECIFIED */
    wxPoint m_TransformPoint;     /* used to undo redo command by the same command: usually
                                   * need to know the rotate point or the move vector */

private:
    std::vector <ITEM_PICKER> m_ItemsList;

public:
    PICKED_ITEMS_LIST();
    ~PICKED_ITEMS_LIST();

    /**
     * Function PushItem
     * pushes \a aItem to the top of the list
     * @param aItem Picker to push on to the list.
     */
    void PushItem( const ITEM_PICKER& aItem );

    /**
     * Function PopItem
     * @return The picker removed from the top of the list.
     */
    ITEM_PICKER PopItem();

    /**
     * Function IsItemInList
     * @return True if \a aItem is found in the pick list.
     */
    bool ContainsItem( const EDA_ITEM* aItem ) const;

    /**
     * Function FindItem
     * @return Index of the searched item. If the item is not stored in the list, negative value
     * is returned.
     */
    int FindItem( const EDA_ITEM* aItem ) const;

    /**
     * Function ClearItemsList
     * deletes only the list of pickers, NOT the picked data itself.
     */
    void ClearItemsList();

    /**
     * Function ClearListAndDeleteItems
     * deletes the list of pickers, AND the data pointed by m_PickedItem or
     * m_PickedItemLink, according to the type of undo/redo command recorded
     */
    void ClearListAndDeleteItems();

    /**
     * Function GetCount
     * @return The count of pickers stored in this list.
     */
    unsigned GetCount() const
    {
        return m_ItemsList.size();
    }

    /**
     * Function ReversePickersListOrder
     * reverses the order of pickers stored in this list.
     * <p>
     * This is useful when pop a list from Undo to Redo (and vice-versa) because
     * sometimes undo (or redo) a command needs to keep the order of successive
     * changes.  Obviously, undo and redo are in reverse order
     */
    void ReversePickersListOrder();

    /**
     * Function GetItemWrapper
     * @return The picker of a picked item.
     * @param aIdx Index of the picker in the picked list
     * if this picker does not exist, a picker is returned,
     * with its members set to 0 or NULL
     */
    ITEM_PICKER GetItemWrapper( unsigned int aIdx ) const;

    /**
     * Function GetPickedItem
     * @return A pointer to the picked item
     * @param aIdx Index of the picked item in the picked list
     */
    EDA_ITEM* GetPickedItem( unsigned int aIdx ) const;

    /**
     * Function GetPickedItemLink
     * @return link of the picked item, or null if does not exist
     * @param aIdx Index of the picked item in the picked list
     */
    EDA_ITEM* GetPickedItemLink( unsigned int aIdx ) const;

    /**
     * Function GetPickedItemStatus
     * @return The type of undo/redo operation associated to the picked item,
     *          or UR_UNSPECIFIED if does not exist
     * @param aIdx Index of the picked item in the picked list
     */
    UNDO_REDO_T GetPickedItemStatus( unsigned int aIdx ) const;

    /**
     * Function GetPickerFlags
     * returns the value of the picker flag.
     * @param aIdx Index of the picker in the picked list
     * @return The value stored in the picker, if the picker exists, or 0 if does not exist
     */
    STATUS_FLAGS GetPickerFlags( unsigned aIdx ) const;

    /**
     * Function SetPickedItem
     * @param aItem A pointer to the item to pick
     * @param aIdx Index of the picker in the picked list
     * @return True if the picker exists or false if does not exist
     */
    bool SetPickedItem( EDA_ITEM* aItem, unsigned aIdx );

    /**
     * Function SetPickedItem
     * @param aItem A pointer to the item to pick
     * @param aStatus The type of undo/redo operation associated to the item to pick
     * @param aIdx Index of the picker in the picked list
     * @return True if the picker exists or false if does not exist
     */
    bool SetPickedItem( EDA_ITEM* aItem, UNDO_REDO_T aStatus, unsigned aIdx );

    /**
     * Function SetPickedItemLink
     * set the link associated to a given picked item.
     * @param aLink = the link to the item associated to the picked item
     * @param aIdx = index of the picker in the picked list
     * @return true if the picker exists, or false if does not exist
     */
    bool SetPickedItemLink( EDA_ITEM* aLink, unsigned aIdx );

    /**
     * Function SetPickedItemStatus
     * sets the type of undo/redo operation for a given picked item.
     * @param aStatus The type of undo/redo operation associated to the picked item
     * @param aIdx Index of the picker in the picked list
     * @return True if the picker exists or false if does not exist
     */
    bool SetPickedItemStatus( UNDO_REDO_T aStatus, unsigned aIdx );

    /**
     * Function SetPickerFlags
     * set the flags of the picker (usually to the picked item m_Flags value)
     * @param aFlags The flag value to save in picker
     * @param aIdx Index of the picker in the picked list
     * @return True if the picker exists or false if does not exist
     */
    bool SetPickerFlags( STATUS_FLAGS aFlags, unsigned aIdx );

    /**
     * Function RemovePicker
     * removes one entry (one picker) from the list of picked items.
     * @param aIdx Index of the picker in the picked list
     * @return True if ok or false if did not exist
     */
    bool RemovePicker( unsigned aIdx );

    /**
     * Function CopyList
     * copies all data from aSource to the list.
     * Items picked are not copied. just pointer in them are copied
     * @param aSource The list of items to copy to the list.
     */
    void CopyList( const PICKED_ITEMS_LIST& aSource );
};


/**
 * Class UNDO_REDO_CONTAINER
 * is a holder to handle alist of undo (or redo) command.
 * this class handles a list of ITEM_PICKER (each manage one schematic or board item).
 */
class UNDO_REDO_CONTAINER
{
public:
    std::vector <PICKED_ITEMS_LIST*> m_CommandsList;   // the list of possible undo/redo commands

public:

    UNDO_REDO_CONTAINER();
    ~UNDO_REDO_CONTAINER();

    void PushCommand( PICKED_ITEMS_LIST* aCommand );

    PICKED_ITEMS_LIST* PopCommand();

    void ClearCommandList();
};


#endif      // _CLASS_UNDOREDO_CONTAINER_H
