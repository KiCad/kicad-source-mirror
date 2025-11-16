/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
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


#include "dialog_board_statistics.h"
#include <board_statistics.h>

#include <wx/filedlg.h>

#include <kiplatform/ui.h>
#include <confirm.h>
#include <pad.h>
#include <macros.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <widgets/wx_grid.h>
#include <convert_basic_shapes_to_polygon.h>
#include <algorithm>


#define COL_LABEL 0
#define COL_AMOUNT 1

// Defines for components view
#define ROW_LABEL 0
#define COL_FRONT_SIDE 1
#define COL_BOTTOM_SIDE 2
#define COL_TOTAL 3

// Defines for board view
#define ROW_BOARD_DIMS 0
#define ROW_BOARD_AREA 1
#define ROW_FRONT_COPPER_AREA 2
#define ROW_BACK_COPPER_AREA 3
#define ROW_MIN_CLEARANCE 4
#define ROW_MIN_TRACK_WIDTH 5
#define ROW_MIN_DRILL_DIAMETER 6
#define ROW_BOARD_THICKNESS 7
#define ROW_FOOTPRINT_COURTYARD_FRONT_AREA 8
#define ROW_FRONT_COMPONENT_DENSITY 9
#define ROW_FOOTPRINT_COURTYARD_BACK_AREA 10
#define ROW_BACK_COMPONENT_DENSITY 11


/**
 * The dialog last saved state.
 */
struct DIALOG_BOARD_STATISTICS_SAVED_STATE
{
    DIALOG_BOARD_STATISTICS_SAVED_STATE() :
            excludeNoPins( false ),
            subtractHoles( false ),
            subtractHolesFromCopper( false ),
            saveReportInitialized(false)
    {
    }

    // Flags to remember last checkboxes state
    bool excludeNoPins;
    bool subtractHoles;
    bool subtractHolesFromCopper;

    // Variables to save last report file name and folder
    bool     saveReportInitialized; // true after the 3 next string are initialized
    wxString saveReportFolder;      // last report folder
    wxString saveReportName;        // last report filename
    wxString m_project;             // name of the project used to create the last report
                                    // used to reinit last state after a project change
};


static DIALOG_BOARD_STATISTICS_SAVED_STATE s_savedDialogState;

