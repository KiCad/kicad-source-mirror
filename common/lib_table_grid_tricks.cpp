/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib_table_grid_tricks.h"
#include "lib_table_grid.h"


LIB_TABLE_GRID_TRICKS::LIB_TABLE_GRID_TRICKS( WX_GRID* aGrid ) : GRID_TRICKS( aGrid )
{
}


void LIB_TABLE_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    menu.Append( LIB_TABLE_GRID_TRICKS_OPTIONS_EDITOR,
                _( "Edit options..." ),
                _( "Edit options for this library entry" ) );

    menu.AppendSeparator();

    bool            showActivate = false;
    bool            showDeactivate = false;
    LIB_TABLE_GRID* tbl = static_cast<LIB_TABLE_GRID*>( m_grid->GetTable() );

    // Ensure selection parameters are up to date
    getSelectedArea();

    for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
    {
        if( tbl->GetValueAsBool( row, 0 ) )
            showDeactivate = true;
        else
            showActivate = true;

        if( showActivate && showDeactivate )
            break;
    }

    if( showActivate )
        menu.Append( LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED, _( "Activate selected" ) );

    if( showDeactivate )
        menu.Append( LIB_TABLE_GRID_TRICKS_DEACTIVATE_SELECTED, _( "Deactivate selected" ) );

    bool showSettings = false;

    if( m_sel_row_count == 1 && tbl->At( m_sel_row_start )->SupportsSettingsDialog() )
    {
        showSettings = true;
        menu.Append( LIB_TABLE_GRID_TRICKS_LIBRARY_SETTINGS,
                     wxString::Format( _( "Library settings for %s..." ),
                                       tbl->GetValue( m_sel_row_start, 2 ) ) );
    }

    if( showActivate || showDeactivate || showSettings )
        menu.AppendSeparator();

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void LIB_TABLE_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    int menu_id = event.GetId();
    LIB_TABLE_GRID* tbl = (LIB_TABLE_GRID*) m_grid->GetTable();

    if( menu_id == LIB_TABLE_GRID_TRICKS_OPTIONS_EDITOR )
    {
        optionsEditor( m_grid->GetGridCursorRow() );
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED
            || menu_id == LIB_TABLE_GRID_TRICKS_DEACTIVATE_SELECTED )
    {
        bool selected_state = menu_id == LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED;

        for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
            tbl->SetValueAsBool( row, 0, selected_state );

        // Ensure the new state (on/off) of the widgets is immediately shown:
        m_grid->Refresh();
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_LIBRARY_SETTINGS )
    {
        LIB_TABLE_ROW* row = tbl->At( m_sel_row_start );
        row->Refresh();
        row->ShowSettingsDialog( m_grid->GetParent() );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


bool LIB_TABLE_GRID_TRICKS::handleDoubleClick( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_OPTIONS )
    {
        optionsEditor( aEvent.GetRow() );
        return true;
    }

    return false;
}

