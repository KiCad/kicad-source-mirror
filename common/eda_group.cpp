/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <eda_group.h>


void EDA_GROUP::AddItem( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Nullptr added to group." ) );

    // Items can only be in one group at a time
    if( EDA_GROUP* parentGroup = aItem->GetParentGroup() )
        parentGroup->RemoveItem( aItem );

    m_items.insert( aItem );
    aItem->SetParentGroup( this );
}


void EDA_GROUP::RemoveItem( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Nullptr removed from group." ) );

    if( m_items.erase( aItem ) == 1 )
        aItem->SetParentGroup( nullptr );
}


void EDA_GROUP::RemoveAll()
{
    for( EDA_ITEM* item : m_items )
        item->SetParentGroup( nullptr );

    m_items.clear();
}


KIID_VECT_LIST EDA_GROUP::GetGroupMemberIds() const
{
    KIID_VECT_LIST members;

    for( EDA_ITEM* item : m_items )
        members.push_back( item->m_Uuid );

    return members;
}


