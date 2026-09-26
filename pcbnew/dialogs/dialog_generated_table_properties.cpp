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

#include "dialog_generated_table_properties.h"

#include <algorithm>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board_tables/generated_table_refresh.h>
#include <confirm.h>
#include <pcb_base_edit_frame.h>
#include <pcb_generated_table.h>
#include <pcb_layer_box_selector.h>
#include <widgets/wx_grid.h>

#include <wx/grid.h>


namespace
{

enum COLUMN_GRID_COL
{
    COL_SHOW = 0,
    COL_NAME,
    COL_HEADING,
    COL_ALIGN,
    COL_WIDTH
};


// What the column reports, whatever the user has retitled its heading to
wxString columnLabel( const GENERATED_TABLE_SCHEMA& aSchema, int aId )
{
    const GENERATED_TABLE_COLUMN_DEF* def = aSchema.Find( aId );

    if( !def )
        return wxEmptyString;

    return wxGetTranslation( wxString( def->m_Label ? def->m_Label : def->m_Heading ) );
}


wxArrayString alignChoices()
{
    wxArrayString choices;
    choices.Add( _( "Left" ) );
    choices.Add( _( "Center" ) );
    choices.Add( _( "Right" ) );

    return choices;
}

} // namespace


DIALOG_GENERATED_TABLE_PROPERTIES::DIALOG_GENERATED_TABLE_PROPERTIES( PCB_BASE_EDIT_FRAME*            aFrame,
                                                                      PCB_GENERATED_TABLE*            aTable,
                                                                      GENERATED_TABLE_OPTIONS_FACTORY aOptions ) :
        DIALOG_GENERATED_TABLE_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( aTable ),
        m_options( nullptr ),
        m_borderWidth( aFrame, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_separatorsWidth( aFrame, m_separatorsWidthLabel, m_separatorsWidthCtrl, m_separatorsWidthUnits )
{
    SetTitle( wxString::Format( _( "%s Properties" ), m_table->GetFriendlyName() ) );

    // Not merely "not copper". Silkscreen, mask, paste, adhesive, Edge.Cuts, Margin and
    // courtyard are manufacturing inputs this would corrupt rather than document
    m_LayerSelectionCtrl->SetNotAllowedLayerSet( ~DocumentationLayers() );
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    m_columnGrid->SetColLabelValue( COL_SHOW, _( "Show" ) );
    m_columnGrid->SetColLabelValue( COL_NAME, _( "Column" ) );
    m_columnGrid->SetColLabelValue( COL_HEADING, _( "Heading" ) );
    m_columnGrid->SetColLabelValue( COL_ALIGN, _( "Align" ) );
    m_columnGrid->SetColLabelValue( COL_WIDTH, _( "Min Width" ) );

    m_columnGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_columnGrid->SetUnitsProvider( m_frame );
    m_columnGrid->SetAutoEvalCols( { COL_WIDTH } );

    m_columnGrid->Bind( wxEVT_SIZE, &DIALOG_GENERATED_TABLE_PROPERTIES::onColumnGridSize, this );

    if( aOptions )
    {
        // The page, not the dialog, since wx insists a sizer's windows are children of its window
        m_options = aOptions( m_columnsPanel );
        m_options->m_GetFormat = [this]()
        {
            return getFormat();
        };
        m_options->m_SetFormat = [this]( const GENERATED_TABLE_FORMAT& aFormat )
        {
            setFormat( aFormat );
        };
        m_optionsSizer->Add( m_options, 0, wxEXPAND, 0 );
    }

    SetupStandardButtons();
    finishDialogSettings();
}


void DIALOG_GENERATED_TABLE_PROPERTIES::fillColumnGrid()
{
    if( m_columnGrid->GetNumberRows() )
        m_columnGrid->DeleteRows( 0, m_columnGrid->GetNumberRows() );

    m_columnGrid->AppendRows( static_cast<int>( m_rows.size() ) );

    const wxArrayString aligns = alignChoices();

    for( size_t row = 0; row < m_rows.size(); ++row )
    {
        const int                     r = static_cast<int>( row );
        const GENERATED_TABLE_COLUMN& col = m_rows[row].m_Column;

        m_columnGrid->SetCellRenderer( r, COL_SHOW, new wxGridCellBoolRenderer );
        m_columnGrid->SetCellEditor( r, COL_SHOW, new wxGridCellBoolEditor );
        m_columnGrid->SetCellValue( r, COL_SHOW, m_rows[row].m_Shown ? wxT( "1" ) : wxEmptyString );

        m_columnGrid->SetCellValue( r, COL_NAME, columnLabel( m_table->Schema(), col.m_Id ) );
        m_columnGrid->SetReadOnly( r, COL_NAME );

        m_columnGrid->SetCellValue( r, COL_HEADING, col.m_Heading );

        m_columnGrid->SetCellEditor( r, COL_ALIGN, new wxGridCellChoiceEditor( aligns ) );
        m_columnGrid->SetCellValue( r, COL_ALIGN, aligns[static_cast<int>( col.m_Align )] );

        // Empty rather than zero, since zero means the column sizes itself to its text
        m_columnGrid->SetOptionalUnitValue( r, COL_WIDTH,
                                            col.m_Width > 0 ? std::optional<int>( col.m_Width ) : std::nullopt );
    }

    m_columnGrid->AutoSizeColumns();
    fitColumnGrid();
}


bool DIALOG_GENERATED_TABLE_PROPERTIES::harvestColumnGrid()
{
    if( !m_columnGrid->CommitPendingChanges() )
        return false;

    const wxArrayString aligns = alignChoices();

    for( size_t row = 0; row < m_rows.size(); ++row )
    {
        const int               r = static_cast<int>( row );
        GENERATED_TABLE_COLUMN& col = m_rows[row].m_Column;

        m_rows[row].m_Shown = !m_columnGrid->GetCellValue( r, COL_SHOW ).IsEmpty();
        col.m_Heading = m_columnGrid->GetCellValue( r, COL_HEADING );

        const int align = aligns.Index( m_columnGrid->GetCellValue( r, COL_ALIGN ) );

        if( align != wxNOT_FOUND )
            col.m_Align = static_cast<GENERATED_TABLE_ALIGN>( align );

        col.m_Width = std::clamp( m_columnGrid->GetOptionalUnitValue( r, COL_WIDTH ).value_or( 0 ), 0,
                                  GENERATED_TABLE_MAX_COLUMN_WIDTH );
    }

    return true;
}


void DIALOG_GENERATED_TABLE_PROPERTIES::setColumns( const std::vector<GENERATED_TABLE_COLUMN>& aColumns )
{
    m_rows.clear();

    for( const GENERATED_TABLE_COLUMN& col : aColumns )
        m_rows.push_back( { col, true } );

    for( const GENERATED_TABLE_COLUMN_DEF& def : m_table->Schema().Defs() )
    {
        const bool present = std::any_of( aColumns.begin(), aColumns.end(),
                                          [&def]( const GENERATED_TABLE_COLUMN& aCol )
                                          {
                                              return aCol.m_Id == def.m_Id;
                                          } );

        if( !present )
            m_rows.push_back( { m_table->Schema().DefaultColumn( def.m_Id ), false } );
    }

    fillColumnGrid();
}


std::vector<GENERATED_TABLE_COLUMN> DIALOG_GENERATED_TABLE_PROPERTIES::shownColumns() const
{
    std::vector<GENERATED_TABLE_COLUMN> columns;

    for( const COLUMN_ROW& row : m_rows )
    {
        if( row.m_Shown )
            columns.push_back( row.m_Column );
    }

    return columns;
}


std::optional<GENERATED_TABLE_FORMAT> DIALOG_GENERATED_TABLE_PROPERTIES::getFormat()
{
    if( !harvestColumnGrid() )
        return std::nullopt;

    GENERATED_TABLE_FORMAT format;
    format.m_Columns = shownColumns();
    format.m_Units = static_cast<GENERATED_TABLE_UNITS>( m_unitsCtrl->GetSelection() );
    format.m_Precision = m_precisionCtrl->GetValue();

    return format;
}


void DIALOG_GENERATED_TABLE_PROPERTIES::setFormat( const GENERATED_TABLE_FORMAT& aFormat )
{
    setColumns( aFormat.m_Columns );
    m_unitsCtrl->SetSelection( static_cast<int>( aFormat.m_Units ) );
    m_precisionCtrl->SetValue( aFormat.m_Precision );
}


bool DIALOG_GENERATED_TABLE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // After DIALOG_SHIM has restored the last page, which is otherwise whatever the user
    // happened to leave open the time before
    m_notebook->SetSelection( 0 );

    m_LayerSelectionCtrl->SetLayerSelection( m_table->GetLayer() );
    m_cbLocked->SetValue( m_table->IsLocked() );

    m_unitsCtrl->SetSelection( static_cast<int>( m_table->GetUnits() ) );
    m_precisionCtrl->SetValue( m_table->GetPrecision() );

    setColumns( m_table->Columns() );

    if( m_options )
        m_options->TransferToWindow( *m_table );

    m_borderCheckbox->SetValue( m_table->StrokeExternal() );
    m_headerBorder->SetValue( m_table->StrokeHeaderSeparator() );
    m_rowSeparators->SetValue( m_table->StrokeRows() );
    m_colSeparators->SetValue( m_table->StrokeColumns() );

    if( m_table->GetBorderStroke().GetWidth() >= 0 )
        m_borderWidth.SetValue( m_table->GetBorderStroke().GetWidth() );

    if( m_table->GetSeparatorsStroke().GetWidth() >= 0 )
        m_separatorsWidth.SetValue( m_table->GetSeparatorsStroke().GetWidth() );

    m_borderWidth.Enable( m_table->StrokeExternal() || m_table->StrokeHeaderSeparator() );
    m_separatorsWidth.Enable( m_table->StrokeRows() || m_table->StrokeColumns() );

    return true;
}