DIALOG_BOARD_STATISTICS::DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame ) :
        DIALOG_BOARD_STATISTICS_BASE( aParentFrame ),
        m_frame( aParentFrame ),
        m_startLayerColInitialSize( 1 ),
        m_stopLayerColInitialSize( 1 )
{
    m_gridDrills->Connect( wxEVT_GRID_COL_SORT,
                           wxGridEventHandler( DIALOG_BOARD_STATISTICS::drillGridSort ),
                           nullptr, this );

    m_checkBoxExcludeComponentsNoPins->SetValue( s_savedDialogState.excludeNoPins );
    m_checkBoxSubtractHoles->SetValue( s_savedDialogState.subtractHoles );
    m_checkBoxSubtractHolesFromCopper->SetValue( s_savedDialogState.subtractHolesFromCopper );

    wxFont labelFont = KIUI::GetSmallInfoFont( this );
    m_componentsLabel->SetFont( labelFont );
    m_boardLabel->SetFont( labelFont );
    m_padsLabel->SetFont( labelFont );
    m_viasLabel->SetFont( labelFont );

    // Make labels for grids
    m_gridComponents->SetCellValue( ROW_LABEL, COL_FRONT_SIDE,  _( "Front Side" ) );
    m_gridComponents->SetCellValue( ROW_LABEL, COL_BOTTOM_SIDE, _( "Back Side" ) );
    m_gridComponents->SetCellValue( ROW_LABEL, COL_TOTAL,       _( "Total" ) );

    wxFont headingFont = KIUI::GetStatusFont( this );

    for( int col = COL_AMOUNT; col < m_gridComponents->GetNumberCols(); col++ )
        m_gridComponents->SetCellFont( ROW_LABEL, col, headingFont );

    m_gridBoard->SetCellValue( ROW_BOARD_DIMS,        COL_LABEL, _( "Dimensions:" ) );
    m_gridBoard->SetCellValue( ROW_BOARD_AREA,        COL_LABEL, _( "Area:" ) );
    m_gridBoard->SetCellValue( ROW_FRONT_COPPER_AREA, COL_LABEL, _( "Front copper area:" ) );
    m_gridBoard->SetCellValue( ROW_BACK_COPPER_AREA,  COL_LABEL, _( "Back copper area:" ) );
    m_gridBoard->SetCellValue( ROW_MIN_CLEARANCE,     COL_LABEL, _( "Min track clearance:" ) );
    m_gridBoard->SetCellValue( ROW_MIN_TRACK_WIDTH,   COL_LABEL, _( "Min track width:" ) );
    m_gridBoard->SetCellValue( ROW_MIN_DRILL_DIAMETER, COL_LABEL, _( "Min drill diameter:" ) );
    m_gridBoard->SetCellValue( ROW_BOARD_THICKNESS, COL_LABEL, _( "Board stackup thickness:" ) );
    m_gridBoard->SetCellValue( ROW_FOOTPRINT_COURTYARD_FRONT_AREA, COL_LABEL, _( "Front footprint area:" ) );
    m_gridBoard->SetCellValue( ROW_FRONT_COMPONENT_DENSITY, COL_LABEL, _( "Front footprint density:" ) );
    m_gridBoard->SetCellValue( ROW_FOOTPRINT_COURTYARD_BACK_AREA, COL_LABEL, _( "Back footprint area:" ) );
    m_gridBoard->SetCellValue( ROW_BACK_COMPONENT_DENSITY, COL_LABEL, _( "Back footprint density:" ) );

    for( wxGrid* grid : { m_gridComponents, m_gridPads, m_gridVias, m_gridBoard } )
    {
        // Remove wxgrid's selection boxes
        grid->SetCellHighlightPenWidth( 0 );
        grid->SetColMinimalAcceptableWidth( 80 );

        for( int row = 0; row < grid->GetNumberRows(); row++ )
        {
            grid->SetCellAlignment( row, COL_LABEL, wxALIGN_LEFT, wxALIGN_CENTRE );

            for( int col = COL_AMOUNT; col < grid->GetNumberCols(); col++ )
                grid->SetCellAlignment( row, col, wxALIGN_RIGHT, wxALIGN_CENTER );
        }
    }

    wxFileName fn = m_frame->GetBoard()->GetFileName();

    if( !s_savedDialogState.saveReportInitialized
            || s_savedDialogState.m_project != Prj().GetProjectFullName() )
    {
        fn.SetName( fn.GetName() + wxT( "_report" ) );
        fn.SetExt( wxT( "txt" ) );
        s_savedDialogState.saveReportName = fn.GetFullName();
        s_savedDialogState.saveReportFolder = wxPathOnly( Prj().GetProjectFullName() );
        s_savedDialogState.m_project = Prj().GetProjectFullName();
        s_savedDialogState.saveReportInitialized = true;
    }

    // The wxStdDialogButtonSizer wxID_CANCLE button is in fact a close button
    // Nothing to cancel:
    m_sdbControlSizerCancel->SetLabel( _( "Close" ) );
}


void DIALOG_BOARD_STATISTICS::refreshItemsTypes()
{
    InitializeBoardStatisticsData( m_statsData );

    int appendRows = static_cast<int>( m_statsData.footprintEntries.size() ) + 2 - m_gridComponents->GetNumberRows();

    if( appendRows > 0 )
        m_gridComponents->AppendRows( appendRows );

    appendRows = static_cast<int>( m_statsData.padEntries.size() ) + 1
                 + static_cast<int>( m_statsData.padPropertyEntries.size() ) - m_gridPads->GetNumberRows();

    if( appendRows > 0 )
        m_gridPads->AppendRows( appendRows );

    appendRows = static_cast<int>( m_statsData.viaEntries.size() ) + 1 - m_gridVias->GetNumberRows();

    if( appendRows > 0 )
        m_gridVias->AppendRows( appendRows );
}


