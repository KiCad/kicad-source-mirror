/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _CLASS_UNDOREDO_CONTAINER_H
#define _CLASS_UNDOREDO_CONTAINER_H

#include <core/typeinfo.h>
#include <eda_item_flags.h>
#include <functional>
#include <kiid.h>
#include <vector>
#include <wx/string.h>

class EDA_ITEM;
class PICKED_ITEMS_LIST;
class BASE_SCREEN;


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


/**
 * Type of undo/redo operations.
 *
 * Each type must be redo/undone by a specific operation.
 */
enum class UNDO_REDO {
    UNSPECIFIED = 0,    // illegal
    CHANGED,            // params of items have a value changed: undo is made by exchange
                        // values with a copy of these values
    NEWITEM,            // new item, undo by changing in deleted
    DELETED,            // deleted item, undo by changing in deleted
    LIBEDIT,            // Specific to the component editor (symbol_editor creates a full copy
                        // of the current component when changed)
    LIB_RENAME,         // As LIBEDIT, but old copy should be removed from library
    DRILLORIGIN,        // origin changed (like CHANGED, contains the origin and a copy)
    GRIDORIGIN,         // origin changed (like CHANGED, contains the origin and a copy)
    PAGESETTINGS,       // page settings or title block changes
    REPEAT_ITEM         // storage entry for the editor's global repeatItems list
};


class ITEM_PICKER
{
public:
    ITEM_PICKER();
    ITEM_PICKER( BASE_SCREEN* aScreen, EDA_ITEM* aItem,
                 UNDO_REDO aStatus = UNDO_REDO::UNSPECIFIED );

    EDA_ITEM* GetItem() const { return m_pickedItem; }
    void SetItem( EDA_ITEM* aItem );

    void SetStatus( UNDO_REDO aStatus ) { m_undoRedoStatus = aStatus; }
    UNDO_REDO GetStatus() const { return m_undoRedoStatus; }

    void SetFlags( EDA_ITEM_FLAGS aFlags ) { m_pickerFlags = aFlags; }
    EDA_ITEM_FLAGS GetFlags() const { return m_pickerFlags; }

    void SetLink( EDA_ITEM* aItem );
    EDA_ITEM* GetLink() const { return m_link; }

    KIID GetGroupId() const { return m_groupId; }
    KIID_VECT_LIST GetGroupMembers() const { return m_groupMembers; }

    BASE_SCREEN* GetScreen() const { return m_screen; }

private:
    EDA_ITEM_FLAGS m_pickerFlags;      /* A copy of m_flags member. Currently used only to flag
                                        * transient items. */
    UNDO_REDO      m_undoRedoStatus;   /* type of operation to undo/redo for this item */
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
    KIID           m_groupId;          /* Id of the parent group */
    KIID_VECT_LIST m_groupMembers;     /* Ids of the members of a group */

    BASE_SCREEN*   m_screen;           /* For new and deleted items the screen the item should
                                        * be added to/removed from. */

};


/**
 * A holder to handle information on schematic or board items.
 *
 * The information held is a pointer on each item, and the command made.
 */
class PICKED_ITEMS_LIST
{
public:
    PICKED_ITEMS_LIST();
    ~PICKED_ITEMS_LIST();

    /**
     * Push \a aItem to the top of the list.
     *
     * @param aItem Picker to push on to the list.
     */
    void PushItem( const ITEM_PICKER& aItem );

    /**
     * @return The picker removed from the top of the list.
     */
    ITEM_PICKER PopItem();

    /**
     * @return True if \a aItem is found in the pick list.
     */
    bool ContainsItem( const EDA_ITEM* aItem ) const;

    /**
     * @return Index of the searched item. If the item is not stored in the list, negative value
     *         is returned.
     */
    int FindItem( const EDA_ITEM* aItem ) const;

    /**
     * Delete only the list of pickers NOT the picked data itself.
     */
    void ClearItemsList();

    /**
     * Delete the list of pickers AND the data pointed by #m_PickedItem or #m_PickedItemLink
     * according to the type of undo/redo command recorded.
     */
    void ClearListAndDeleteItems( std::function<void(EDA_ITEM*)> aItemDeleter );

    /**
     * @return The count of pickers stored in this list.
     */
    unsigned GetCount() const
    {
        return m_ItemsList.size();
    }

