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

#include <algorithm>
#include <filesystem>
#include <memory>
#include <set>
#include <vector>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <project/net_settings.h>
#include <netclass.h>
#include <netinfo.h>
#include <json_common.h>
#include <collectors.h>
#include <common.h>
#include <footprint.h>
#include <pad.h>
#include <connectivity/connectivity_data.h>
#include <lset.h>
#include <netinfo.h>
#include <pcb_group.h>
#include <inspectable_impl.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <generators/pcb_via_stack.h>
#include <generators_mgr.h>
#include <router/router_tool.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

BOOST_AUTO_TEST_SUITE( ViaStackSaveLoad )


static PCB_TRACK* makeSegment( BOARD* aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_TRACK* track = new PCB_TRACK( aBoard );
    track->SetStart( aStart );
    track->SetEnd( aEnd );
    track->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
    track->SetLayer( In1_Cu );
    aBoard->Add( track );

    return track;
}


// The factory must know the via_stack type (registration happened at static init).
BOOST_AUTO_TEST_CASE( FactoryRegistered )
{
    std::unique_ptr<PCB_GENERATOR> gen( GENERATORS_MGR::Instance().CreateFromType( wxS( "via_stack" ) ) );

    BOOST_REQUIRE( gen );
    BOOST_CHECK_EQUAL( gen->GetGeneratorType(), wxS( "via_stack" ) );
    BOOST_CHECK( dynamic_cast<PCB_VIA_STACK*>( gen.get() ) != nullptr );
}


// Build a via_stack with members and every stack level property, save, reload, and
// verify the properties and membership survive.
BOOST_AUTO_TEST_CASE( PropertiesAndMembersRoundTrip )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    PCB_TRACK* seg0 = makeSegment( board.get(), VECTOR2I( 0, 0 ), VECTOR2I( pcbIUScale.mmToIU( 0.35 ), 0 ) );
    PCB_TRACK* seg1 = makeSegment( board.get(), VECTOR2I( pcbIUScale.mmToIU( 0.35 ), 0 ),
                                   VECTOR2I( pcbIUScale.mmToIU( 0.70 ), 0 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.35 ) );
    stack->SetFilled( true );
    stack->SetCapped( true );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetUseNetclass( true );
    stack->SetPresetName( wxS( "HDI stack" ) );

    SHAPE_LINE_CHAIN hops;
    hops.Append( VECTOR2I( 0, 0 ) );
    hops.Append( VECTOR2I( pcbIUScale.mmToIU( 0.35 ), 0 ) );
    hops.Append( VECTOR2I( pcbIUScale.mmToIU( 0.70 ), 0 ) );
    stack->SetHops( hops );

    stack->AddItem( seg0 );
    stack->AddItem( seg1 );
    board->Add( stack );

    auto path = std::filesystem::temp_directory_path() / "via_stack_saveload_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board, path.string() );

    std::unique_ptr<BOARD> reloaded = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_REQUIRE_EQUAL( reloaded->Generators().size(), 1u );

    PCB_VIA_STACK* loaded = dynamic_cast<PCB_VIA_STACK*>( reloaded->Generators().front() );
    BOOST_REQUIRE( loaded );

    BOOST_CHECK_EQUAL( (int) loaded->GetStartLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) loaded->GetEndLayer(), (int) In2_Cu );
    BOOST_CHECK( loaded->GetStyle() == VIA_STACK_STYLE::STAGGERED );
    BOOST_CHECK_EQUAL( loaded->GetPitch(), pcbIUScale.mmToIU( 0.35 ) );
    BOOST_CHECK_EQUAL( loaded->IsFilled(), true );
    BOOST_CHECK_EQUAL( loaded->IsCapped(), true );
    BOOST_CHECK_EQUAL( loaded->GetViaSize(), pcbIUScale.mmToIU( 0.30 ) );
    BOOST_CHECK_EQUAL( loaded->GetViaDrill(), pcbIUScale.mmToIU( 0.15 ) );
    BOOST_CHECK_EQUAL( loaded->GetUseNetclass(), true );
    BOOST_CHECK_EQUAL( loaded->GetPresetName(), wxS( "HDI stack" ) );

    BOOST_REQUIRE( loaded->GetHops().has_value() );
    BOOST_CHECK_EQUAL( loaded->GetHops()->PointCount(), 3 );
    BOOST_CHECK( loaded->GetHops()->CPoint( 2 ) == VECTOR2I( pcbIUScale.mmToIU( 0.70 ), 0 ) );

    BOOST_CHECK_EQUAL( loaded->GetItems().size(), 2u );
}


// A stacked stack materializes one coaxial microvia per adjacent copper pair, no traces.
BOOST_AUTO_TEST_CASE( StackedGeometry )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I origin( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( origin );
    board->Add( stack );

    stack->Regenerate( board.get(), nullptr );

    std::vector<PCB_VIA*> vias;
    int                   traces = 0;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            vias.push_back( static_cast<PCB_VIA*>( item ) );
        else if( item->Type() == PCB_TRACE_T )
            traces++;
    }

    BOOST_CHECK_EQUAL( vias.size(), 2u ); // F.Cu-In1 and In1-In2
    BOOST_CHECK_EQUAL( traces, 0 );

    std::set<std::pair<int, int>> pairs;

    for( PCB_VIA* via : vias )
    {
        BOOST_CHECK( via->GetViaType() == VIATYPE::MICROVIA );
        BOOST_CHECK( via->GetPosition() == origin ); // coaxial
        pairs.insert( { (int) via->TopLayer(), (int) via->BottomLayer() } );
    }

    BOOST_CHECK_EQUAL( pairs.count( { (int) F_Cu, (int) In1_Cu } ), 1u );
    BOOST_CHECK_EQUAL( pairs.count( { (int) In1_Cu, (int) In2_Cu } ), 1u );
}


// A staggered stack offsets each hop along the hop polyline and joins them with traces.
BOOST_AUTO_TEST_CASE( StaggeredGeometry )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I p0( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I p1( pcbIUScale.mmToIU( 10.35 ), pcbIUScale.mmToIU( 10 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.35 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );

    SHAPE_LINE_CHAIN hops;
    hops.Append( p0 );
    hops.Append( p1 );
    stack->SetHops( hops );
    board->Add( stack );

    stack->Regenerate( board.get(), nullptr );

    int                     vias = 0;
    std::vector<PCB_TRACK*> traces;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            vias++;
        else if( item->Type() == PCB_TRACE_T )
            traces.push_back( static_cast<PCB_TRACK*>( item ) );
    }

    BOOST_CHECK_EQUAL( vias, 2 );             // one per hop
    BOOST_REQUIRE_EQUAL( traces.size(), 1u ); // one shared-layer trace between the two hops

    PCB_TRACK* trace = traces.front();
    BOOST_CHECK_EQUAL( (int) trace->GetLayer(), (int) In1_Cu );
    BOOST_CHECK( trace->GetStart() == p0 );
    BOOST_CHECK( trace->GetEnd() == p1 );
}