bool DIALOG_BOARD_STATISTICS::TransferDataToWindow()
{
    refreshItemsTypes();
    getDataFromPCB();
    updateWidgets();

    Layout();
    m_drillsPanel->Layout();

    m_gridDrills->AutoSizeColumns();
    m_startLayerColInitialSize = m_gridDrills->GetColSize( DRILL_LINE_ITEM::COL_START_LAYER );
    m_stopLayerColInitialSize = m_gridDrills->GetColSize( DRILL_LINE_ITEM::COL_STOP_LAYER );

    // Add space for the vertical scrollbar, so that it won't overlap with the cells.
    m_gridDrills->SetMinSize( wxSize( m_gridDrills->GetEffectiveMinSize().x
                                            + wxSystemSettings::GetMetric( wxSYS_VSCROLL_X ),
                                      60 ) );

    adjustDrillGridColumns();

    finishDialogSettings();
    return true;
}


void DIALOG_BOARD_STATISTICS::getDataFromPCB()
{
    BOARD_STATISTICS_OPTIONS options;
    options.excludeFootprintsWithoutPads = m_checkBoxExcludeComponentsNoPins->GetValue();
    options.subtractHolesFromBoardArea = m_checkBoxSubtractHoles->GetValue();
    options.subtractHolesFromCopperAreas = m_checkBoxSubtractHolesFromCopper->GetValue();

    ComputeBoardStatistics( m_frame->GetBoard(), options, m_statsData );

    m_gridDrills->ClearRows();

    if( !m_statsData.drillEntries.empty() )
        m_gridDrills->AppendRows( static_cast<int>( m_statsData.drillEntries.size() ) );
}


static wxString formatCount( int aCount )
{
    return wxString::Format( wxT( "%i" ), aCount );
};


