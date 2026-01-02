/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "board_statistics_report.h"
#include "build_version.h"

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_segment.h>
#include <board_stackup_manager/board_stackup.h>
#include <i18n_utility.h>
#include <nlohmann/json.hpp>
#include <string_utils.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <wx/datetime.h>

BOARD_STATISTICS_DATA::BOARD_STATISTICS_DATA() :
        hasOutline( false ),
        boardWidth( 0 ),
        boardHeight( 0 ),
        boardArea( 0.0 ),
        frontCopperArea( 0.0 ),
        backCopperArea( 0.0 ),
        frontFootprintCourtyardArea( 0.0 ),
        backFootprintCourtyardArea( 0.0 ),
        frontFootprintDensity( 0.0 ),
        backFootprintDensity( 0.0 ),
        minClearanceTrackToTrack( std::numeric_limits<int>::max() ),
        minTrackWidth( std::numeric_limits<int>::max() ),
        minDrillSize( std::numeric_limits<int>::max() ),
        boardThickness( 0 ),
        footprintEntries(),
        padEntries(),
        padPropertyEntries(),
        viaEntries(),
        drillEntries()
{
}


void BOARD_STATISTICS_DATA::ResetCounts()
{
    hasOutline = false;
    boardWidth = 0;
    boardHeight = 0;
    boardArea = 0.0;
    frontCopperArea = 0.0;
    backCopperArea = 0.0;
    frontFootprintCourtyardArea = 0.0;
    backFootprintCourtyardArea = 0.0;
    minClearanceTrackToTrack = std::numeric_limits<int>::max();
    minTrackWidth = std::numeric_limits<int>::max();
    minDrillSize = std::numeric_limits<int>::max();
    boardThickness = 0;

    for( BOARD_STATISTICS_FP_ENTRY& fp : footprintEntries )
    {
        fp.frontCount = 0;
        fp.backCount = 0;
    }

    for( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>& pad : padEntries )
        pad.quantity = 0;

    for( BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>& prop : padPropertyEntries )
        prop.quantity = 0;

    for( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>& via : viaEntries )
        via.quantity = 0;

    drillEntries.clear();
}


void InitializeBoardStatisticsData( BOARD_STATISTICS_DATA& aData )
{
    aData.footprintEntries.clear();
    aData.padEntries.clear();
    aData.padPropertyEntries.clear();
    aData.viaEntries.clear();
    aData.drillEntries.clear();

    aData.footprintEntries.push_back( BOARD_STATISTICS_FP_ENTRY( FP_THROUGH_HOLE, FP_THROUGH_HOLE, _( "THT:" ) ) );
    aData.footprintEntries.push_back( BOARD_STATISTICS_FP_ENTRY( FP_SMD, FP_SMD, _( "SMD:" ) ) );
    aData.footprintEntries.push_back( BOARD_STATISTICS_FP_ENTRY( FP_THROUGH_HOLE | FP_SMD, 0, _( "Unspecified:" ) ) );

    aData.padEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>( PAD_ATTRIB::PTH, _( "Through hole:" ) ) );
    aData.padEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>( PAD_ATTRIB::SMD, _( "SMD:" ) ) );
    aData.padEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>( PAD_ATTRIB::CONN, _( "Connector:" ) ) );
    aData.padEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>( PAD_ATTRIB::NPTH, _( "NPTH:" ) ) );

    aData.padPropertyEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>( PAD_PROP::CASTELLATED,
                                                                               _( "Castellated:" ) ) );
    aData.padPropertyEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>( PAD_PROP::PRESSFIT,
                                                                               _( "Press-fit:" ) ) );

    aData.viaEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>( VIATYPE::THROUGH, _( "Through vias:" ) ) );
    aData.viaEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>( VIATYPE::BLIND, _( "Blind vias:" ) ) );
    aData.viaEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>( VIATYPE::BURIED, _( "Buried vias:" ) ) );
    aData.viaEntries.push_back( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>( VIATYPE::MICROVIA, _( "Micro vias:" ) ) );

    aData.ResetCounts();
}


static void updatePadCounts( FOOTPRINT* aFootprint, BOARD_STATISTICS_DATA& aData )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        for( BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>& padEntry : aData.padEntries )
        {
            if( pad->GetAttribute() == padEntry.attribute )
            {
                padEntry.quantity++;
                break;
            }
        }

        for( BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>& propEntry : aData.padPropertyEntries )
        {
            if( pad->GetProperty() == propEntry.attribute )
            {
                propEntry.quantity++;
                break;
            }
        }
    }
}