// Moving a staggered stack must carry the hop polyline along, or the next regenerate snaps
// the vias back to the old location.
BOOST_AUTO_TEST_CASE( MoveTranslatesHops )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I p0( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I p1( pcbIUScale.mmToIU( 10.35 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I delta( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( -3 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.35 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( p0 );

    SHAPE_LINE_CHAIN hops;
    hops.Append( p0 );
    hops.Append( p1 );
    stack->SetHops( hops );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    stack->Move( delta );
    stack->Regenerate( board.get(), nullptr );

    std::set<VECTOR2I> positions;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            positions.insert( static_cast<PCB_VIA*>( item )->GetPosition() );
    }

    BOOST_CHECK_EQUAL( positions.count( p0 + delta ), 1u );
    BOOST_CHECK_EQUAL( positions.count( p1 + delta ), 1u );
    BOOST_CHECK_EQUAL( positions.count( p0 ), 0u );
}


// Half a turn about the first hop puts the second the same distance the other side, whichever
// way the rotation runs.
BOOST_AUTO_TEST_CASE( RotateTurnsTheHopsWithTheStack )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I p0( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I p1( pcbIUScale.mmToIU( 10.35 ), pcbIUScale.mmToIU( 10 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.35 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( p0 );

    SHAPE_LINE_CHAIN hops;
    hops.Append( p0 );
    hops.Append( p1 );
    stack->SetHops( hops );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    stack->Rotate( p0, EDA_ANGLE( 180, DEGREES_T ) );
    stack->Regenerate( board.get(), nullptr );

    std::set<VECTOR2I> positions;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            positions.insert( static_cast<PCB_VIA*>( item )->GetPosition() );
    }

    BOOST_CHECK_EQUAL( positions.count( p0 ), 1u );
    BOOST_CHECK_EQUAL( positions.count( p0 - ( p1 - p0 ) ), 1u );
    BOOST_CHECK_EQUAL( positions.count( p1 ), 0u );
}


// Flipping puts the stack on the other side of the board, so its span has to flip with it.
// Mirroring stays in plane and must leave the span alone.
BOOST_AUTO_TEST_CASE( FlipSwapsTheSpanButMirrorDoesNot )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I p0( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I p1( pcbIUScale.mmToIU( 10.35 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I centre( 0, 0 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.35 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( p0 );

    SHAPE_LINE_CHAIN hops;
    hops.Append( p0 );
    hops.Append( p1 );
    stack->SetHops( hops );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    stack->Flip( centre, FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( (int) stack->GetStartLayer(), (int) B_Cu );
    BOOST_CHECK_EQUAL( (int) stack->GetEndLayer(), (int) In1_Cu );

    BOOST_REQUIRE( stack->GetHops().has_value() );
    BOOST_CHECK( stack->GetHops()->CPoint( 0 ) == VECTOR2I( -p0.x, p0.y ) );
    BOOST_CHECK( stack->GetHops()->CPoint( 1 ) == VECTOR2I( -p1.x, p1.y ) );

    stack->Flip( centre, FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( (int) stack->GetStartLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) stack->GetEndLayer(), (int) In2_Cu );

    stack->Mirror( centre, FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( (int) stack->GetStartLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) stack->GetEndLayer(), (int) In2_Cu );

    BOOST_REQUIRE( stack->GetHops().has_value() );
    BOOST_CHECK( stack->GetHops()->CPoint( 0 ) == VECTOR2I( -p0.x, p0.y ) );
    BOOST_CHECK( stack->GetHops()->CPoint( 1 ) == VECTOR2I( -p1.x, p1.y ) );
}


// A span referencing layers absent from the board must build nothing instead of iterating
// LAYER_RANGE forever.
BOOST_AUTO_TEST_CASE( InvalidSpanBuildsNothing )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    board->SetCopperLayerCount( 4 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In6_Cu ); // not present on a 4 layer board
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board->Add( stack );

    stack->Regenerate( board.get(), nullptr );

    BOOST_CHECK_EQUAL( stack->GetItems().size(), 0u );
    BOOST_CHECK( !PCB_VIA_STACK::IsSpanValid( board.get(), F_Cu, In6_Cu ) );
    BOOST_CHECK( !PCB_VIA_STACK::IsSpanValid( board.get(), F_Cu, F_Cu ) );

    // A layer combo the board cannot fill reports UNDEFINED_LAYER.
    BOOST_CHECK( !PCB_VIA_STACK::IsSpanValid( board.get(), F_Cu, UNDEFINED_LAYER ) );
    BOOST_CHECK( !PCB_VIA_STACK::IsSpanValid( board.get(), UNDEFINED_LAYER, In2_Cu ) );
    BOOST_CHECK( PCB_VIA_STACK::IsSpanValid( board.get(), F_Cu, In2_Cu ) );
    BOOST_CHECK( PCB_VIA_STACK::IsSpanValid( board.get(), F_Cu, B_Cu ) );
}


static PCB_VIA* makeMicrovia( BOARD* aBoard, const VECTOR2I& aPos, PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom,
                              int aNet = 0 )
{
    PCB_VIA* via = new PCB_VIA( aBoard );
    via->SetViaType( VIATYPE::MICROVIA );
    via->SetLayerPair( aTop, aBottom );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
    via->SetPosition( aPos );
    via->SetNetCode( aNet );
    aBoard->Add( via );

    return via;
}


// Two coaxial single hop microvias make a stacked stack.
BOOST_AUTO_TEST_CASE( CreateFromItemsStacked )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );

    std::vector<BOARD_ITEM*> items;
    items.push_back( makeMicrovia( board.get(), pos, F_Cu, In1_Cu ) );
    items.push_back( makeMicrovia( board.get(), pos, In1_Cu, In2_Cu ) );

    PCB_VIA_STACK* stack = PCB_VIA_STACK::CreateFromItems( items, board.get() );

    BOOST_REQUIRE( stack );
    BOOST_CHECK( stack->GetStyle() == VIA_STACK_STYLE::STACKED );
    BOOST_CHECK_EQUAL( (int) stack->GetStartLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) stack->GetEndLayer(), (int) In2_Cu );
    BOOST_CHECK_EQUAL( stack->GetViaSize(), pcbIUScale.mmToIU( 0.3 ) );

    delete stack;
}


// A skipped layer must be rejected.
BOOST_AUTO_TEST_CASE( CreateFromItemsRejectsGap )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );

    std::vector<BOARD_ITEM*> items;
    items.push_back( makeMicrovia( board.get(), pos, F_Cu, In1_Cu ) );
    items.push_back( makeMicrovia( board.get(), pos, In2_Cu, B_Cu ) );

    BOOST_CHECK( PCB_VIA_STACK::CreateFromItems( items, board.get() ) == nullptr );
}


// The router hand-off resumes at the via reaching the end layer, which on a staggered stack
// is nowhere near the stack's own position.
BOOST_AUTO_TEST_CASE( OnlyTheLastHopReachesTheEndLayer )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I first( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );
    VECTOR2I last( pcbIUScale.mmToIU( 7 ), pcbIUScale.mmToIU( 5 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );

    SHAPE_LINE_CHAIN hops;
    hops.Append( first );
    hops.Append( last );
    stack->SetHops( hops );
    stack->SetPosition( first );

    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    int      onEnd = 0;
    VECTOR2I found;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );

        if( via && via->IsOnLayer( stack->GetEndLayer() ) )
        {
            ++onEnd;
            found = via->GetPosition();
        }
    }

    BOOST_CHECK_EQUAL( onEnd, 1 );
    BOOST_CHECK_MESSAGE( found == last, "the end layer via is the last hop, not the stack position" );
    BOOST_CHECK( stack->GetPosition() != found );
}


BOOST_AUTO_TEST_CASE( PendingStackExpansionMatcher )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    board->Add( new NETINFO_ITEM( board.get(), wxT( "sig" ), 1 ) );
    board->Add( new NETINFO_ITEM( board.get(), wxT( "other" ), 2 ) );

    auto makeVia = [&]( PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom, int aNet )
    {
        PCB_VIA* via = new PCB_VIA( board.get() );
        via->SetViaType( VIATYPE::MICROVIA );
        via->SetLayerPair( aTop, aBottom );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetNetCode( aNet );
        board->Add( via );
        return via;
    };

    VIA_STACK_PRESET preset;
    preset.m_Name = wxT( "fanout" );

    std::vector<PENDING_STACK_EXPANSION> pending = { { F_Cu, In2_Cu, 1, preset } };
    std::set<KIID>                       preRoute;

    PCB_VIA* match = makeVia( F_Cu, In2_Cu, 1 );

    BOOST_REQUIRE( MatchPendingStackExpansion( match, preRoute, pending ) );
    BOOST_CHECK_EQUAL( MatchPendingStackExpansion( match, preRoute, pending )->m_Name, wxT( "fanout" ) );

    // Routing upward records the span end to start, while a via always holds it top down.
    std::vector<PENDING_STACK_EXPANSION> upward = { { In2_Cu, F_Cu, 1, preset } };
    BOOST_CHECK( MatchPendingStackExpansion( match, preRoute, upward ) );

    PCB_VIA* wrongNet = makeVia( F_Cu, In2_Cu, 2 );
    BOOST_CHECK( MatchPendingStackExpansion( wrongNet, preRoute, pending ) == nullptr );

    PCB_VIA* wrongSpan = makeVia( F_Cu, In1_Cu, 1 );
    BOOST_CHECK( MatchPendingStackExpansion( wrongSpan, preRoute, pending ) == nullptr );

    // A via that already existed is not one the route created.
    preRoute.insert( match->m_Uuid );
    BOOST_CHECK( MatchPendingStackExpansion( match, preRoute, pending ) == nullptr );
}


BOOST_AUTO_TEST_CASE( DuplicatePresetNamesAreDropped )
{
    BOARD_DESIGN_SETTINGS bds( nullptr, "board.design_settings" );

    nlohmann::json entries = nlohmann::json::array();

    entries.push_back( { { "name", "HDI" }, { "start_layer", "F.Cu" }, { "end_layer", "In1.Cu" } } );
    entries.push_back( { { "name", "hdi" }, { "start_layer", "F.Cu" }, { "end_layer", "In2.Cu" } } );
    entries.push_back( { { "name", "Other" }, { "start_layer", "F.Cu" }, { "end_layer", "In1.Cu" } } );

    bds.Set( "via_stack_presets", entries );
    bds.Load();

    BOOST_REQUIRE_EQUAL( bds.m_ViaStackPresets.size(), 2u );
    BOOST_CHECK_EQUAL( bds.m_ViaStackPresets[0].m_Name, wxS( "HDI" ) );
    BOOST_CHECK_EQUAL( bds.m_ViaStackPresets[1].m_Name, wxS( "Other" ) );
}


// Capping is an IPC-4761 attribute that reaches the fab, so grouping loose vias must not drop it.
// Only a hop touching an outer layer carries it, and that hop is the last one on a stack anchored
// at the back.
BOOST_AUTO_TEST_CASE( CreateFromItemsKeepsCapping )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    auto capped = []( PCB_VIA* aVia )
    {
        aVia->Padstack().Drill().is_capped = true;
        return aVia;
    };

    VECTOR2I front( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );

    std::vector<BOARD_ITEM*> fromFront = { capped( makeMicrovia( board.get(), front, F_Cu, In1_Cu ) ),
                                           makeMicrovia( board.get(), front, In1_Cu, In2_Cu ) };

    std::unique_ptr<PCB_VIA_STACK> topAnchored( PCB_VIA_STACK::CreateFromItems( fromFront, board.get() ) );

    BOOST_REQUIRE( topAnchored );
    BOOST_CHECK_MESSAGE( topAnchored->IsCapped(), "capping was dropped on a stack anchored at the front" );

    VECTOR2I back( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 5 ) );

    std::vector<BOARD_ITEM*> fromBack = { makeMicrovia( board.get(), back, In1_Cu, In2_Cu ),
                                          capped( makeMicrovia( board.get(), back, In2_Cu, B_Cu ) ) };

    std::unique_ptr<PCB_VIA_STACK> bottomAnchored( PCB_VIA_STACK::CreateFromItems( fromBack, board.get() ) );

    BOOST_REQUIRE( bottomAnchored );
    BOOST_CHECK_MESSAGE( bottomAnchored->IsCapped(), "capping was dropped on a stack anchored at the back" );
}


// Two hops can share a position while a third is offset. The hop chain must keep one point per
// via, or a later regenerate matches hop n to the wrong coordinate.
BOOST_AUTO_TEST_CASE( CreateFromItemsKeepsCoincidentHops )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    VECTOR2I at( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I offset( at.x + pcbIUScale.mmToIU( 0.45 ), at.y );

    std::vector<BOARD_ITEM*> items = { makeMicrovia( board.get(), at, F_Cu, In1_Cu ),
                                       makeMicrovia( board.get(), at, In1_Cu, In2_Cu ),
                                       makeMicrovia( board.get(), offset, In2_Cu, In3_Cu ) };

    std::unique_ptr<PCB_VIA_STACK> stack( PCB_VIA_STACK::CreateFromItems( items, board.get() ) );

    BOOST_REQUIRE( stack );
    BOOST_REQUIRE( stack->GetStyle() == VIA_STACK_STYLE::STAGGERED );
    BOOST_REQUIRE( stack->GetHops().has_value() );

    BOOST_CHECK_MESSAGE( stack->GetHops()->PointCount() == 3,
                         "hop chain has " << stack->GetHops()->PointCount() << " points for 3 vias" );

    BOOST_CHECK( stack->GetHops()->CPoint( 0 ) == at );
    BOOST_CHECK( stack->GetHops()->CPoint( 1 ) == at );
    BOOST_CHECK( stack->GetHops()->CPoint( 2 ) == offset );
}


