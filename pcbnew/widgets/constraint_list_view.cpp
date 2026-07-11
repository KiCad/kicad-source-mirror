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

#include <widgets/constraint_list_view.h>

#include <set>

#include <wx/listctrl.h>

#include <board.h>
#include <footprint.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>


void AddConstraintListColumns( wxListCtrl* aList )
{
    aList->InsertColumn( CONSTRAINT_COL_ITEM_1, _( "Item 1" ), wxLIST_FORMAT_LEFT, 150 );
    aList->InsertColumn( CONSTRAINT_COL_ITEM_2, _( "Item 2" ), wxLIST_FORMAT_LEFT, 150 );
    aList->InsertColumn( CONSTRAINT_COL_TYPE, _( "Constraint" ), wxLIST_FORMAT_LEFT, 150 );
    aList->InsertColumn( CONSTRAINT_COL_STATE, _( "State" ), wxLIST_FORMAT_LEFT, 130 );
}


void PopulateConstraintList( wxListCtrl* aList, BOARD* aBoard, UNITS_PROVIDER* aUnits,
                             const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag,
                             const std::function<void( long, PCB_CONSTRAINT* )>& aOnRow )
{
    aList->DeleteAllItems();

    if( !aBoard )
        return;

    std::set<KIID> errored( aDiag.errored.begin(), aDiag.errored.end() );
    std::set<KIID> conflicting( aDiag.conflicting.begin(), aDiag.conflicting.end() );

    auto memberLabel =
            [&]( const CONSTRAINT_MEMBER& aMember ) -> wxString
            {
                return ConstraintMemberLabel( aBoard->ResolveItem( aMember.m_item, true ),
                                              aMember.m_anchor, aUnits );
            };

    auto addRows =
            [&]( const CONSTRAINTS& aConstraints )
            {
                for( PCB_CONSTRAINT* constraint : aConstraints )
                {
                    const std::vector<CONSTRAINT_MEMBER>& members = constraint->GetMembers();

                    wxString item1 = members.size() > 0 ? memberLabel( members[0] ) : wxString();
                    wxString item2 = members.size() > 1 ? memberLabel( members[1] ) : wxString();

                    // Constraints with more than two members (e.g. symmetric) still get two
                    // columns; flag the extras so the count is not silently lost.
                    if( members.size() > 2 )
                        item2 += wxString::Format( _( " (+%zu)" ), members.size() - 2 );

                    long row = aList->InsertItem( aList->GetItemCount(), item1 );
                    aList->SetItem( row, CONSTRAINT_COL_ITEM_2, item2 );
                    aList->SetItem( row, CONSTRAINT_COL_TYPE,
                                    ConstraintDisplayLabel( *constraint, aUnits->GetUserUnits() ) );

                    wxString state;

                    if( errored.count( constraint->m_Uuid ) )
                        state = _( "Error (missing item)" );
                    else if( conflicting.count( constraint->m_Uuid ) )
                        state = _( "Over-constrained" );
                    else
                        state = _( "OK" );

                    aList->SetItem( row, CONSTRAINT_COL_STATE, state );
                    aOnRow( row, constraint );
                }
            };

    addRows( aBoard->Constraints() );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        addRows( footprint->Constraints() );
}
