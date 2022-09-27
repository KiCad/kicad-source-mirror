/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiplatform/ui.h>
#include <confirm.h>
#include <pad.h>
#include <macros.h>
#include <wildcards_and_files_ext.h>
#include <widgets/wx_grid.h>
#include <wx/filedlg.h>

#define COL_LABEL 0
#define COL_AMOUNT 1

// Defines for components view
#define ROW_LABEL 0
#define COL_FRONT_SIDE 1
#define COL_BOTTOM_SIDE 2
#define COL_TOTAL 3

// Defines for board view
#define ROW_BOARD_WIDTH 0
#define ROW_BOARD_HEIGHT 1
#define ROW_BOARD_AREA 2


/**
 * The dialog last saved state.
 */
struct DIALOG_BOARD_STATISTICS_SAVED_STATE
{
    DIALOG_BOARD_STATISTICS_SAVED_STATE() :
            excludeNoPins( false ),
            subtractHoles( false ),
            saveReportInitialized(false)
    {
    }

    // Flags to remember last checkboxes state
    bool excludeNoPins;
    bool subtractHoles;

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
        m_parentFrame(aParentFrame),
        m_boardWidth( 0 ),
        m_boardHeight( 0 ),
        m_boardArea( 0.0 ),
        m_hasOutline( false ),
        m_startLayerColInitialSize( 1 ),
        m_stopLayerColInitialSize( 1 )
{
    m_gridDrills->Connect( wxEVT_GRID_COL_SORT,
                           wxGridEventHandler( DIALOG_BOARD_STATISTICS::drillGridSort ),
                           nullptr, this );

    m_checkBoxExcludeComponentsNoPins->SetValue( s_savedDialogState.excludeNoPins );
    m_checkBoxSubtractHoles->SetValue( s_savedDialogState.subtractHoles );

    // Make labels for grids
    wxFont headingFont = KIUI::GetStatusFont( this );
    m_gridComponents->SetCellValue( ROW_LABEL, COL_FRONT_SIDE, _( "Front Side" ) );
    m_gridComponents->SetCellFont( ROW_LABEL, COL_FRONT_SIDE, headingFont );
    m_gridComponents->SetCellValue( ROW_LABEL, COL_BOTTOM_SIDE, _( "Back Side" ) );
    m_gridComponents->SetCellFont( ROW_LABEL, COL_BOTTOM_SIDE, headingFont );
    m_gridComponents->SetCellValue( ROW_LABEL, COL_TOTAL, _( "Total" ) );
    m_gridComponents->SetCellFont( ROW_LABEL, COL_TOTAL, headingFont );

    m_gridBoard->SetCellValue( 0, 0, _( "Width:" ) );
    m_gridBoard->SetCellAlignment( 0, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridBoard->SetCellValue( 1, 0, _( "Height:" ) );
    m_gridBoard->SetCellAlignment( 1, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridBoard->SetCellValue( 2, 0, _( "Area:" ) );
    m_gridBoard->SetCellAlignment( 2, 0, wxALIGN_LEFT, wxALIGN_CENTRE );

    wxGrid* grids[] = { m_gridComponents, m_gridPads, m_gridVias, m_gridBoard };

    for( auto& grid : grids )
    {
        // Remove wxgrid's selection boxes
        grid->SetCellHighlightPenWidth( 0 );
        grid->SetColMinimalAcceptableWidth( 80 );
        for( int i = 0; i < grid->GetNumberRows(); i++ )
            grid->SetCellAlignment( i, COL_LABEL, wxALIGN_LEFT, wxALIGN_CENTRE );
    }

    wxFileName fn = m_parentFrame->GetBoard()->GetFileName();

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
    m_fpTypes.clear();

    // If you need some more types to be shown, simply add them to the corresponding list
    m_fpTypes.push_back( FP_LINE_ITEM( FP_THROUGH_HOLE,        FP_THROUGH_HOLE, _( "THT:" ) ) );
    m_fpTypes.push_back( FP_LINE_ITEM( FP_SMD,                 FP_SMD,          _( "SMD:" ) ) );
    m_fpTypes.push_back( FP_LINE_ITEM( FP_THROUGH_HOLE|FP_SMD, 0,               _( "Unspecified:" ) ) );

    m_padTypes.clear();
    m_padTypes.push_back( LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::PTH,  _( "Through hole:" ) ) );
    m_padTypes.push_back( LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::SMD,  _( "SMD:" ) ) );
    m_padTypes.push_back( LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::CONN, _( "Connector:" ) ) );
    m_padTypes.push_back( LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::NPTH, _( "NPTH:" ) ) );

    m_viaTypes.clear();
    m_viaTypes.push_back( LINE_ITEM<VIATYPE>( VIATYPE::THROUGH,      _( "Through vias:" ) ) );
    m_viaTypes.push_back( LINE_ITEM<VIATYPE>( VIATYPE::BLIND_BURIED, _( "Blind/buried:" ) ) );
    m_viaTypes.push_back( LINE_ITEM<VIATYPE>( VIATYPE::MICROVIA,     _( "Micro vias:" ) ) );

    // If there not enough rows in grids, append some
    int appendRows = m_fpTypes.size() + 2 - m_gridComponents->GetNumberRows();

    if( appendRows > 0 )
        m_gridComponents->AppendRows( appendRows );

    appendRows = m_padTypes.size() + 1 - m_gridPads->GetNumberRows();

    if( appendRows > 0 )
        m_gridPads->AppendRows( appendRows );

    appendRows = m_viaTypes.size() + 1 - m_gridVias->GetNumberRows();

    if( appendRows )
        m_gridVias->AppendRows( appendRows );
}