// Vias on different nets must be rejected.
BOOST_AUTO_TEST_CASE( CreateFromItemsRejectsMixedNets )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    board->Add( new NETINFO_ITEM( board.get(), wxT( "A" ), 1 ) );
    board->Add( new NETINFO_ITEM( board.get(), wxT( "B" ), 2 ) );

    VECTOR2I pos( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );

    PCB_VIA* upper = makeMicrovia( board.get(), pos, F_Cu, In1_Cu );
    PCB_VIA* lower = makeMicrovia( board.get(), pos, In1_Cu, In2_Cu );

    upper->SetNetCode( 1 );
    lower->SetNetCode( 2 );

    std::vector<BOARD_ITEM*> items = { upper, lower };

    BOOST_CHECK( PCB_VIA_STACK::CreateFromItems( items, board.get() ) == nullptr );

    // Same net still works.
    lower->SetNetCode( 1 );

    std::unique_ptr<PCB_VIA_STACK> stack( PCB_VIA_STACK::CreateFromItems( items, board.get() ) );

    BOOST_REQUIRE( stack );
    BOOST_CHECK_EQUAL( stack->GetNetCode(), 1 );
}


// A layer name a hand-edited file got wrong must not resolve to a real layer, or the stack
// loads as a plausible span nobody asked for.
BOOST_AUTO_TEST_CASE( AnUnknownLayerNameLeavesTheSpanAlone )
{
    PCB_VIA_STACK stack( nullptr, In1_Cu );
    stack.SetStartLayer( In1_Cu );
    stack.SetEndLayer( In3_Cu );

    STRING_ANY_MAP props;
    props.set( "start_layer", wxString( wxS( "Not.A.Layer" ) ) );
    props.set( "end_layer", wxString( wxS( "B.Adhes" ) ) );

    stack.SetProperties( props );

    BOOST_CHECK_EQUAL( (int) stack.GetStartLayer(), (int) In1_Cu );
    BOOST_CHECK_EQUAL( (int) stack.GetEndLayer(), (int) In3_Cu );
}


// A preset names two layers, so placement anchors on one of them and lands on the other.
// Anywhere else is refused rather than walking a span the preset never described.
BOOST_AUTO_TEST_CASE( ViaStackPlacesOnlyFromItsTerminals )
{
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, In2_Cu, F_Cu ), (int) In2_Cu );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, In2_Cu, In2_Cu ), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( B_Cu, In3_Cu, B_Cu ), (int) In3_Cu );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( B_Cu, In3_Cu, In3_Cu ), (int) B_Cu );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, In2_Cu, In1_Cu ), (int) UNDEFINED_LAYER );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, In2_Cu, In4_Cu ), (int) UNDEFINED_LAYER );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, In2_Cu, Edge_Cuts ), (int) UNDEFINED_LAYER );
    BOOST_CHECK_EQUAL( (int) ViaStackTargetLayer( F_Cu, F_Cu, F_Cu ), (int) UNDEFINED_LAYER );
}


// A stack rebuilds what it owns, so a stray trace taken in here is deleted on the next
// regenerate.
BOOST_AUTO_TEST_CASE( CreateFromItemsTakesOnlyTheTracesLinkingItsHops )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    board->Add( new NETINFO_ITEM( board.get(), wxT( "A" ), 1 ) );
    board->Add( new NETINFO_ITEM( board.get(), wxT( "B" ), 2 ) );

    VECTOR2I p0( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );
    VECTOR2I p1( pcbIUScale.mmToIU( 5.35 ), pcbIUScale.mmToIU( 5 ) );
    VECTOR2I away( pcbIUScale.mmToIU( 9 ), pcbIUScale.mmToIU( 5 ) );

    PCB_VIA* hop0 = makeMicrovia( board.get(), p0, F_Cu, In1_Cu, 1 );
    PCB_VIA* hop1 = makeMicrovia( board.get(), p1, In1_Cu, In2_Cu, 1 );

    PCB_TRACK* connector = makeSegment( board.get(), p0, p1 );
    connector->SetNetCode( 1 );

    PCB_TRACK* offLanding = makeSegment( board.get(), p0, p1 );
    offLanding->SetLayer( In2_Cu );
    offLanding->SetNetCode( 1 );

    PCB_TRACK* otherNet = makeSegment( board.get(), p0, p1 );
    otherNet->SetNetCode( 2 );

    PCB_TRACK* loose = makeSegment( board.get(), p0, away );
    loose->SetNetCode( 1 );

    PCB_TRACK* zeroLength = makeSegment( board.get(), p0, p0 );
    zeroLength->SetNetCode( 1 );

    std::vector<BOARD_ITEM*> items = { hop0, hop1, connector, offLanding, otherNet, loose, zeroLength };
    std::vector<BOARD_ITEM*> members;

    std::unique_ptr<PCB_VIA_STACK> stack( PCB_VIA_STACK::CreateFromItems( items, board.get(), &members ) );

    BOOST_REQUIRE( stack );
    BOOST_CHECK_EQUAL( members.size(), 3u );
    BOOST_CHECK_EQUAL( std::count( members.begin(), members.end(), (BOARD_ITEM*) connector ), 1 );

    for( PCB_TRACK* stray : { offLanding, otherNet, loose, zeroLength } )
    {
        BOOST_CHECK_MESSAGE( std::count( members.begin(), members.end(), (BOARD_ITEM*) stray ) == 0,
                             "a trace that does not link two hops joined the stack" );
    }
}


// Taking a via a generator already owns would leave both rebuilding the same copper.
BOOST_AUTO_TEST_CASE( CreateFromItemsRejectsViasOwnedByAGenerator )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );

    PCB_VIA_STACK* owner = new PCB_VIA_STACK( board.get(), F_Cu );
    owner->SetStartLayer( F_Cu );
    owner->SetEndLayer( In2_Cu );
    owner->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    owner->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    owner->SetPosition( pos );
    board->Add( owner );
    owner->Regenerate( board.get(), nullptr );

    std::vector<BOARD_ITEM*> items;

    for( BOARD_ITEM* item : owner->GetBoardItems() )
        items.push_back( item );

    BOOST_REQUIRE_EQUAL( items.size(), 2u );

    BOOST_CHECK( PCB_VIA_STACK::CreateFromItems( items, board.get() ) == nullptr );
}


// A router style microvia spanning several hops expands into a stack of single hop vias.
BOOST_AUTO_TEST_CASE( ExpandMultiHopMicrovia )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "N1" ), 1 );
    board->Add( net );

    VECTOR2I pos( pcbIUScale.mmToIU( 7 ), pcbIUScale.mmToIU( 7 ) );
    makeMicrovia( board.get(), pos, F_Cu, In2_Cu, 1 );

    int expanded = PCB_VIA_STACK::ExpandMultiHopMicrovias( board.get(), nullptr );

    BOOST_CHECK_EQUAL( expanded, 1 );
    BOOST_REQUIRE_EQUAL( board->Generators().size(), 1u );

    PCB_VIA_STACK* stack = dynamic_cast<PCB_VIA_STACK*>( board->Generators().front() );
    BOOST_REQUIRE( stack );
    BOOST_CHECK_EQUAL( stack->GetNetCode(), 1 );

    int hops = 0;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );
        BOOST_REQUIRE( via );
        BOOST_CHECK( via->GetPosition() == pos );
        BOOST_CHECK_EQUAL( via->GetNetCode(), 1 );
        hops++;
    }

    BOOST_CHECK_EQUAL( hops, 2 );

    // The original multi hop via must be gone.
    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            BOOST_CHECK( via->GetParentGroup() != nullptr );
        }
    }

    // A second run finds nothing.
    BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( board.get(), nullptr ), 0 );
}


// The router expands through a commit so the route and its stacks undo as one step.
BOOST_AUTO_TEST_CASE( ExpansionThroughACommitIsUndoable )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    mgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    board.Add( new NETINFO_ITEM( &board, wxT( "N1" ), 1 ) );

    VECTOR2I pos( pcbIUScale.mmToIU( 7 ), pcbIUScale.mmToIU( 7 ) );
    makeMicrovia( &board, pos, F_Cu, In2_Cu, 1 );

    auto countVias = [&]() -> size_t
    {
        size_t n = 0;

        for( PCB_TRACK* track : board.Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
                n++;
        }

        return n;
    };

    BOOST_REQUIRE_EQUAL( countVias(), 1u );

    {
        BOARD_COMMIT reverted( &mgr, true, false );

        BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( &board, &reverted ), 1 );
        reverted.Revert();

        BOOST_CHECK_EQUAL( board.Generators().size(), 0u );
        BOOST_REQUIRE_EQUAL( countVias(), 1u );

        PCB_VIA* restored = dynamic_cast<PCB_VIA*>( board.Tracks().front() );

        BOOST_REQUIRE( restored );
        BOOST_CHECK_EQUAL( (int) restored->TopLayer(), (int) F_Cu );
        BOOST_CHECK_EQUAL( (int) restored->BottomLayer(), (int) In2_Cu );
        BOOST_CHECK( restored->GetParentGroup() == nullptr );
    }

    BOARD_COMMIT commit( &mgr, true, false );

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( &board, &commit ), 1 );
    commit.Push( wxT( "Expand Microvia Stacks" ) );

    BOOST_REQUIRE_EQUAL( board.Generators().size(), 1u );
    BOOST_CHECK_EQUAL( countVias(), 2u );

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            BOOST_CHECK( track->GetParentGroup() != nullptr );
    }
}