    /**
     * Reverse the order of pickers stored in this list.
     *
     * This is useful when pop a list from Undo to Redo (and vice-versa) because
     * sometimes undo (or redo) a command needs to keep the order of successive
     * changes.  Obviously, undo and redo are in reverse order
     */
    void ReversePickersListOrder();

    /**
     * @return The picker of a picked item.
     * @param aIdx Index of the picker in the picked list if this picker does not exist,
     *             a picker is returned, with its members set to 0 or NULL.
     */
    const ITEM_PICKER& GetItemWrapper( unsigned int aIdx ) const;
    ITEM_PICKER& GetItemWrapper( unsigned int aIdx );

    /**
     * @return A pointer to the picked item.
     * @param aIdx Index of the picked item in the picked list.
     */
    EDA_ITEM* GetPickedItem( unsigned int aIdx ) const;

    /**
     * @return A pointer to the picked item's screen.
     * @param aIdx Index of the picked item in the picked list.
     */
    BASE_SCREEN* GetScreenForItem( unsigned int aIdx ) const;

    /**
     * @return link of the picked item, or null if does not exist.
     * @param aIdx Index of the picked item in the picked list.
     */
    EDA_ITEM* GetPickedItemLink( unsigned int aIdx ) const;

    /**
     * @return The type of undo/redo operation associated to the picked item,
     *          or UNSPECIFIED if does not exist.
     * @param aIdx Index of the picked item in the picked list.
     */
    UNDO_REDO GetPickedItemStatus( unsigned int aIdx ) const;

    /**
     * Return the value of the picker flag.
     *
     * @param aIdx Index of the picker in the picked list.
     * @return The value stored in the picker, if the picker exists, or 0 if does not exist.
     */
    EDA_ITEM_FLAGS GetPickerFlags( unsigned aIdx ) const;

    /**
     * @param aItem A pointer to the item to pick.
     * @param aIdx Index of the picker in the picked list.
     * @return True if the picker exists or false if does not exist.
     */
    bool SetPickedItem( EDA_ITEM* aItem, unsigned aIdx );

    /**
     * Set the link associated to a given picked item.
     *
     * @param aLink is the link to the item associated to the picked item.
     * @param aIdx is index of the picker in the picked list.
     * @return true if the picker exists, or false if does not exist.
     */
    bool SetPickedItemLink( EDA_ITEM* aLink, unsigned aIdx );

    /**
     * Set the type of undo/redo operation for a given picked item.
     *
     * @param aStatus The type of undo/redo operation associated to the picked item
     * @param aIdx Index of the picker in the picked list
     * @return True if the picker exists or false if does not exist
     */
    bool SetPickedItemStatus( UNDO_REDO aStatus, unsigned aIdx );

    /**
     * Set the flags of the picker (usually to the picked item m_flags value).
     *
     * @param aFlags The flag value to save in picker.
     * @param aIdx Index of the picker in the picked list.
     * @return True if the picker exists or false if does not exist.
     */
    bool SetPickerFlags( EDA_ITEM_FLAGS aFlags, unsigned aIdx );

    /**
     * Remove one entry (one picker) from the list of picked items.
     *
     * @param aIdx Index of the picker in the picked list.
     * @return True if ok or false if did not exist.
     */
    bool RemovePicker( unsigned aIdx );

    /**
     * Copy all data from aSource to the list.
     *
     * Items picked are not copied. just pointer in them are copied.
     *
     * @param aSource The list of items to copy to the list.
     */
    void CopyList( const PICKED_ITEMS_LIST& aSource );

    wxString GetDescription() const                     { return m_description; }
    void SetDescription( const wxString& aDescription ) { m_description = aDescription; }

private:
    wxString                 m_description;
    std::vector<ITEM_PICKER> m_ItemsList;
};


/**
 * A holder to handle a list of undo (or redo) commands.
 */
class UNDO_REDO_CONTAINER
{
public:
    UNDO_REDO_CONTAINER();
    ~UNDO_REDO_CONTAINER();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    UNDO_REDO_CONTAINER( const UNDO_REDO_CONTAINER& ) = delete;
    UNDO_REDO_CONTAINER& operator=( const UNDO_REDO_CONTAINER& ) = delete;

    void PushCommand( PICKED_ITEMS_LIST* aCommand );

    PICKED_ITEMS_LIST* PopCommand();

    void ClearCommandList();

    std::vector <PICKED_ITEMS_LIST*> m_CommandsList;   // the list of possible undo/redo commands
};


#endif      // _CLASS_UNDOREDO_CONTAINER_H