void DIALOG_BOARD_STATISTICS::updateWidgets()
{
    int totalPads  = 0;
    int row = 0;

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>& line : m_statsData.padEntries )
    {
        m_gridPads->SetCellValue( row, COL_LABEL, line.title );
        m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( line.quantity ) );
        totalPads += line.quantity;
        row++;
    }

    m_gridPads->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( totalPads ) );
    row++;

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>& line : m_statsData.padPropertyEntries )
    {
        m_gridPads->SetCellValue( row, COL_LABEL, line.title );
        m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( line.quantity ) );
        row++;
    }

    int totalVias = 0;
    row = 0;

    for( const BOARD_STATISTICS_INFO_ENTRY<VIATYPE>& line : m_statsData.viaEntries )
    {
        m_gridVias->SetCellValue( row, COL_LABEL, line.title );
        m_gridVias->SetCellValue( row, COL_AMOUNT, formatCount( line.quantity ) );
        totalVias += line.quantity;
        row++;
    }

    m_gridVias->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridVias->SetCellValue( row, COL_AMOUNT, formatCount( totalVias ) );


    int totalFront = 0;
    int totalBack  = 0;

    // We don't use row 0, as there labels are
    row = 1;

    for( const BOARD_STATISTICS_FP_ENTRY& line : m_statsData.footprintEntries )
    {
        m_gridComponents->SetCellValue( row, COL_LABEL, line.title );
        m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( line.frontCount ) );
        m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( line.backCount ) );
        m_gridComponents->SetCellValue( row, 3, formatCount( line.frontCount + line.backCount ) );
        totalFront += line.frontCount;
        totalBack += line.backCount;
        row++;
    }

    m_gridComponents->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( totalFront ) );
    m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( totalBack ) );
    m_gridComponents->SetCellValue( row, COL_TOTAL, formatCount( totalFront + totalBack ) );

    if( m_statsData.hasOutline )
    {
        m_gridBoard->SetCellValue( ROW_BOARD_DIMS, COL_AMOUNT,
                                   wxString::Format( wxT( "%s x %s" ),
                                                     m_frame->MessageTextFromValue( m_statsData.boardWidth, false ),
                                                     m_frame->MessageTextFromValue( m_statsData.boardHeight, true ) ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT,
                                   m_frame->MessageTextFromValue( m_statsData.boardArea, true, EDA_DATA_TYPE::AREA ) );

        m_gridBoard->SetCellValue( ROW_FRONT_COMPONENT_DENSITY, COL_AMOUNT,
                                   wxString::Format( "%.2f %%", m_statsData.frontFootprintDensity ) );

        m_gridBoard->SetCellValue( ROW_BACK_COMPONENT_DENSITY, COL_AMOUNT,
                                   wxString::Format( "%.2f %%", m_statsData.backFootprintDensity ) );
    }
    else
    {
        m_gridBoard->SetCellValue( ROW_BOARD_DIMS, COL_AMOUNT, _( "unknown" ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT, _( "unknown" ) );
    }

    m_gridBoard->SetCellValue(
            ROW_FOOTPRINT_COURTYARD_FRONT_AREA, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.frontFootprintCourtyardArea, true, EDA_DATA_TYPE::AREA ) );

    m_gridBoard->SetCellValue(
            ROW_FOOTPRINT_COURTYARD_BACK_AREA, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.backFootprintCourtyardArea, true, EDA_DATA_TYPE::AREA ) );

    m_gridBoard->SetCellValue(
            ROW_FRONT_COPPER_AREA, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.frontCopperArea, true, EDA_DATA_TYPE::AREA ) );
    m_gridBoard->SetCellValue( ROW_BACK_COPPER_AREA, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_statsData.backCopperArea, true, EDA_DATA_TYPE::AREA ) );

    m_gridBoard->SetCellValue(
            ROW_MIN_CLEARANCE, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.minClearanceTrackToTrack, true, EDA_DATA_TYPE::DISTANCE ) );

    m_gridBoard->SetCellValue(
            ROW_MIN_TRACK_WIDTH, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.minTrackWidth, true, EDA_DATA_TYPE::DISTANCE ) );

    m_gridBoard->SetCellValue(
            ROW_BOARD_THICKNESS, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.boardThickness, true, EDA_DATA_TYPE::DISTANCE ) );

    updateDrillGrid();

    m_gridBoard->SetCellValue(
            ROW_MIN_DRILL_DIAMETER, COL_AMOUNT,
            m_frame->MessageTextFromValue( m_statsData.minDrillSize, true, EDA_DATA_TYPE::DISTANCE ) );

    m_gridComponents->AutoSize();
    m_gridPads->AutoSize();
    m_gridBoard->AutoSize();
    m_gridVias->AutoSize();

    adjustDrillGridColumns();
}


void DIALOG_BOARD_STATISTICS::updateDrillGrid()
{
    BOARD* board = m_frame->GetBoard();
    int    row = 0;

    for( const DRILL_LINE_ITEM& line : m_statsData.drillEntries )
    {
        wxString shapeStr;
        wxString startLayerStr;
        wxString stopLayerStr;

        switch( line.shape )
        {
        case PAD_DRILL_SHAPE::CIRCLE: shapeStr = _( "Round" ); break;
        case PAD_DRILL_SHAPE::OBLONG: shapeStr = _( "Slot" );  break;
        default:                      shapeStr = _( "???" );   break;
        }

        if( line.startLayer == UNDEFINED_LAYER )
            startLayerStr = _( "N/A" );
        else
            startLayerStr = board->GetLayerName( line.startLayer );

        if( line.stopLayer == UNDEFINED_LAYER )
            stopLayerStr = _( "N/A" );
        else
            stopLayerStr = board->GetLayerName( line.stopLayer );

        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_COUNT, formatCount( line.m_Qty ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_SHAPE, shapeStr );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_X_SIZE, m_frame->MessageTextFromValue( line.xSize ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_Y_SIZE, m_frame->MessageTextFromValue( line.ySize ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_PLATED, line.isPlated ? _( "PTH" ) : _( "NPTH" ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_VIA_PAD, line.isPad ? _( "Pad" ) : _( "Via" ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_START_LAYER, startLayerStr );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_STOP_LAYER, stopLayerStr );

        row++;
    }
}


void DIALOG_BOARD_STATISTICS::adjustDrillGridColumns()
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_gridDrills );

    m_gridDrills->EnsureColLabelsVisible();

    double remainingWidth = KIPLATFORM::UI::GetUnobscuredSize( m_gridDrills ).x;

    // Find the total current width
    for( int i = 0; i < m_gridDrills->GetNumberCols(); i++ )
    {
        if( i != DRILL_LINE_ITEM::COL_START_LAYER && i != DRILL_LINE_ITEM::COL_STOP_LAYER )
            remainingWidth -= m_gridDrills->GetColSize( i );
    }

    double scalingFactor = std::max( 1.0,
                                     remainingWidth / ( m_startLayerColInitialSize + m_stopLayerColInitialSize ) );
    int startLayerColWidth = static_cast<int>( m_startLayerColInitialSize * scalingFactor );
    int stopLayerColWidth = static_cast<int>( m_stopLayerColInitialSize * scalingFactor );

    m_gridDrills->SetColSize( DRILL_LINE_ITEM::COL_START_LAYER, startLayerColWidth );
    m_gridDrills->SetColSize( DRILL_LINE_ITEM::COL_STOP_LAYER, stopLayerColWidth );
}


