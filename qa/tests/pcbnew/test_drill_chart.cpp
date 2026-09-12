/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <filesystem>
#include <set>

#include <board.h>
#include <eda_text.h>
#include <i18n_utility.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_drill_chart.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_drill_map.h>
#include <pcb_shape.h>
#include <drill/drill_enumerator.h>
#include <drill/drill_symbol_assigner.h>
#include <drill/drill_symbol_profile.h>
#include <pcb_track.h>
#include <base_units.h>
#include <core/kicad_algo.h>
#include <properties/property_mgr.h>
#include <view/view.h>
#include <pcbnew_utils/board_file_utils.h>

#include <google/protobuf/any.pb.h>
#include <wx/filename.h>


namespace
{

/// Two through vias of one size, one of another, plus an NPTH mounting hole
void buildHoles( BOARD& aBoard )
{
    aBoard.SetCopperLayerCount( 6 );

    for( int i = 0; i < 2; ++i )
    {
        PCB_VIA* via = new PCB_VIA( &aBoard );
        via->SetPadstackMode( PADSTACK::MODE::NORMAL );
        via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( i * 5 ), 0 ) );
        via->SetLayerPair( F_Cu, B_Cu );
        via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.60 ) );
        aBoard.Add( via );
    }

    PCB_VIA* big = new PCB_VIA( &aBoard );
    big->SetPadstackMode( PADSTACK::MODE::NORMAL );
    big->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );
    big->SetLayerPair( F_Cu, B_Cu );
    big->SetDrill( pcbIUScale.mmToIU( 0.60 ) );
    big->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 1.0 ) );
    aBoard.Add( big );

    FOOTPRINT* fp = new FOOTPRINT( &aBoard );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 30 ), 0 ) );

    PAD* pad = new PAD( fp );
    pad->SetAttribute( PAD_ATTRIB::NPTH );
    pad->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 30 ), 0 ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 3.2 ), pcbIUScale.mmToIU( 3.2 ) ) );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 3.2 ), pcbIUScale.mmToIU( 3.2 ) ) );
    fp->Add( pad );
    aBoard.Add( fp );
}


void buildOutline( BOARD& aBoard, const BOX2I& aBox )
{
    const VECTOR2I corners[] = {
        aBox.GetOrigin(),
        { aBox.GetRight(), aBox.GetTop() },
        aBox.GetEnd(),
        { aBox.GetLeft(), aBox.GetBottom() },
    };

    for( int ii = 0; ii < 4; ++ii )
    {
        PCB_SHAPE* edge = new PCB_SHAPE( &aBoard, SHAPE_T::SEGMENT );
        edge->SetLayer( Edge_Cuts );
        edge->SetStart( corners[ii] );
        edge->SetEnd( corners[( ii + 1 ) % 4] );
        aBoard.Add( edge );
    }
}


int cellCount( const PCB_DRILL_CHART& aChart, const wxString& aText )
{
    int count = 0;

    for( const PCB_TABLECELL* cell : aChart.GetCells() )
    {
        if( cell->GetText() == aText )
            count++;
    }

    return count;
}

} // namespace


BOOST_AUTO_TEST_SUITE( DrillChart )


BOOST_AUTO_TEST_CASE( GroupsHolesBySizeAndPlating )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    // Heading, three groups (0.3 PTH, 0.6 PTH, 3.2 NPTH) and totals. No caption row
    BOOST_CHECK_EQUAL( chart.GetRowCount(), 5 );

    // Two vias share one size, so they collapse into a single row reporting two operations
    BOOST_CHECK_EQUAL( cellCount( chart, wxT( "2" ) ), 1 );

    BOOST_CHECK_EQUAL( cellCount( chart, wxT( "Yes" ) ), 2 );
    BOOST_CHECK_EQUAL( cellCount( chart, wxT( "No" ) ), 1 );
}


BOOST_AUTO_TEST_CASE( RebuildKeepsCellIdentity )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    std::vector<KIID> before;

    for( const PCB_TABLECELL* cell : chart.GetCells() )
        before.push_back( cell->m_Uuid );

    chart.RebuildCells( board );

    std::vector<KIID> after;

    for( const PCB_TABLECELL* cell : chart.GetCells() )
        after.push_back( cell->m_Uuid );

    // Clearing and repopulating would mint new UUIDs every rebuild and break selection
    // restore and file diffs
    BOOST_CHECK( before == after );
}


