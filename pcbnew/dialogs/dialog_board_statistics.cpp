/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wildcards_and_files_ext.h>

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
 * Struct containing the dialog last saved state
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
        m_boardWidth( 0 ),
        m_boardHeight( 0 ),
        m_boardArea( 0.0 ),
        m_hasOutline( false )
{
    m_parentFrame = aParentFrame;

    m_gridDrills->UseNativeColHeader();
    m_gridDrills->Connect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_BOARD_STATISTICS::drillGridSort ), NULL, this );

    m_checkBoxExcludeComponentsNoPins->SetValue( s_savedDialogState.excludeNoPins );
    m_checkBoxSubtractHoles->SetValue( s_savedDialogState.subtractHoles );

    // Make labels for grids
    wxFont headingFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    headingFont.SetSymbolicSize( wxFONTSIZE_SMALL );
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
        fn.SetName( fn.GetName() + "_report" );
        fn.SetExt( "txt" );
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
    m_componentsTypes.clear();

    // If you need some more types to be shown, simply add them to the
    // corresponding list
    m_componentsTypes.push_back( componentsType_t( MOD_DEFAULT, _( "THT:" ) ) );
    m_componentsTypes.push_back( componentsType_t( MOD_CMS, _( "SMD:" ) ) );
    m_componentsTypes.push_back( componentsType_t( MOD_VIRTUAL, _( "Virtual:" ) ) );

    m_padsTypes.clear();
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_STANDARD, _( "Through hole:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_SMD, _( "SMD:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_CONN, _( "Connector:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_HOLE_NOT_PLATED, _( "NPTH:" ) ) );

    m_viasTypes.clear();
    m_viasTypes.push_back( viasType_t( VIA_THROUGH, _( "Through vias:" ) ) );
    m_viasTypes.push_back( viasType_t( VIA_BLIND_BURIED, _( "Blind/buried:" ) ) );
    m_viasTypes.push_back( viasType_t( VIA_MICROVIA, _( "Micro vias:" ) ) );

    // If there not enough rows in grids, append some
    int appendRows = m_componentsTypes.size() + 2 - m_gridComponents->GetNumberRows();

    if( appendRows > 0 )
        m_gridComponents->AppendRows( appendRows );

    appendRows = m_padsTypes.size() + 1 - m_gridPads->GetNumberRows();

    if( appendRows > 0 )
        m_gridPads->AppendRows( appendRows );

    appendRows = m_viasTypes.size() + 1 - m_gridVias->GetNumberRows();

    if( appendRows )
        m_gridVias->AppendRows( appendRows );
}

bool DIALOG_BOARD_STATISTICS::TransferDataToWindow()
{
    refreshItemsTypes();
    getDataFromPCB();
    updateWidets();
    Layout();
    drillsPanel->Layout();
    FinishDialogSettings();
    return true;
}

void DIALOG_BOARD_STATISTICS::getDataFromPCB()
{
    auto board = m_parentFrame->GetBoard();

    // Get modules and pads count
    for( MODULE* module : board->Modules() )
    {
        auto& pads = module->Pads();

        // Do not proceed modules with no pads if checkbox checked
        if( m_checkBoxExcludeComponentsNoPins->GetValue() && !pads.size() )
            continue;

        // Go through components types list
        for( auto& type : m_componentsTypes )
        {
            if( module->GetAttributes() == type.attribute )
            {
                if( module->IsFlipped() )
                    type.backSideQty++;
                else
                    type.frontSideQty++;
                break;
            }
        }

        for( auto& pad : pads )
        {
            // Go through pads types list
            for( auto& type : m_padsTypes )
            {
                if( pad->GetAttribute() == type.attribute )
                {
                    type.qty++;
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

                drillType_t drill( pad->GetDrillSize().x, pad->GetDrillSize().y,
                        pad->GetDrillShape(), pad->GetAttribute() != PAD_ATTRIB_HOLE_NOT_PLATED,
                        true, top, bottom );

                auto it = m_drillTypes.begin();
                for( ; it != m_drillTypes.end(); it++ )
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
    for( auto& track : board->Tracks() )
    {
        if( auto via = dyn_cast<VIA*>( track ) )
        {
            for( auto& type : m_viasTypes )
            {
                if( via->GetViaType() == type.attribute )
                {
                    type.qty++;
                    break;
                }
            }

            drillType_t drill( via->GetDrillValue(), via->GetDrillValue(), PAD_DRILL_SHAPE_CIRCLE,
                    true, false, via->TopLayer(), via->BottomLayer() );

            auto it = m_drillTypes.begin();
            for( ; it != m_drillTypes.end(); it++ )
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
            drillType_t::COMPARE( drillType_t::COL_COUNT, false ) );

    bool           boundingBoxCreated = false; //flag if bounding box initialized
    BOX2I          bbox;
    SHAPE_POLY_SET polySet;
    m_hasOutline = board->GetBoardPolygonOutlines( polySet );

    // If board has no Edge Cuts lines, board->GetBoardPolygonOutlines will
    // return small rectangle, so we double check that
    bool edgeCutsExists = false;
    for( auto& drawing : board->Drawings() )
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
            m_boardArea += std::fabs( outline.Area() );

            // If checkbox "subtract holes" is checked
            if( m_checkBoxSubtractHoles->GetValue() )
            {
                for( int j = 0; j < polySet.HoleCount( i ); j++ )
                    m_boardArea -= std::fabs( polySet.Hole( i, j ).Area() );
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

        if( GetUserUnits() == INCHES )
            m_boardArea /= ( IU_PER_MILS * IU_PER_MILS * 1000000 );
        else
            m_boardArea /= ( IU_PER_MM * IU_PER_MM );

        m_boardWidth = bbox.GetWidth();
        m_boardHeight = bbox.GetHeight();
    }
}

void DIALOG_BOARD_STATISTICS::updateWidets()
{
    int totalPads = 0;
    int currentRow = 0;

    for( auto& type : m_padsTypes )
    {
        m_gridPads->SetCellValue( currentRow, COL_LABEL, type.title );
        m_gridPads->SetCellValue(
                currentRow, COL_AMOUNT, wxString::Format( wxT( "%i " ), type.qty ) );
        totalPads += type.qty;
        currentRow++;
    }

    m_gridPads->SetCellValue( currentRow, COL_LABEL, _( "Total:" ) );
    m_gridPads->SetCellValue( currentRow, COL_AMOUNT, wxString::Format( "%i ", totalPads ) );

    int totalVias = 0;
    currentRow = 0;

    for( auto& type : m_viasTypes )
    {
        m_gridVias->SetCellValue( currentRow, COL_LABEL, type.title );
        m_gridVias->SetCellValue(
                currentRow, COL_AMOUNT, wxString::Format( "%i ", type.qty ) );
        totalVias += type.qty;
        currentRow++;
    }

    m_gridVias->SetCellValue( currentRow, COL_LABEL, _( "Total:" ) );
    m_gridVias->SetCellValue( currentRow, COL_AMOUNT, wxString::Format( "%i ", totalVias ) );


    int totalFront = 0;
    int totalBack = 0;

    // We don't use row 0, as there labels are
    currentRow = 1;

    for( auto& type : m_componentsTypes )
    {
        m_gridComponents->SetCellValue( currentRow, COL_LABEL, type.title );
        m_gridComponents->SetCellValue(
                currentRow, COL_FRONT_SIDE, wxString::Format( "%i ", type.frontSideQty ) );
        m_gridComponents->SetCellValue(
                currentRow, COL_BOTTOM_SIDE, wxString::Format( "%i ", type.backSideQty ) );
        m_gridComponents->SetCellValue( currentRow, 3,
                wxString::Format( wxT( "%i " ), type.frontSideQty + type.backSideQty ) );
        totalFront += type.frontSideQty;
        totalBack += type.backSideQty;
        currentRow++;
    }

    m_gridComponents->SetCellValue( currentRow, COL_LABEL, _( "Total:" ) );
    m_gridComponents->SetCellValue( currentRow, COL_FRONT_SIDE,
                                    wxString::Format( "%i ", totalFront ) );
    m_gridComponents->SetCellValue( currentRow, COL_BOTTOM_SIDE,
                                    wxString::Format( "%i ", totalBack ) );
    m_gridComponents->SetCellValue( currentRow, COL_TOTAL,
                                    wxString::Format( "%i ", totalFront + totalBack ) );

    if( m_hasOutline )
    {
        m_gridBoard->SetCellValue( ROW_BOARD_WIDTH, COL_AMOUNT,
                                   MessageTextFromValue( GetUserUnits(), m_boardWidth ) + " " );
        m_gridBoard->SetCellValue( ROW_BOARD_HEIGHT, COL_AMOUNT,
                                   MessageTextFromValue( GetUserUnits(), m_boardHeight ) + " " );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT,
                                   wxString::Format( wxT( "%.3f %s²" ),
                                                     m_boardArea,
                                                     GetAbbreviatedUnitsLabel( GetUserUnits() ) ) );
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
    m_gridDrills->AutoSize();

    adjustDrillGridColumns();
}

void DIALOG_BOARD_STATISTICS::updateDrillGrid()
{
    BOARD* board = m_parentFrame->GetBoard();
    int    currentRow = 0;

    for( auto& type : m_drillTypes )
    {
        wxString shapeStr;
        wxString startLayerStr;
        wxString stopLayerStr;

        switch( type.shape )
        {
        case PAD_DRILL_SHAPE_CIRCLE:
            shapeStr = _( "Round" );
            break;
        case PAD_DRILL_SHAPE_OBLONG:
            shapeStr = _( "Slot" );
            break;
        default:
            shapeStr = _( "???" );
            break;
        }

        if( type.startLayer == UNDEFINED_LAYER )
            startLayerStr = _( "N/A" );
        else
            startLayerStr = board->GetLayerName( type.startLayer );

        if( type.stopLayer == UNDEFINED_LAYER )
            stopLayerStr = _( "N/A" );
        else
            stopLayerStr = board->GetLayerName( type.stopLayer );

        m_gridDrills->SetCellValue(
                currentRow, drillType_t::COL_COUNT, wxString::Format( "%i", type.qty ) );
        m_gridDrills->SetCellValue( currentRow, drillType_t::COL_SHAPE, shapeStr );
        m_gridDrills->SetCellValue( currentRow, drillType_t::COL_X_SIZE,
                MessageTextFromValue( GetUserUnits(), type.xSize ) );
        m_gridDrills->SetCellValue( currentRow, drillType_t::COL_Y_SIZE,
                MessageTextFromValue( GetUserUnits(), type.ySize ) );
        m_gridDrills->SetCellValue(
                currentRow, drillType_t::COL_PLATED, type.isPlated ? _( "PTH" ) : _( "NPTH" ) );
        m_gridDrills->SetCellValue(
                currentRow, drillType_t::COL_VIA_PAD, type.isPad ? _( "Pad" ) : _( "Via" ) );
        m_gridDrills->SetCellValue( currentRow, drillType_t::COL_START_LAYER, startLayerStr );
        m_gridDrills->SetCellValue( currentRow, drillType_t::COL_STOP_LAYER, stopLayerStr );

        currentRow++;
    }
}

void DIALOG_BOARD_STATISTICS::printGridToStringAsTable( wxGrid* aGrid, wxString& aStr,
        bool aUseRowLabels, bool aUseColLabels, bool aUseFirstColAsLabel )
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

    aStr << "|";

    if( aUseRowLabels )
    {
        aStr.Append( ' ', rowLabelsWidth );
        aStr << " |";
    }

    for( int col = 0; col < aGrid->GetNumberCols(); col++ )
    {
        if( aUseColLabels )
            tmp.Printf( " %*s |", widths[col], aGrid->GetColLabelValue( col ) );
        else
            tmp.Printf( " %*s |", widths[col], aGrid->GetCellValue( 0, col ) );
        aStr << tmp;
    }

    aStr << "\n";

    // Print column label horizontal separators.

    aStr << "|";

    if( aUseRowLabels )
    {
        aStr.Append( '-', rowLabelsWidth );
        aStr << "-|";
    }

    for( int col = 0; col < aGrid->GetNumberCols(); col++ )
    {
        aStr << "-";
        aStr.Append( '-', widths[col] );
        aStr << "-|";
    }

    aStr << "\n";

    // Print regular cells.

    int firstRow = 0, firstCol = 0;

    if( !aUseColLabels )
        firstRow = 1;

    if( !aUseRowLabels && aUseFirstColAsLabel )
        firstCol = 1;

    for( int row = firstRow; row < aGrid->GetNumberRows(); row++ )
    {
        if( aUseRowLabels )
            tmp.Printf( "|%-*s |", rowLabelsWidth, aGrid->GetRowLabelValue( row ) );
        else if( aUseFirstColAsLabel )
            tmp.Printf( "|%-*s  |", widths[0], aGrid->GetCellValue( row, 0 ) );
        else
            tmp.Printf( "|" );
        aStr << tmp;

        for( int col = firstCol; col < aGrid->GetNumberCols(); col++ )
        {
            tmp.Printf( " %*s |", widths[col], aGrid->GetCellValue( row, col ) );
            aStr << tmp;
        }
        aStr << "\n";
    }
}

void DIALOG_BOARD_STATISTICS::adjustDrillGridColumns()
{
    int newTotalWidth = m_gridDrills->GetClientSize().GetWidth();
    int curTotalWidth = 0;

    // Find the total current width
    for( int i = 0; i < m_gridDrills->GetNumberCols(); i++ )
    {
        if( i != drillType_t::COL_START_LAYER && i != drillType_t::COL_STOP_LAYER )
            curTotalWidth += m_gridDrills->GetColSize( i );
    }

    // Resize the last two columns to fill all available space

    int remainingWidth = newTotalWidth - curTotalWidth;

    m_gridDrills->SetColSize( drillType_t::COL_START_LAYER, remainingWidth / 2 );
    m_gridDrills->SetColSize( drillType_t::COL_STOP_LAYER, remainingWidth - remainingWidth / 2 );

    m_gridDrills->Refresh();
}

// If any checkbox clicked, we have to refresh dialog data
void DIALOG_BOARD_STATISTICS::checkboxClicked( wxCommandEvent& aEvent )
{
    s_savedDialogState.excludeNoPins = m_checkBoxExcludeComponentsNoPins->GetValue();
    s_savedDialogState.subtractHoles = m_checkBoxSubtractHoles->GetValue();
    refreshItemsTypes();
    getDataFromPCB();
    updateWidets();
    Layout();
    drillsPanel->Layout();
}

void DIALOG_BOARD_STATISTICS::saveReportClicked( wxCommandEvent& aEvent )
{
    FILE*    outFile;
    wxString msg;
    wxString boardName;

    wxFileName fn = m_parentFrame->GetBoard()->GetFileName();
    boardName = fn.GetName();
    wxFileDialog saveFileDialog( this, _( "Save Report File" ),
                                 s_savedDialogState.saveReportFolder,
                                 s_savedDialogState.saveReportName,
                                 TextFileWildcard(),
                                 wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveFileDialog.ShowModal() == wxID_CANCEL )
        return;

    s_savedDialogState.saveReportFolder = wxPathOnly( saveFileDialog.GetPath() );
    s_savedDialogState.saveReportName = saveFileDialog.GetFilename();

    outFile = wxFopen( saveFileDialog.GetPath(), "wt" );

    if( outFile == NULL )
    {
        msg.Printf( _( "Unable to create file \"%s\"" ), saveFileDialog.GetPath() );
        DisplayErrorMessage( this, msg );
        return;
    }

    msg << _( "PCB statistics report\n=====================" ) << "\n";
    msg << _( "- Date: " ) << wxDateTime::Now().Format() << "\n";
    msg << _( "- Project: " ) << Prj().GetProjectName() << "\n";
    msg << _( "- Board name: " ) << boardName << "\n";

    msg << "\n";
    msg << _( "Board\n-----" ) << "\n";

    if( m_hasOutline )
    {
        msg << _( "- Width: " ) << MessageTextFromValue( GetUserUnits(), m_boardWidth ) << "\n";
        msg << _( "- Height: " ) << MessageTextFromValue( GetUserUnits(), m_boardHeight ) << "\n";
        msg << _( "- Area: " )
            << wxString::Format(
                       wxT( "%.3f %s^2" ), m_boardArea, GetAbbreviatedUnitsLabel( GetUserUnits() ) )
            << "\n";
    }
    else
    {
        msg << _( "- Width: " ) << _( "unknown" ) << "\n";
        msg << _( "- Height: " ) << _( "unknown" ) << "\n";
        msg << _( "- Area: " ) << _( "unknown" ) << "\n";
    }

    msg << "\n";
    msg << _( "Pads\n----" ) << "\n";

    for( auto& type : m_padsTypes )
        msg << "- " << type.title << " " << type.qty << "\n";

    msg << "\n";
    msg << _( "Vias\n----" ) << "\n";

    for( auto& type : m_viasTypes )
        msg << "- " << type.title << " " << type.qty << "\n";

    // We will save data about components in the table.
    // We have to calculate column widths
    std::vector<int>      widths;
    std::vector<wxString> labels{ "", _( "Front Side" ), _( "Back Side" ), _( "Total" ) };
    wxString tmp;

    widths.reserve( labels.size() );
    for( const auto& label : labels )
        widths.push_back( label.size() );

    int frontTotal = 0;
    int backTotal = 0;

    for( const auto& type : m_componentsTypes )
    {
        // Get maximum width for left label column
        widths[0] = std::max<int>( type.title.size(), widths[0] );
        frontTotal += type.frontSideQty;
        backTotal += type.backSideQty;
    }

    // Get maximum width for other columns
    tmp.Printf( "%i", frontTotal );
    widths[1] = std::max<int>( tmp.size(), widths[1] );
    tmp.Printf( "%i", backTotal );
    widths[2] = std::max<int>( tmp.size(), widths[2] );
    tmp.Printf( "%i", frontTotal + backTotal );
    widths[3] = std::max<int>( tmp.size(), widths[3] );

    //Write components amount to file
    msg << "\n";
    msg << _( "Components\n----------" ) << "\n";
    msg << "\n";

    printGridToStringAsTable( m_gridComponents, msg, false, false, true );

    msg << "\n";
    msg << _( "Drill holes\n-----------" ) << "\n";
    msg << "\n";

    printGridToStringAsTable( m_gridDrills, msg, false, true, false );

    if( fprintf( outFile, "%s", TO_UTF8( msg ) ) < 0 )
    {
        msg.Printf( _( "Error writing to file \"%s\"" ), saveFileDialog.GetPath() );
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
    drillType_t::COL_ID colId = static_cast<drillType_t::COL_ID>( aEvent.GetCol() );
    bool ascending =
            !( m_gridDrills->IsSortingBy( colId ) && m_gridDrills->IsSortOrderAscending() );

    sort( m_drillTypes.begin(), m_drillTypes.end(), drillType_t::COMPARE( colId, ascending ) );

    updateDrillGrid();
}

DIALOG_BOARD_STATISTICS::~DIALOG_BOARD_STATISTICS()
{
}
