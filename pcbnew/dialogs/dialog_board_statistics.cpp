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
        m_boardWidth( 0 ),
        m_boardHeight( 0 ),
        m_boardArea( 0.0 ),
        m_frontCopperArea( 0.0 ),
        m_backCopperArea( 0.0 ),
        m_minClearanceTrackToTrack( std::numeric_limits<int>::max() ),
        m_minTrackWidth( std::numeric_limits<int>::max() ),
        m_minDrillSize( std::numeric_limits<int>::max() ),
        m_boardThickness( 0 ),
        m_hasOutline( false ),
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
    m_gridBoard->SetCellValue( ROW_MIN_DRILL_DIAMETER,COL_LABEL, _( "Min drill diameter:" ) );
    m_gridBoard->SetCellValue( ROW_BOARD_THICKNESS,   COL_LABEL, _( "Board stackup thickness:" ) );

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
    m_fpTypes.clear();

    // If you need some more types to be shown, simply add them to the corresponding list
    m_fpTypes.push_back( FP_LINE_ITEM( FP_THROUGH_HOLE,        FP_THROUGH_HOLE, _( "THT:" ) ) );
    m_fpTypes.push_back( FP_LINE_ITEM( FP_SMD,                 FP_SMD,          _( "SMD:" ) ) );
    m_fpTypes.push_back( FP_LINE_ITEM( FP_THROUGH_HOLE|FP_SMD, 0,               _( "Unspecified:" ) ) );

    m_padTypes.clear();
    m_padTypes.push_back( INFO_LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::PTH,  _( "Through hole:" ) ) );
    m_padTypes.push_back( INFO_LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::SMD,  _( "SMD:" ) ) );
    m_padTypes.push_back( INFO_LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::CONN, _( "Connector:" ) ) );
    m_padTypes.push_back( INFO_LINE_ITEM<PAD_ATTRIB>( PAD_ATTRIB::NPTH, _( "NPTH:" ) ) );

    m_padFabProps.clear();
    m_padFabProps.push_back( INFO_LINE_ITEM<PAD_PROP>( PAD_PROP::CASTELLATED, _( "Castellated pad:" ) ) );
    m_padFabProps.push_back( INFO_LINE_ITEM<PAD_PROP>( PAD_PROP::PRESSFIT,    _( "Press-fit pad:" ) ) );

    m_viaTypes.clear();
    m_viaTypes.push_back( INFO_LINE_ITEM<VIATYPE>( VIATYPE::THROUGH,      _( "Through vias:" ) ) );
    m_viaTypes.push_back( INFO_LINE_ITEM<VIATYPE>( VIATYPE::BLIND_BURIED, _( "Blind/buried:" ) ) );
    m_viaTypes.push_back( INFO_LINE_ITEM<VIATYPE>( VIATYPE::MICROVIA,     _( "Micro vias:" ) ) );

    // If there not enough rows in grids, append some
    int appendRows = (int) m_fpTypes.size() + 2 - m_gridComponents->GetNumberRows();

    if( appendRows > 0 )
        m_gridComponents->AppendRows( appendRows );

    appendRows = (int) m_padTypes.size() + 1 + (int)m_padFabProps.size() - m_gridPads->GetNumberRows();

    if( appendRows > 0 )
        m_gridPads->AppendRows( appendRows );

    appendRows = (int) m_viaTypes.size() + 1 - m_gridVias->GetNumberRows();

    if( appendRows )
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
    BOARD* board = m_frame->GetBoard();
    SHAPE_POLY_SET frontCopper;
    SHAPE_POLY_SET backCopper;
    SHAPE_POLY_SET frontHoles;
    SHAPE_POLY_SET backHoles;

    // Type list for track-related statistics gathering
    static const std::vector<KICAD_T> trackTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T };

    // Get footprints and pads count
    for( FOOTPRINT* footprint : board->Footprints() )
    {
        // Do not proceed footprints with no pads if checkbox checked
        if( m_checkBoxExcludeComponentsNoPins->GetValue() && ! footprint->Pads().size() )
            continue;

        // Go through components types list
        for( FP_LINE_ITEM& line : m_fpTypes )
        {
            if( ( footprint->GetAttributes() & line.m_Attribute_mask ) == line.m_Attribute_value )
            {
                switch( footprint->GetSide() )
                {
                case F_Cu: line.m_FrontSideQty++;                  break;
                case B_Cu: line.m_BackSideQty++;                   break;
                default:   /* unsided: user-layers only, etc. */ break;
                }

                break;
            }
        }

        for( PAD* pad : footprint->Pads() )
        {
            // Go through pads types list
            for( INFO_LINE_ITEM<PAD_ATTRIB>& line : m_padTypes )
            {
                if( pad->GetAttribute() == line.m_Attribute )
                {
                    line.m_Qty++;
                    break;
                }
            }

            // Go through pads prop list
            for( INFO_LINE_ITEM<PAD_PROP>& line : m_padFabProps )
            {
                if( pad->GetProperty() == line.m_Attribute )
                {
                    line.m_Qty++;
                    break;
                }
            }
        }
    }

    // Get via counts
    for( PCB_TRACK* track : board->Tracks() )
    {
        // Get min track width
        if( track->Type() == PCB_TRACE_T )
            m_minTrackWidth = std::min( m_minTrackWidth, track->GetWidth() );

        if( !track->IsType( trackTypes ) )
            continue;

        // Get min clearance between tracks
        auto layer = track->GetLayer();
        auto trackShapeA = track->GetEffectiveShape( layer );

        for( PCB_TRACK* otherTrack : board->Tracks() )
        {
            if( layer != otherTrack->GetLayer() )
                continue;

            if( track->GetNetCode() == otherTrack->GetNetCode() )
                continue;

            if( !otherTrack->IsType( trackTypes ) )
                continue;

            int  actual = 0;
            auto trackShapeB = otherTrack->GetEffectiveShape( layer );
            bool collide = trackShapeA->Collide( trackShapeB.get(), m_minClearanceTrackToTrack, &actual );

            if( collide )
                m_minClearanceTrackToTrack = std::min( m_minClearanceTrackToTrack, actual );
        }

        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            for( INFO_LINE_ITEM<VIATYPE>& line : m_viaTypes )
            {
                if( via->GetViaType() == line.m_Attribute )
                {
                    line.m_Qty++;
                    break;
                }
            }
        }
    }

    // Collect drill information
    m_drillTypes.clear();
    m_gridDrills->ClearRows();

    std::vector<DRILL_LINE_ITEM> drills;
    CollectDrillLineItems( board, drills );

    for( const auto& d : drills )
    {
        m_drillTypes.push_back( d );
        m_gridDrills->InsertRows();
    }

    sort( m_drillTypes.begin(), m_drillTypes.end(),
          DRILL_LINE_ITEM::COMPARE( DRILL_LINE_ITEM::COL_COUNT, false ) );

    SHAPE_POLY_SET polySet;
    m_hasOutline = board->GetBoardPolygonOutlines( polySet );

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

                for( FOOTPRINT* fp : board->Footprints() )
                {
                    for( PAD* pad : fp->Pads() )
                    {
                        if( !pad->HasHole() )
                            continue;

                        auto hole = pad->GetEffectiveHoleShape();
                        const SEG& seg = hole->GetSeg();
                        double width = hole->GetWidth();
                        double area = seg.Length() * width;

                        // Each end of the hole is a half-circle, so together, we have one
                        // full circle.  The area of a circle is pi * r^2, so the area of the
                        // hole is pi * (d/2)^2 = pi * 1/4 * d^2.
                        area += M_PI * 0.25 * width * width;
                        m_boardArea -= area;
                    }
                }

                for( PCB_TRACK* track : board->Tracks() )
                {
                    if( track->Type() == PCB_VIA_T )
                    {
                        PCB_VIA* via = static_cast<PCB_VIA*>( track );
                        double drill = via->GetDrillValue();
                        m_boardArea -= M_PI * 0.25 * drill * drill;
                    }
                }
            }
        }

        // Compute the bounding box to get a rectangular size
        // We use the polySet bounding box, not the board bounding box, because
        // we do not want the thickness of graphic items defining the board outlines
        // to be taken in account to calculate the physical board bbox
        BOX2I bbox = polySet.BBox();

        m_boardWidth = (int) bbox.GetWidth();
        m_boardHeight = (int) bbox.GetHeight();
    }

    board->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                if( child->Type() == PCB_FOOTPRINT_T
                    || child->Type() == PCB_GROUP_T
                    || child->Type() == PCB_GENERATOR_T )
                {
                    // Wait for recursion into children
                    return;
                }

                if( child->IsOnLayer( F_Cu ) )
                    child->TransformShapeToPolySet( frontCopper, F_Cu, 0, ARC_LOW_DEF, ERROR_INSIDE );

                if( child->IsOnLayer( B_Cu ) )
                    child->TransformShapeToPolySet( backCopper, B_Cu, 0, ARC_LOW_DEF, ERROR_INSIDE );

                if( child->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( child );

                    if( pad->HasHole() )
                    {
                        pad->TransformHoleToPolygon( frontHoles, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
                        pad->TransformHoleToPolygon( backHoles, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
                    }
                }
                else if( child->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( child );
                    VECTOR2I center = via->GetPosition();
                    int      R = via->GetDrillValue() / 2;

                    if( via->IsOnLayer( F_Cu ) )
                        TransformCircleToPolygon( frontHoles, center, R, ARC_LOW_DEF, ERROR_OUTSIDE );

                    if( via->IsOnLayer( B_Cu ) )
                        TransformCircleToPolygon( backHoles, center, R, ARC_LOW_DEF, ERROR_OUTSIDE );
                }
            },
            RECURSE_MODE::RECURSE );

    if( m_checkBoxSubtractHolesFromCopper->GetValue() )
    {
        frontCopper.BooleanSubtract( frontHoles );
        backCopper.BooleanSubtract( backHoles );
    }

    m_frontCopperArea = frontCopper.Area();
    m_backCopperArea = backCopper.Area();

    m_boardThickness = board->GetStackupOrDefault().BuildBoardThicknessFromStackup();
}