BOOST_AUTO_TEST_CASE( AChartFollowsTheBoardWithoutBeingAsked )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &board );
    chart->SetLayer( User_1 );
    board.Add( chart );
    chart->RebuildCells( board );

    const int      rowsBefore = chart->GetRowCount();
    const uint64_t builtAt = chart->GetBuiltGeneration();

    // Nothing moved, so the refresh is one comparison and the cells stay as they are
    RefreshDrillCharts( board );

    BOOST_CHECK_EQUAL( chart->GetBuiltGeneration(), builtAt );
    BOOST_CHECK_EQUAL( chart->GetRowCount(), rowsBefore );

    PCB_VIA* via = new PCB_VIA( &board );
    via->SetPadstackMode( PADSTACK::MODE::NORMAL );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 40 ), 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.45 ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.80 ) );
    board.Add( via );

    RefreshDrillCharts( board );

    // A new hole size is a new row, and nobody had to ask for it
    BOOST_CHECK_EQUAL( chart->GetRowCount(), rowsBefore + 1 );
    BOOST_CHECK_EQUAL( chart->GetBuiltGeneration(), board.GetDrillModelGeneration() );
}


BOOST_AUTO_TEST_CASE( HolesReportTheDrillSymbolLayerOfAMap )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    board.Add( map );

    BOOST_REQUIRE( board.DrillSymbolLayers().Contains( User_1 ) );

    const int symbolLayer = DRILL_SYMBOL_LAYER_FOR( User_1 );
    int       reporting = 0;

    for( FOOTPRINT* fp : board.Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            const std::vector<int> layers = pad->ViewGetLayers();

            if( alg::contains( layers, symbolLayer ) )
                reporting++;
        }
    }

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const std::vector<int> layers = track->ViewGetLayers();

        if( alg::contains( layers, symbolLayer ) )
            reporting++;
    }

    // Without this the canvas has nothing to draw the marks on, even though the plotter
    // renders them perfectly well from the same data

    // note: for an ubscure reason, on MINGW, KIGFX::VIEW::VIEW_MAX_LAYERS need to be copied
    // to an intermediate variable to avoid link issues
    const int view_max_layers = KIGFX::VIEW::VIEW_MAX_LAYERS;

    BOOST_TEST_MESSAGE( "DRILL_SYMBOL_START=" << (int) LAYER_DRILL_SYMBOL_START
                        << " END=" << (int) LAYER_DRILL_SYMBOL_END
                        << " GAL_END=" << (int) GAL_LAYER_ID_END
                        << " VIEW_MAX=" << view_max_layers
                        << " symbolLayer=" << symbolLayer );

    // The view creates layers 0..VIEW_MAX_LAYERS-1. Anything above that is silently dropped
    // when an item is indexed, so it would never be drawn
    BOOST_CHECK_MESSAGE( symbolLayer < view_max_layers,
                         "layer " << symbolLayer << " exceeds VIEW_MAX_LAYERS "
                                  << view_max_layers );

    BOOST_CHECK_MESSAGE( reporting > 0, "no hole reports layer " << symbolLayer );

    map->SetLayer( Eco1_User );
    board.RefreshDrillSymbolLayers();

    BOOST_CHECK( !board.DrillSymbolLayers().Contains( User_1 ) );
    BOOST_REQUIRE( board.DrillSymbolLayers().Contains( Eco1_User ) );

    const int migratedSymbolLayer = DRILL_SYMBOL_LAYER_FOR( Eco1_User );

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const std::vector<int> layers = track->ViewGetLayers();
        BOOST_CHECK( !alg::contains( layers, symbolLayer ) );
        BOOST_CHECK( alg::contains( layers, migratedSymbolLayer ) );
    }
}


BOOST_AUTO_TEST_CASE( MapIsHitAtItsMarksAndNowhereElse )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    board.Add( map );

    const VECTOR2I hole( 0, 0 );
    const VECTOR2I empty( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) );

    BOOST_CHECK( map->HitTest( hole, 0 ) );
    BOOST_CHECK( !map->HitTest( empty, 0 ) );

    // The marks travel with the offset. The holes they mark stay where they are
    const VECTOR2I offset( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 40 ) );
    map->SetOffset( offset );

    BOOST_CHECK( !map->HitTest( hole, 0 ) );
    BOOST_CHECK( map->HitTest( hole + offset, 0 ) );
}