void ComputeBoardStatistics( BOARD* aBoard, const BOARD_STATISTICS_OPTIONS& aOptions, BOARD_STATISTICS_DATA& aData )
{
    aData.ResetCounts();

    if( !aBoard )
        return;

    static const std::vector<KICAD_T> trackTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T };

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        if( aOptions.excludeFootprintsWithoutPads && footprint->Pads().empty() )
            continue;

        for( BOARD_STATISTICS_FP_ENTRY& entry : aData.footprintEntries )
        {
            if( ( footprint->GetAttributes() & entry.attributeMask ) == entry.attributeValue )
            {
                switch( footprint->GetSide() )
                {
                case F_Cu: entry.frontCount++; break;
                case B_Cu: entry.backCount++;  break;
                default:                       break;
                }

                break;
            }
        }

        updatePadCounts( footprint, aData );
    }

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
            aData.minTrackWidth = std::min( aData.minTrackWidth, track->GetWidth() );

        if( !track->IsType( trackTypes ) )
            continue;

        PCB_LAYER_ID layer = track->GetLayer();
        auto         trackShapeA = track->GetEffectiveShape( layer );

        for( PCB_TRACK* otherTrack : aBoard->Tracks() )
        {
            if( layer != otherTrack->GetLayer() )
                continue;

            if( track->GetNetCode() == otherTrack->GetNetCode() )
                continue;

            if( !otherTrack->IsType( trackTypes ) )
                continue;

            int  actual = 0;
            auto trackShapeB = otherTrack->GetEffectiveShape( layer );
            bool collide = trackShapeA->Collide( trackShapeB.get(), aData.minClearanceTrackToTrack, &actual );

            if( collide )
                aData.minClearanceTrackToTrack = std::min( aData.minClearanceTrackToTrack, actual );
        }

        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            for( BOARD_STATISTICS_INFO_ENTRY<VIATYPE>& entry : aData.viaEntries )
            {
                if( via->GetViaType() == entry.attribute )
                {
                    entry.quantity++;
                    break;
                }
            }
        }
    }

    {
        std::vector<DRILL_LINE_ITEM> drills;
        CollectDrillLineItems( aBoard, drills );

        aData.drillEntries = std::move( drills );
    }

    std::sort( aData.drillEntries.begin(), aData.drillEntries.end(),
               DRILL_LINE_ITEM::COMPARE( DRILL_LINE_ITEM::COL_COUNT, false ) );

    aData.minDrillSize = std::numeric_limits<int>::max();

    for( const DRILL_LINE_ITEM& drill : aData.drillEntries )
    {
        if( drill.shape == PAD_DRILL_SHAPE::CIRCLE )
            aData.minDrillSize = std::min( aData.minDrillSize, drill.xSize );
    }

    SHAPE_POLY_SET polySet;
    aData.hasOutline = aBoard->GetBoardPolygonOutlines( polySet, false );

    if( aData.hasOutline )
    {
        aData.boardArea = 0.0;

        for( int i = 0; i < polySet.OutlineCount(); ++i )
        {
            SHAPE_LINE_CHAIN& outline = polySet.Outline( i );
            aData.boardArea += outline.Area();

            if( aOptions.subtractHolesFromBoardArea )
            {
                for( int j = 0; j < polySet.HoleCount( i ); ++j )
                    aData.boardArea -= polySet.Hole( i, j ).Area();

                for( FOOTPRINT* footprint : aBoard->Footprints() )
                {
                    for( PAD* pad : footprint->Pads() )
                    {
                        if( !pad->HasHole() )
                            continue;

                        std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();

                        if( !hole )
                            continue;

                        const SEG& seg = hole->GetSeg();
                        double     width = hole->GetWidth();
                        double     area = seg.Length() * width;

                        area += M_PI * 0.25 * width * width;
                        aData.boardArea -= area;
                    }
                }

                for( PCB_TRACK* track : aBoard->Tracks() )
                {
                    if( track->Type() == PCB_VIA_T )
                    {
                        PCB_VIA* via = static_cast<PCB_VIA*>( track );
                        double   drill = via->GetDrillValue();
                        aData.boardArea -= M_PI * 0.25 * drill * drill;
                    }
                }
            }
        }

        BOX2I bbox = polySet.BBox();

        aData.boardWidth = static_cast<int>( bbox.GetWidth() );
        aData.boardHeight = static_cast<int>( bbox.GetHeight() );

    }

    // Now determine the courtyard areas which reflects how much space is occupied by components
    // This will always assume all components are populated as its intended for
    // layout purposes and arguing with people saying theres not enough space.
    SHAPE_POLY_SET frontShapesForArea;
    SHAPE_POLY_SET backShapesForArea;

    std::shared_ptr<NET_SETTINGS>& netSettings = aBoard->GetDesignSettings().m_NetSettings;
    int                            minPadClearanceOuter = netSettings->GetDefaultNetclass()->GetClearance();

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        const SHAPE_POLY_SET& frontA = fp->GetCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backA = fp->GetCourtyard( B_CrtYd );

        if( frontA.OutlineCount() != 0 )
            frontShapesForArea.Append( frontA );

        if( backA.OutlineCount() != 0 )
            backShapesForArea.Append( backA );

        // PTH/NPTH holes in footprints can be outside the main courtyard and also
        // consume space on the other side of the board but without a courtyard
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            if( pad->GetAttribute() != PAD_ATTRIB::NPTH )
            {
                pad->TransformShapeToPolygon( frontShapesForArea, F_Cu,
                                              std::min( minPadClearanceOuter, pad->GetOwnClearance( F_Cu ) ),
                                              ARC_LOW_DEF, ERROR_INSIDE );
                pad->TransformShapeToPolygon( backShapesForArea, B_Cu,
                                              std::min( minPadClearanceOuter, pad->GetOwnClearance( B_Cu ) ),
                                              ARC_LOW_DEF, ERROR_INSIDE );
            }
            else
            {
                pad->TransformHoleToPolygon( frontShapesForArea, 0, ARC_LOW_DEF, ERROR_INSIDE );
                pad->TransformHoleToPolygon( backShapesForArea, 0, ARC_LOW_DEF, ERROR_INSIDE );
            }
        }
    }

    // deal with overlapping courtyards (if people are ignoring DRC or something)
    // and such through simplify
    frontShapesForArea.Simplify();
    backShapesForArea.Simplify();

    aData.frontFootprintCourtyardArea = frontShapesForArea.Area();
    aData.backFootprintCourtyardArea = backShapesForArea.Area();

    if( aData.hasOutline )
    {
        aData.frontFootprintDensity = aData.frontFootprintCourtyardArea * 100 / aData.boardArea;
        aData.backFootprintDensity = aData.backFootprintCourtyardArea * 100 / aData.boardArea;
    }

    SHAPE_POLY_SET frontCopper;
    SHAPE_POLY_SET backCopper;
    SHAPE_POLY_SET frontHoles;
    SHAPE_POLY_SET backHoles;

    aBoard->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                if( child->Type() == PCB_FOOTPRINT_T
                    || child->Type() == PCB_GROUP_T
                    || child->Type() == PCB_GENERATOR_T )
                {
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
                    int      radius = via->GetDrillValue() / 2;

                    if( via->IsOnLayer( F_Cu ) )
                        TransformCircleToPolygon( frontHoles, center, radius, ARC_LOW_DEF, ERROR_OUTSIDE );

                    if( via->IsOnLayer( B_Cu ) )
                        TransformCircleToPolygon( backHoles, center, radius, ARC_LOW_DEF, ERROR_OUTSIDE );
                }
            },
            RECURSE_MODE::RECURSE );

    if( aOptions.subtractHolesFromCopperAreas )
    {
        frontCopper.BooleanSubtract( frontHoles );
        backCopper.BooleanSubtract( backHoles );
    }

    aData.frontCopperArea = frontCopper.Area();
    aData.backCopperArea = backCopper.Area();

    aData.boardThickness = aBoard->GetStackupOrDefault().BuildBoardThicknessFromStackup();
}


