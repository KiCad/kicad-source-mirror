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

#include "dialog_drill_chart_properties.h"

#include <board.h>
#include <drill/drill_chart_template.h>
#include <board_commit.h>
#include <confirm.h>
#include <board_design_settings.h>
#include <drill/drill_chart_model.h>
#include <drill/drill_symbol_profile.h>
#include <pcb_base_edit_frame.h>
#include <pcb_drill_chart.h>
#include <pcb_layer_box_selector.h>
#include <widgets/wx_grid.h>
#include <widgets/wx_infobar.h>

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/grid.h>


namespace
{

enum COLUMN_GRID_COL
{
    COL_SHOW = 0,
    COL_NAME,
    COL_HEADING,
    COL_ALIGN,
    COL_GROUP_BY
};


// The grouping key a column reports on, for the columns that report on one. Grouping is the
// board's, not the chart's, so ticking one here changes every chart and map on the board.
bool columnGroupKey( DRILL_CHART_COLUMN_ID aId, DRILL_GROUP_KEY& aKey )
{
    switch( aId )
    {
    case DRILL_CHART_COLUMN_ID::DRILL_DIAMETER: aKey = DRILL_GROUP_KEY::SIZE_GRP; return true;
    case DRILL_CHART_COLUMN_ID::SLOT_SIZE:      aKey = DRILL_GROUP_KEY::SLOT; return true;
    case DRILL_CHART_COLUMN_ID::PLATING:        aKey = DRILL_GROUP_KEY::PLATING; return true;
    case DRILL_CHART_COLUMN_ID::LAYER_SPAN:     aKey = DRILL_GROUP_KEY::SPAN; return true;
    case DRILL_CHART_COLUMN_ID::OPERATION:      aKey = DRILL_GROUP_KEY::OPERATION; return true;
    case DRILL_CHART_COLUMN_ID::PROTECTION:     aKey = DRILL_GROUP_KEY::PROTECTION; return true;
    default:                                    return false;
    }
}


/// Every column a chart can carry, in the order the grid offers them
const DRILL_CHART_COLUMN_ID g_allColumns[] = {
    DRILL_CHART_COLUMN_ID::SYMBOL,          DRILL_CHART_COLUMN_ID::DRILL_DIAMETER,
    DRILL_CHART_COLUMN_ID::SLOT_SIZE,       DRILL_CHART_COLUMN_ID::PLATING,
    DRILL_CHART_COLUMN_ID::OPERATION_COUNT, DRILL_CHART_COLUMN_ID::SITE_COUNT,
    DRILL_CHART_COLUMN_ID::LAYER_SPAN,      DRILL_CHART_COLUMN_ID::OPERATION,
    DRILL_CHART_COLUMN_ID::PROTECTION,      DRILL_CHART_COLUMN_ID::BACKDRILL_STUB,
    DRILL_CHART_COLUMN_ID::ASPECT_RATIO,    DRILL_CHART_COLUMN_ID::DESCRIPTION
};


wxString columnLabel( DRILL_CHART_COLUMN_ID aId )
{
    switch( aId )
    {
    case DRILL_CHART_COLUMN_ID::SYMBOL:          return _( "Symbol" );
    case DRILL_CHART_COLUMN_ID::DRILL_DIAMETER:  return _( "Drill diameter" );
    case DRILL_CHART_COLUMN_ID::SLOT_SIZE:       return _( "Slot size" );
    case DRILL_CHART_COLUMN_ID::PLATING:         return _( "Plating" );
    case DRILL_CHART_COLUMN_ID::OPERATION_COUNT: return _( "Operation count" );
    case DRILL_CHART_COLUMN_ID::SITE_COUNT:      return _( "Site count" );
    case DRILL_CHART_COLUMN_ID::LAYER_SPAN:      return _( "Layer span" );
    case DRILL_CHART_COLUMN_ID::OPERATION:       return _( "Operation" );
    case DRILL_CHART_COLUMN_ID::PROTECTION:      return _( "Protection" );
    case DRILL_CHART_COLUMN_ID::BACKDRILL_STUB:  return _( "Backdrill stub" );
    case DRILL_CHART_COLUMN_ID::ASPECT_RATIO:    return _( "Aspect ratio" );
    case DRILL_CHART_COLUMN_ID::DESCRIPTION:     return _( "Description" );
    }

    return wxEmptyString;
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


DIALOG_DRILL_CHART_PROPERTIES::DIALOG_DRILL_CHART_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame,
                                                              PCB_DRILL_CHART*     aChart ) :
        DIALOG_DRILL_CHART_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_chart( aChart ),
        m_borderWidth( aFrame, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_separatorsWidth( aFrame, m_separatorsWidthLabel, m_separatorsWidthCtrl,
                           m_separatorsWidthUnits )
{
    // Not merely "not copper". Silkscreen, mask, paste, adhesive, Edge.Cuts, Margin and
    // courtyard are manufacturing inputs this would corrupt rather than document
    m_LayerSelectionCtrl->SetNotAllowedLayerSet( ~DrillDocumentationLayers() );
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    m_columnGrid->SetColLabelValue( COL_SHOW, _( "Show" ) );
    m_columnGrid->SetColLabelValue( COL_NAME, _( "Column" ) );
    m_columnGrid->SetColLabelValue( COL_HEADING, _( "Heading" ) );
    m_columnGrid->SetColLabelValue( COL_ALIGN, _( "Align" ) );
    m_columnGrid->SetColLabelValue( COL_GROUP_BY, _( "Group By" ) );

    m_columnGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_columnGrid->Bind( wxEVT_SIZE, &DIALOG_DRILL_CHART_PROPERTIES::onColumnGridSize, this );

    SetupStandardButtons();
    finishDialogSettings();
}


DIALOG_DRILL_CHART_PROPERTIES::~DIALOG_DRILL_CHART_PROPERTIES()
{
}


void DIALOG_DRILL_CHART_PROPERTIES::fillColumnGrid()
{
    if( m_columnGrid->GetNumberRows() )
        m_columnGrid->DeleteRows( 0, m_columnGrid->GetNumberRows() );

    m_columnGrid->AppendRows( static_cast<int>( m_columns.size() ) );

    const wxArrayString aligns = alignChoices();

    for( size_t row = 0; row < m_columns.size(); ++row )
    {
        const int r = static_cast<int>( row );

        m_columnGrid->SetCellRenderer( r, COL_SHOW, new wxGridCellBoolRenderer );
        m_columnGrid->SetCellEditor( r, COL_SHOW, new wxGridCellBoolEditor );
        m_columnGrid->SetCellValue( r, COL_SHOW, m_shown[row] ? wxT( "1" ) : wxEmptyString );

        m_columnGrid->SetCellValue( r, COL_NAME, columnLabel( m_columns[row].m_Id ) );
        m_columnGrid->SetReadOnly( r, COL_NAME );

        m_columnGrid->SetCellValue( r, COL_HEADING, m_columns[row].m_Heading );

        m_columnGrid->SetCellEditor( r, COL_ALIGN, new wxGridCellChoiceEditor( aligns ) );
        m_columnGrid->SetCellValue( r, COL_ALIGN,
                                    aligns[static_cast<int>( m_columns[row].m_Align )] );

        DRILL_GROUP_KEY key;

        if( columnGroupKey( m_columns[row].m_Id, key ) )
        {
            m_columnGrid->SetCellRenderer( r, COL_GROUP_BY, new wxGridCellBoolRenderer );
            m_columnGrid->SetCellEditor( r, COL_GROUP_BY, new wxGridCellBoolEditor );
            m_columnGrid->SetCellValue( r, COL_GROUP_BY,
                                        m_profile.IsGroupedBy( key ) ? wxT( "1" )
                                                                     : wxEmptyString );
        }
        else
        {
            // Nothing to group on, so an empty read-only cell rather than a dead checkbox
            m_columnGrid->SetReadOnly( r, COL_GROUP_BY );
        }
    }

    m_columnGrid->AutoSizeColumns();
    fitColumnGrid();
}


bool DIALOG_DRILL_CHART_PROPERTIES::harvestColumnGrid()
{
    if( !m_columnGrid->CommitPendingChanges() )
        return false;

    const wxArrayString aligns = alignChoices();

    for( size_t row = 0; row < m_columns.size(); ++row )
    {
        const int r = static_cast<int>( row );

        m_shown[row] = !m_columnGrid->GetCellValue( r, COL_SHOW ).IsEmpty();
        m_columns[row].m_Heading = m_columnGrid->GetCellValue( r, COL_HEADING );

        const int align = aligns.Index( m_columnGrid->GetCellValue( r, COL_ALIGN ) );

        if( align != wxNOT_FOUND )
            m_columns[row].m_Align = static_cast<DRILL_CHART_ALIGN>( align );

        DRILL_GROUP_KEY key;

        if( columnGroupKey( m_columns[row].m_Id, key ) )
        {
            m_profile.SetGroupedBy( key,
                                    !m_columnGrid->GetCellValue( r, COL_GROUP_BY ).IsEmpty() );
        }
    }

    return true;
}


bool DIALOG_DRILL_CHART_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // After DIALOG_SHIM has restored the last page, which is otherwise whatever the user
    // happened to leave open the time before
    m_notebook->SetSelection( 0 );

    m_profile = m_frame->GetBoard()->GetDesignSettings().GetDrillSymbolProfile();

    m_LayerSelectionCtrl->SetLayerSelection( m_chart->GetLayer() );
    m_cbLocked->SetValue( m_chart->IsLocked() );

    m_unitsCtrl->SetSelection( static_cast<int>( m_chart->GetUnits() ) );
    m_precisionCtrl->SetValue( m_chart->GetPrecision() );

    const DRILL_CHART_FILTER& filter = m_chart->Filter();
    m_filterPlated->SetValue( filter.m_Plated );
    m_filterNonPlated->SetValue( filter.m_NonPlated );
    m_filterVias->SetValue( filter.m_Vias );
    m_filterSlots->SetValue( filter.m_Slots );
    m_filterBackdrills->SetValue( filter.m_Backdrills );
    m_filterCastellated->SetValue( filter.m_Castellated );

    m_showTotals->SetValue( m_chart->GetShowTotals() );

    // Chart columns first in their own order, then whatever is left, so unchecking and
    // rechecking a column does not lose the heading the user typed
    m_columns = m_chart->Columns();
    m_shown.assign( m_columns.size(), true );

    const DRILL_CHART_TEMPLATE defaults = DRILL_CHART_TEMPLATE::MakeDefault();

    for( DRILL_CHART_COLUMN_ID id : g_allColumns )
    {
        const bool present = std::any_of( m_columns.begin(), m_columns.end(),
                                          [id]( const DRILL_CHART_COLUMN& aCol )
                                          {
                                              return aCol.m_Id == id;
                                          } );

        if( present )
            continue;

        DRILL_CHART_COLUMN col;
        col.m_Id = id;
        col.m_Heading = columnLabel( id ).Upper();

        for( const DRILL_CHART_COLUMN& def : defaults.Columns() )
        {
            if( def.m_Id == id )
            {
                col = def;
                break;
            }
        }

        m_columns.push_back( col );
        m_shown.push_back( false );
    }

    fillColumnGrid();

    m_borderCheckbox->SetValue( m_chart->StrokeExternal() );
    m_headerBorder->SetValue( m_chart->StrokeHeaderSeparator() );
    m_rowSeparators->SetValue( m_chart->StrokeRows() );
    m_colSeparators->SetValue( m_chart->StrokeColumns() );

    if( m_chart->GetBorderStroke().GetWidth() >= 0 )
        m_borderWidth.SetValue( m_chart->GetBorderStroke().GetWidth() );

    if( m_chart->GetSeparatorsStroke().GetWidth() >= 0 )
        m_separatorsWidth.SetValue( m_chart->GetSeparatorsStroke().GetWidth() );

    m_borderWidth.Enable( m_chart->StrokeExternal() || m_chart->StrokeHeaderSeparator() );
    m_separatorsWidth.Enable( m_chart->StrokeRows() || m_chart->StrokeColumns() );

    return true;
}


bool DIALOG_DRILL_CHART_PROPERTIES::TransferDataFromWindow()
{
    if( !harvestColumnGrid() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::vector<DRILL_CHART_COLUMN> columns;

    for( size_t row = 0; row < m_columns.size(); ++row )
    {
        if( m_shown[row] )
            columns.push_back( m_columns[row] );
    }

    if( columns.empty() )
    {
        DisplayError( this, _( "A drill chart needs at least one column." ) );
        return false;
    }

    BOARD*       board = m_frame->GetBoard();
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_chart );

    m_chart->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_chart->SetLocked( m_cbLocked->GetValue() );

    m_chart->SetUnits( static_cast<DRILL_CHART_UNITS>( m_unitsCtrl->GetSelection() ) );
    m_chart->SetPrecision( m_precisionCtrl->GetValue() );
    m_chart->SetShowTotals( m_showTotals->GetValue() );

    DRILL_CHART_FILTER& filter = m_chart->Filter();
    filter.m_Plated = m_filterPlated->GetValue();
    filter.m_NonPlated = m_filterNonPlated->GetValue();
    filter.m_Vias = m_filterVias->GetValue();
    filter.m_Slots = m_filterSlots->GetValue();
    filter.m_Backdrills = m_filterBackdrills->GetValue();
    filter.m_Castellated = m_filterCastellated->GetValue();

    m_chart->Columns() = columns;

    m_chart->SetStrokeExternal( m_borderCheckbox->GetValue() );
    m_chart->SetStrokeHeaderSeparator( m_headerBorder->GetValue() );
    m_chart->SetStrokeRows( m_rowSeparators->GetValue() );
    m_chart->SetStrokeColumns( m_colSeparators->GetValue() );

    {
        STROKE_PARAMS stroke = m_chart->GetBorderStroke();
        stroke.SetWidth( m_borderCheckbox->GetValue() || m_headerBorder->GetValue()
                                 ? std::max( 0, m_borderWidth.GetIntValue() )
                                 : -1 );
        m_chart->SetBorderStroke( stroke );
    }

    {
        STROKE_PARAMS stroke = m_chart->GetSeparatorsStroke();
        stroke.SetWidth( m_rowSeparators->GetValue() || m_colSeparators->GetValue()
                                 ? std::max( 0, m_separatorsWidth.GetIntValue() )
                                 : -1 );
        m_chart->SetSeparatorsStroke( stroke );
    }

    DRILL_SYMBOL_PROFILE&           boardProfile = board->GetDesignSettings().GetDrillSymbolProfile();
    const std::set<DRILL_GROUP_KEY> wasGroupedBy = boardProfile.GroupKeys();

    for( DRILL_CHART_COLUMN_ID id : g_allColumns )
    {
        DRILL_GROUP_KEY key;

        if( columnGroupKey( id, key ) )
            boardProfile.SetGroupedBy( key, m_profile.IsGroupedBy( key ) );
    }

    if( boardProfile.GroupKeys() != wasGroupedBy )
    {
        // Grouping is board-wide, so every chart and map has to be rebuilt against it
        board->BumpDrillModelGeneration();
        m_frame->OnModify();
    }

    // New assignments are discarded on purpose. The profile is not on the undo stack, so
    // persisting here would survive an undo of this very edit
    m_chart->RebuildCells( *board );

    commit.Push( _( "Edit Drill Chart" ), SKIP_CONNECTIVITY );

    return true;
}


void DIALOG_DRILL_CHART_PROPERTIES::fitColumnGrid()
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


void DIALOG_DRILL_CHART_PROPERTIES::onColumnGridSize( wxSizeEvent& aEvent )
{
    fitColumnGrid();
    aEvent.Skip();
}


void DIALOG_DRILL_CHART_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    const int defaultWidth = m_frame->GetDesignSettings().GetLineThickness(
            ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    const bool border = m_borderCheckbox->GetValue() || m_headerBorder->GetValue();
    const bool separators = m_rowSeparators->GetValue() || m_colSeparators->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( defaultWidth );

    if( separators && m_separatorsWidth.GetValue() < 0 )
        m_separatorsWidth.SetValue( defaultWidth );

    m_borderWidth.Enable( border );
    m_separatorsWidth.Enable( separators );
}


void DIALOG_DRILL_CHART_PROPERTIES::moveColumn( int aDelta )
{
    if( !harvestColumnGrid() )
        return;

    const int row = m_columnGrid->GetGridCursorRow();
    const int target = row + aDelta;

    if( row < 0 || target < 0 || target >= static_cast<int>( m_columns.size() ) )
        return;

    std::swap( m_columns[row], m_columns[target] );

    // std::vector<bool> has no swap-by-reference, so go through a copy
    const bool shown = m_shown[row];
    m_shown[row] = m_shown[target];
    m_shown[target] = shown;

    fillColumnGrid();
    m_columnGrid->SetGridCursor( target, m_columnGrid->GetGridCursorCol() );
    m_columnGrid->SelectRow( target );
}


void DIALOG_DRILL_CHART_PROPERTIES::onMoveUp( wxCommandEvent& aEvent )
{
    moveColumn( -1 );
}


void DIALOG_DRILL_CHART_PROPERTIES::onMoveDown( wxCommandEvent& aEvent )
{
    moveColumn( 1 );
}


void DIALOG_DRILL_CHART_PROPERTIES::applyTemplate( const DRILL_CHART_TEMPLATE& aTemplate )
{
    m_columns = aTemplate.Columns();
    m_shown.assign( m_columns.size(), true );

    // Columns the template does not use stay in the grid unchecked, so turning one back on
    // does not mean retyping its heading
    for( DRILL_CHART_COLUMN_ID id : g_allColumns )
    {
        const bool present = std::any_of( m_columns.begin(), m_columns.end(),
                                          [id]( const DRILL_CHART_COLUMN& aCol )
                                          {
                                              return aCol.m_Id == id;
                                          } );

        if( present )
            continue;

        DRILL_CHART_COLUMN col;
        col.m_Id = id;
        col.m_Heading = columnLabel( id ).Upper();
        m_columns.push_back( col );
        m_shown.push_back( false );
    }

    fillColumnGrid();
}


void DIALOG_DRILL_CHART_PROPERTIES::onResetColumns( wxCommandEvent& aEvent )
{
    applyTemplate( DRILL_CHART_TEMPLATE::MakeDefault() );
}


void DIALOG_DRILL_CHART_PROPERTIES::onImportTemplate( wxCommandEvent& aEvent )
{
    wxFileDialog dlg( this, _( "Import Drill Chart Template" ), wxEmptyString, wxEmptyString,
                      _( "Drill chart templates" ) + wxT( " (*.json)|*.json" ),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DRILL_CHART_TEMPLATE tmpl;
    wxString             error;

    if( !tmpl.LoadFromFile( dlg.GetPath(), &error ) )
    {
        DisplayError( this, error );
        return;
    }

    applyTemplate( tmpl );

    m_unitsCtrl->SetSelection( static_cast<int>( tmpl.GetUnits() ) );
    m_precisionCtrl->SetValue( tmpl.GetPrecision() );
    m_showTotals->SetValue( tmpl.GetShowTotals() );
}


void DIALOG_DRILL_CHART_PROPERTIES::onExportTemplate( wxCommandEvent& aEvent )
{
    if( !harvestColumnGrid() )
        return;

    wxFileDialog dlg( this, _( "Export Drill Chart Template" ), wxEmptyString, wxT( "drill_chart" ),
                      _( "Drill chart templates" ) + wxT( " (*.json)|*.json" ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    DRILL_CHART_TEMPLATE tmpl;

    // The file names the template, so whoever opens the json can tell which one it is
    tmpl.SetName( wxFileName( dlg.GetPath() ).GetName() );
    tmpl.SetUnits( static_cast<DRILL_CHART_UNITS>( m_unitsCtrl->GetSelection() ) );
    tmpl.SetPrecision( m_precisionCtrl->GetValue() );
    tmpl.SetShowTotals( m_showTotals->GetValue() );

    for( size_t row = 0; row < m_columns.size(); ++row )
    {
        if( m_shown[row] )
            tmpl.Columns().push_back( m_columns[row] );
    }

    wxString error;

    if( !tmpl.SaveToFile( dlg.GetPath(), &error ) )
        DisplayError( this, error );
}