BOOST_AUTO_TEST_CASE( MapIgnoresRotateAndFlipsOnlyItsLayer )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP  map( &board );
    const VECTOR2I offset( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 40 ) );
    const VECTOR2I centre( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 20 ) );

    map.SetLayer( F_Fab );
    map.SetOffset( offset );
    map.Rotate( centre, EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK( map.GetOffset() == offset );
    BOOST_CHECK_EQUAL( map.GetLayer(), F_Fab );

    // The offset displaces a mark from its own hole, so there is nothing to mirror. The side
    // the map documents does move with the board
    map.SetOffset( offset );
    map.Flip( centre, FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK( map.GetOffset() == offset );
    BOOST_CHECK_EQUAL( map.GetLayer(), B_Fab );

    map.Flip( centre, FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK( map.GetOffset() == offset );
    BOOST_CHECK_EQUAL( map.GetLayer(), F_Fab );

    // A user layer has no other side, so it stays put
    map.SetLayer( User_1 );
    map.Flip( centre, FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( map.GetLayer(), User_1 );
}


BOOST_AUTO_TEST_CASE( MapWithoutABoardIsUsableAndFlippable )
{
    // Every current caller passes a board, but a 1 nm symbol would be invisible and
    // unhittable, and flipping a detached map must not dereference the board it has not got
    PCB_DRILL_MAP map( nullptr );

    BOOST_CHECK_GT( map.GetSymbolSize(), pcbIUScale.mmToIU( 0.1 ) );
    BOOST_CHECK_GT( map.GetSymbolExtent(), 0 );

    map.SetLayer( F_Fab );
    map.Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( map.GetLayer(), B_Fab );
}


BOOST_AUTO_TEST_CASE( MapIsHitOnItsOutlineWhenTheBoardHasNoHoles )
{
    BOARD board;

    const BOX2I outlineBox( { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 20 ) },
                            { pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 40 ) } );
    buildOutline( board, outlineBox );

    PCB_DRILL_MAP  map( &board );
    const VECTOR2I offset( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) );

    map.SetLayer( User_1 );
    map.SetOffset( offset );

    // The outline is drawn and plotted, so it has to be selectable. Without holes there are
    // no marks to hit instead
    BOOST_CHECK( map.HitTest( outlineBox.GetOrigin() + offset, 0 ) );
    BOOST_CHECK( map.HitTest( VECTOR2I( outlineBox.GetLeft(), outlineBox.GetCenter().y ) + offset, 0 ) );

    // Inside the outline is not on it, and neither is the undisplaced board
    BOOST_CHECK( !map.HitTest( outlineBox.GetCenter() + offset, 0 ) );
    BOOST_CHECK( !map.HitTest( outlineBox.GetOrigin(), 0 ) );
}


BOOST_AUTO_TEST_CASE( MapOwnsItsSymbolSizeAndOnlyExposesOffsets )
{
    BOARD board;
    const int defaultSize = pcbIUScale.mmToIU( 2.5 );
    board.GetDesignSettings().GetDrillSymbolProfile().SetSymbolSize( defaultSize );

    PCB_DRILL_MAP map( &board );
    BOOST_CHECK_EQUAL( map.GetSymbolSize(), defaultSize );

    const auto& properties = PROPERTY_MANAGER::Instance().GetProperties( TYPE_HASH( PCB_DRILL_MAP ) );
    std::set<wxString> names;

    for( const PROPERTY_BASE* property : properties )
        names.insert( property->Name() );

    BOOST_CHECK( names.contains( wxT( "Offset X" ) ) );
    BOOST_CHECK( names.contains( wxT( "Offset Y" ) ) );
    BOOST_CHECK( names.contains( wxT( "Symbol Size" ) ) );
    BOOST_CHECK( !names.contains( wxT( "Position X" ) ) );
    BOOST_CHECK( !names.contains( wxT( "Position Y" ) ) );
}


