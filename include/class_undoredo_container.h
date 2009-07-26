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
    UR_ROTATED,             // Rotated item, undo by rotating it
    UR_FLIPPED,             // flipped (board items only), undo by flipping it
    UR_WIRE_IMAGE           // Specific to eeschema: handle wires changes
};

class ITEM_PICKER
{
public:
    UndoRedoOpType  m_UndoRedoStatus;   /* type of operation to undo/redo for this item */
    EDA_BaseStruct* m_Item;             /* Pointer on the schematic or board item that is concerned,
                                         *  or in undo redo commands, the copy of an edited item.
                                         */
    EDA_BaseStruct* m_Link;             /* Pointer on an other item. Used in undo redo command
                                         * used when a duplicate exists i.e. when an item is modified,
                                         * and the copy of initial item exists (the duplicate)
                                         * m_Item points the duplicate (i.e the old copy of an active item)
                                         * and m_Link points the active item in schematic
                                         */

public:
    ITEM_PICKER( EDA_BaseStruct* aItem = NULL, UndoRedoOpType aUndoRedoStatus = UR_UNSPECIFIED )
    {
        m_UndoRedoStatus = aUndoRedoStatus;
        m_Item = aItem;
        m_Link = NULL;
    }
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
    void        PushItem( ITEM_PICKER& aItem );
    ITEM_PICKER PopItem();

    /** Function ClearItemsList
     * delete only the list of EDA_BaseStruct * pointers, NOT the pointed data itself
     */
    void        ClearItemsList();

    /** Function ClearListAndDeleteItems
     * delete only the list of EDA_BaseStruct * pointers, AND the data pinted by m_Item
     */
    void        ClearListAndDeleteItems();

    unsigned        GetCount() const
    {
        return m_ItemsList.size();
    }


    ITEM_PICKER     GetItemWrapper( unsigned int aIdx );
    EDA_BaseStruct* GetItemData( unsigned int aIdx );
    EDA_BaseStruct* GetImage( unsigned int aIdx );
    UndoRedoOpType  GetItemStatus( unsigned int aIdx );
    bool            SetItem( EDA_BaseStruct* aItem, unsigned aIdx );
    bool            SetItem( EDA_BaseStruct* aItem, UndoRedoOpType aStatus, unsigned aIdx );
    bool            SetLink( EDA_BaseStruct* aItem, unsigned aIdx );
    bool            SetItemStatus( UndoRedoOpType aStatus, unsigned aIdx );
    bool            RemoveItem( unsigned aIdx );

    /** Function CopyList
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
