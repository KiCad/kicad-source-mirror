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

#include "fctsys.h"
#include "common.h"
#include "base_struct.h"

//#include "sch_item_struct.h"
#include "base_struct.h"
#include "class_undoredo_container.h"


ITEM_PICKER::ITEM_PICKER( EDA_BaseStruct* aItem, UndoRedoOpType aUndoRedoStatus )
{
    m_UndoRedoStatus = aUndoRedoStatus;
    m_PickedItem     = aItem;
    m_PickedItemType = TYPE_NOT_INIT;
    m_PickerFlags = 0;
    m_Link = NULL;
}


PICKED_ITEMS_LIST::PICKED_ITEMS_LIST()
{
    m_Status = UR_UNSPECIFIED;
}

PICKED_ITEMS_LIST::~PICKED_ITEMS_LIST()
{
}


/** PushItem
 * push a picker to the top of the list
 * @param aItem = picker to push
 */
void PICKED_ITEMS_LIST::PushItem( ITEM_PICKER& aItem )
{
    m_ItemsList.push_back( aItem );
}


/** PopItem
 * @return the picker from the top of the list
 * the picker is removed from the list
 */
ITEM_PICKER PICKED_ITEMS_LIST::PopItem()
{
    ITEM_PICKER item;

    if( m_ItemsList.size() != 0 )
    {
        item = m_ItemsList.back();
        m_ItemsList.pop_back();
    }
    return item;
}


/**
 * Function ClearItemsList
 * delete only the list of pickers, NOT the picked data itself
 */
void PICKED_ITEMS_LIST::ClearItemsList()
{
    m_ItemsList.clear();
}


/**
 * Function ClearListAndDeleteItems
 * delete the list of pickers, AND the data pointed
 * by m_PickedItem or m_PickedItemLink, according to the type of undo/redo command recorded
 */
void PICKED_ITEMS_LIST::ClearListAndDeleteItems()
{
    bool show_error_message = true;

    // Delete items is they are not flagged UR_NEW, or if this is a block operation
    while( GetCount() > 0 )
    {
        ITEM_PICKER wrapper = PopItem();
        if( wrapper.m_PickedItem == NULL ) // No more item in list.
            break;
        switch( wrapper.m_UndoRedoStatus )
        {
        case UR_UNSPECIFIED:
            if( show_error_message )
                wxMessageBox( wxT( "ClearUndoORRedoList() error: UR_UNSPECIFIED command type" ) );
            show_error_message = false;
            break;

        case UR_WIRE_IMAGE:
        {
            // Specific to eeschema: a linked list of wires is stored.
            // the wrapper picks only the first item (head of list), and is owner of all picked items
            EDA_BaseStruct* item = wrapper.m_PickedItem;
            while( item )
            {
                // Delete old copy of wires
                EDA_BaseStruct* nextitem = item->Next();
                delete item;
                item = nextitem;
            }
        }
        break;

        case UR_MOVED:
        case UR_FLIPPED:
        case UR_MIRRORED_X:
        case UR_MIRRORED_Y:
        case UR_ROTATED:
        case UR_ROTATED_CLOCKWISE:
        case UR_NEW:        // Do nothing, items are in use, the picker is not owner of items
            break;

        case UR_CHANGED:
            delete wrapper.m_Link;   //  the picker is owner of this item
            break;

        case UR_DELETED:            // the picker is owner of this item
        case UR_LIBEDIT:            /* Libedit save always a copy of the current item
                                     *  So, the picker is always owner of the picked item
                                     */
        case UR_MODEDIT:            /* Specific to the module editor
                                     *  (modedit creates a full copy of the current module when changed),
                                     *  and the picker is owner of this item
                                     */
            delete wrapper.m_PickedItem;
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "ClearUndoORRedoList() error: unknown command type %d" ),
                        wrapper.m_UndoRedoStatus );
            wxMessageBox( msg );
        }
        break;
        }
    }
}


/**
 * Function GetItemWrapper
 * @return the picker of a picked item
 * @param aIdx = index of the picker in the picked list
 * if this picker does not exist, a picker is returned,
 * with its members set to 0 or NULL
 */
ITEM_PICKER PICKED_ITEMS_LIST::GetItemWrapper( unsigned int aIdx )
{
    ITEM_PICKER picker;

    if( aIdx < m_ItemsList.size() )
        picker = m_ItemsList[aIdx];

    return picker;
}


/**
 * Function GetPickedItem
 * @return a pointer to the picked item, or null if does not exist
 * @param aIdx = index of the picked item in the picked list
 */
EDA_BaseStruct* PICKED_ITEMS_LIST::GetPickedItem( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_PickedItem;
    else
        return NULL;
}


/**
 * Function GetPickedItemLink
 * @return link of the picked item, or null if does not exist
 * @param aIdx = index of the picked item in the picked list
 */