BOOST_AUTO_TEST_CASE( MapSymbolSizeSurvivesCloneSwapAndProtobuf )
{
    BOARD board;
    const int customSize = pcbIUScale.mmToIU( 3.75 );

    PCB_DRILL_MAP map( &board );
    map.SetLayer( User_1 );
    map.SetSymbolSize( customSize );

    std::unique_ptr<EDA_ITEM> clone( map.Clone() );
    PCB_DRILL_MAP* copy = dynamic_cast<PCB_DRILL_MAP*>( clone.get() );
    BOOST_REQUIRE( copy );
    BOOST_CHECK_EQUAL( copy->GetSymbolSize(), customSize );

    PCB_DRILL_MAP other( &board );
    other.SetLayer( User_1 );
    const int otherSize = pcbIUScale.mmToIU( 1.25 );
    other.SetSymbolSize( otherSize );
    map.SwapItemData( &other );
    BOOST_CHECK_EQUAL( map.GetSymbolSize(), otherSize );
    BOOST_CHECK_EQUAL( other.GetSymbolSize(), customSize );

    google::protobuf::Any container;
    other.Serialize( container );

    PCB_DRILL_MAP restored( &board );
    BOOST_REQUIRE( restored.Deserialize( container ) );
    BOOST_CHECK_EQUAL( restored.GetSymbolSize(), customSize );
}


BOOST_AUTO_TEST_CASE( MapSymbolSizeSurvivesBoardFileRoundTrip )
{
    BOARD board;
    const int customSize = pcbIUScale.mmToIU( 4.25 );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    map->SetSymbolSize( customSize );
    board.Add( map );

    const std::filesystem::path path(
            wxFileName::CreateTempFileName( wxT( "qa_drill_map_roundtrip" ) ).ToStdString() );
    KI_TEST::DumpBoardToFile( board, path.string() );

    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );

    const std::vector<const PCB_DRILL_MAP*> maps = restored->DrillMapsOnLayer( User_1 );
    BOOST_REQUIRE_EQUAL( maps.size(), 1 );
    BOOST_CHECK_EQUAL( maps.front()->GetSymbolSize(), customSize );
}


BOOST_AUTO_TEST_CASE( HoleViewBoundsIncludeDisplacedMapSymbols )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    map->SetOffset( { pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) } );
    map->SetSymbolSize( pcbIUScale.mmToIU( 12 ) );
    board.Add( map );

    PCB_TRACK*     via = board.Tracks().front();
    const VECTOR2I displacedSymbol = via->GetPosition() + map->GetOffset();

    BOOST_CHECK( via->ViewBBox().Contains( displacedSymbol ) );
    BOOST_CHECK( via->ViewBBox().Contains( displacedSymbol + VECTOR2I( 2 * map->GetSymbolSize(), 0 ) ) );
}


BOOST_AUTO_TEST_CASE( MapBoardOutlineFollowsItsOffset )
{
    BOARD board;
    buildHoles( board );

    const BOX2I outlineBox( { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 20 ) },
                            { pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 40 ) } );
    buildOutline( board, outlineBox );

    PCB_DRILL_MAP  map( &board );
    const VECTOR2I offset( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) );
    map.SetOffset( offset );

    const std::shared_ptr<const SHAPE_POLY_SET> outlines = map.GetBoardOutlines();

    BOOST_REQUIRE_EQUAL( outlines->OutlineCount(), 1 );
    BOOST_CHECK( outlines->BBox() == BOX2I( outlineBox.GetOrigin() + offset, outlineBox.GetSize() ) );
}


BOOST_AUTO_TEST_CASE( SpanChoiceRoundTripsThroughTheBoardSpans )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    board.Add( map );

    BOOST_CHECK_EQUAL( map->GetSpanChoice(), -1 );

    const std::vector<DRILL_SPAN> spans = EnumerateDrillSpans( board );

    BOOST_REQUIRE( !spans.empty() );

    map->SetSpanChoice( 0 );

    BOOST_CHECK( !map->GetAllSpans() );
    BOOST_CHECK( map->GetSpan() == spans[0] );
    BOOST_CHECK_EQUAL( map->GetSpanChoice(), 0 );

    // A choice the board cannot honour means every span, not a map with nothing on it
    map->SetSpanChoice( static_cast<int>( spans.size() ) );

    BOOST_CHECK( map->GetAllSpans() );
    BOOST_CHECK_EQUAL( map->GetSpanChoice(), -1 );
}


