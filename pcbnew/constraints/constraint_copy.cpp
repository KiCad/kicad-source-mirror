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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>

#include <constraints/constraint_copy.h>
#include <constraints/pcb_constraint.h>
#include <tools/pcb_selection.h>


std::set<KIID> CollectConstraintScopeIds( const PCB_SELECTION& aSelection )
{
    std::set<KIID> ids;

    for( EDA_ITEM* item : aSelection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        ids.insert( boardItem->m_Uuid );
        boardItem->RunOnChildren( [&]( BOARD_ITEM* aChild ) { ids.insert( aChild->m_Uuid ); },
                                  RECURSE_MODE::RECURSE );
    }

    return ids;
}


bool ConstraintFullySelected( const PCB_CONSTRAINT* aConstraint, const std::set<KIID>& aScopeIds )
{
    const std::vector<CONSTRAINT_MEMBER>& members = aConstraint->GetMembers();

    return !members.empty()
           && std::all_of( members.begin(), members.end(),
                           [&]( const CONSTRAINT_MEMBER& aMember )
                           {
                               return aScopeIds.count( aMember.m_item ) > 0;
                           } );
}


std::vector<PCB_CONSTRAINT*> CloneFullySelectedConstraints( const CONSTRAINTS& aSource,
                                                            const std::map<KIID, KIID>& aIdMap )
{
    std::set<KIID> scopeIds;

    for( const auto& [origId, dupeId] : aIdMap )
        scopeIds.insert( origId );

    std::vector<PCB_CONSTRAINT*> clones;

    for( PCB_CONSTRAINT* constraint : aSource )
    {
        if( !ConstraintFullySelected( constraint, scopeIds ) )
            continue;

        PCB_CONSTRAINT* clone = static_cast<PCB_CONSTRAINT*>( constraint->Clone() );

        // Clone copies the source UUID but the duplicate is a distinct object beside the
        // original so it needs its own identity before members are repointed
        clone->ResetUuid();
        clone->RemapKIIDs( aIdMap );
        clones.push_back( clone );
    }

    return clones;
}
