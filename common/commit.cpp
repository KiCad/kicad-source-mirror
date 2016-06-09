/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <commit.h>
#include <class_undoredo_container.h>


COMMIT::COMMIT()
{
    m_committed = false;
}


COMMIT::~COMMIT()
{
    if( !m_committed )
    {
        for( COMMIT_LINE& ent : m_changes )
        {
            if( ent.m_copy )
                delete ent.m_copy;
        }
    }
}


void COMMIT::makeEntry( EDA_ITEM* aItem, CHANGE_TYPE type, bool saveCopy )
{
    COMMIT_LINE ent;

    ent.m_item = aItem;
    ent.m_type = type;
    ent.m_copy = saveCopy ? (EDA_ITEM*) aItem->Clone() : NULL;

    m_changedItems.insert( aItem );
    m_changes.push_back( ent );
}


COMMIT& COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType )
{
    switch( aChangeType )
    {
        case CHT_ADD:
            assert( m_changedItems.find( aItem ) == m_changedItems.end() );
            makeEntry( aItem, CHT_ADD, false );
            return *this;

        case CHT_REMOVE:
            makeEntry( aItem, CHT_REMOVE, false );
            return *this;

        case CHT_MODIFY:
        {
            EDA_ITEM* parent = parentObject( aItem );

            if( m_changedItems.find( parent ) != m_changedItems.end() )
                return *this; // item already modifed once

            makeEntry( parent, CHT_MODIFY, true );
            return *this;
        }

        default:
            assert( false );
    }
}


COMMIT& COMMIT::Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType )
{
    for( EDA_ITEM* item : container )
    {
        Stage( item, aChangeType );
    }

    return *this;
}


void COMMIT::Stage( const PICKED_ITEMS_LIST& aItems, CHANGE_TYPE aChangeType )
{
    for( unsigned int i = 0; i < aItems.GetCount(); i++ )
        Stage( aItems.GetPickedItem( i ), aChangeType );
}