BOOST_AUTO_TEST_CASE( ChartCellTextIsNotUserEditableButItsFormattingIs )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    PCB_TABLE table( &board );
    table.SetColCount( 1 );
    table.ResizeCells( 1, 1 );

    BOOST_REQUIRE( !chart.GetCells().empty() );
    BOOST_REQUIRE( !table.GetCells().empty() );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    PROPERTY_BASE* text = propMgr.GetProperty( TYPE_HASH( EDA_TEXT ), _HKI( "Text" ) );

    BOOST_REQUIRE( text );

    // Scoped to PCB_TABLECELL rather than set on the EDA_TEXT descriptor, which eeschema
    // shares. The base property itself must stay writeable
    BOOST_CHECK( text->Writeable( chart.GetCells().front() ) );

    // The chart says what the board says. An ordinary table is still the user's to write
    BOOST_CHECK( !propMgr.IsWriteableFor( TYPE_HASH( PCB_TABLECELL ), text, chart.GetCells().front() ) );
    BOOST_CHECK( propMgr.IsWriteableFor( TYPE_HASH( PCB_TABLECELL ), text, table.GetCells().front() ) );
}


BOOST_AUTO_TEST_CASE( OnlyHolesCarryTheDrillSymbolMargin )
{
    BOARD board;
    buildHoles( board );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50 ), 0 ) );

    PAD* smd = new PAD( fp );
    smd->SetAttribute( PAD_ATTRIB::SMD );
    smd->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50 ), 0 ) );
    smd->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    fp->Add( smd );
    board.Add( fp );

    const BOX2I unmapped = smd->ViewBBox();

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    map->SetOffset( { pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) } );
    board.Add( map );
    board.RefreshDrillSymbolLayers();

    // An SMD pad draws no symbol, so widening its bounds to the far side of the board would
    // only defeat culling
    BOOST_CHECK( smd->ViewBBox() == unmapped );

    PCB_TRACK* segment = new PCB_TRACK( &board );
    segment->SetStart( VECTOR2I( 0, 0 ) );
    segment->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );
    segment->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
    board.Add( segment );

    BOOST_CHECK( !segment->ViewBBox().Contains( segment->GetStart() + map->GetOffset() ) );
}


BOOST_AUTO_TEST_CASE( MovingAMapUpdatesTheHoleViewBounds )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &board );
    map->SetLayer( User_1 );
    map->SetSymbolSize( pcbIUScale.mmToIU( 2 ) );
    board.Add( map );
    board.RefreshDrillSymbolLayers();

    PCB_TRACK* via = board.Tracks().front();

    map->SetOffset( { pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) } );
    board.RefreshDrillSymbolLayers();

    // The layer set has not changed, only the offset, and the cached placements have to
    // notice that or the marks are culled where they now are
    BOOST_CHECK( via->ViewBBox().Contains( via->GetPosition() + map->GetOffset() ) );
}


BOOST_AUTO_TEST_CASE( CellFormattingSurvivesARebuild )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    // The last data row, past the heading
    const int row = chart.GetRowCount() - 2;

    BOOST_REQUIRE( row > 1 );

    for( int col = 0; col < chart.GetColCount(); ++col )
    {
        chart.GetCell( row, col )->SetBold( true );
        chart.GetCell( row, col )->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 2.5 ),
                                                          pcbIUScale.mmToIU( 2.5 ) ) );
    }

    chart.RebuildCells( board );

    // Cells are reused rather than recreated, which is what lets the ordinary table
    // formatting be the chart's formatting
    BOOST_CHECK( chart.GetCell( row, 0 )->IsBold() );
    BOOST_CHECK_EQUAL( chart.GetCell( row, 0 )->GetTextHeight(), pcbIUScale.mmToIU( 2.5 ) );
}


/// The row reporting a given drill group, or -1. Groups are ordered by size, so the diameter
/// text is what identifies the row to a reader.
int rowShowingDiameter( const PCB_DRILL_CHART& aChart, const wxString& aText )
{
    for( const PCB_TABLECELL* cell : aChart.GetCells() )
    {
        if( cell->GetText() == aText )
            return cell->GetRow();
    }

    return -1;
}