static wxString formatCount( int aCount )
{
    return wxString::Format( wxT( "%i" ), aCount );
}


static void appendTable( const std::vector<std::vector<wxString>>& aRows, bool aUseFirstColAsLabel, wxString& aOut )
{
    if( aRows.empty() )
        return;

    size_t columnCount = 0;

    for( const std::vector<wxString>& row : aRows )
    {
        if( row.size() > columnCount )
            columnCount = row.size();
    }

    if( columnCount == 0 )
        return;

    std::vector<int> widths( columnCount, 0 );

    for( const std::vector<wxString>& row : aRows )
    {
        for( size_t col = 0; col < columnCount; ++col )
        {
            if( col >= row.size() )
                continue;

            int cellWidth = static_cast<int>( row[col].length() );

            if( cellWidth > widths[col] )
                widths[col] = cellWidth;
        }
    }

    auto appendDataRow =
            [&]( const std::vector<wxString>& row, bool treatFirstAsLabel )
            {
                if( treatFirstAsLabel && aUseFirstColAsLabel )
                {
                    wxString formatted;
                    wxString firstColumn;

                    if( !row.empty() )
                        firstColumn = row[0];

                    formatted.Printf( wxS( "|%-*s  |" ), widths[0], firstColumn );
                    aOut << formatted;

                    for( size_t col = 1; col < columnCount; ++col )
                    {
                        wxString value;
                        if( col < row.size() )
                            value = row[col];

                        formatted.Printf( wxS( " %*s |" ), widths[col], value );
                        aOut << formatted;
                    }
                }
                else
                {
                    aOut << wxS( "|" );

                    for( size_t col = 0; col < columnCount; ++col )
                    {
                        wxString value;
                        if( col < row.size() )
                            value = row[col];

                        wxString formatted;
                        formatted.Printf( wxS( " %*s |" ), widths[col], value );
                        aOut << formatted;
                    }
                }

                aOut << wxS( "\n" );
            };

    appendDataRow( aRows.front(), false );

    aOut << wxS( "|" );

    for( size_t col = 0; col < columnCount; ++col )
    {
        int dashCount = widths[col] + 2;

        if( dashCount < 3 )
            dashCount = 3;

        wxString dashes;

        for( int i = 0; i < dashCount; ++i )
            dashes << wxS( "-" );

        aOut << dashes << wxS( "|" );
    }

    aOut << wxS( "\n" );

    for( size_t rowIdx = 1; rowIdx < aRows.size(); ++rowIdx )
        appendDataRow( aRows[rowIdx], true );
}


