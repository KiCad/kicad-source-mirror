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

PICKED_ITEMS_LIST::PICKED_ITEMS_LIST()
{
    m_UndoRedoType = 0;
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

ITEM_PICKER PICKED_ITEMS_LIST::GetItemWrapper( unsigned int aIdx )
{
    ITEM_PICKER picker;
    if( aIdx < m_ItemsList.size() )
        picker = m_ItemsList[aIdx];

    return picker;
}

EDA_BaseStruct* PICKED_ITEMS_LIST::GetItemData( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_Item;
    else
        return NULL;
}


EDA_BaseStruct* PICKED_ITEMS_LIST::GetImage( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_Link;
    else
        return NULL;
}


int PICKED_ITEMS_LIST::GetItemStatus( unsigned int aIdx )
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].m_UndoRedoStatus;
    else
        return 0;
}


bool PICKED_ITEMS_LIST::SetItem( EDA_BaseStruct* aItem, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_Item = aItem;
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetLink( EDA_BaseStruct* aItem, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_Link = aItem;
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetItem( EDA_BaseStruct* aItem, int aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_Item = aItem;
        m_ItemsList[aIdx].m_UndoRedoStatus = aStatus;
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetItemStatus( int aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].m_UndoRedoStatus = aStatus;
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::RemoveItem( unsigned aIdx )
{
    if( aIdx >= m_ItemsList.size() )
        return false;
    m_ItemsList.erase( m_ItemsList.begin() + aIdx );
    return true;
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