// Connectivity spike: two coaxial microvia hops share the inner landing layer at one point.
// The connectivity engine must treat them as one net, otherwise a stacked stack would be
// electrically broken. This is the load-bearing assumption of the whole feature.
BOOST_AUTO_TEST_CASE( CoaxialHopsConnect )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "N1" ), 1 );
    board->Add( net );

    VECTOR2I origin( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.30 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( origin );
    stack->SetNetCode( net->GetNetCode() );
    board->Add( stack );

    stack->Regenerate( board.get(), nullptr );
    board->BuildConnectivity();

    std::vector<PCB_VIA*> vias;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            vias.push_back( static_cast<PCB_VIA*>( item ) );
    }

    BOOST_REQUIRE_EQUAL( vias.size(), 2u );

    std::vector<BOARD_CONNECTED_ITEM*> connected = board->GetConnectivity()->GetConnectedItems( vias[0] );

    bool otherFound = std::find( connected.begin(), connected.end(), static_cast<BOARD_CONNECTED_ITEM*>( vias[1] ) )
                      != connected.end();

    BOOST_CHECK_MESSAGE( otherFound, "Coaxial microvia hops sharing the inner layer must be connected." );
}


// Mirror the placement tool's commit sequence and verify the board state selection
// depends on: members on the board exactly once, parented to the stack, promotable.
BOOST_AUTO_TEST_CASE( PlacedStackCommitState )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) );

    PCB_VIA_STACK settings( &board, F_Cu );
    settings.SetStartLayer( F_Cu );
    settings.SetEndLayer( In2_Cu );
    settings.SetStyle( VIA_STACK_STYLE::STACKED );
    settings.SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    settings.SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    settings.SetPosition( pos );

    BOARD_COMMIT commit( &mgr, true, false );

    PCB_VIA_STACK* stack = static_cast<PCB_VIA_STACK*>( settings.Clone() );
    stack->SetUuidDirect( KIID() );
    stack->SetParent( &board );
    stack->SetNetCode( 0 );
    stack->SetFlags( IS_NEW );

    stack->EditStart( nullptr, &board, &commit );
    stack->Update( nullptr, &board, &commit );
    stack->EditFinish( nullptr, &board, &commit );
    stack->ClearFlags( IS_NEW );

    commit.Push( wxT( "Place Microvia Stack" ) );

    BOOST_REQUIRE_EQUAL( board.Generators().size(), 1u );
    BOOST_CHECK_EQUAL( board.Generators().front(), static_cast<PCB_GENERATOR*>( stack ) );

    // Exactly two vias on the board, no duplicates.
    std::vector<PCB_VIA*> boardVias;

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            boardVias.push_back( static_cast<PCB_VIA*>( track ) );
    }

    BOOST_REQUIRE_EQUAL( boardVias.size(), 2u );
    BOOST_CHECK_EQUAL( stack->GetItems().size(), 2u );

    for( PCB_VIA* via : boardVias )
    {
        // Parented to the stack and promotable to it, which is how selection works.
        BOOST_CHECK_EQUAL( via->GetParentGroup(), static_cast<EDA_GROUP*>( stack ) );

        EDA_GROUP* top = PCB_GROUP::TopLevelGroup( via, nullptr, false );
        BOOST_CHECK_EQUAL( top, static_cast<EDA_GROUP*>( stack ) );

        // No leftover edit flags that would make items invisible to hit testing.
        BOOST_CHECK( !via->HasFlag( IS_NEW ) );
        BOOST_CHECK( !via->HasFlag( IN_EDIT ) );
    }

    BOOST_CHECK( !stack->HasFlag( IN_EDIT ) );
    BOOST_CHECK( !stack->HasFlag( IS_NEW ) );

    // Selection heuristics and the view index need a real bounding box covering the
    // members, the PCB_GENERATOR base returns an empty one.
    BOOST_CHECK( stack->GetBoundingBox().Contains( pos ) );
    BOOST_CHECK( stack->ViewBBox().Contains( pos ) );

    // Generator members are vetoed by PCB_SELECTION_TOOL::Selectable, so the generator
    // itself must be hit testable or nothing about the stack can be clicked.
    BOOST_CHECK( stack->HitTest( pos, 0 ) );

    BOX2I around( pos - VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ),
                  VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ) ) );
    BOOST_CHECK( stack->HitTest( around, false, 0 ) );
    BOOST_CHECK( stack->HitTest( around, true, 0 ) );

    // Box select and select-all query the view by layer, and LAYER_ANCHOR alone is
    // skipped there. The stack must also register on a real layer.
    std::vector<int> viewLayers = stack->ViewGetLayers();
    BOOST_CHECK( std::find( viewLayers.begin(), viewLayers.end(), (int) stack->GetLayer() ) != viewLayers.end() );
}


// Every connecting trace of a staggered stack must be a member of the stack, or the
// router locks only some of them and the rest float free.
BOOST_AUTO_TEST_CASE( StaggeredAllTracesAreMembers )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    // F.Cu -> In3 spans 4 copper layers = 3 hops = 3 via positions and 2 connecting traces.
    SHAPE_LINE_CHAIN hops;
    hops.Append( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    hops.Append( VECTOR2I( pcbIUScale.mmToIU( 10.5 ), pcbIUScale.mmToIU( 10 ) ) );
    hops.Append( VECTOR2I( pcbIUScale.mmToIU( 11 ), pcbIUScale.mmToIU( 10 ) ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In3_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.5 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetHops( hops );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    int vias = 0;
    int traces = 0;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            vias++;
        else if( item->Type() == PCB_TRACE_T )
            traces++;

        // The point of the test: EVERY member is parented to the stack.
        BOOST_CHECK_EQUAL( item->GetParentGroup(), static_cast<EDA_GROUP*>( stack ) );
    }

    BOOST_CHECK_EQUAL( vias, 3 );
    BOOST_CHECK_EQUAL( traces, 2 );

    // And on the board, no connecting trace is left ungrouped.
    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
            BOOST_CHECK_EQUAL( track->GetParentGroup(), static_cast<EDA_GROUP*>( stack ) );
    }
}


// A placed stack must inherit the net of the copper it lands on.
BOOST_AUTO_TEST_CASE( NetInheritedFromLandingCopper )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* netA = new NETINFO_ITEM( board.get(), wxT( "A" ), 1 );
    board->Add( netA );
    NETINFO_ITEM* netB = new NETINFO_ITEM( board.get(), wxT( "B" ), 2 );
    board->Add( netB );

    VECTOR2I padPos( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );
    VECTOR2I trackPos( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) );
    VECTOR2I emptyPos( pcbIUScale.mmToIU( 40 ), pcbIUScale.mmToIU( 40 ) );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetPosition( padPos );
    board->Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetPosition( padPos );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetNetCode( 1 );
    fp->Add( pad );

    PCB_TRACK* track = new PCB_TRACK( board.get() );
    track->SetStart( trackPos - VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetEnd( trackPos + VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
    track->SetLayer( F_Cu );
    track->SetNetCode( 2 );
    board->Add( track );

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), padPos, F_Cu ), 1 );
    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), trackPos, F_Cu ), 2 );
    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), emptyPos, F_Cu ), 0 );

    // The inherited net propagates to every hop on regeneration.
    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( padPos );
    stack->SetNetCode( PCB_VIA_STACK::FindNetAtPosition( board.get(), padPos, F_Cu ) );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( item );
        BOOST_REQUIRE( via );
        BOOST_CHECK_EQUAL( via->GetNetCode(), 1 );
    }

    // SetNetCode re-nets every existing member, so a stack can be re-netted as a unit.
    stack->SetNetCode( 2 );

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            BOOST_CHECK_EQUAL( bci->GetNetCode(), 2 );
    }

    BOOST_CHECK_EQUAL( stack->GetNetCode(), 2 );
}


// Placement snaps to copper on any layer the stack spans, so the net has to be looked for
// there too and not just on the start layer.
BOOST_AUTO_TEST_CASE( NetInheritedFromCopperOnAnInnerLayer )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    board->Add( new NETINFO_ITEM( board.get(), wxT( "inner" ), 1 ) );

    VECTOR2I landing( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    PCB_TRACK* track = new PCB_TRACK( board.get() );
    track->SetStart( landing - VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetEnd( landing + VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
    track->SetLayer( In1_Cu );
    track->SetNetCode( 1 );
    board->Add( track );

    LSET span( { F_Cu, In1_Cu, In2_Cu } );

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), landing, F_Cu ), 0 );
    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), landing, span ), 1 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( landing );
    stack->SetNetCode( PCB_VIA_STACK::FindNetAtPosition( board.get(), landing, span ) );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_CHECK_EQUAL( stack->GetNetCode(), 1 );

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            BOOST_CHECK_EQUAL( bci->GetNetCode(), 1 );
    }
}


BOOST_AUTO_TEST_CASE( NetLookupIgnoresMechanicalHoles )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    board->Add( new NETINFO_ITEM( board.get(), wxT( "sig" ), 1 ) );

    VECTOR2I holePos( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    PCB_TRACK* track = new PCB_TRACK( board.get() );
    track->SetStart( holePos - VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetEnd( holePos + VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
    track->SetLayer( F_Cu );
    track->SetNetCode( 1 );
    board->Add( track );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetPosition( holePos );
    board->Add( fp );

    PAD* hole = new PAD( fp );
    hole->SetPosition( holePos );
    hole->SetAttribute( PAD_ATTRIB::NPTH );
    hole->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    hole->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ) ) );
    hole->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
    hole->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    hole->SetLayerSet( LSET::AllCuMask( 4 ) );
    fp->Add( hole );

    BOOST_REQUIRE_EQUAL( hole->GetNetCode(), 0 );

    // The hole sits over the track. It must not shadow it.
    BOOST_CHECK_EQUAL( PCB_VIA_STACK::FindNetAtPosition( board.get(), holePos, F_Cu ), 1 );
}