wxString FormatBoardStatisticsReport( const BOARD_STATISTICS_DATA& aData, BOARD* aBoard,
                                      const UNITS_PROVIDER& aUnitsProvider, const wxString& aProjectName,
                                      const wxString& aBoardName )
{
    wxString report;

    report << _( "PCB statistics report\n=====================" ) << wxS( "\n" );
    report << wxS( "- " ) << _( "Date" ) << wxS( ": " ) << wxDateTime::Now().Format() << wxS( "\n" );
    report << wxS( "- " ) << _( "Project" ) << wxS( ": " ) << aProjectName << wxS( "\n" );
    report << wxS( "- " ) << _( "Board name" ) << wxS( ": " ) << aBoardName << wxS( "\n\n" );

    report << _( "Board" ) << wxS( "\n-----\n" );

    if( aData.hasOutline )
    {
        report << wxS( "- " ) << _( "Width" ) << wxS( ": " ) << aUnitsProvider.MessageTextFromValue( aData.boardWidth )
               << wxS( "\n" );
        report << wxS( "- " ) << _( "Height" ) << wxS( ": " )
               << aUnitsProvider.MessageTextFromValue( aData.boardHeight ) << wxS( "\n" );
        report << wxS( "- " ) << _( "Area" ) << wxS( ": " )
               << aUnitsProvider.MessageTextFromValue( aData.boardArea, true, EDA_DATA_TYPE::AREA ) << wxS( "\n" );
    }
    else
    {
        report << wxS( "- " ) << _( "Dimensions" ) << wxS( ": " ) << _( "unknown" ) << wxS( "\n" );
        report << wxS( "- " ) << _( "Area" ) << wxS( ": " ) << _( "unknown" ) << wxS( "\n" );
    }

    report << wxS( "- " ) << _( "Front copper area" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.frontCopperArea, true, EDA_DATA_TYPE::AREA ) << wxS( "\n" );
    report << wxS( "- " ) << _( "Back copper area" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.backCopperArea, true, EDA_DATA_TYPE::AREA ) << wxS( "\n" );

    report << wxS( "- " ) << _( "Min track clearance" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.minClearanceTrackToTrack, true, EDA_DATA_TYPE::DISTANCE )
           << wxS( "\n" );

    report << wxS( "- " ) << _( "Min track width" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.minTrackWidth, true, EDA_DATA_TYPE::DISTANCE ) << wxS( "\n" );

    int minDrill = aData.minDrillSize;

    report << wxS( "- " ) << _( "Min drill diameter" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( minDrill, true, EDA_DATA_TYPE::DISTANCE ) << wxS( "\n" );

    report << wxS( "- " ) << _( "Board stackup thickness" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.boardThickness, true, EDA_DATA_TYPE::DISTANCE )
           << wxS( "\n\n" );

    report << wxS( "- " ) << _( "Front footprint area" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.frontFootprintCourtyardArea, true, EDA_DATA_TYPE::AREA ) << wxS( "\n" );
    report << wxS( "- " ) << _( "Back footprint area" ) << wxS( ": " )
           << aUnitsProvider.MessageTextFromValue( aData.backFootprintCourtyardArea, true, EDA_DATA_TYPE::AREA ) << wxS( "\n" );

    report << wxS( "- " ) << _( "Front component density" ) << wxS( ": " );

    if( aData.hasOutline )
        report << wxString::Format( "%.2f %", aData.frontFootprintDensity );
    else
        report << _( "unknown" );

    report << wxS( "\n" );

    report << wxS( "- " ) << _( "Back component density" ) << wxS( ": " );

    if( aData.hasOutline )
        report << wxString::Format( "%.2f %", aData.backFootprintDensity );
    else
        report << _( "unknown" );

    report << wxS( "\n" );

    report << _( "Pads" ) << wxS( "\n----\n" );

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>& padEntry : aData.padEntries )
        report << wxS( "- " ) << padEntry.title << wxS( " " ) << padEntry.quantity << wxS( "\n" );

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>& propEntry : aData.padPropertyEntries )
        report << wxS( "- " ) << propEntry.title << wxS( " " ) << propEntry.quantity << wxS( "\n" );

    report << wxS( "\n" );
    report << _( "Vias" ) << wxS( "\n----\n" );

    for( const BOARD_STATISTICS_INFO_ENTRY<VIATYPE>& viaEntry : aData.viaEntries )
        report << wxS( "- " ) << viaEntry.title << wxS( " " ) << viaEntry.quantity << wxS( "\n" );

    report << wxS( "\n" );
    report << _( "Components" ) << wxS( "\n----------\n\n" );

    std::vector<std::vector<wxString>> componentRows;
    std::vector<wxString>              header;
    header.push_back( wxString() );
    header.push_back( _( "Front Side" ) );
    header.push_back( _( "Back Side" ) );
    header.push_back( _( "Total" ) );
    componentRows.push_back( std::move( header ) );

    int frontTotal = 0;
    int backTotal = 0;

    for( const BOARD_STATISTICS_FP_ENTRY& fpEntry : aData.footprintEntries )
    {
        std::vector<wxString> row;
        row.push_back( fpEntry.title );
        row.push_back( formatCount( fpEntry.frontCount ) );
        row.push_back( formatCount( fpEntry.backCount ) );
        row.push_back( formatCount( fpEntry.frontCount + fpEntry.backCount ) );
        componentRows.push_back( std::move( row ) );

        frontTotal += fpEntry.frontCount;
        backTotal += fpEntry.backCount;
    }

    std::vector<wxString> totalRow;
    totalRow.push_back( _( "Total:" ) );
    totalRow.push_back( formatCount( frontTotal ) );
    totalRow.push_back( formatCount( backTotal ) );
    totalRow.push_back( formatCount( frontTotal + backTotal ) );
    componentRows.push_back( std::move( totalRow ) );

    appendTable( componentRows, true, report );

    report << wxS( "\n" );
    report << _( "Drill holes" ) << wxS( "\n-----------\n\n" );

    std::vector<std::vector<wxString>> drillRows;
    std::vector<wxString>              drillHeader;
    drillHeader.push_back( _( "Count" ) );
    drillHeader.push_back( _( "Shape" ) );
    drillHeader.push_back( _( "X Size" ) );
    drillHeader.push_back( _( "Y Size" ) );
    drillHeader.push_back( _( "Plated" ) );
    drillHeader.push_back( _( "Via/Pad" ) );
    drillHeader.push_back( _( "Start Layer" ) );
    drillHeader.push_back( _( "Stop Layer" ) );
    drillRows.push_back( std::move( drillHeader ) );

    for( const DRILL_LINE_ITEM& drill : aData.drillEntries )
    {
        wxString shapeStr;

        switch( drill.shape )
        {
        case PAD_DRILL_SHAPE::CIRCLE: shapeStr = _( "Round" ); break;
        case PAD_DRILL_SHAPE::OBLONG: shapeStr = _( "Slot" );  break;
        default:                      shapeStr = _( "???" );   break;
        }

        wxString platedStr = drill.isPlated ? _( "PTH" ) : _( "NPTH" );
        wxString itemStr = drill.isPad ? _( "Pad" ) : _( "Via" );

        wxString startLayerStr;
        wxString stopLayerStr;

        if( drill.startLayer == UNDEFINED_LAYER )
            startLayerStr = _( "N/A" );
        else if( aBoard )
            startLayerStr = aBoard->GetLayerName( drill.startLayer );
        else
            startLayerStr = _( "N/A" );

        if( drill.stopLayer == UNDEFINED_LAYER )
            stopLayerStr = _( "N/A" );
        else if( aBoard )
            stopLayerStr = aBoard->GetLayerName( drill.stopLayer );
        else
            stopLayerStr = _( "N/A" );

        std::vector<wxString> row;
        row.push_back( formatCount( drill.m_Qty ) );
        row.push_back( shapeStr );
        row.push_back( aUnitsProvider.MessageTextFromValue( drill.xSize ) );
        row.push_back( aUnitsProvider.MessageTextFromValue( drill.ySize ) );
        row.push_back( platedStr );
        row.push_back( itemStr );
        row.push_back( startLayerStr );
        row.push_back( stopLayerStr );
        drillRows.push_back( std::move( row ) );
    }

    appendTable( drillRows, false, report );

    return report;
}