EDA_BaseStruct* PICKED_ITEMS_LIST::GetPickedItemLink( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_Link;
    else
        return NULL;
}


/**
 * Function GetPickedItemStatus
 * @return the type of undo/redo opertaion associated to the picked item,
 *   or UR_UNSPECIFIED if does not exist
 * @param aIdx = index of the picked item in the picked list
 */
UndoRedoOpType PICKED_ITEMS_LIST::GetPickedItemStatus( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_UndoRedoStatus;
    else
        return UR_UNSPECIFIED;
}

/**
 * Function GetPickerFlags
 * return the value of the picker flag
  * @param aIdx = index of the picker in the picked list
 * @return the value stored in the picker, if the picker exists, or 0 if does not exist
 */
int PICKED_ITEMS_LIST::GetPickerFlags( unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_PickerFlags;
    else
        return 0;
}

/**
 * Function SetPickedItem
 * @param aItem = a pointer to the item to pick
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickedItem( EDA_BaseStruct* aItem, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_PickedItem = aItem;
        return true;
    }
    else
        return false;
}


/**
 * Function SetPickedItemLink
 * Set the link associated to a given picked item
 * @param aLink = the link to the item associated to the picked item
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickedItemLink( EDA_BaseStruct* aLink, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_Link = aLink;
        return true;
    }
    else
        return false;
}


/**
 * Function SetPickedItem
 * @param aItem = a pointer to the item to pick
 * @param aStatus = the type of undo/redo operation associated to the item to pick
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickedItem( EDA_BaseStruct* aItem,
                                       UndoRedoOpType  aStatus,
                                       unsigned        aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_PickedItem     = aItem;
        m_ItemsList[aIdx].m_UndoRedoStatus = aStatus;
        return true;
    }
    else
        return false;
}


/**
 * Function SetPickedItemStatus
 * Set the the type of undo/redo operation for a given picked item
 * @param aStatus = the type of undo/redo operation associated to the picked item
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickedItemStatus( UndoRedoOpType aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_UndoRedoStatus = aStatus;
        return true;
    }
    else
        return false;
}
/**
 * Function SetPickerFlags
 * Set the flags of the picker (usually to the picked item m_Flags value)
 * @param aFlags = the value to save in picker
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickerFlags( int aFlags, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_PickerFlags = aFlags;
        return true;
    }
    else
        return false;
}


/**
 * Function RemovePicker
 * rem�ove one entry (one picker) from the list of picked items
 * @param aIdx = index of the picker in the picked list
 * @return true if ok, or false if did not exist
 */
bool PICKED_ITEMS_LIST::RemovePicker( unsigned aIdx )
{
    if( aIdx >= m_ItemsList.size() )
        return false;
    m_ItemsList.erase( m_ItemsList.begin() + aIdx );
    return true;
}


/**
 * Function CopyList
 * copy all data from aSource
 * Picked items are not copied. just pointers on them are copied
 */
void PICKED_ITEMS_LIST::CopyList( const PICKED_ITEMS_LIST& aSource )
{
    m_ItemsList = aSource.m_ItemsList;  // Vector's copy
}

/**
 * Function ReversePickersListOrder()
 * reverses the order of pickers stored in this list
 * Useful when pop a list from Undo to Redo (and vice-versa)
 * because sometimes undo (or redo) a command needs to keep the
 * order of successive changes.
 * and obviously, undo and redo are in reverse order
 */
void PICKED_ITEMS_LIST::ReversePickersListOrder()
{
    std::vector <ITEM_PICKER> tmp;
    while( !m_ItemsList.empty() )
    {
        tmp.push_back( m_ItemsList.back() );
        m_ItemsList.pop_back();
    }

    m_ItemsList.swap( tmp );
}


/**********************************************/
/********** UNDO_REDO_CONTAINER ***************/
/**********************************************/

UNDO_REDO_CONTAINER::UNDO_REDO_CONTAINER()
{
}


UNDO_REDO_CONTAINER::~UNDO_REDO_CONTAINER()
{
    ClearCommandList();
}


void UNDO_REDO_CONTAINER::ClearCommandList()
{
    for( unsigned ii = 0; ii < m_CommandsList.size(); ii++ )
        delete m_CommandsList[ii];

    m_CommandsList.clear();
}


void UNDO_REDO_CONTAINER::PushCommand( PICKED_ITEMS_LIST* aItem )
{
    m_CommandsList.push_back( aItem );
}


PICKED_ITEMS_LIST* UNDO_REDO_CONTAINER::PopCommand()
{
    if( m_CommandsList.size() != 0 )
    {
        PICKED_ITEMS_LIST* item = m_CommandsList.back();
        m_CommandsList.pop_back();
        return item;
    }
    return NULL;
}