BOOST_AUTO_TEST_CASE( CellFormattingFollowsItsGroupWhenAGroupIsAdded )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    // The 3.2 mm mounting hole, which is the largest and so the last data row
    const int before = rowShowingDiameter( chart, wxT( "3.200" ) );

    BOOST_REQUIRE( before > 1 );
    BOOST_REQUIRE( chart.IsDataRow( before ) );

    for( int col = 0; col < chart.GetColCount(); ++col )
        chart.GetCell( before, col )->SetBold( true );

    // A smaller hole than any already present, so it takes the first data row and pushes
    // every existing group down one
    PCB_VIA* tiny = new PCB_VIA( &board );
    tiny->SetPadstackMode( PADSTACK::MODE::NORMAL );
    tiny->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 40 ), 0 ) );
    tiny->SetLayerPair( F_Cu, B_Cu );
    tiny->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
    tiny->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.40 ) );
    board.Add( tiny );

    chart.RebuildCells( board );

    const int after = rowShowingDiameter( chart, wxT( "3.200" ) );

    BOOST_REQUIRE( after > 1 );
    BOOST_CHECK_NE( after, before );

    // The formatting belongs to the group, not to the row number it happened to occupy
    BOOST_CHECK( chart.GetCell( after, 0 )->IsBold() );
    BOOST_CHECK( !chart.GetCell( before, 0 )->IsBold() );
}


BOOST_AUTO_TEST_CASE( CellFormattingFollowsItsGroupWhenAGroupIsRemoved )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    const int before = rowShowingDiameter( chart, wxT( "3.200" ) );

    BOOST_REQUIRE( before > 1 );

    for( int col = 0; col < chart.GetColCount(); ++col )
        chart.GetCell( before, col )->SetBold( true );

    // Drop the smallest group entirely, so the rows below it move up. Both 0.30 vias have to
    // go. One of them left behind keeps the group and the table the same shape.
    std::vector<PCB_TRACK*> doomed;

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() == PCB_VIA_T
            && static_cast<PCB_VIA*>( track )->GetDrillValue() == pcbIUScale.mmToIU( 0.30 ) )
        {
            doomed.push_back( track );
        }
    }

    BOOST_REQUIRE_EQUAL( doomed.size(), 2 );

    for( PCB_TRACK* track : doomed )
    {
        board.Remove( track );
        delete track;
    }

    const int rowsBefore = chart.GetRowCount();

    chart.RebuildCells( board );

    // The group really did go, or the rest of this proves nothing
    BOOST_REQUIRE_EQUAL( chart.GetRowCount(), rowsBefore - 1 );

    const int after = rowShowingDiameter( chart, wxT( "3.200" ) );

    BOOST_REQUIRE( after > 1 );
    BOOST_REQUIRE_NE( after, before );
    BOOST_CHECK( chart.GetCell( after, 0 )->IsBold() );
}


BOOST_AUTO_TEST_CASE( RowsKeepSeparateCellsWhenARebuildReordersThem )
{
    BOARD board;
    buildHoles( board );

    // A buried via drilled like the through ones. The profile groups by span, so it reports
    // its own row rather than joining theirs
    PCB_VIA* buried = new PCB_VIA( &board );
    buried->SetPadstackMode( PADSTACK::MODE::NORMAL );
    buried->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );

    // Before the layer pair: SanitizeLayers() puts a through via back on F_Cu/B_Cu, which
    // would quietly merge this into the through group and leave nothing to tell apart
    buried->SetViaType( VIATYPE::BURIED );
    buried->SetLayerPair( In1_Cu, In2_Cu );
    buried->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    buried->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.60 ) );
    board.Add( buried );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    const std::map<int, std::string> keys = chart.RowKeys();
    std::set<std::string>            distinct;

    for( const auto& [row, key] : keys )
        distinct.insert( key );

    // The through 0.30s, the buried 0.30, the 0.60 via and the 3.2 mm mounting hole
    BOOST_REQUIRE_EQUAL( keys.size(), 4 );

    // Identity has to tell apart rows the chart is showing separately
    BOOST_CHECK_EQUAL( distinct.size(), keys.size() );

    // A rebuild that reorders the rows is what makes the migration run
    PCB_VIA* tiny = new PCB_VIA( &board );
    tiny->SetPadstackMode( PADSTACK::MODE::NORMAL );
    tiny->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 40 ), 0 ) );
    tiny->SetLayerPair( F_Cu, B_Cu );
    tiny->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
    tiny->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.40 ) );
    board.Add( tiny );

    chart.RebuildCells( board );

    // Two rows holding one cell would be freed twice when the table goes
    std::set<const PCB_TABLECELL*> seen;

    for( const PCB_TABLECELL* cell : chart.GetCells() )
    {
        BOOST_REQUIRE( cell );
        BOOST_CHECK( seen.insert( cell ).second );
    }

    BOOST_CHECK_EQUAL( seen.size(), chart.GetCells().size() );
}