// Minimal guide so the real GENERAL_COLLECTOR can run headlessly.
class STUB_COLLECTORS_GUIDE : public COLLECTORS_GUIDE
{
public:
    bool         IsLayerVisible( PCB_LAYER_ID ) const override { return true; }
    PCB_LAYER_ID GetPreferredLayer() const override { return F_Cu; }
    bool         IgnoreLockedItems() const override { return false; }
    bool         IncludeSecondary() const override { return true; }
    bool         IgnoreFPTextOnBack() const override { return false; }
    bool         IgnoreFPTextOnFront() const override { return false; }
    bool         IgnoreFootprintsOnBack() const override { return false; }
    bool         IgnoreFootprintsOnFront() const override { return false; }
    bool         IgnorePadsOnBack() const override { return false; }
    bool         IgnorePadsOnFront() const override { return false; }
    bool         IgnoreThroughHolePads() const override { return false; }
    bool         IgnoreFPValues() const override { return false; }
    bool         IgnoreFPReferences() const override { return false; }
    bool         IgnoreThroughVias() const override { return false; }
    bool         IgnoreBlindBuriedVias() const override { return false; }
    bool         IgnoreMicroVias() const override { return false; }
    bool         IgnoreTracks() const override { return false; }
    bool         IgnoreZoneFills() const override { return true; }
    bool         IgnoreNoNets() const override { return false; }
    int          Accuracy() const override { return 0; }
    double       OnePixelInIU() const override { return 1.0; }
};


// Run the real click collection over a placed stack, a click on the stack position must
// collect the member vias so selection can promote them to the stack.
BOOST_AUTO_TEST_CASE( PlacedStackClickCollection )
{
    auto board = std::make_unique<BOARD>();
    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 15 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( pos );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    STUB_COLLECTORS_GUIDE guide;
    GENERAL_COLLECTOR     collector;

    collector.Collect( board.get(), GENERAL_COLLECTOR::AllBoardItems, pos, guide );

    int viasCollected = 0;

    for( int i = 0; i < collector.GetCount(); i++ )
    {
        if( collector[i]->Type() == PCB_VIA_T )
            viasCollected++;
    }

    BOOST_CHECK_MESSAGE( viasCollected == 2, "Expected both member vias in the click collection, got "
                                                     << viasCollected << " of " << collector.GetCount()
                                                     << " collected items" );

    // And the promotion target resolves to the stack.
    for( int i = 0; i < collector.GetCount(); i++ )
    {
        if( collector[i]->Type() == PCB_VIA_T )
        {
            BOOST_CHECK_EQUAL( PCB_GROUP::TopLevelGroup( collector[i], nullptr, false ),
                               static_cast<EDA_GROUP*>( stack ) );
        }
    }
}


// Reverting an edit must put the stack back exactly as it was: the old member vias back on
// the board, still bound to the stack. swapData() exchanges the group's member set with the
// undo image, so unless the members' back-pointers are repaired afterwards they still name
// the image, and the restored stack is a group of vias that do not agree they are in it.
BOOST_AUTO_TEST_CASE( RevertEditRestoresMemberGrouping )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    mgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    // The edit below deepens the stack to In3.Cu, so the board has to be able to hold it.
    board.SetCopperLayerCount( 6 );
    board.SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 12 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( pos );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 2u );

    // Deepen the stack by one hop, then throw the edit away.
    BOARD_COMMIT commit( &mgr, true, false );

    commit.Modify( stack );
    stack->SetEndLayer( In3_Cu );
    stack->Regenerate( &board, &commit );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 3u );

    commit.Revert();

    BOOST_CHECK_EQUAL( stack->GetEndLayer(), In2_Cu );
    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 2u );

    std::vector<PCB_VIA*> boardVias;

    for( PCB_TRACK* track : board.Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            boardVias.push_back( static_cast<PCB_VIA*>( track ) );
    }

    BOOST_CHECK_MESSAGE( boardVias.size() == 2,
                         "Revert left " << boardVias.size() << " vias on the board, expected 2" );

    // The crux: every restored member must point back at the stack. A stale back-pointer
    // makes the via unselectable as part of the stack, unlocked in the router, and orphaned
    // when the stack is deleted.
    for( PCB_VIA* via : boardVias )
    {
        BOOST_CHECK_MESSAGE( via->GetParentGroup() == static_cast<EDA_GROUP*>( stack ),
                             "Member via at " << via->GetPosition().x << "," << via->GetPosition().y
                                              << " lost its back-pointer to the stack after revert" );
    }

    // And the group's own view of its members must agree with the board's.
    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        BOOST_CHECK_MESSAGE( std::find( board.Tracks().begin(), board.Tracks().end(), item ) != board.Tracks().end(),
                             "Stack holds a member that is not on the board after revert" );
    }
}


// A stack's members sit on every layer it spans, so its own layer cannot be taken from a
// member: the loader picks one out of an unordered set, and which one it gets varies between
// loads of the same file. The stack's layer is the start layer, saved and restored as such.
BOOST_AUTO_TEST_CASE( LoadedStackKeepsItsOwnLayer )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) );

    // Deliberately not F_Cu, and spanning three layers so members exist on In1/In2/In3.
    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( In1_Cu );
    stack->SetEndLayer( In3_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( pos );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetLayer(), In1_Cu );
    BOOST_REQUIRE( stack->GetItems().size() > 1 );

    // The layer must survive a properties round trip, which is what save/load does.
    PCB_VIA_STACK reloaded( nullptr, F_Cu );
    reloaded.SetProperties( stack->GetProperties() );

    BOOST_CHECK_MESSAGE( reloaded.GetStartLayer() == In1_Cu, "Start layer lost in the properties round trip" );
    BOOST_CHECK_MESSAGE( reloaded.GetLayer() == In1_Cu, "Stack layer " << reloaded.GetLayer()
                                                                       << " does not track the start layer " << In1_Cu
                                                                       << " after a properties round trip" );

    // ViewGetLayers drives box select and the view index; it must name the real layer.
    std::vector<int> viewLayers = reloaded.ViewGetLayers();
    BOOST_CHECK( std::find( viewLayers.begin(), viewLayers.end(), (int) In1_Cu ) != viewLayers.end() );
}


// A stack whose members are gone has nothing left to describe: it reloads with no size, so it
// cannot be seen or picked, and with no net, so anything that rebuilds it emits unconnected
// vias. Shrinking the board out from under a stack's span produces exactly that, because
// BuildMembers() declines an impossible span after Regenerate() has already dropped the old
// members. Such a stack must not reach the file at all.
BOOST_AUTO_TEST_CASE( MemberlessStackIsNotSaved )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "MICROVIA_NET" ), 1 );
    board->Add( net );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    stack->SetNetCode( net->GetNetCode() );
    board->Add( stack );

    // Never regenerated, so it holds nothing. A file, a script or the API can carry one.
    BOOST_REQUIRE_MESSAGE( stack->GetItems().empty(), "Precondition failed: the stack has members, so this test is "
                                                      "not exercising a ghost" );

    auto path = std::filesystem::temp_directory_path() / "via_stack_ghost_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board, path.string() );

    std::unique_ptr<BOARD> reloaded = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_CHECK_MESSAGE( reloaded->Generators().empty(),
                         "A member-less via stack was written to the board file; it reloads "
                         "invisible, unpickable and on no net" );
}


// The ordinary case must keep working: a stack that still has members is saved, and its net
// comes back with it.
BOOST_AUTO_TEST_CASE( PopulatedStackKeepsItsNetAcrossReload )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 4 );
    board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "MICROVIA_NET" ), 1 );
    board->Add( net );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    stack->SetNetCode( net->GetNetCode() );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 2u );

    auto path = std::filesystem::temp_directory_path() / "via_stack_netkeep_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board, path.string() );

    std::unique_ptr<BOARD> reloaded = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_REQUIRE_EQUAL( reloaded->Generators().size(), 1u );

    PCB_VIA_STACK* loaded = dynamic_cast<PCB_VIA_STACK*>( reloaded->Generators().front() );
    BOOST_REQUIRE( loaded );

    NETINFO_ITEM* loadedNet = reloaded->FindNet( wxT( "MICROVIA_NET" ) );
    BOOST_REQUIRE( loadedNet );

    BOOST_CHECK_MESSAGE( loaded->GetNetCode() == loadedNet->GetNetCode(),
                         "Reloaded stack reports net " << loaded->GetNetCode() << ", expected "
                                                       << loadedNet->GetNetCode() );
}


// The Properties panel is built from the property manager; a type that declares nothing shows
// an empty panel. The rows report only: the panel cannot rebuild a stack, so editing one of
// these would change the settings and leave the drawn vias behind.
BOOST_AUTO_TEST_CASE( StackPropertiesAreRegisteredAndReadOnly )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    for( const wxString& name : { wxT( "Start Layer" ), wxT( "End Layer" ), wxT( "Style" ), wxT( "Net" ),
                                  wxT( "Pitch" ), wxT( "Via Diameter" ), wxT( "Via Hole" ), wxT( "Copper-filled" ),
                                  wxT( "Capped" ), wxT( "Use Netclass Values" ) } )
    {
        PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( PCB_VIA_STACK ), name );

        BOOST_REQUIRE_MESSAGE( prop, "Property \"" << name
                                                   << "\" is missing, so the Properties "
                                                      "panel cannot show it" );
        BOOST_CHECK_MESSAGE( !prop->Writeable( nullptr ), "Property \"" << name
                                                                        << "\" is editable, but editing it cannot "
                                                                           "rebuild the stack" );
    }

    // A stack spans layers; the single inherited layer would be misleading.
    BOOST_CHECK_MESSAGE( propMgr.GetProperty( TYPE_HASH( PCB_VIA_STACK ), wxT( "Layer" ) ) == nullptr,
                         "The inherited \"Layer\" property should be masked for a via stack" );
}


