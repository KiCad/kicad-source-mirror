/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2016-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <core/kicad_algo.h>
#include <commit.h>
#include <eda_item.h>
#include <eda_group.h>
#include <macros.h>

COMMIT::COMMIT()
{
}


COMMIT::~COMMIT()
{
    for( COMMIT_LINE& ent : m_entries )
        delete ent.m_copy;
}


COMMIT& COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen, RECURSE_MODE aRecurse )
{
    int       flags = aChangeType & CHT_FLAGS;
    int       changeType = aChangeType & CHT_TYPE;
    EDA_ITEM* undoItem = undoLevelItem( aItem );

    if( undoItem != aItem )
        changeType = CHT_MODIFY;

    // CHT_MODIFY and CHT_DONE are not compatible
    if( changeType == CHT_MODIFY )
        wxASSERT( ( flags & CHT_DONE ) == 0 );

    switch( changeType )
    {
    case CHT_ADD:
        if( m_addedItems.find( { aItem, aScreen } ) != m_addedItems.end() )
            break;

        makeEntry( aItem, CHT_ADD | flags, nullptr, aScreen );
        break;

    case CHT_REMOVE:
        if( m_deletedItems.find( { aItem, aScreen } ) != m_deletedItems.end() )
            break;

        makeEntry( aItem, CHT_REMOVE | flags, makeImage( aItem ), aScreen );

        if( EDA_GROUP* parentGroup = aItem->GetParentGroup() )
        {
            if( parentGroup->AsEdaItem()->GetFlags() & STRUCT_DELETED )
                Modify( parentGroup->AsEdaItem(), aScreen, RECURSE_MODE::NO_RECURSE );
        }

        break;

    case CHT_MODIFY:
        if( m_addedItems.find( { aItem, aScreen } ) != m_addedItems.end() )
            break;

        if( m_changedItems.find( { undoItem, aScreen } ) != m_changedItems.end() )
            break;

        makeEntry( undoItem, CHT_MODIFY | flags, makeImage( undoItem ), aScreen );
        break;

    default:
        UNIMPLEMENTED_FOR( undoItem->GetClass() );
    }

    return *this;
}


COMMIT& COMMIT::Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen )
{
    for( EDA_ITEM* item : container )
        Stage( item, aChangeType, aScreen);

    return *this;
}


COMMIT& COMMIT::Stage( const PICKED_ITEMS_LIST &aItems, UNDO_REDO aModFlag, BASE_SCREEN *aScreen )
{
    for( unsigned int i = 0; i < aItems.GetCount(); i++ )
    {
        UNDO_REDO change_type = aItems.GetPickedItemStatus( i );
        EDA_ITEM* item = aItems.GetPickedItem( i );

        if( change_type == UNDO_REDO::UNSPECIFIED )
            change_type = aModFlag;

        if( EDA_ITEM* copy = aItems.GetPickedItemLink( i ) )
        {
            assert( change_type == UNDO_REDO::CHANGED );

            // There was already a copy created, so use it
            Modified( item, copy, aScreen );
        }
        else
        {
            Stage( item, convert( change_type ), aScreen );
        }
    }

    return *this;
}


void COMMIT::Unstage( EDA_ITEM* aItem, BASE_SCREEN* aScreen )
{
    std::erase_if( m_entries,
                   [&]( COMMIT_LINE& line )
                   {
                       if( line.m_item == aItem && line.m_screen == aScreen )
                       {
                           // Only new items which have never been committed can be unstaged
                           wxASSERT( line.m_item->IsNew() );

                           delete line.m_item;
                           delete line.m_copy;
                           return true;
                       }

                       return false;
                   } );
}


COMMIT& COMMIT::Modified( EDA_ITEM* aItem, EDA_ITEM* aCopy, BASE_SCREEN *aScreen )
{
    if( undoLevelItem( aItem ) != aItem )
        wxFAIL_MSG( "We've no way to get a copy of the undo level item at this point" );
    else
        makeEntry( aItem, CHT_MODIFY, aCopy, aScreen );

    return *this;
}


int COMMIT::GetStatus( EDA_ITEM* aItem, BASE_SCREEN *aScreen )
{
    COMMIT_LINE* entry = findEntry( undoLevelItem( aItem ), aScreen );

    return entry ? entry->m_type : 0;
}


void COMMIT::makeEntry( EDA_ITEM* aItem, CHANGE_TYPE aType, EDA_ITEM* aCopy, BASE_SCREEN *aScreen )
{
    COMMIT_LINE ent;

    ent.m_item = aItem;
    ent.m_type = aType;
    ent.m_copy = aCopy;
    ent.m_screen = aScreen;

    // N.B. Do not throw an assertion for multiple changed items.  An item can be changed
    // multiple times in a single commit such as when importing graphics and grouping them.

    switch( aType & CHT_TYPE )
    {
    case CHT_ADD:    m_addedItems.insert( { aItem, aScreen } );   break;
    case CHT_REMOVE: m_deletedItems.insert( { aItem, aScreen } ); break;
    case CHT_MODIFY: m_changedItems.insert( { aItem, aScreen } ); break;
    default:         wxFAIL;                                      break;
    }

    m_entries.push_back( ent );
}


COMMIT::COMMIT_LINE* COMMIT::findEntry( EDA_ITEM* aItem, BASE_SCREEN *aScreen )
{
    for( COMMIT_LINE& entry : m_entries )
    {
        if( entry.m_item == aItem && entry.m_screen == aScreen )
            return &entry;
    }

    return nullptr;
}


CHANGE_TYPE COMMIT::convert( UNDO_REDO aType ) const
{
    switch( aType )
    {
    case UNDO_REDO::NEWITEM: return CHT_ADD;
    case UNDO_REDO::DELETED: return CHT_REMOVE;
    case UNDO_REDO::CHANGED: return CHT_MODIFY;
    default:     wxFAIL;     return CHT_MODIFY;
    }
}


UNDO_REDO COMMIT::convert( CHANGE_TYPE aType ) const
{
    switch( aType )
    {
    case CHT_ADD:    return UNDO_REDO::NEWITEM;
    case CHT_REMOVE: return UNDO_REDO::DELETED;
    case CHT_MODIFY: return UNDO_REDO::CHANGED;
    default: wxFAIL; return UNDO_REDO::CHANGED;
    }
}