BOOST_AUTO_TEST_CASE( RowKeysSurviveABoardFileRoundTrip )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &board );
    chart->SetLayer( User_1 );
    chart->RebuildCells( board );
    board.Add( chart );

    const std::map<int, std::string> keys = chart->RowKeys();

    BOOST_REQUIRE( !keys.empty() );

    const std::filesystem::path path(
            wxFileName::CreateTempFileName( wxT( "qa_drill_chart_rowkeys" ) ).ToStdString() );
    KI_TEST::DumpBoardToFile( board, path.string() );

    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );

    const PCB_DRILL_CHART* reloaded = nullptr;

    for( const BOARD_ITEM* item : restored->Drawings() )
    {
        if( item->Type() == PCB_DRILL_CHART_T )
            reloaded = static_cast<const PCB_DRILL_CHART*>( item );
    }

    BOOST_REQUIRE( reloaded );

    // Without these the first rebuild after a load has nothing to match the loaded rows
    // against, and formatting would migrate by position
    BOOST_CHECK( reloaded->RowKeys() == keys );
}


BOOST_AUTO_TEST_CASE( ChartIsVisitedOnceWhenBothTypesAreScanned )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.RebuildCells( board );

    int hits = 0;

    INSPECTOR_FUNC inspector =
            [&hits]( EDA_ITEM* aItem, void* aTestData )
            {
                if( aItem->Type() == PCB_DRILL_CHART_T )
                    hits++;

                return INSPECT_RESULT::CONTINUE;
            };

    // A chart answers to both, and collecting it twice puts it in the disambiguation menu
    // twice
    chart.Visit( inspector, nullptr, { PCB_TABLE_T, PCB_DRILL_CHART_T } );

    BOOST_CHECK_EQUAL( hits, 1 );
}


BOOST_AUTO_TEST_CASE( FreshChartIsBuiltAtTheOrigin )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.SetLayer( Cmts_User );
    chart.RebuildCells( board );

    const BOX2I bbox = chart.GetBoundingBox();

    // A new cell carries a half-INT_MAX rectangle and Normalize() anchors on cell 0's centre,
    // so without correction the chart lands half a metre off the board
    BOOST_CHECK_MESSAGE( std::abs( bbox.GetLeft() ) < pcbIUScale.mmToIU( 1 )
                                 && std::abs( bbox.GetTop() ) < pcbIUScale.mmToIU( 1 ),
                         "chart origin is (" << bbox.GetLeft() << "," << bbox.GetTop()
                                             << "), expected near 0" );

    // Cells keep their wide default rectangle so the text does not wrap. A chart this size
    // must be wider than it is tall
    BOOST_CHECK( bbox.GetWidth() > bbox.GetHeight() );
}


BOOST_AUTO_TEST_CASE( RebuildKeepsChartPosition )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.SetLayer( Cmts_User );
    chart.RebuildCells( board );

    const VECTOR2I placed( pcbIUScale.mmToIU( 60 ), pcbIUScale.mmToIU( 40 ) );
    chart.Move( placed );

    const VECTOR2I before = chart.GetPosition();
    chart.RebuildCells( board );

    // A rebuild must not walk the chart across the board
    BOOST_CHECK_EQUAL( chart.GetPosition().x, before.x );
    BOOST_CHECK_EQUAL( chart.GetPosition().y, before.y );
}


BOOST_AUTO_TEST_CASE( CloneAndSwapPreserveChartSettings )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART chart( &board );
    chart.SetUnits( DRILL_CHART_UNITS::INCH );
    chart.SetPrecision( 5 );
    chart.RebuildCells( board );

    std::unique_ptr<EDA_ITEM> clone( chart.Clone() );

    // A base PCB_TABLE clone would still report PCB_DRILL_CHART_T and then be cast to one
    BOOST_REQUIRE_EQUAL( clone->Type(), PCB_DRILL_CHART_T );

    PCB_DRILL_CHART* copy = dynamic_cast<PCB_DRILL_CHART*>( clone.get() );
    BOOST_REQUIRE( copy );
    BOOST_CHECK( copy->GetUnits() == DRILL_CHART_UNITS::INCH );
    BOOST_CHECK_EQUAL( copy->GetPrecision(), 5 );

    copy->SetPrecision( 2 );
    chart.SwapItemData( copy );

    BOOST_CHECK_EQUAL( chart.GetPrecision(), 2 );
    BOOST_CHECK_EQUAL( copy->GetPrecision(), 5 );
}


