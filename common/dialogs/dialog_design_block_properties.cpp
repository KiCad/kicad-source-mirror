/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mikebwilliams@gmail.com>
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


#include <dialogs/dialog_design_block_properties.h>
#include <wx/msgdlg.h>
#include <wx/tooltip.h>
#include <wx/wupdlock.h>
#include <grid_tricks.h>
#include <widgets/std_bitmap_button.h>
#include <bitmaps.h>

#include <design_block.h>

DIALOG_DESIGN_BLOCK_PROPERTIES::DIALOG_DESIGN_BLOCK_PROPERTIES( wxWindow*     aParent,
                                                                DESIGN_BLOCK* aDesignBlock,
                                                                bool          aDisableName ) :
        DIALOG_DESIGN_BLOCK_PROPERTIES_BASE( aParent ), m_designBlock( aDesignBlock )
{
    m_textName->SetEditable( !aDisableName );

    wxToolTip::Enable( true );
    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    m_fieldsGrid->SetUseNativeColLabels();

    m_fieldsGrid->PushEventHandler( new GRID_TRICKS( m_fieldsGrid,
                                                     [this]( wxCommandEvent& aEvent )
                                                     {
                                                         OnAddField( aEvent );
                                                     } ) );
    m_fieldsGrid->SetupColumnAutosizer( 1 );
    m_fieldsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
}


DIALOG_DESIGN_BLOCK_PROPERTIES::~DIALOG_DESIGN_BLOCK_PROPERTIES()
{
    // Delete the GRID_TRICKS.
    m_fieldsGrid->PopEventHandler( true );
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToWindow()
{
    m_textName->ChangeValue( m_designBlock->GetLibId().GetLibItemName() );
    m_textKeywords->ChangeValue( m_designBlock->GetKeywords() );
    m_textDescription->ChangeValue( m_designBlock->GetLibDescription() );

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
    unsigned int illegalCh = LIB_ID::FindIllegalLibraryNameChar( m_textName->GetValue() );

    // Also check for / in the name, since this is a path character which is illegal for
    // design blocks and footprints but not symbols
    if( illegalCh == 0 && m_textName->GetValue().Find( '/' ) != wxNOT_FOUND )
        illegalCh = '/';

    if( illegalCh )
    {
        wxString msg = wxString::Format( _( "Illegal character '%c' in name '%s'." ),
                                         illegalCh,
                                         m_textName->GetValue() );

        wxMessageDialog errdlg( this, msg, _( "Error" ) );
        errdlg.ShowModal();
        return false;
    }

    m_designBlock->SetLibId( LIB_ID( m_designBlock->GetLibId().GetLibNickname(), m_textName->GetValue() ) );
    m_designBlock->SetLibDescription( m_textDescription->GetValue() );
    m_designBlock->SetKeywords( m_textKeywords->GetValue() );

    return TransferDataFromGrid();
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    m_fieldsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                int row = m_fieldsGrid->GetNumberRows();
                m_fieldsGrid->AppendRows( 1 );

                m_fieldsGrid->SetCellValue( row, 0, _( "Untitled Field" ) );
                //m_fieldsGrid->SetCellValue( row, 1, wxEmptyString );

                // Set cell properties
                m_fieldsGrid->SetCellAlignment( row, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
                m_fieldsGrid->SetCellAlignment( row, 1, wxALIGN_LEFT, wxALIGN_CENTRE );

                return { row, 0 };
            } );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    m_fieldsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_fieldsGrid->DeleteRows( row );
            } );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldUp( wxCommandEvent& event )
{
    m_fieldsGrid->OnMoveRowUp(
            [&]( int row )
            {
                m_fieldsGrid->SwapRows( row, row - 1 );
            } );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldDown( wxCommandEvent& event )
{
    m_fieldsGrid->OnMoveRowUp(
            [&]( int row )
            {
                m_fieldsGrid->SwapRows( row, row + 1 );
            } );
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToGrid()
{
    wxWindowUpdateLocker updateLock( m_fieldsGrid );

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

    return true;
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataFromGrid()
{
    if( !m_fieldsGrid->CommitPendingChanges() )
        return false;

    m_designBlock->GetFields().clear();

    for( int row = 0; row < m_fieldsGrid->GetNumberRows(); row++ )
    {
        wxString fieldName = m_fieldsGrid->GetCellValue( row, 0 ).Strip();
        fieldName.Replace( wxT( "\n" ), wxT( "" ), true );  // strip all newlines
        fieldName.Replace( wxT( "  " ), wxT( " " ), true ); // double space to single

        if( m_designBlock->GetFields().count( fieldName ) )
        {
            wxMessageBox( _( "Duplicate fields are not allowed." ) );
            return false;
        }

        m_designBlock->GetFields()[fieldName] = m_fieldsGrid->GetCellValue( row, 1 );
    }

    return true;
}


