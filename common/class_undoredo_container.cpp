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
    m_PickedItem = aItem;
    m_PickedItemType = TYPE_NOT_INIT;
    m_Link = NULL;
}


PICKED_ITEMS_LIST::PICKED_ITEMS_LIST()
{
    m_Status = UR_UNSPECIFIED;
};

PICKED_ITEMS_LIST::~PICKED_ITEMS_LIST()
{
}


void PICKED_ITEMS_LIST::PushItem( ITEM_PICKER& aItem)
{
    m_ItemsList.push_back( aItem );
}


ITEM_PICKER PICKED_ITEMS_LIST::PICKED_ITEMS_LIST::PopItem()
{
    ITEM_PICKER item;

    if( m_ItemsList.size() != 0 )
    {
        item = m_ItemsList.back();
        m_ItemsList.pop_back();
    }
    return item;
}


void PICKED_ITEMS_LIST::PICKED_ITEMS_LIST::ClearItemsList()

/* delete only the list of EDA_BaseStruct * pointers, NOT the pointed data itself
 */
{
    m_ItemsList.clear();
}

void PICKED_ITEMS_LIST::ClearListAndDeleteItems()
{
    for(unsigned ii = 0; ii < m_ItemsList.size(); ii++ )
        delete m_ItemsList[ii].m_PickedItem;
    m_ItemsList.clear();
}


/** function GetItemWrapper
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

/** function GetPickedItem
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


/** function GetLink
 * @return link of the picked item, or null if does not exist
 * @param aIdx = index of the picked item in the picked list
 */
EDA_BaseStruct* PICKED_ITEMS_LIST::GetLink( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_Link;
    else
        return NULL;
}


/** function GetPickedItemStatus
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


/** function SetPickedItem
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


/** function SetLink
 * Set the link associated to a given picked item
 * @param aLink = the link to the item associated to the picked item
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetLink( EDA_BaseStruct* aLink, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_Link = aLink;
        return true;
    }
    else
        return false;
}


/** function SetPickedItem
 * @param aItem = a pointer to the item to pick
 * @param aStatus = the type of undo/redo operation associated to the item to pick
 * @param aIdx = index of the picker in the picked list
 * @return true if the picker exists, or false if does not exist
 */
bool PICKED_ITEMS_LIST::SetPickedItem( EDA_BaseStruct* aItem, UndoRedoOpType aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_PickedItem = aItem;
        m_ItemsList[aIdx].m_UndoRedoStatus = aStatus;
        return true;
    }
    else
        return false;
}


/** function SetPickedItemStatus
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


/** function RemovePickedItem
 * remùove one entry (one picker) from the list of picked items
 * @param aIdx = index of the picker in the picked list
 * @return true if ok, or false if did not exist
 */
bool PICKED_ITEMS_LIST::RemovePickedItem( unsigned aIdx )
{
    if( aIdx >= m_ItemsList.size() )
        return false;
    m_ItemsList.erase( m_ItemsList.begin() + aIdx );
    return true;
}

/** Function CopyList
 * copy all data from aSource
 * Items picked are not copied. just pointer on them are copied
 */
void PICKED_ITEMS_LIST::CopyList(const PICKED_ITEMS_LIST & aSource)
{
    ITEM_PICKER picker;
    for(unsigned ii = 0; ii < aSource.GetCount(); ii++ )
    {
        picker = aSource.m_ItemsList[ii];
        PushItem(picker);
    }
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