BOOST_AUTO_TEST_CASE( EverySymbolResolvesWithoutAChart )
{
    BOARD board;
    buildHoles( board );

    // A drill map must show marks on a board where no chart has ever been placed, so the
    // resolver has to cover every group rather than reading assignments the profile lacks
    BOOST_CHECK( board.GetDesignSettings().GetDrillSymbolProfile().Assignments().empty() );

    const std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> resolved = ResolveDrillSymbols( board );

    DRILL_CHART_MODEL model( board.GetDesignSettings().GetDrillSymbolProfile() );
    model.Build( board, EnumerateDrillSpans( board ) );

    BOOST_REQUIRE( !model.Groups().empty() );
    BOOST_CHECK_EQUAL( resolved.size(), model.Groups().size() );

    for( const DRILL_CHART_GROUP& group : model.Groups() )
        BOOST_CHECK_MESSAGE( resolved.count( group.m_Key ), "no symbol for " << group.m_Key );

    // Resolving must not have written anything back to the board
    BOOST_CHECK( board.GetDesignSettings().GetDrillSymbolProfile().Assignments().empty() );
}


BOOST_AUTO_TEST_CASE( HandFormattedCellsSurviveABoardFileRoundTrip )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &board );
    chart->SetLayer( User_1 );
    board.Add( chart );
    chart->RebuildCells( board );

    const int row = chart->GetRowCount() - 2;

    BOOST_REQUIRE( row > 1 );

    const VECTOR2I textSize( pcbIUScale.mmToIU( 2.5 ), pcbIUScale.mmToIU( 2.5 ) );

    for( int col = 0; col < chart->GetColCount(); ++col )
    {
        chart->GetCell( row, col )->SetBold( true );
        chart->GetCell( row, col )->SetTextSize( textSize );
    }

    const std::filesystem::path path(
            wxFileName::CreateTempFileName( wxT( "qa_drill_chart_cellfmt" ) ).ToStdString() );
    KI_TEST::DumpBoardToFile( board, path.string() );

    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );

    PCB_DRILL_CHART* reloaded = nullptr;

    for( BOARD_ITEM* item : restored->Drawings() )
    {
        if( item->Type() == PCB_DRILL_CHART_T )
            reloaded = static_cast<PCB_DRILL_CHART*>( item );
    }

    BOOST_REQUIRE( reloaded );

    // The rebuild every load performs has to migrate the formatting onto the new cells, which
    // it can only do if the old ones came back out of the file
    reloaded->RebuildCells( *restored );

    BOOST_REQUIRE( reloaded->GetRowCount() - 2 == row );
    BOOST_CHECK( reloaded->GetCell( row, 0 )->IsBold() );
    BOOST_CHECK_EQUAL( reloaded->GetCell( row, 0 )->GetTextHeight(), textSize.y );
}


BOOST_AUTO_TEST_CASE( DefaultColumnHeadingsSurviveBeingLeftOutOfTheFile )
{
    BOARD board;
    buildHoles( board );

    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &board );
    chart->SetLayer( User_1 );
    board.Add( chart );
    chart->RebuildCells( board );

    const std::vector<DRILL_CHART_COLUMN> columns = chart->Columns();

    const std::filesystem::path path(
            wxFileName::CreateTempFileName( wxT( "qa_drill_chart_coldefaults" ) ).ToStdString() );
    KI_TEST::DumpBoardToFile( board, path.string() );

    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );

    const PCB_DRILL_CHART* reloaded = nullptr;

    for( const BOARD_ITEM* item : restored->Drawings() )
    {
        if( item->Type() == PCB_DRILL_CHART_T )
            reloaded = static_cast<const PCB_DRILL_CHART*>( item );
    }

    BOOST_REQUIRE( reloaded );
    BOOST_REQUIRE_EQUAL( reloaded->Columns().size(), columns.size() );

    for( size_t ii = 0; ii < columns.size(); ++ii )
    {
        BOOST_CHECK( reloaded->Columns()[ii].m_Heading == columns[ii].m_Heading );
        BOOST_CHECK( reloaded->Columns()[ii].m_Align == columns[ii].m_Align );
    }
}


BOOST_AUTO_TEST_SUITE_END()