bool DIALOG_BOARD_STATISTICS::TransferDataToWindow()
{
    refreshItemsTypes();
    getDataFromPCB();
    updateWidets();

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
    BOARD* board = m_parentFrame->GetBoard();
    m_drillTypes.clear();

    // Get footprints and pads count
    for( FOOTPRINT* footprint : board->Footprints() )
    {
        // Do not proceed footprints with no pads if checkbox checked
        if( m_checkBoxExcludeComponentsNoPins->GetValue() && ! footprint->Pads().size() )
            continue;

        // Go through components types list
        for( FP_LINE_ITEM& line : m_fpTypes )
        {
            if( ( footprint->GetAttributes() & line.attribute_mask ) == line.attribute_value )
            {
                if( footprint->IsFlipped() )
                    line.backSideQty++;
                else
                    line.frontSideQty++;
                break;
            }
        }

        for( PAD* pad : footprint->Pads() )
        {
            // Go through pads types list
            for( LINE_ITEM<PAD_ATTRIB>& line : m_padTypes )
            {
                if( pad->GetAttribute() == line.attribute )
                {
                    line.qty++;
                    break;
                }
            }

            if( pad->GetDrillSize().x > 0 && pad->GetDrillSize().y > 0 )
            {
                PCB_LAYER_ID top, bottom;

                if( pad->GetLayerSet().CuStack().empty() )
                {
                    // The pad is not on any copper layer
                    top = UNDEFINED_LAYER;
                    bottom = UNDEFINED_LAYER;
                }
                else
                {
                    top = pad->GetLayerSet().CuStack().front();
                    bottom = pad->GetLayerSet().CuStack().back();
                }

                DRILL_LINE_ITEM drill( pad->GetDrillSize().x, pad->GetDrillSize().y,
                                       pad->GetDrillShape(),
                                       pad->GetAttribute() != PAD_ATTRIB::NPTH,
                                       true, top, bottom );

                auto it = m_drillTypes.begin();

                for( ; it != m_drillTypes.end(); ++it )
                {
                    if( *it == drill )
                    {
                        it->qty++;
                        break;
                    }
                }

                if( it == m_drillTypes.end() )
                {
                    drill.qty = 1;
                    m_drillTypes.push_back( drill );
                    m_gridDrills->InsertRows();
                }
            }
        }
    }

    // Get via counts
    for( PCB_TRACK* track : board->Tracks() )
    {
        if( PCB_VIA* via = dyn_cast<PCB_VIA*>( track ) )
        {
            for( LINE_ITEM<VIATYPE>& line : m_viaTypes )
            {
                if( via->GetViaType() == line.attribute )
                {
                    line.qty++;
                    break;
                }
            }

            DRILL_LINE_ITEM drill( via->GetDrillValue(), via->GetDrillValue(),
                                   PAD_DRILL_SHAPE_CIRCLE, true, false, via->TopLayer(),
                                   via->BottomLayer() );

            auto it = m_drillTypes.begin();

            for( ; it != m_drillTypes.end(); ++it )
            {
                if( *it == drill )
                {
                    it->qty++;
                    break;
                }
            }

            if( it == m_drillTypes.end() )
            {
                drill.qty = 1;
                m_drillTypes.push_back( drill );
                m_gridDrills->InsertRows();
            }
        }
    }

    sort( m_drillTypes.begin(), m_drillTypes.end(),
          DRILL_LINE_ITEM::COMPARE( DRILL_LINE_ITEM::COL_COUNT, false ) );

    bool           boundingBoxCreated = false; //flag if bounding box initialized
    BOX2I          bbox;
    SHAPE_POLY_SET polySet;
    m_hasOutline = board->GetBoardPolygonOutlines( polySet );

    // If board has no Edge Cuts lines, board->GetBoardPolygonOutlines will
    // return small rectangle, so we double check that
    bool edgeCutsExists = false;

    for( BOARD_ITEM* drawing : board->Drawings() )
    {
        if( drawing->GetLayer() == Edge_Cuts )
        {
            edgeCutsExists = true;
            break;
        }
    }

    if( !edgeCutsExists )
        m_hasOutline = false;

    if( m_hasOutline )
    {
        m_boardArea = 0.0;

        for( int i = 0; i < polySet.OutlineCount(); i++ )
        {
            SHAPE_LINE_CHAIN& outline = polySet.Outline( i );
            m_boardArea += outline.Area();

            // If checkbox "subtract holes" is checked
            if( m_checkBoxSubtractHoles->GetValue() )
            {
                for( int j = 0; j < polySet.HoleCount( i ); j++ )
                    m_boardArea -= polySet.Hole( i, j ).Area();
            }

            if( boundingBoxCreated )
            {
                bbox.Merge( outline.BBox() );
            }
            else
            {
                bbox = outline.BBox();
                boundingBoxCreated = true;
            }
        }

        m_boardWidth = bbox.GetWidth();
        m_boardHeight = bbox.GetHeight();
    }
}