bool DIALOG_GENERATED_TABLE_PROPERTIES::TransferDataFromWindow()
{
    if( !harvestColumnGrid() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::vector<GENERATED_TABLE_COLUMN> columns = shownColumns();

    if( columns.empty() )
    {
        DisplayError( this, _( "A table needs at least one column." ) );
        return false;
    }

    if( !m_table->Schema().Validate( columns ) )
    {
        DisplayError( this, _( "The columns are too wide in total." ) );
        return false;
    }

    BOARD*                  board = m_frame->GetBoard();
    BOARD_COMMIT            commit( m_frame );
    GENERATED_TABLE_REFRESH refresh( *board );

    commit.Modify( m_table );

    if( m_options && !m_options->TransferFromWindow( *m_table, refresh ) )
    {
        commit.Revert();
        return false;
    }

    m_table->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_table->SetLocked( m_cbLocked->GetValue() );

    m_table->SetUnits( static_cast<GENERATED_TABLE_UNITS>( m_unitsCtrl->GetSelection() ) );
    m_table->SetPrecision( m_precisionCtrl->GetValue() );

    m_table->Columns() = columns;

    m_table->SetStrokeExternal( m_borderCheckbox->GetValue() );
    m_table->SetStrokeHeaderSeparator( m_headerBorder->GetValue() );
    m_table->SetStrokeRows( m_rowSeparators->GetValue() );
    m_table->SetStrokeColumns( m_colSeparators->GetValue() );

    const auto strokeWidth =
            []( bool aStroked, UNIT_BINDER& aWidth )
            {
                return aStroked ? std::max( 0, aWidth.GetIntValue() ) : -1;
            };

    STROKE_PARAMS border = m_table->GetBorderStroke();
    border.SetWidth( strokeWidth( m_borderCheckbox->GetValue() || m_headerBorder->GetValue(), m_borderWidth ) );
    m_table->SetBorderStroke( border );

    STROKE_PARAMS separators = m_table->GetSeparatorsStroke();
    separators.SetWidth( strokeWidth( m_rowSeparators->GetValue() || m_colSeparators->GetValue(), m_separatorsWidth ) );
    m_table->SetSeparatorsStroke( separators );

    // Committed, or the table reads fresh while board state it was built against is still pending
    m_table->RebuildCells( *board, &refresh );
    refresh.Commit();

    commit.Push( wxString::Format( _( "Edit %s" ), m_table->GetFriendlyName() ), SKIP_CONNECTIVITY );

    return true;
}


void DIALOG_GENERATED_TABLE_PROPERTIES::fitColumnGrid()
{
    const int cols = m_columnGrid->GetNumberCols();
    const int width = m_columnGrid->GetClientSize().x;

    if( cols < 1 || width < 1 )
        return;

    // The checkbox and choice columns keep the width they auto-sized to. Whatever is left
    // over goes to the two text columns, so the grid has no unused strip down its side
    int fixed = 0;

    for( int col = 0; col < cols; ++col )
    {
        if( col != COL_NAME && col != COL_HEADING )
            fixed += m_columnGrid->GetColSize( col );
    }

    const int spare = width - fixed;

    if( spare < 2 )
        return;

    m_columnGrid->SetColSize( COL_NAME, spare / 2 );
    m_columnGrid->SetColSize( COL_HEADING, spare - spare / 2 );
}


void DIALOG_GENERATED_TABLE_PROPERTIES::onColumnGridSize( wxSizeEvent& aEvent )
{
    fitColumnGrid();
    aEvent.Skip();
}


void DIALOG_GENERATED_TABLE_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    const int defaultWidth =
            m_frame->GetDesignSettings().GetLineThickness( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    const bool border = m_borderCheckbox->GetValue() || m_headerBorder->GetValue();
    const bool separators = m_rowSeparators->GetValue() || m_colSeparators->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( defaultWidth );

    if( separators && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( defaultWidth );

    m_borderWidth.Enable( border );
    m_separatorsWidth.Enable( separators );
}


void DIALOG_GENERATED_TABLE_PROPERTIES::moveColumn( int aDelta )
{
    if( !harvestColumnGrid() )
        return;

    const int row = m_columnGrid->GetGridCursorRow();
    const int target = row + aDelta;

    if( row < 0 || target < 0 || target >= static_cast<int>( m_rows.size() ) )
        return;

    std::swap( m_rows[row], m_rows[target] );

    fillColumnGrid();
    m_columnGrid->SetGridCursor( target, m_columnGrid->GetGridCursorCol() );
    m_columnGrid->SelectRow( target );
}


void DIALOG_GENERATED_TABLE_PROPERTIES::onMoveUp( wxCommandEvent& aEvent )
{
    moveColumn( -1 );
}


void DIALOG_GENERATED_TABLE_PROPERTIES::onMoveDown( wxCommandEvent& aEvent )
{
    moveColumn( 1 );
}


void DIALOG_GENERATED_TABLE_PROPERTIES::onResetColumns( wxCommandEvent& aEvent )
{
    setColumns( m_table->Schema().Defaults() );
}
