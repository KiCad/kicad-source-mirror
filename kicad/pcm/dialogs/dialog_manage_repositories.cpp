/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_manage_repositories.h"
#include "bitmaps/bitmap_types.h"
#include "bitmaps/bitmaps_list.h"
#include "grid_tricks.h"
#include "widgets/wx_grid.h"


#define GRID_CELL_MARGIN 4


DIALOG_MANAGE_REPOSITORIES::DIALOG_MANAGE_REPOSITORIES(
        wxWindow* parent, std::shared_ptr<PLUGIN_CONTENT_MANAGER> pcm ) :
        DIALOG_MANAGE_REPOSITORIES_BASE( parent ),
        m_pcm( pcm )
{
    m_buttonAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_buttonRemove->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_buttonMoveUp->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_buttonMoveDown->SetBitmap( KiBitmap( BITMAPS::small_down ) );

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        const wxString& heading = m_grid->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        // Set the minimal width to the column label size.
        m_grid->SetColMinimalWidth( col, headingWidth );
    }

    // fix sizers now widgets are set.
    finishDialogSettings();
}


DIALOG_MANAGE_REPOSITORIES::~DIALOG_MANAGE_REPOSITORIES()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


void DIALOG_MANAGE_REPOSITORIES::setColumnWidths()
{
    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        // Set the width to see the full contents
        m_grid->SetColSize( col, m_grid->GetVisibleWidth( col, true, true, false ) );
    }
}


void DIALOG_MANAGE_REPOSITORIES::OnAddButtonClicked( wxCommandEvent& event )
{
    wxTextEntryDialog entry_dialog( this,
                                    _( "Please enter fully qualified repository url" ),
                                    _( "Add repository" ) );

    if( entry_dialog.ShowModal() == wxID_OK )
    {
        PCM_REPOSITORY repository;
        wxString       url = entry_dialog.GetValue();

        const auto find_row = [&]( const int col, const wxString& val )
        {
            for( int row = 0; row < m_grid->GetNumberRows(); row++ )
            {
                if( m_grid->GetCellValue( row, col ) == val )
                    return row;
            }

            return -1;
        };

        int matching_row;

        if( ( matching_row = find_row( 1, url ) ) >= 0 )
        {
            selectRow( matching_row );
        }
        else
        {
            WX_PROGRESS_REPORTER reporter( GetParent(), wxT( "" ), 1 );

            if( m_pcm->FetchRepository( url, repository, &reporter ) )
            {
                wxString name = repository.name;
                int      increment = 1;

                while( find_row( 0, name ) >= 0 )
                    name = wxString::Format( "%s (%d)", repository.name, increment++ );

                m_grid->Freeze();

                m_grid->AppendRows();
                int row = m_grid->GetNumberRows() - 1;

                m_grid->SetCellValue( row, 0, name );
                m_grid->SetCellValue( row, 1, url );

                setColumnWidths();
                m_grid->Thaw();

                selectRow( row );
            }
        }
    }
}


void DIALOG_MANAGE_REPOSITORIES::OnRemoveButtonClicked( wxCommandEvent& event )
{
    auto selectedRows = m_grid->GetSelectedRows();

    // If nothing is selected or multiple rows are selected don't do anything.
    if( selectedRows.size() != 1 )
    {
        wxBell();
        return;
    }

    int selectedRow = selectedRows[0];
    m_grid->DeleteRows( selectedRow );
    setColumnWidths();

    if( m_grid->GetNumberRows() > 0 )
        m_grid->SelectRow( selectedRow == m_grid->GetNumberRows() ? selectedRow - 1 : selectedRow );
}


void DIALOG_MANAGE_REPOSITORIES::OnMoveUpButtonClicked( wxCommandEvent& event )
{
    auto selectedRows = m_grid->GetSelectedRows();

    // If nothing is selected or multiple rows are selected don't do anything.
    if( selectedRows.size() != 1 )
        return;

    int selectedRow = selectedRows[0];

    // If first row is selected, then it can't go any further up.
    if( selectedRow == 0 )
    {
        wxBell();
        return;
    }

    swapRows( selectedRow, selectedRow - 1 );

    selectRow( selectedRow - 1 );
}


void DIALOG_MANAGE_REPOSITORIES::OnMoveDownButtonClicked( wxCommandEvent& event )
{
    auto selectedRows = m_grid->GetSelectedRows();

    // If nothing is selected or multiple rows are selected don't do anything.
    if( selectedRows.size() != 1 )
        return;

    int selectedRow = selectedRows[0];

    // If last row is selected, then it can't go any further down.
    if( selectedRow + 1 == m_grid->GetNumberRows() )
    {
        wxBell();
        return;
    }

    swapRows( selectedRow, selectedRow + 1 );

    selectRow( selectedRow + 1 );
}


void DIALOG_MANAGE_REPOSITORIES::swapRows( int aRowA, int aRowB )
{
    m_grid->Freeze();

    wxString tempStr;

    for( int column = 0; column < m_grid->GetNumberCols(); column++ )
    {
        tempStr = m_grid->GetCellValue( aRowA, column );
        m_grid->SetCellValue( aRowA, column, m_grid->GetCellValue( aRowB, column ) );
        m_grid->SetCellValue( aRowB, column, tempStr );
    }

    m_grid->Thaw();
}


void DIALOG_MANAGE_REPOSITORIES::OnGridCellClicked( wxGridEvent& event )
{
    selectRow( event.GetRow() );
}


void DIALOG_MANAGE_REPOSITORIES::selectRow( int aRow )
{
    m_grid->ClearSelection();
    m_grid->SelectRow( aRow );
}


void DIALOG_MANAGE_REPOSITORIES::SetData( const std::vector<std::pair<wxString, wxString>>& aData )
{
    m_grid->Freeze();

    m_grid->DeleteRows( 0, m_grid->GetNumberRows() );
    m_grid->AppendRows( aData.size() );

    for( size_t i = 0; i < aData.size(); i++ )
    {
        m_grid->SetCellValue( i, 0, aData[i].first );
        m_grid->SetCellValue( i, 1, aData[i].second );
    }

    setColumnWidths();

    m_grid->Thaw();
}


std::vector<std::pair<wxString, wxString>> DIALOG_MANAGE_REPOSITORIES::GetData()
{
    std::vector<std::pair<wxString, wxString>> result;

    for( int i = 0; i < m_grid->GetNumberRows(); i++ )
    {
        result.push_back( std::make_pair( m_grid->GetCellValue( i, 0 ),
                                          m_grid->GetCellValue( i, 1 ) ) );
    }

    return result;
}


void DIALOG_MANAGE_REPOSITORIES::OnSaveClicked( wxCommandEvent& event )
{
    EndModal( wxID_SAVE );
}