static wxString formatCount( int aCount )
{
    return wxString::Format( wxT( "%i" ), aCount );
};


void DIALOG_BOARD_STATISTICS::updateWidgets()
{
    int totalPads  = 0;
    int row = 0;

    for( const INFO_LINE_ITEM<PAD_ATTRIB>& line : m_padTypes )
    {
        m_gridPads->SetCellValue( row, COL_LABEL, line.m_Title );
        m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( line.m_Qty ) );
        totalPads += line.m_Qty;
        row++;
    }

    m_gridPads->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( totalPads ) );
    row++;

    for( const INFO_LINE_ITEM<PAD_PROP>& line : m_padFabProps )
    {
        m_gridPads->SetCellValue( row, COL_LABEL, line.m_Title );
        m_gridPads->SetCellValue( row, COL_AMOUNT, formatCount( line.m_Qty ) );
        row++;
    }

    int totalVias = 0;
    row = 0;

    for( const INFO_LINE_ITEM<VIATYPE>& line : m_viaTypes )
    {
        m_gridVias->SetCellValue( row, COL_LABEL, line.m_Title );
        m_gridVias->SetCellValue( row, COL_AMOUNT, formatCount( line.m_Qty ) );
        totalVias += line.m_Qty;
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
        m_gridComponents->SetCellValue( row, COL_LABEL, line.m_Title );
        m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( line.m_FrontSideQty ) );
        m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( line.m_BackSideQty ) );
        m_gridComponents->SetCellValue( row, 3, formatCount( line.m_FrontSideQty + line.m_BackSideQty ) );
        totalFront += line.m_FrontSideQty;
        totalBack  += line.m_BackSideQty;
        row++;
    }

    m_gridComponents->SetCellValue( row, COL_LABEL, _( "Total:" ) );
    m_gridComponents->SetCellValue( row, COL_FRONT_SIDE, formatCount( totalFront ) );
    m_gridComponents->SetCellValue( row, COL_BOTTOM_SIDE, formatCount( totalBack ) );
    m_gridComponents->SetCellValue( row, COL_TOTAL, formatCount( totalFront + totalBack ) );

    if( m_hasOutline )
    {
        m_gridBoard->SetCellValue( ROW_BOARD_DIMS, COL_AMOUNT,
                                   wxString::Format( wxT( "%s x %s" ),
                                                     m_frame->MessageTextFromValue( m_boardWidth, false ),
                                                     m_frame->MessageTextFromValue( m_boardHeight, true ) ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT,
                                   m_frame->MessageTextFromValue( m_boardArea, true, EDA_DATA_TYPE::AREA ) );
    }
    else
    {
        m_gridBoard->SetCellValue( ROW_BOARD_DIMS, COL_AMOUNT, _( "unknown" ) );
        m_gridBoard->SetCellValue( ROW_BOARD_AREA, COL_AMOUNT, _( "unknown" ) );
    }

    m_gridBoard->SetCellValue( ROW_FRONT_COPPER_AREA, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_frontCopperArea, true, EDA_DATA_TYPE::AREA ) );
    m_gridBoard->SetCellValue( ROW_BACK_COPPER_AREA, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_backCopperArea, true, EDA_DATA_TYPE::AREA ) );

    m_gridBoard->SetCellValue( ROW_MIN_CLEARANCE, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_minClearanceTrackToTrack, true, EDA_DATA_TYPE::DISTANCE ) );

    m_gridBoard->SetCellValue( ROW_MIN_TRACK_WIDTH, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_minTrackWidth, true, EDA_DATA_TYPE::DISTANCE ) );

    m_gridBoard->SetCellValue( ROW_BOARD_THICKNESS, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_boardThickness, true, EDA_DATA_TYPE::DISTANCE ) );

    updateDrillGrid();

    m_gridBoard->SetCellValue( ROW_MIN_DRILL_DIAMETER, COL_AMOUNT,
                               m_frame->MessageTextFromValue( m_minDrillSize, true, EDA_DATA_TYPE::DISTANCE ) );

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

    for( const DRILL_LINE_ITEM& line : m_drillTypes )
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

        if( line.shape == PAD_DRILL_SHAPE::CIRCLE )
            m_minDrillSize = std::min( m_minDrillSize, line.xSize );

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
            widths[col] = (int) aGrid->GetColLabelValue( col ).length();
    }

    for( int row = 0; row < aGrid->GetNumberRows(); row++ )
    {
        rowLabelsWidth = std::max( rowLabelsWidth, (int) aGrid->GetRowLabelValue( row ).length() );

        for( int col = 0; col < aGrid->GetNumberCols(); col++ )
            widths[col] = std::max( widths[col], (int) aGrid->GetCellValue( row, col ).length() );
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

    msg << _( "PCB statistics report\n=====================" ) << wxT( "\n" );
    msg << wxS( "- " ) << _( "Date" ) << wxS( ": " ) << wxDateTime::Now().Format() << wxT( "\n" );
    msg << wxS( "- " ) << _( "Project" ) << wxS( ": " )<< Prj().GetProjectName() << wxT( "\n" );
    msg << wxS( "- " ) << _( "Board name" ) << wxS( ": " )<< boardName << wxT( "\n" );

    msg << wxT( "\n" );
    msg << _( "Board" ) << wxT( "\n-----\n" );

    if( m_hasOutline )
    {
        msg << wxS( "- " ) << _( "Width" ) << wxS( ": " )
                << m_frame->MessageTextFromValue( m_boardWidth ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Height" ) << wxS( ": " )
                << m_frame->MessageTextFromValue( m_boardHeight ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Area" ) + wxS( ": " )
                << m_frame->MessageTextFromValue( m_boardArea, true, EDA_DATA_TYPE::AREA ) << wxT( "\n" );
    }
    else
    {
        msg << wxS( "- " ) << _( "Dimensions" ) << wxS( ": " ) << _( "unknown" ) << wxT( "\n" );
        msg << wxS( "- " ) << _( "Area" ) << wxS( ": " ) << _( "unknown" ) << wxT( "\n" );
    }

    msg << wxS( "- " ) << _( "Front copper area" ) + wxS( ": " )
            << m_frame->MessageTextFromValue( m_frontCopperArea, true, EDA_DATA_TYPE::AREA ) << wxT( "\n" );
    msg << wxS( "- " ) << _( "Back copper area" ) + wxS( ": " )
            << m_frame->MessageTextFromValue( m_backCopperArea, true, EDA_DATA_TYPE::AREA ) << wxT( "\n" );

    msg << wxS( "- " ) << _( "Min track clearance" ) + wxS( ": " )
        << m_frame->MessageTextFromValue( m_minClearanceTrackToTrack, true, EDA_DATA_TYPE::DISTANCE ) << wxT( "\n" );

    msg << wxS( "- " ) << _( "Min track width" ) + wxS( ": " )
        << m_frame->MessageTextFromValue( m_minTrackWidth, true, EDA_DATA_TYPE::DISTANCE ) << wxT( "\n" );

    msg << wxS( "- " ) << _( "Min drill diameter" ) + wxS( ": " )
        << m_frame->MessageTextFromValue( m_minDrillSize, true, EDA_DATA_TYPE::DISTANCE ) << wxT( "\n" );

    msg << wxS( "- " ) << _( "Board stackup thickness" ) + wxS( ": " )
        << m_frame->MessageTextFromValue( m_boardThickness, true, EDA_DATA_TYPE::DISTANCE ) << wxT( "\n" );

    msg << wxT( "\n" );
    msg << _( "Pads" ) << wxT( "\n----\n" );

    for( auto& type : m_padTypes )
        msg << wxT( "- " ) << type.m_Title << wxS( " " ) << type.m_Qty << wxT( "\n" );

    msg << wxT( "\n" );
    msg << _( "Vias" ) << wxT( "\n----\n" );

    for( auto& type : m_viaTypes )
        msg << wxT( "- " ) << type.m_Title << wxS( " " ) << type.m_Qty << wxT( "\n" );

    // We will save data about components in the table.
    // We have to calculate column widths
    std::vector<int>      widths;
    std::vector<wxString> labels{ wxT( "" ), _( "Front Side" ), _( "Back Side" ), _( "Total" ) };
    wxString tmp;

    widths.reserve( labels.size() );

    for( const wxString& label : labels )
        widths.push_back( (int) label.size() );

    int frontTotal = 0;
    int backTotal = 0;

    for( const FP_LINE_ITEM& line : m_fpTypes )
    {
        // Get maximum width for left label column
        widths[0] = std::max( (int) line.m_Title.size(), widths[0] );
        frontTotal += line.m_FrontSideQty;
        backTotal += line.m_BackSideQty;
    }

    // Get maximum width for other columns
    tmp.Printf( wxT( "%i" ), frontTotal );
    widths[1] = std::max( (int) tmp.size(), widths[1] );
    tmp.Printf( wxT( "%i" ), backTotal );
    widths[2] = std::max( (int) tmp.size(), widths[2] );
    tmp.Printf( wxT( "%i" ), frontTotal + backTotal );
    widths[3] = std::max( (int) tmp.size(), widths[3] );

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