BOOST_AUTO_TEST_CASE( CollectExpandableMicroviasNamesOnlyLooseMultiHopVias )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    auto addVia = [&]( const VECTOR2I& aPos, PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom, VIATYPE aType )
    {
        PCB_VIA* via = new PCB_VIA( board.get() );
        via->SetViaType( aType );
        via->SetLayerPair( aTop, aBottom );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetPosition( aPos );
        board->Add( via );

        return via;
    };

    PCB_VIA* multiHop =
            addVia( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In2_Cu, VIATYPE::MICROVIA );

    addVia( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In1_Cu, VIATYPE::MICROVIA );
    addVia( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In2_Cu, VIATYPE::BURIED );

    // Already owned by a stack, so not loose.
    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    board->Add( stack );
    PCB_VIA* owned =
            addVia( VECTOR2I( pcbIUScale.mmToIU( 40 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In2_Cu, VIATYPE::MICROVIA );
    stack->AddItem( owned );

    std::set<KIID> ids = PCB_VIA_STACK::CollectExpandableMicrovias( board.get() );

    BOOST_CHECK_MESSAGE( ids.count( multiHop->m_Uuid ) == 1,
                         "A loose multi-hop microvia must be reported as expandable" );
    BOOST_CHECK_MESSAGE( ids.count( owned->m_Uuid ) == 0, "A via already in a stack is not expandable" );
    BOOST_CHECK_EQUAL( ids.size(), 1u );
}


static const VECTOR2I PRE_ROUTE_POS( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) );
static const VECTOR2I ROUTED_POS( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );


// Two loose multi-hop microvias sharing a net and layer pair: one already on the board, one
// standing in for the via a route just placed. Expansion deletes the vias it takes, so tests
// identify them by position afterwards rather than by pointer.
static void makeSharedNetMultiHopPair( BOARD* aBoard )
{
    aBoard->SetCopperLayerCount( 6 );
    aBoard->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( aBoard, wxT( "SHARED" ), 1 );
    aBoard->Add( net );

    for( const VECTOR2I& pos : { PRE_ROUTE_POS, ROUTED_POS } )
    {
        PCB_VIA* via = new PCB_VIA( aBoard );
        via->SetViaType( VIATYPE::MICROVIA );
        via->SetLayerPair( F_Cu, In2_Cu );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetPosition( pos );
        via->SetNetCode( net->GetNetCode() );
        aBoard->Add( via );
    }
}


static std::vector<VECTOR2I> looseMicroviaPositions( BOARD* aBoard )
{
    std::vector<VECTOR2I> positions;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && !track->GetParentGroup()
            && static_cast<PCB_VIA*>( track )->GetViaType() == VIATYPE::MICROVIA )
        {
            positions.push_back( track->GetPosition() );
        }
    }

    return positions;
}


// Matching on net and layer pair alone, as the router did, reaches across the whole board.
// This is what the snapshot guard exists to prevent.
BOOST_AUTO_TEST_CASE( ExpansionWithoutAGuardTakesEveryMatchingVia )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    makeSharedNetMultiHopPair( board.get() );

    VIA_STACK_PRESET preset;

    auto unguarded = [&]( PCB_VIA* aVia ) -> const VIA_STACK_PRESET*
    {
        return aVia->GetNetCode() == 1 ? &preset : nullptr;
    };

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( board.get(), nullptr, unguarded ), 2 );
    BOOST_CHECK_EQUAL( board->Generators().size(), 2u );
}


// With the snapshot taken before routing, only the via the route created is expanded.
BOOST_AUTO_TEST_CASE( ExpansionLeavesPreRouteViasAlone )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "SHARED" ), 1 );
    board->Add( net );

    auto addMultiHop = [&]( const VECTOR2I& aPos )
    {
        PCB_VIA* via = new PCB_VIA( board.get() );
        via->SetViaType( VIATYPE::MICROVIA );
        via->SetLayerPair( F_Cu, In2_Cu );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetPosition( aPos );
        via->SetNetCode( net->GetNetCode() );
        board->Add( via );
    };

    addMultiHop( PRE_ROUTE_POS );

    std::set<KIID> beforeRouting = PCB_VIA_STACK::CollectExpandableMicrovias( board.get() );
    BOOST_REQUIRE_EQUAL( beforeRouting.size(), 1u );

    addMultiHop( ROUTED_POS );

    VIA_STACK_PRESET preset;

    std::vector<PENDING_STACK_EXPANSION> pending = { { F_Cu, In2_Cu, net->GetNetCode(), preset } };

    auto guarded = [&]( PCB_VIA* aVia ) -> const VIA_STACK_PRESET*
    {
        return MatchPendingStackExpansion( aVia, beforeRouting, pending );
    };

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( board.get(), nullptr, guarded ), 1 );

    BOOST_REQUIRE_EQUAL( board->Generators().size(), 1u );
    BOOST_CHECK_MESSAGE( board->Generators().front()->GetPosition() == ROUTED_POS,
                         "The stack was built at the wrong via" );

    std::vector<VECTOR2I> loose = looseMicroviaPositions( board.get() );

    BOOST_REQUIRE_MESSAGE( loose.size() == 1,
                           "Expected the pre-existing via to be left loose, found " << loose.size() );
    BOOST_CHECK_MESSAGE( loose.front() == PRE_ROUTE_POS, "A via that predates the route was swallowed into a stack" );
}


BOOST_AUTO_TEST_CASE( MovingAStackReusesItsMembers )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    VECTOR2I origin( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In4_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.5 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( origin );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    auto memberIds = [&]()
    {
        std::set<KIID> ids;

        for( BOARD_ITEM* item : stack->GetBoardItems() )
            ids.insert( item->m_Uuid );

        return ids;
    };

    auto countVias = [&]() -> size_t
    {
        size_t n = 0;

        for( PCB_TRACK* track : board->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
                n++;
        }

        return n;
    };

    const std::set<KIID> before = memberIds();
    const size_t         viasBefore = countVias();

    BOOST_REQUIRE( before.size() > 1 );
    BOOST_REQUIRE( viasBefore > 1 );

    for( int step = 0; step < 5; ++step )
    {
        stack->Move( VECTOR2I( pcbIUScale.mmToIU( 0.2 ), 0 ) );
        stack->Regenerate( board.get(), nullptr );
    }

    BOOST_CHECK_MESSAGE( memberIds() == before, "Moving the stack replaced its members instead of moving them, so a "
                                                "drag would stage one generation of vias per motion event" );

    BOOST_CHECK_MESSAGE( countVias() == viasBefore, "The board gained or lost vias while the stack was moved: "
                                                            << countVias() << " now, " << viasBefore << " before" );
}


// Pinned against a committed board file rather than against the writer, so a renamed token or
// a changed unit is caught instead of round-tripping quietly. Regenerate this fixture only
// alongside a deliberate file format change.
BOOST_AUTO_TEST_CASE( GoldenFileStillLoads )
{
    std::string path = KI_TEST::GetPcbnewTestDataDir() + "via_stacks.kicad_pcb";

    std::unique_ptr<BOARD> board = ::KI_TEST::ReadBoardFromFileOrStream( path );
    BOOST_REQUIRE( board );

    BOOST_REQUIRE_EQUAL( board->Generators().size(), 2u );

    PCB_VIA_STACK* stacked = nullptr;
    PCB_VIA_STACK* staggered = nullptr;

    for( PCB_GENERATOR* gen : board->Generators() )
    {
        PCB_VIA_STACK* stack = dynamic_cast<PCB_VIA_STACK*>( gen );
        BOOST_REQUIRE( stack );

        if( stack->GetStyle() == VIA_STACK_STYLE::STACKED )
            stacked = stack;
        else
            staggered = stack;
    }

    BOOST_REQUIRE( stacked );
    BOOST_REQUIRE( staggered );

    BOOST_CHECK_EQUAL( (int) stacked->GetStartLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( (int) stacked->GetEndLayer(), (int) In2_Cu );
    BOOST_CHECK_EQUAL( (int) stacked->GetLayer(), (int) F_Cu );
    BOOST_CHECK_EQUAL( stacked->GetViaSize(), pcbIUScale.mmToIU( 0.25 ) );
    BOOST_CHECK_EQUAL( stacked->GetViaDrill(), pcbIUScale.mmToIU( 0.10 ) );
    BOOST_CHECK_EQUAL( stacked->IsFilled(), true );
    BOOST_CHECK_EQUAL( stacked->IsCapped(), true );
    BOOST_CHECK_EQUAL( stacked->GetUseNetclass(), false );
    BOOST_CHECK_EQUAL( stacked->GetPresetName(), wxS( "HDI 1+N+1" ) );
    BOOST_CHECK_EQUAL( stacked->GetItems().size(), 2u );
    BOOST_CHECK( stacked->GetPosition() == VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) );

    BOOST_CHECK_EQUAL( (int) staggered->GetStartLayer(), (int) In1_Cu );
    BOOST_CHECK_EQUAL( (int) staggered->GetEndLayer(), (int) In3_Cu );
    BOOST_CHECK_EQUAL( (int) staggered->GetLayer(), (int) In1_Cu );
    BOOST_CHECK_EQUAL( staggered->GetPitch(), pcbIUScale.mmToIU( 0.45 ) );
    BOOST_CHECK_EQUAL( staggered->IsFilled(), false );
    BOOST_CHECK_EQUAL( staggered->GetPresetName(), wxS( "Staggered 0.45" ) );

    // One hop position per microvia, so an In1.Cu to In3.Cu span carries two.
    BOOST_REQUIRE( staggered->GetHops().has_value() );
    BOOST_CHECK_EQUAL( staggered->GetHops()->PointCount(), 2 );
    BOOST_CHECK( staggered->GetHops()->CPoint( 1 ) == VECTOR2I( pcbIUScale.mmToIU( 40.45 ), pcbIUScale.mmToIU( 20 ) ) );

    // The members must come back attached, not as loose vias.
    for( BOARD_ITEM* item : stacked->GetBoardItems() )
        BOOST_CHECK( item->GetParentGroup() == static_cast<EDA_GROUP*>( stacked ) );

    // Only a hop reaching an outer layer is capped, so the fixture must say what a regenerate
    // would produce.
    for( BOARD_ITEM* item : stacked->GetBoardItems() )
    {
        PCB_VIA* hop = static_cast<PCB_VIA*>( item );
        bool     outer = IsExternalCopperLayer( hop->TopLayer() ) || IsExternalCopperLayer( hop->BottomLayer() );

        BOOST_CHECK_MESSAGE( hop->Padstack().Drill().is_capped.value_or( false ) == outer,
                             "hop " << LSET::Name( hop->TopLayer() ) << "-" << LSET::Name( hop->BottomLayer() )
                                    << " capping disagrees with the generator" );
    }
}


