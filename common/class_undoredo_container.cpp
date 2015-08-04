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

#include <fctsys.h>
#include <common.h>
#include <base_struct.h>

#include <base_struct.h>
#include <class_undoredo_container.h>


ITEM_PICKER::ITEM_PICKER( EDA_ITEM* aItem, UNDO_REDO_T aUndoRedoStatus )
{
    m_undoRedoStatus = aUndoRedoStatus;
    SetItem( aItem );
    m_pickerFlags = 0;
    m_link = NULL;
}


PICKED_ITEMS_LIST::PICKED_ITEMS_LIST()
{
    m_Status = UR_UNSPECIFIED;
}

PICKED_ITEMS_LIST::~PICKED_ITEMS_LIST()
{
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
    for( size_t i = 0;  i < m_ItemsList.size();  i++ )
    {
        if( m_ItemsList[ i ].GetItem() == aItem )
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


void PICKED_ITEMS_LIST::ClearListAndDeleteItems()
{
    bool show_error_message = true;

    // Delete items is they are not flagged UR_NEW, or if this is a block operation
    while( GetCount() > 0 )
    {
        ITEM_PICKER wrapper = PopItem();
        if( wrapper.GetItem() == NULL ) // No more item in list.
            break;
        switch( wrapper.GetStatus() )
        {
        case UR_UNSPECIFIED:
            if( show_error_message )
                wxMessageBox( wxT( "ClearListAndDeleteItems() error: UR_UNSPECIFIED command type" ) );

            show_error_message = false;
            break;

        case UR_WIRE_IMAGE:
        {
            // Specific to eeschema: a linked list of wires is stored.  The wrapper picks only
            // the first item (head of list), and is owner of all picked items.
            EDA_ITEM* item = wrapper.GetItem();

            while( item )
            {
                // Delete old copy of wires
                EDA_ITEM* nextitem = item->Next();
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
        case UR_EXCHANGE_T:
            delete wrapper.GetLink();   //  the picker is owner of this item
            break;

        case UR_DELETED:            // the picker is owner of this item
        case UR_LIBEDIT:            /* Libedit save always a copy of the current item
                                     *  So, the picker is always owner of the picked item
                                     */
        case UR_MODEDIT:            /* Specific to the module editor (modedit creates a full
                                     * copy of the current module when changed),
                                     * and the picker is owner of this item
                                     */
            delete wrapper.GetItem();
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Cannot clear unknown undo/redo command %d" ),
                                          wrapper.GetStatus() ) );
            break;
        }
    }
}


ITEM_PICKER PICKED_ITEMS_LIST::GetItemWrapper( unsigned int aIdx ) const
{
    ITEM_PICKER picker;

    if( aIdx < m_ItemsList.size() )
        picker = m_ItemsList[aIdx];

    return picker;
}


EDA_ITEM* PICKED_ITEMS_LIST::GetPickedItem( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetItem();
    else
        return NULL;
}


EDA_ITEM* PICKED_ITEMS_LIST::GetPickedItemLink( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetLink();
    else
        return NULL;
}


UNDO_REDO_T PICKED_ITEMS_LIST::GetPickedItemStatus( unsigned int aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetStatus();
    else
        return UR_UNSPECIFIED;
}


STATUS_FLAGS PICKED_ITEMS_LIST::GetPickerFlags( unsigned aIdx ) const
{
    if( aIdx < m_ItemsList.size() )
        return m_ItemsList[aIdx].GetFlags();
    else
        return 0;
}


bool PICKED_ITEMS_LIST::SetPickedItem( EDA_ITEM* aItem, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetItem( aItem );
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetPickedItemLink( EDA_ITEM* aLink, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetLink( aLink );
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetPickedItem( EDA_ITEM* aItem, UNDO_REDO_T aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetItem( aItem );
        m_ItemsList[aIdx].SetStatus( aStatus );
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetPickedItemStatus( UNDO_REDO_T aStatus, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetStatus( aStatus );
        return true;
    }
    else
        return false;
}


bool PICKED_ITEMS_LIST::SetPickerFlags( STATUS_FLAGS aFlags, unsigned aIdx )
{
    if( aIdx < m_ItemsList.size() )
    {
        m_ItemsList[aIdx].SetFlags( aFlags );
        return true;
    }
    else
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