wxString FormatBoardStatisticsJson( const BOARD_STATISTICS_DATA& aData, BOARD* aBoard,
                                    const UNITS_PROVIDER& aUnitsProvider, const wxString& aProjectName,
                                    const wxString& aBoardName )
{
    nlohmann::ordered_json root;

    nlohmann::ordered_json metadata;
    metadata["date"] = GetISO8601CurrentDateTime();
    metadata["generator"] = "KiCad " + GetBuildVersion();
    metadata["project"] = aProjectName;
    metadata["board_name"] = aBoardName;
    root["metadata"] = metadata;

    nlohmann::ordered_json board;
    board["has_outline"] = aData.hasOutline;

    if( aData.hasOutline )
    {
        board["width"] = aUnitsProvider.MessageTextFromValue( aData.boardWidth );
        board["height"] = aUnitsProvider.MessageTextFromValue( aData.boardHeight );
        board["area"] = aUnitsProvider.MessageTextFromValue( aData.boardArea, true, EDA_DATA_TYPE::AREA );
        board["front_component_density"] = wxString::Format( "%.2f", aData.frontFootprintDensity );
        board["back_component_density"] = wxString::Format( "%.2f", aData.backFootprintDensity );
    }
    else
    {
        board["width"] = nlohmann::json();
        board["height"] = nlohmann::json();
        board["area"] = nlohmann::json();
        board["front_component_density"] = nlohmann::json();
        board["back_component_density"] = nlohmann::json();
    }

    board["front_copper_area"] = aUnitsProvider.MessageTextFromValue( aData.frontCopperArea, true,
                                                                      EDA_DATA_TYPE::AREA );
    board["back_copper_area"] = aUnitsProvider.MessageTextFromValue( aData.backCopperArea, true,
                                                                     EDA_DATA_TYPE::AREA );
    board["min_track_clearance"] = aUnitsProvider.MessageTextFromValue( aData.minClearanceTrackToTrack );
    board["min_track_width"] = aUnitsProvider.MessageTextFromValue( aData.minTrackWidth );
    board["min_drill_diameter"] = aUnitsProvider.MessageTextFromValue( aData.minDrillSize );
    board["board_thickness"] = aUnitsProvider.MessageTextFromValue( aData.boardThickness );
    board["front_footprint_area"] = aUnitsProvider.MessageTextFromValue( aData.frontFootprintCourtyardArea, true,
                                                                         EDA_DATA_TYPE::AREA );
    board["back_footprint_area"] = aUnitsProvider.MessageTextFromValue( aData.backFootprintCourtyardArea, true,
                                                                        EDA_DATA_TYPE::AREA );

    if( aData.hasOutline )
    {
        board["front_footprint_density"] = wxString::Format( "%.2f", aData.frontFootprintDensity );
        board["back_footprint_density"] = wxString::Format( "%.2f", aData.backFootprintDensity );
    }
    else
    {
        board["front_footprint_density"] = nlohmann::json();
        board["back_footprint_density"] = nlohmann::json();
    }

    root["board"] = board;

    // The UI strings end in colons, often have a suffix like "via", and need
    // to be snake_cased
    auto jsonize =
            []( const wxString& title, bool removeSuffix ) -> wxString
            {
                wxString json = title;

                if( removeSuffix )
                    json = json.BeforeLast( ' ' );

                if( json.EndsWith( wxS( ":" ) ) )
                    json.RemoveLast();

                json.Replace( wxS( " " ), wxS( "_" ) );
                json.Replace( wxS( "-" ), wxS( "_" ) );

                return json.MakeLower();
            };

    nlohmann::ordered_json pads = nlohmann::ordered_json::object();

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_ATTRIB>& padEntry : aData.padEntries )
        pads[jsonize( padEntry.title, false )] = padEntry.quantity;

    for( const BOARD_STATISTICS_INFO_ENTRY<PAD_PROP>& propEntry : aData.padPropertyEntries )
        pads[jsonize( propEntry.title, false )] = propEntry.quantity;

    root["pads"] = pads;

    nlohmann::ordered_json vias = nlohmann::ordered_json::object();

    for( const BOARD_STATISTICS_INFO_ENTRY<VIATYPE>& viaEntry : aData.viaEntries )
        vias[jsonize( viaEntry.title, true )] = viaEntry.quantity;

    root["vias"] = vias;

    int                    frontTotal = 0;
    int                    backTotal = 0;
    nlohmann::ordered_json components = nlohmann::ordered_json::object();

    for( const BOARD_STATISTICS_FP_ENTRY& fpEntry : aData.footprintEntries )
    {
        nlohmann::ordered_json component;
        component["front"] = fpEntry.frontCount;
        component["back"] = fpEntry.backCount;
        component["total"] = fpEntry.frontCount + fpEntry.backCount;
        components[jsonize( fpEntry.title, false )] = component;

        frontTotal += fpEntry.frontCount;
        backTotal += fpEntry.backCount;
    }

    nlohmann::ordered_json totals;
    totals["front"] = frontTotal;
    totals["back"] = backTotal;
    totals["total"] = frontTotal + backTotal;
    components["total"] = totals;

    root["components"] = components;

    nlohmann::ordered_json drillHoles = nlohmann::ordered_json::array();

    for( const DRILL_LINE_ITEM& drill : aData.drillEntries )
    {
        nlohmann::ordered_json drillJson;

        wxString shapeStr;

        switch( drill.shape )
        {
        case PAD_DRILL_SHAPE::CIRCLE: shapeStr = _( "Round" ); break;
        case PAD_DRILL_SHAPE::OBLONG: shapeStr = _( "Slot" ); break;
        default: shapeStr = _( "???" ); break;
        }

        wxString sourceStr = drill.isPad ? _( "Pad" ) : _( "Via" );
        wxString startLayerStr = _( "N/A" );
        wxString stopLayerStr = _( "N/A" );

        if( aBoard && drill.startLayer != UNDEFINED_LAYER )
            startLayerStr = aBoard->GetLayerName( drill.startLayer );

        if( aBoard && drill.stopLayer != UNDEFINED_LAYER )
            stopLayerStr = aBoard->GetLayerName( drill.stopLayer );

        drillJson["count"] = drill.m_Qty;
        drillJson["shape"] = shapeStr;
        drillJson["x_size"] = aUnitsProvider.MessageTextFromValue( drill.xSize );
        drillJson["y_size"] = aUnitsProvider.MessageTextFromValue( drill.ySize );
        drillJson["plated"] = drill.isPlated;
        drillJson["source"] = sourceStr;
        drillJson["start_layer"] = startLayerStr;
        drillJson["stop_layer"] = stopLayerStr;
        drillHoles.push_back( std::move( drillJson ) );
    }

    root["drill_holes"] = drillHoles;

    std::string jsonText = root.dump( 2 );
    return wxString::FromUTF8( jsonText.c_str() );
}