// Deleting a stack takes its members with it, and reverting the same staging puts them back.
BOOST_AUTO_TEST_CASE( DeletingAStackRemovesItsMembersAndRevertRestoresThem )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    mgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    auto countVias = [&]() -> size_t
    {
        size_t n = 0;

        for( PCB_TRACK* track : board.Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
                n++;
        }

        return n;
    };

    const size_t before = countVias();
    BOOST_REQUIRE_EQUAL( before, 2u );

    {
        BOARD_COMMIT reverted( &mgr, true, false );

        stack->Remove( nullptr, &board, &reverted );
        reverted.Revert();

        BOOST_CHECK_MESSAGE( countVias() == before,
                             "Reverting the delete left " << countVias() << " vias, expected " << before );
        BOOST_CHECK_EQUAL( board.Generators().size(), 1u );
    }

    BOARD_COMMIT commit( &mgr, true, false );

    stack->Remove( nullptr, &board, &commit );
    commit.Push( wxT( "Delete Microvia Stack" ) );

    BOOST_CHECK_EQUAL( countVias(), 0u );
    BOOST_CHECK_EQUAL( board.Generators().size(), 0u );
}


// The preset list is project-file data with a hand-written reader, and no test covered it.
BOOST_AUTO_TEST_CASE( ViaStackPresetsRoundTripThroughSettings )
{
    BOARD_DESIGN_SETTINGS bds( nullptr, "board.design_settings" );

    VIA_STACK_PRESET preset;
    preset.m_Name = wxS( "HDI 1+N+1" );
    preset.m_StartLayer = F_Cu;
    preset.m_EndLayer = In2_Cu;
    preset.m_Staggered = true;
    preset.m_ViaSize = pcbIUScale.mmToIU( 0.25 );
    preset.m_ViaDrill = pcbIUScale.mmToIU( 0.10 );
    preset.m_UseNetclass = true;
    preset.m_Filled = false;
    preset.m_Capped = true;
    preset.m_Pitch = pcbIUScale.mmToIU( 0.45 );

    bds.m_ViaStackPresets.push_back( preset );
    bds.Store();

    bds.m_ViaStackPresets.clear();
    bds.Load();

    BOOST_REQUIRE_EQUAL( bds.m_ViaStackPresets.size(), 1u );
    BOOST_CHECK( bds.m_ViaStackPresets[0] == preset );

    // Layers are stored by canonical name, not by layer id.
    std::optional<nlohmann::json> stored = bds.Get<nlohmann::json>( "via_stack_presets" );

    BOOST_REQUIRE( stored.has_value() );
    BOOST_REQUIRE( stored->is_array() && stored->size() == 1 );
    BOOST_CHECK_EQUAL( ( *stored )[0]["start_layer"].get<std::string>(), "F.Cu" );
    BOOST_CHECK_EQUAL( ( *stored )[0]["end_layer"].get<std::string>(), "In2.Cu" );
}


// The reader sanitises hand-edited project files: a bad layer id would make every LAYER_RANGE
// built from the preset run away, and an entry with no name is not a preset at all.
BOOST_AUTO_TEST_CASE( MalformedPresetEntriesAreRejected )
{
    BOARD_DESIGN_SETTINGS bds( nullptr, "board.design_settings" );

    nlohmann::json entries = nlohmann::json::array();

    entries.push_back( { { "name", "unknown layer" }, { "start_layer", "Nonsense" }, { "end_layer", "" } } );
    entries.push_back( { { "start_layer", "F.Cu" }, { "end_layer", "In1.Cu" } } ); // no name
    entries.push_back( "not an object" );
    entries.push_back( { { "name", "not copper" }, { "start_layer", "F.SilkS" }, { "end_layer", "Edge.Cuts" } } );
    entries.push_back( { { "name", "still numeric" }, { "start_layer", 0 }, { "end_layer", 4 } } );

    bds.Set( "via_stack_presets", entries );
    bds.Load();

    BOOST_REQUIRE_EQUAL( bds.m_ViaStackPresets.size(), 3u );

    for( const VIA_STACK_PRESET& p : bds.m_ViaStackPresets )
    {
        BOOST_CHECK_MESSAGE( IsCopperLayer( p.m_StartLayer ) && IsCopperLayer( p.m_EndLayer ),
                             "Preset \"" << p.m_Name << "\" kept a non-copper layer" );
    }
}


// Widening a stack's span must leave exactly the hops the new span needs.
BOOST_AUTO_TEST_CASE( EditingAnExistingStackRebuildsItsHops )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In1_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 1u );

    stack->SetEndLayer( In4_Cu );
    stack->Regenerate( board.get(), nullptr );

    std::set<std::pair<int, int>> pairs;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        BOOST_REQUIRE_EQUAL( item->Type(), PCB_VIA_T );
        PCB_VIA* via = static_cast<PCB_VIA*>( item );
        pairs.insert( { (int) via->TopLayer(), (int) via->BottomLayer() } );
    }

    BOOST_CHECK_EQUAL( pairs.size(), 4u );
    BOOST_CHECK_EQUAL( pairs.count( { (int) In3_Cu, (int) In4_Cu } ), 1u );

    size_t vias = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            vias++;
    }

    BOOST_CHECK_MESSAGE( vias == 4, "Widening the span left " << vias << " vias on the board, "
                                                              << "expected 4" );
}


// Only microvias make a stack; a selection holding any other via type is refused outright.
BOOST_AUTO_TEST_CASE( CreateFromItemsRejectsNonMicrovias )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    VECTOR2I pos( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    auto addVia = [&]( PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom, VIATYPE aType )
    {
        PCB_VIA* via = new PCB_VIA( board.get() );
        via->SetViaType( aType );
        via->SetLayerPair( aTop, aBottom );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetPosition( pos );
        board->Add( via );

        return via;
    };

    for( VIATYPE type : { VIATYPE::BLIND, VIATYPE::BURIED, VIATYPE::THROUGH } )
    {
        std::vector<BOARD_ITEM*> items = { addVia( F_Cu, In1_Cu, VIATYPE::MICROVIA ), addVia( In1_Cu, In2_Cu, type ) };

        BOOST_CHECK_MESSAGE( PCB_VIA_STACK::CreateFromItems( items, board.get() ) == nullptr,
                             "A selection containing a non-microvia was accepted as a stack" );
    }

    // Two microvias that do tile a span are accepted, so the rejection above is about the type.
    std::vector<BOARD_ITEM*> good = { addVia( In2_Cu, In3_Cu, VIATYPE::MICROVIA ),
                                      addVia( In3_Cu, In4_Cu, VIATYPE::MICROVIA ) };

    std::unique_ptr<PCB_VIA_STACK> made( PCB_VIA_STACK::CreateFromItems( good, board.get() ) );
    BOOST_CHECK( made != nullptr );
}


// Capping plates a lid over a filled via so the outer face is flat and solderable. Only a hop
// that reaches an outer layer has such a face; the rest are buried under the next build-up
// layer, and asking the fabricator to cap those describes a step it cannot perform.
BOOST_AUTO_TEST_CASE( CappingOnlyMarksTheHopOnTheOuterLayer )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In3_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetCapped( true );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 3u );

    int capped = 0;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        BOOST_CHECK_MESSAGE( via->Padstack().Drill().is_filled.value_or( false ),
                             "Every hop of a stacked stack must be filled" );

        if( via->Padstack().Drill().is_capped.value_or( false ) )
        {
            capped++;

            BOOST_CHECK_MESSAGE( IsExternalCopperLayer( via->TopLayer() )
                                         || IsExternalCopperLayer( via->BottomLayer() ),
                                 "A buried hop was marked capped" );
        }
    }

    BOOST_CHECK_MESSAGE( capped == 1, "Expected exactly the outer hop to be capped, found " << capped );
}


// A stack that never reaches an outer layer has nothing to cap.
BOOST_AUTO_TEST_CASE( CappingAnInternalStackMarksNothing )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    board->SetCopperLayerCount( 6 );
    board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( board.get(), In1_Cu );
    stack->SetStartLayer( In1_Cu );
    stack->SetEndLayer( In3_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetCapped( true );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board->Add( stack );
    stack->Regenerate( board.get(), nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 2u );

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        BOOST_CHECK_MESSAGE( !via->Padstack().Drill().is_capped.value_or( false ),
                             "A wholly internal stack has no outer face to cap" );
    }
}


// A stack draws and hit-tests on its own layer, so a preset must set it too.
BOOST_AUTO_TEST_CASE( ApplyPresetKeepsTheStackLayerInStep )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    PCB_VIA_STACK stack( &board, F_Cu );

    VIA_STACK_PRESET preset;
    preset.m_Name = wxT( "buried" );
    preset.m_StartLayer = In1_Cu;
    preset.m_EndLayer = In2_Cu;

    stack.ApplyPreset( preset );

    BOOST_CHECK_EQUAL( stack.GetStartLayer(), In1_Cu );
    BOOST_CHECK_MESSAGE( stack.GetLayer() == In1_Cu, "the stack's own layer must follow the preset's start layer" );

    std::vector<int> layers = stack.ViewGetLayers();

    BOOST_CHECK_MESSAGE( std::find( layers.begin(), layers.end(), In1_Cu ) != layers.end(),
                         "the stack must register in the view on its start layer" );
}


