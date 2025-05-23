/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 jp.charras at wanadoo.fr
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

#include <eda_item.h>
#include <eda_group.h>
#include <undo_redo_container.h>


ITEM_PICKER::ITEM_PICKER()
{
    m_undoRedoStatus = UNDO_REDO::UNSPECIFIED;
    SetItem( nullptr );
    m_pickerFlags = 0;
    m_link = nullptr;
    m_screen = nullptr;
}


ITEM_PICKER::ITEM_PICKER( BASE_SCREEN* aScreen, EDA_ITEM* aItem, UNDO_REDO aUndoRedoStatus )
{
    m_undoRedoStatus = aUndoRedoStatus;
    SetItem( aItem );
    m_pickerFlags = 0;
    m_link = nullptr;
    m_screen = aScreen;
}


PICKED_ITEMS_LIST::PICKED_ITEMS_LIST()
{
}


PICKED_ITEMS_LIST::~PICKED_ITEMS_LIST()
{
}


void ITEM_PICKER::SetItem( EDA_ITEM* aItem )
{
    m_pickedItem = nullptr;
    m_pickedItemType = TYPE_NOT_INIT;

    if( aItem )
    {
        m_pickedItem = aItem;
        m_pickedItemType = aItem->Type();

        if( EDA_GROUP* group = dynamic_cast<EDA_GROUP*>( aItem ) )
            m_groupMembers = group->GetGroupMemberIds();

        m_groupId = aItem->GetParentGroupId();
    }
}


void ITEM_PICKER::SetLink( EDA_ITEM* aItem )
{
    m_link = aItem;

    if( aItem )
    {
        if( EDA_GROUP* group = dynamic_cast<EDA_GROUP*>( aItem ) )
            m_groupMembers = group->GetGroupMemberIds();

        m_groupId = aItem->GetParentGroupId();
    }
}


void PICKED_ITEMS_LIST::PushItem( const ITEM_PICKER& aItem )
{
    m_ItemsList.push_back( aItem );
}


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


bool PICKED_ITEMS_LIST::ContainsItem( const EDA_ITEM* aItem ) const
{
    for( const ITEM_PICKER& picker : m_ItemsList )
    {
        if( picker.GetItem() == aItem )
            return true;
    }

    return false;
}


int PICKED_ITEMS_LIST::FindItem( const EDA_ITEM* aItem ) const
{
    for( size_t i = 0; i < m_ItemsList.size(); i++ )
    {
        if( m_ItemsList[i].GetItem() == aItem )
            return i;
    }

    return -1;
}


void PICKED_ITEMS_LIST::ClearItemsList()
{
    m_ItemsList.clear();
}


void PICKED_ITEMS_LIST::ClearListAndDeleteItems( std::function<void(EDA_ITEM*)> aItemDeleter )
{
    while( GetCount() > 0 )
    {
        ITEM_PICKER wrapper = PopItem();

        if( wrapper.GetItem() == nullptr ) // No more items in list.
            break;

        // The Link is an undo construct; it is always owned by the undo/redo container
        if( wrapper.GetLink() )
            aItemDeleter( wrapper.GetLink() );

        if( wrapper.GetFlags() & UR_TRANSIENT )
        {
            aItemDeleter( wrapper.GetItem() );
        }
        else if( wrapper.GetStatus() == UNDO_REDO::DELETED )
        {
            // This should really be replaced with UR_TRANSIENT, but currently many clients
            // (eeschema in particular) abuse this to achieve non-undo-related deletions.
            aItemDeleter( wrapper.GetItem() );
        }
    }
}


const ITEM_PICKER& PICKED_ITEMS_LIST::GetItemWrapper( unsigned int aIdx ) const
{
    return m_ItemsList.at( aIdx );
}


ITEM_PICKER& PICKED_ITEMS_LIST::GetItemWrapper( unsigned int aIdx )
{
    return m_ItemsList.at( aIdx );
}


EDA_ITEM* PICKED_ITEMS_LIST::GetPickedItem( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetItem();

    return nullptr;
}


BASE_SCREEN* PICKED_ITEMS_LIST::GetScreenForItem( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetScreen();

    return nullptr;
}


EDA_ITEM* PICKED_ITEMS_LIST::GetPickedItemLink( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetLink();

    return nullptr;
}


UNDO_REDO PICKED_ITEMS_LIST::GetPickedItemStatus( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetStatus();

    return UNDO_REDO::UNSPECIFIED;
}


EDA_ITEM_FLAGS PICKED_ITEMS_LIST::GetPickerFlags( unsigned aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetFlags();

    return 0;
}


bool PICKED_ITEMS_LIST::SetPickedItem( EDA_ITEM* aItem, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetItem( aItem );
        return true;
    }

    return false;
}


bool PICKED_ITEMS_LIST::SetPickedItemLink( EDA_ITEM* aLink, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetLink( aLink );
        return true;
    }

    return false;
}


bool PICKED_ITEMS_LIST::SetPickedItemStatus( UNDO_REDO aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetStatus( aStatus );
        return true;
    }

    return false;
}


bool PICKED_ITEMS_LIST::SetPickerFlags( EDA_ITEM_FLAGS aFlags, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetFlags( aFlags );
        return true;
    }

    return false;
}


bool PICKED_ITEMS_LIST::RemovePicker( unsigned aIdx )
{
    if( aIdx >= m_ItemsList.size() )
        return false;

    m_ItemsList.erase( m_ItemsList.begin() + aIdx );
    return true;
}


void PICKED_ITEMS_LIST::CopyList( const PICKED_ITEMS_LIST& aSource )
{
    m_ItemsList = aSource.m_ItemsList;  // Vector's copy
}


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

    return nullptr;
}
