/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mikebwilliams@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <dialogs/dialog_design_block_properties.h>
#include <sch_edit_frame.h>
#include <wx/msgdlg.h>
#include <wx/tooltip.h>
#include <grid_tricks.h>
#include <widgets/std_bitmap_button.h>

#include <design_block.h>

DIALOG_DESIGN_BLOCK_PROPERTIES::DIALOG_DESIGN_BLOCK_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                                DESIGN_BLOCK*   aDesignBlock ) :
        DIALOG_DESIGN_BLOCK_PROPERTIES_BASE( aParent ), m_designBlock( aDesignBlock )
{
    if( !m_textName->IsEmpty() )
        m_textName->SetEditable( false );

    wxToolTip::Enable( true );
    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    m_fieldsGrid->SetUseNativeColLabels();

    m_fieldsGrid->PushEventHandler( new GRID_TRICKS( m_fieldsGrid, [this]( wxCommandEvent& aEvent )
                                                       {
                                                           OnAddField( aEvent );
                                                       } ) );
    m_fieldsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
}


DIALOG_DESIGN_BLOCK_PROPERTIES::~DIALOG_DESIGN_BLOCK_PROPERTIES()
{
    // Delete the GRID_TRICKS.
    m_fieldsGrid->PopEventHandler( true );
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToWindow()
{
    m_textName->AppendText( m_designBlock->GetLibId().GetLibItemName() );
    m_textKeywords->AppendText( m_designBlock->GetKeywords() );
    m_textDescription->AppendText( m_designBlock->GetLibDescription() );

    // Typical assignment operator does not work here because of the ordered_map
    auto source = m_designBlock->GetFields();
    m_fields.clear();
    for( const auto& field : source )
    {
        m_fields[field.first] = field.second;
    }

    return TransferDataToGrid();
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataFromWindow()
{
    m_designBlock->SetLibId(
            LIB_ID( m_designBlock->GetLibId().GetLibNickname(), m_textName->GetValue() ) );
    m_designBlock->SetLibDescription( m_textDescription->GetValue() );
    m_designBlock->SetKeywords( m_textKeywords->GetValue() );

    return TransferDataFromGrid();
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    int row = m_fieldsGrid->GetNumberRows();

    m_fieldsGrid->AppendRows( 1 );

    m_fieldsGrid->SetCellValue( row, 0, _( "Untitled Field" ) );
    //m_fieldsGrid->SetCellValue( row, 1, wxEmptyString );

    // Set cell properties
    m_fieldsGrid->SetCellAlignment( row, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_fieldsGrid->SetCellAlignment( row, 1, wxALIGN_LEFT, wxALIGN_CENTRE );

    // wx documentation is wrong, SetGridCursor does not make visible.
    m_fieldsGrid->MakeCellVisible( row, 0 );
    m_fieldsGrid->SetGridCursor( row, 0 );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_fieldsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_fieldsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_fieldsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
    {
        m_fieldsGrid->DeleteRows( row );

        m_fieldsGrid->MakeCellVisible( std::max( 0, row - 1 ), m_fieldsGrid->GetGridCursorCol() );
        m_fieldsGrid->SetGridCursor( std::max( 0, row - 1 ), m_fieldsGrid->GetGridCursorCol() );
    }
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldUp( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    int row = m_fieldsGrid->GetGridCursorRow();

    if( m_fieldsGrid->GetNumberRows() < 2 || row == 0 )
        return;

    // Swap the grid at row with the grid at row - 1
    wxString temp0 = m_fieldsGrid->GetCellValue( row, 0 );
    m_fieldsGrid->SetCellValue( row, 0, m_fieldsGrid->GetCellValue( row - 1, 0 ) );
    m_fieldsGrid->SetCellValue( row - 1, 0, temp0 );

    wxString temp1 = m_fieldsGrid->GetCellValue( row, 1 );
    m_fieldsGrid->SetCellValue( row, 1, m_fieldsGrid->GetCellValue( row - 1, 1 ) );
    m_fieldsGrid->SetCellValue( row - 1, 1, temp1 );

    m_fieldsGrid->SetGridCursor( row - 1, 0 );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldDown( wxCommandEvent& event )
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return;

    int row = m_fieldsGrid->GetGridCursorRow();

    if( m_fieldsGrid->GetNumberRows() < 2 || row == ( (int) m_fieldsGrid->GetNumberRows() - 1 ) )
        return;

    // Swap the grid at row with the grid at row + 1
    wxString temp0 = m_fieldsGrid->GetCellValue( row, 0 );
    m_fieldsGrid->SetCellValue( row, 0, m_fieldsGrid->GetCellValue( row + 1, 0 ) );
    m_fieldsGrid->SetCellValue( row + 1, 0, temp0 );

    wxString temp1 = m_fieldsGrid->GetCellValue( row, 1 );
    m_fieldsGrid->SetCellValue( row, 1, m_fieldsGrid->GetCellValue( row + 1, 1 ) );
    m_fieldsGrid->SetCellValue( row + 1, 1, temp1 );

    m_fieldsGrid->SetGridCursor( row + 1, 0 );
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToGrid()
{
    m_fieldsGrid->Freeze();

    m_fieldsGrid->ClearRows();
    m_fieldsGrid->AppendRows( m_fields.size() );

    int row = 0;

    for( const auto& [fieldName, fieldValue] : m_fields )
    {
        m_fieldsGrid->SetCellValue( row, 0, fieldName );
        m_fieldsGrid->SetCellValue( row, 1, fieldValue );

        // Set cell properties
        m_fieldsGrid->SetCellAlignment( row, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
        m_fieldsGrid->SetCellAlignment( row, 1, wxALIGN_LEFT, wxALIGN_CENTRE );

        row++;
    }

    m_fieldsGrid->Thaw();

    return true;
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataFromGrid()
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return false;

    nlohmann::ordered_map<wxString, wxString> newFields;

    for( int row = 0; row < m_fieldsGrid->GetNumberRows(); row++ )
    {
        wxString fieldName = m_fieldsGrid->GetCellValue( row, 0 ).Strip();
        fieldName.Replace( wxT( "\n" ), wxT( "" ), true );  // strip all newlines
        fieldName.Replace( wxT( "  " ), wxT( " " ), true ); // double space to single

        if( newFields.count( fieldName ) )
        {
            wxMessageBox( _( "Duplicate fields are not allowed." ) );
            return false;
        }

        newFields[fieldName] = m_fieldsGrid->GetCellValue( row, 1 );
    }

    m_designBlock->SetFields( newFields );

    return true;
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::AdjustGridColumns( int aWidth )
{
    if( aWidth <= 0 )
        return;

    // Account for scroll bars
    aWidth -= ( m_fieldsGrid->GetSize().x - m_fieldsGrid->GetClientSize().x );

    m_fieldsGrid->SetColSize( 1, aWidth - m_fieldsGrid->GetColSize( 0 ) );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    AdjustGridColumns( event.GetSize().GetX() );

    event.Skip();
}
