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

#include <set>
#include <vector>


bool EDA_GROUP::ContainsItem( const EDA_ITEM* aItem ) const
{
    if( !aItem )
        return false;

    std::set<const EDA_GROUP*>    visitedGroups;
    std::vector<const EDA_GROUP*> pendingGroups;

    pendingGroups.push_back( this );

    while( !pendingGroups.empty() )
    {
        const EDA_GROUP* group = pendingGroups.back();
        pendingGroups.pop_back();

        if( !visitedGroups.insert( group ).second )
            continue;

        for( EDA_ITEM* member : group->GetItems() )
        {
            if( member == aItem )
                return true;

            if( const EDA_GROUP* childGroup = dynamic_cast<const EDA_GROUP*>( member ) )
                pendingGroups.push_back( childGroup );
        }
    }

    return false;
}


void EDA_GROUP::AddItem( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Nullptr added to group." ) );
    wxCHECK_RET( aItem != AsEdaItem(), wxT( "Group added to itself." ) );

    if( EDA_GROUP* group = dynamic_cast<EDA_GROUP*>( aItem ) )
    {
        wxCHECK_RET( !group->ContainsItem( AsEdaItem() ), wxT( "Ancestor group added to group." ) );
    }

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


std::vector<KIID> EDA_GROUP::GetGroupMemberIds() const
{
    std::vector<KIID> members;

    for( EDA_ITEM* item : m_items )
        members.push_back( item->m_Uuid );

    return members;
}
