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

#include <dialogs/dialog_constraint_list.h>

#include <board.h>
#include <pcb_base_frame.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <widgets/constraint_list_view.h>


DIALOG_CONSTRAINT_LIST::DIALOG_CONSTRAINT_LIST( PCB_BASE_FRAME* aParent, BOARD* aBoard,
                                                std::function<void( PCB_CONSTRAINT* )> aHighlight,
                                                std::function<void( PCB_CONSTRAINT* )> aRemove ) :
        DIALOG_CONSTRAINT_LIST_BASE( aParent ),
        m_parentFrame( aParent ),
        m_board( aBoard ),
        m_highlight( std::move( aHighlight ) ),
        m_remove( std::move( aRemove ) )
{
    AddConstraintListColumns( m_list );

    Populate();
    finishDialogSettings();
}


void DIALOG_CONSTRAINT_LIST::Populate()
{
    m_rows.clear();

    BOARD_CONSTRAINT_DIAGNOSTICS diag =
            m_board ? DiagnoseBoardConstraints( m_board ) : BOARD_CONSTRAINT_DIAGNOSTICS();

    PopulateConstraintList( m_list, m_board, m_parentFrame, diag,
                            [&]( long aRow, PCB_CONSTRAINT* aConstraint )
                            {
                                m_rows.push_back( aConstraint );
                            } );
}


PCB_CONSTRAINT* DIALOG_CONSTRAINT_LIST::selectedConstraint() const
{
    long row = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( row < 0 || row >= static_cast<long>( m_rows.size() ) )
        return nullptr;

    return m_rows[row];
}


void DIALOG_CONSTRAINT_LIST::onRowActivated( wxListEvent& aEvent )
{
    if( PCB_CONSTRAINT* constraint = selectedConstraint() )
    {
        m_highlight( constraint );

        // Close so the highlighted members are visible on the canvas behind the dialog.
        EndModal( wxID_OK );
    }
}


void DIALOG_CONSTRAINT_LIST::onDelete( wxCommandEvent& aEvent )
{
    if( PCB_CONSTRAINT* constraint = selectedConstraint() )
    {
        m_remove( constraint );
        Populate();   // the constraint set changed
    }
}