// Editing a stack must not rewrite its connecting traces to whatever width the toolbar shows.
BOOST_AUTO_TEST_CASE( ConnectingTraceWidthSurvivesAnEdit )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = board.GetDesignSettings();

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pcbIUScale.mmToIU( 0.6 ) );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    auto traceWidth = [&]() -> int
    {
        for( BOARD_ITEM* item : stack->GetBoardItems() )
        {
            if( item->Type() == PCB_TRACE_T )
                return static_cast<PCB_TRACK*>( item )->GetWidth();
        }

        return 0;
    };

    int placed = traceWidth();
    BOOST_REQUIRE( placed > 0 );

    // The user changes the toolbar track width, then nudges the stack's pitch.
    bds.SetCustomTrackWidth( placed * 3 );
    bds.UseCustomTrackViaSize( true );

    stack->SetPitch( pcbIUScale.mmToIU( 0.7 ) );
    stack->Regenerate( &board, nullptr );

    BOOST_CHECK_MESSAGE( traceWidth() == placed, "trace width changed from " << placed << " to " << traceWidth()
                                                                             << " without the user touching it" );
}


// Ctrl+D on a stack goes through DeepDuplicate, which must give the copy its own hops.
BOOST_AUTO_TEST_CASE( DeepDuplicateGivesTheCopyItsOwnMembers )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetBoardItems().size(), 2u );

    PCB_GROUP* copy = stack->DeepDuplicate( IGNORE_PARENT_GROUP );

    BOOST_REQUIRE( copy );
    BOOST_CHECK( dynamic_cast<PCB_VIA_STACK*>( copy ) );
    BOOST_CHECK( copy->m_Uuid != stack->m_Uuid );

    // A group holds an item only once, so copying by reference would move the vias to the
    // copy rather than alias them.
    BOOST_CHECK_EQUAL( stack->GetBoardItems().size(), 2u );
    BOOST_CHECK_EQUAL( copy->GetBoardItems().size(), 2u );

    delete copy;
}


// A preset that says "use netclass" must still do so after the router expands its via.
BOOST_AUTO_TEST_CASE( ExpansionHonoursAPresetsUseNetclass )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = board.GetDesignSettings();

    const int netclassDia = pcbIUScale.mmToIU( 0.25 );
    const int netclassDrill = pcbIUScale.mmToIU( 0.12 );

    bds.m_NetSettings->GetDefaultNetclass()->SetuViaDiameter( netclassDia );
    bds.m_NetSettings->GetDefaultNetclass()->SetuViaDrill( netclassDrill );

    // A bare test board has no project, so SynchronizeNetsAndNetClasses cannot do this.
    NETINFO_ITEM* netinfo = new NETINFO_ITEM( &board, wxT( "N1" ), 1 );
    netinfo->SetNetClass( bds.m_NetSettings->GetDefaultNetclass() );
    board.Add( netinfo );

    // A routed multi-hop microvia, deliberately not the netclass size.
    PCB_VIA* via = new PCB_VIA( &board );
    via->SetViaType( VIATYPE::MICROVIA );
    via->SetLayerPair( F_Cu, In2_Cu );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.40 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.20 ) );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    via->SetNetCode( netinfo->GetNetCode() );
    board.Add( via );

    VIA_STACK_PRESET preset;
    preset.m_Name = wxT( "netclass stack" );
    preset.m_UseNetclass = true;

    int expanded = PCB_VIA_STACK::ExpandMultiHopMicrovias( &board, nullptr,
                                                           [&]( PCB_VIA* )
                                                           {
                                                               return &preset;
                                                           } );

    BOOST_REQUIRE_EQUAL( expanded, 1 );
    BOOST_REQUIRE_EQUAL( board.Generators().size(), 1u );

    PCB_VIA_STACK* stack = dynamic_cast<PCB_VIA_STACK*>( board.Generators().front() );
    BOOST_REQUIRE( stack );
    BOOST_CHECK( stack->GetUseNetclass() );

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* hop = static_cast<PCB_VIA*>( item );

        BOOST_CHECK_MESSAGE( hop->GetWidth( PADSTACK::ALL_LAYERS ) == netclassDia,
                             "hop diameter " << hop->GetWidth( PADSTACK::ALL_LAYERS ) << " should be the netclass "
                                             << netclassDia );
        BOOST_CHECK_EQUAL( hop->GetDrillValue(), netclassDrill );
    }
}


BOOST_AUTO_TEST_CASE( AnUnbuildableSpanKeepsTheExistingHops )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    BOOST_REQUIRE_EQUAL( stack->GetBoardItems().size(), 2u );

    // The span becomes unbuildable, as it would from a hand-edited layer name or a board
    // whose copper layer count was reduced.
    stack->SetEndLayer( In6_Cu );
    stack->Regenerate( &board, nullptr );

    BOOST_CHECK_MESSAGE( stack->GetBoardItems().size() == 2u, "hops were destroyed, leaving an invisible stack: "
                                                                      << stack->GetBoardItems().size() << " members" );
}


// Widening a staggered span adds a hop, and it has to carry on from where the chain runs,
// not jump back onto the +X axis.
BOOST_AUTO_TEST_CASE( WideningAStaggeredSpanContinuesTheChain )
{
    BOARD board;
    board.SetCopperLayerCount( 6 );
    board.SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    const int pitch = pcbIUScale.mmToIU( 0.5 );
    VECTOR2I  origin( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetStyle( VIA_STACK_STYLE::STAGGERED );
    stack->SetPitch( pitch );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( origin );

    // The user steered this chain downwards, not along +X.
    SHAPE_LINE_CHAIN hops;
    hops.Append( origin );
    hops.Append( origin + VECTOR2I( 0, pitch ) );
    stack->SetHops( hops );

    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    // Deepen it by one hop.
    stack->SetEndLayer( In3_Cu );
    stack->Regenerate( &board, nullptr );

    std::vector<VECTOR2I> centres;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            centres.push_back( item->GetPosition() );
    }

    // Member order is not guaranteed, so ask where the hops are rather than which is which.
    auto has = [&]( const VECTOR2I& aPoint )
    {
        return std::find( centres.begin(), centres.end(), aPoint ) != centres.end();
    };

    BOOST_REQUIRE_EQUAL( centres.size(), 3u );

    for( const VECTOR2I& c : centres )
        BOOST_TEST_MESSAGE( "hop at " << c.x << "," << c.y );

    BOOST_CHECK_MESSAGE( has( origin + VECTOR2I( 0, 2 * pitch ) ), "the new hop did not continue the chain" );
    BOOST_CHECK_MESSAGE( !has( origin + VECTOR2I( 2 * pitch, 0 ) ), "the new hop fell back onto the +X axis" );
}


// An unset fill on the source via means "ask the board", not "filled".
BOOST_AUTO_TEST_CASE( CreateFromItemsTakesFillFromTheBoardDefault )
{
    for( bool boardFills : { false, true } )
    {
        BOARD board;
        board.SetCopperLayerCount( 4 );
        board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
        board.GetDesignSettings().m_FillVias = boardFills;

        // Offset, so this is a staggered stack. A coaxial one is forced filled whatever the
        // board says, because something lands on every hop.
        VECTOR2I at( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
        VECTOR2I next( at.x + pcbIUScale.mmToIU( 0.5 ), at.y );

        std::vector<BOARD_ITEM*> items;
        int                      hop = 0;

        for( auto pair : { std::make_pair( F_Cu, In1_Cu ), std::make_pair( In1_Cu, In2_Cu ) } )
        {
            PCB_VIA* via = new PCB_VIA( &board );
            via->SetViaType( VIATYPE::MICROVIA );
            via->SetLayerPair( pair.first, pair.second );
            via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
            via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
            via->SetPosition( hop++ == 0 ? at : next );
            board.Add( via );
            items.push_back( via );
        }

        // is_filled deliberately left unset on both.
        std::unique_ptr<PCB_VIA_STACK> stack( PCB_VIA_STACK::CreateFromItems( items, &board ) );

        BOOST_REQUIRE( stack );
        BOOST_CHECK_MESSAGE( stack->IsFilled() == boardFills,
                             "board fill " << boardFills << " gave stack fill " << stack->IsFilled() );
    }
}


// Expansion is for microvias only. A plain via on the same net and span, placed for ordinary
// routing, must be left exactly as the user drew it.
BOOST_AUTO_TEST_CASE( ExpansionLeavesPlainViasAlone )
{
    BOARD board;
    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( &board, wxT( "N" ), 1 );
    board.Add( net );

    auto addVia = [&]( VIATYPE aType, const VECTOR2I& aPos )
    {
        PCB_VIA* via = new PCB_VIA( &board );
        via->SetViaType( aType );
        via->SetLayerPair( F_Cu, In2_Cu );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
        via->SetPosition( aPos );
        via->SetNetCode( net->GetNetCode() );
        board.Add( via );
        return via;
    };

    // Same net, same span, same everything except the via type.
    KIID throughId = addVia( VIATYPE::THROUGH, VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) )->m_Uuid;
    KIID buriedId = addVia( VIATYPE::BURIED, VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) )->m_Uuid;
    KIID microId = addVia( VIATYPE::MICROVIA, VECTOR2I( pcbIUScale.mmToIU( 30 ), 0 ) )->m_Uuid;

    VIA_STACK_PRESET preset;
    preset.m_Name = wxT( "p" );

    // The router's matcher: anything on this net is fair game as far as it is concerned.
    auto matcher = [&]( PCB_VIA* aVia ) -> const VIA_STACK_PRESET*
    {
        return aVia->GetNetCode() == net->GetNetCode() ? &preset : nullptr;
    };

    BOOST_CHECK_EQUAL( PCB_VIA_STACK::ExpandMultiHopMicrovias( &board, nullptr, matcher ), 1 );

    BOOST_REQUIRE_EQUAL( board.Generators().size(), 1u );

    // Compare by uuid: the expanded microvia is deleted, so its pointer must not be touched.
    std::set<KIID> remaining;

    for( PCB_TRACK* track : board.Tracks() )
        remaining.insert( track->m_Uuid );

    BOOST_CHECK_MESSAGE( remaining.count( throughId ), "a through via was converted" );
    BOOST_CHECK_MESSAGE( remaining.count( buriedId ), "a buried via was converted" );
    BOOST_CHECK_MESSAGE( !remaining.count( microId ), "the microvia should have been expanded" );
}


BOOST_AUTO_TEST_SUITE_END()