void DIALOG_BOARD_STATISTICS::checkboxClicked( wxCommandEvent& aEvent )
{
    s_savedDialogState.excludeNoPins = m_checkBoxExcludeComponentsNoPins->GetValue();
    s_savedDialogState.subtractHoles = m_checkBoxSubtractHoles->GetValue();
    s_savedDialogState.subtractHolesFromCopper = m_checkBoxSubtractHolesFromCopper->GetValue();
    refreshItemsTypes();
    getDataFromPCB();
    updateWidgets();
    Layout();
    m_drillsPanel->Layout();
}


void DIALOG_BOARD_STATISTICS::saveReportClicked( wxCommandEvent& aEvent )
{
    FILE*    outFile;
    wxString msg;
    wxString boardName;

    wxFileName fn = m_frame->GetBoard()->GetFileName();
    boardName = fn.GetName();
    wxFileDialog dlg( this, _( "Save Report File" ), s_savedDialogState.saveReportFolder,
                      s_savedDialogState.saveReportName, FILEEXT::TextFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    s_savedDialogState.saveReportFolder = wxPathOnly( dlg.GetPath() );
    s_savedDialogState.saveReportName = dlg.GetFilename();

    outFile = wxFopen( dlg.GetPath(), wxT( "wt" ) );

    if( outFile == nullptr )
    {
        msg.Printf( _( "Failed to create file '%s'." ), dlg.GetPath() );
        DisplayErrorMessage( this, msg );
        return;
    }

    const UNITS_PROVIDER& unitsProvider = *m_frame;
    wxString              report = FormatBoardStatisticsReport( m_statsData, m_frame->GetBoard(), unitsProvider,
                                                                Prj().GetProjectName(), boardName );

    if( fprintf( outFile, "%s", TO_UTF8( report ) ) < 0 )
    {
        msg.Printf( _( "Error writing file '%s'." ), dlg.GetPath() );
        DisplayErrorMessage( this, msg );
    }

    fclose( outFile );
}


void DIALOG_BOARD_STATISTICS::drillGridSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
    adjustDrillGridColumns();
}

void DIALOG_BOARD_STATISTICS::drillGridSort( wxGridEvent& aEvent )
{
    DRILL_LINE_ITEM::COL_ID colId = static_cast<DRILL_LINE_ITEM::COL_ID>( aEvent.GetCol() );
    bool                    ascending = !( m_gridDrills->IsSortingBy( colId )
                                              && m_gridDrills->IsSortOrderAscending() );

    sort( m_statsData.drillEntries.begin(), m_statsData.drillEntries.end(),
          DRILL_LINE_ITEM::COMPARE( colId, ascending ) );

    updateDrillGrid();
}


DIALOG_BOARD_STATISTICS::~DIALOG_BOARD_STATISTICS()
{
}