static wxString formatCount( int aCount )
{
    return wxString::Format( wxT( "%i" ), aCount );
};


void DIALOG_BOARD_STATISTICS::updateWidets()
{
    int totalPads  = 0;
    int row = 0;

    for( const LINE_ITEM<PAD_ATTRIB>& line : m_padTypes )
    {
        m_gridPads->SetCellValue( row, COL_LABEL, line.title );
        m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( line.qty ) );
        totalPads += line.qty;
        row++;
    }

    m_gridPads->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( totalPads ) );

    int totalVias = 0;
    row = 0;

    for( const LINE_ITEM<VIATYPE>& line : m_viaTypes )
    {
        m_gridVias->SetCellValue( row, COL_LABEL, line.title );
        m_gridVias->SetCellValue( row, COL_AMOUNT, formatCount( line.qty ) );
        totalVias += line.qty;
        row++;
    }

    m_gridVias->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridVias->SetCellValue( row, COL_AMOUNT, formatCount( totalVias ) );


    int totalFront = 0;
    int totalBack  = 0;

    // We don't use row 0, as there labels are
    row = 1;

    for( const FP_LINE_ITEM& line : m_fpTypes )
    {
        m_gridComponents->SetCellValue( row, COL_LABEL, line.title );
        m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( line.frontSideQty ) );
        m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( line.backSideQty ) );
        m_gridComponents->SetCellValue( row, 3, formatCount( line.frontSideQty + line.backSideQty ) );
        totalFront += line.frontSideQty;
        totalBack  += line.backSideQty;
        row++;
    }

    m_gridComponents->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( totalFront ) );
    m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( totalBack ) );
    m_gridComponents->SetCellValue( row, COL_TOTAL, formatCount( totalFront + totalBack ) );

    if( m_hasOutline )
    {
        m_gridBoard->SetCellValue( ROW_BOARD_WIDTH, COL_AMOUNT,
                                   m_parentFrame->MessageTextFromValue( m_boardWidth ) );
        m_gridBoard->SetCellValue( ROW_BOARD_HEIGHT, COL_AMOUNT,
                                   m_parentFrame->MessageTextFromValue( m_boardHeight ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT,
                                   m_parentFrame->MessageTextFromValue( m_boardArea, true,
                                                                        EDA_DATA_TYPE::AREA ) );
    }
    else
    {
        m_gridBoard->SetCellValue( ROW_BOARD_WIDTH, COL_AMOUNT, _( "unknown" ) );
        m_gridBoard->SetCellValue( ROW_BOARD_HEIGHT, COL_AMOUNT, _( "unknown" ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT, _( "unknown" ) );
    }

    updateDrillGrid();

    m_gridComponents->AutoSize();
    m_gridPads->AutoSize();
    m_gridBoard->AutoSize();
    m_gridVias->AutoSize();

    adjustDrillGridColumns();
}


void DIALOG_BOARD_STATISTICS::updateDrillGrid()
{
    BOARD* board = m_parentFrame->GetBoard();
    int    row = 0;

    for( const DRILL_LINE_ITEM& line : m_drillTypes )
    {
        wxString shapeStr;
        wxString startLayerStr;
        wxString stopLayerStr;

        switch( line.shape )
        {
        case PAD_DRILL_SHAPE_CIRCLE: shapeStr = _( "Round" ); break;
        case PAD_DRILL_SHAPE_OBLONG: shapeStr = _( "Slot" );  break;
        default:                     shapeStr = _( "???" );   break;
        }

        if( line.startLayer == UNDEFINED_LAYER )
            startLayerStr = _( "N/A" );
        else
            startLayerStr = board->GetLayerName( line.startLayer );

        if( line.stopLayer == UNDEFINED_LAYER )
            stopLayerStr = _( "N/A" );
        else
            stopLayerStr = board->GetLayerName( line.stopLayer );

        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_COUNT, formatCount( line.qty ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_SHAPE, shapeStr );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_X_SIZE,
                                    m_parentFrame->MessageTextFromValue( line.xSize ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_Y_SIZE,
                                    m_parentFrame->MessageTextFromValue( line.ySize ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_PLATED,
                                    line.isPlated ? _( "PTH" ) : _( "NPTH" ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_VIA_PAD,
                                    line.isPad ? _( "Pad" ) : _( "Via" ) );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_START_LAYER, startLayerStr );
        m_gridDrills->SetCellValue( row, DRILL_LINE_ITEM::COL_STOP_LAYER, stopLayerStr );

        row++;
    }
}


void DIALOG_BOARD_STATISTICS::printGridToStringAsTable( wxGrid* aGrid, wxString& aStr,
                                                        bool aUseColLabels,
                                                        bool aUseFirstColAsLabel )
{
    std::vector<int> widths( aGrid->GetNumberCols(), 0 );
    int              rowLabelsWidth = 0;

    // Determine column widths.

    if( aUseColLabels )
    {
        for( int col = 0; col < aGrid->GetNumberCols(); col++ )
            widths[col] = aGrid->GetColLabelValue( col ).length();
    }

    for( int row = 0; row < aGrid->GetNumberRows(); row++ )
    {
        rowLabelsWidth = std::max<int>( rowLabelsWidth, aGrid->GetRowLabelValue( row ).length() );

        for( int col = 0; col < aGrid->GetNumberCols(); col++ )
            widths[col] = std::max<int>( widths[col], aGrid->GetCellValue( row, col ).length() );
    }

    // Print the cells.

    wxString tmp;

    // Print column labels.

    aStr << wxT( "|" );

    for( int col = 0; col < aGrid->GetNumberCols(); col++ )
    {
        if( aUseColLabels )
            tmp.Printf( wxT( " %*s |" ), widths[col], aGrid->GetColLabelValue( col ) );
        else
            tmp.Printf( wxT( " %*s |" ), widths[col], aGrid->GetCellValue( 0, col ) );

        aStr << tmp;
    }

    aStr << wxT( "\n" );

    // Print column label horizontal separators.

    aStr << wxT( "|" );

    for( int col = 0; col < aGrid->GetNumberCols(); col++ )
    {
        aStr << wxT( "-" );
        aStr.Append( '-', widths[col] );
        aStr << wxT( "-|" );
    }

    aStr << wxT( "\n" );

    // Print regular cells.

    int firstRow = 0, firstCol = 0;

    if( !aUseColLabels )
        firstRow = 1;

    if( aUseFirstColAsLabel )
        firstCol = 1;

    for( int row = firstRow; row < aGrid->GetNumberRows(); row++ )
    {
        if( aUseFirstColAsLabel )
            tmp.Printf( wxT( "|%-*s  |" ), widths[0], aGrid->GetCellValue( row, 0 ) );
        else
            tmp.Printf( wxT( "|" ) );

        aStr << tmp;

        for( int col = firstCol; col < aGrid->GetNumberCols(); col++ )
        {
            tmp.Printf( wxT( " %*s |" ), widths[col], aGrid->GetCellValue( row, col ) );
            aStr << tmp;
        }

        aStr << wxT( "\n" );
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
                                     remainingWidth
                                     / ( m_startLayerColInitialSize + m_stopLayerColInitialSize ) );
    int startLayerColWidth = static_cast<int>( m_startLayerColInitialSize * scalingFactor );
    int stopLayerColWidth = static_cast<int>( m_stopLayerColInitialSize * scalingFactor );

    m_gridDrills->SetColSize( DRILL_LINE_ITEM::COL_START_LAYER, startLayerColWidth );
    m_gridDrills->SetColSize( DRILL_LINE_ITEM::COL_STOP_LAYER, stopLayerColWidth );
}


void DIALOG_BOARD_STATISTICS::checkboxClicked( wxCommandEvent& aEvent )
{
    s_savedDialogState.excludeNoPins = m_checkBoxExcludeComponentsNoPins->GetValue();
    s_savedDialogState.subtractHoles = m_checkBoxSubtractHoles->GetValue();
    refreshItemsTypes();
    getDataFromPCB();
    updateWidets();
    Layout();
    m_drillsPanel->Layout();
}


void DIALOG_BOARD_STATISTICS::saveReportClicked( wxCommandEvent& aEvent )
{
    FILE*    outFile;
    wxString msg;
    wxString boardName;

    wxFileName fn = m_parentFrame->GetBoard()->GetFileName();
    boardName = fn.GetName();
    wxFileDialog dlg( this, _( "Save Report File" ), s_savedDialogState.saveReportFolder,
                      s_savedDialogState.saveReportName, TextFileWildcard(),
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

    msg << _( "PCB statistics report\n=====================" ) << wxT( "\n" );
    msg << wxS( "- " ) << _( "Date" ) << wxS( ": " ) << wxDateTime::Now().Format() << wxT( "\n" );
    msg << wxS( "- " ) << _( "Project" ) << wxS( ": " )<< Prj().GetProjectName() << wxT( "\n" );
    msg << wxS( "- " ) << _( "Board name" ) << wxS( ": " )<< boardName << wxT( "\n" );

    msg << wxT( "\n" );
    msg << _( "Board" ) << wxT( "\n-----\n" );

    if( m_hasOutline )
    {
        msg << wxS( "- " ) << _( "Width" ) << wxS( ": " )
                << m_parentFrame->MessageTextFromValue( m_boardWidth ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Height" ) << wxS( ": " )
                << m_parentFrame->MessageTextFromValue( m_boardHeight ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Area" ) + wxS( ": " )
                << m_parentFrame->MessageTextFromValue( m_boardArea, true, EDA_DATA_TYPE::AREA );
        msg << wxT( "\n" );
    }
    else
    {
        msg << wxS( "- " ) << _( "Width" ) << wxS( ": " ) << _( "unknown" ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Height" ) << wxS( ": " ) << _( "unknown" ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Area" ) << wxS( ": " ) << _( "unknown" ) << wxT( "\n" );
    }

    msg << wxT( "\n" );
    msg << _( "Pads" ) << wxT( "\n----\n" );

    for( auto& type : m_padTypes )
        msg << wxT( "- " ) << type.title << wxS( " " ) << type.qty << wxT( "\n" );

    msg << wxT( "\n" );
    msg << _( "Vias" ) << wxT( "\n----\n" );

    for( auto& type : m_viaTypes )
        msg << wxT( "- " ) << type.title << wxS( " " ) << type.qty << wxT( "\n" );

    // We will save data about components in the table.
    // We have to calculate column widths
    std::vector<int>      widths;
    std::vector<wxString> labels{ wxT( "" ), _( "Front Side" ), _( "Back Side" ), _( "Total" ) };
    wxString tmp;

    widths.reserve( labels.size() );

    for( const wxString& label : labels )
        widths.push_back( label.size() );

    int frontTotal = 0;
    int backTotal = 0;

    for( const FP_LINE_ITEM& line : m_fpTypes )
    {
        // Get maximum width for left label column
        widths[0] = std::max<int>( line.title.size(), widths[0] );
        frontTotal += line.frontSideQty;
        backTotal += line.backSideQty;
    }

    // Get maximum width for other columns
    tmp.Printf( wxT( "%i" ), frontTotal );
    widths[1] = std::max<int>( tmp.size(), widths[1] );
    tmp.Printf( wxT( "%i" ), backTotal );
    widths[2] = std::max<int>( tmp.size(), widths[2] );
    tmp.Printf( wxT( "%i" ), frontTotal + backTotal );
    widths[3] = std::max<int>( tmp.size(), widths[3] );

    //Write components amount to file
    msg << wxT( "\n" );
    msg << _( "Components" ) << wxT( "\n----------\n" );
    msg << wxT( "\n" );

    printGridToStringAsTable( m_gridComponents, msg, false, true );

    msg << wxT( "\n" );
    msg << _( "Drill holes" ) << wxT( "\n-----------\n" );
    msg << wxT( "\n" );

    printGridToStringAsTable( m_gridDrills, msg, true, false );

    if( fprintf( outFile, "%s", TO_UTF8( msg ) ) < 0 )
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

    sort( m_drillTypes.begin(), m_drillTypes.end(), DRILL_LINE_ITEM::COMPARE( colId, ascending ) );

    updateDrillGrid();
}


DIALOG_BOARD_STATISTICS::~DIALOG_BOARD_STATISTICS()
{
}
