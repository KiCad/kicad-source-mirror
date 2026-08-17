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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include <base_units.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <generators/pcb_via_stitch.h>
#include <netinfo.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <zone.h>

#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

BOOST_AUTO_TEST_SUITE( ViaStitch )

static SHAPE_POLY_SET makeRectPoly( int aX1, int aY1, int aX2, int aY2 )
{
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append( aX1, aY1 );
    poly.Append( aX2, aY1 );
    poly.Append( aX2, aY2 );
    poly.Append( aX1, aY2 );
    return poly;
}


static void configureStitch( PCB_VIA_STITCH* aStitch )
{
    aStitch->SetOutline( makeRectPoly( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ),
                                       pcbIUScale.mmToIU( 18 ), pcbIUScale.mmToIU( 18 ) ) );
    aStitch->SetPitch( pcbIUScale.mmToIU( 2 ) );
    aStitch->SetLayout( PCB_VIA_STITCH_LAYOUT::STAGGERED );
    aStitch->SetMode( PCB_VIA_STITCH_MODE::STITCH );
    aStitch->SetSeed( 1234 );
    aStitch->ViaTemplate()->SetWidth( pcbIUScale.mmToIU( 0.6 ) );
    aStitch->ViaTemplate()->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    aStitch->SetNetCode( 1 );
}


static void checkOutlinesEqual( const SHAPE_POLY_SET& aExpected, const SHAPE_POLY_SET& aActual )
{
    BOOST_REQUIRE_EQUAL( aActual.OutlineCount(), aExpected.OutlineCount() );

    const SHAPE_LINE_CHAIN& expected = aExpected.COutline( 0 );
    const SHAPE_LINE_CHAIN& actual = aActual.COutline( 0 );

    BOOST_REQUIRE_EQUAL( actual.PointCount(), expected.PointCount() );

    for( int i = 0; i < expected.PointCount(); ++i )
        BOOST_CHECK_EQUAL( actual.CPoint( i ), expected.CPoint( i ) );
}


BOOST_AUTO_TEST_CASE( BakedPatternConstantsLocked )
{
    // Extra idiot checking that nobody touches the constants
    // Or else existing boards will change stitch patterns
    BOOST_CHECK_EQUAL( PCB_VIA_STITCH::POISSON_TILE_SEED, 0x542c21fcu );
    BOOST_CHECK_EQUAL( PCB_VIA_STITCH::POISSON_TILE_PITCHES, 8 );
    BOOST_CHECK( !PCB_VIA_STITCH::bakedPoissonTile().empty() );
}


BOOST_AUTO_TEST_CASE( PropertiesRoundTrip )
{
    BOARD board;
    board.Add( new NETINFO_ITEM( &board, wxT( "GND" ), 1 ) );

    PCB_VIA_STITCH a;
    a.SetParent( &board );
    configureStitch( &a );
    a.ExcludePosition( VECTOR2I( pcbIUScale.mmToIU( 6 ), pcbIUScale.mmToIU( 6 ) ) );

    PCB_VIA_STITCH b;
    b.SetParent( &board );
    b.SetProperties( a.GetProperties() );

    BOOST_CHECK_EQUAL( b.GetPitch(), a.GetPitch() );
    BOOST_CHECK_EQUAL( (int) b.GetLayout(), (int) a.GetLayout() );
    BOOST_CHECK_EQUAL( (int) b.GetMode(), (int) a.GetMode() );
    BOOST_CHECK_EQUAL( b.GetSeed(), a.GetSeed() );
    BOOST_CHECK_EQUAL( b.GetNetCode(), 1 );

    checkOutlinesEqual( a.Outline(), b.Outline() );

    auto aCells = a.GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );
    auto bCells = b.GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );

    BOOST_REQUIRE( aCells.has_value() );
    BOOST_REQUIRE( bCells.has_value() );
    BOOST_REQUIRE_EQUAL( bCells->size(), aCells->size() );

    for( size_t i = 0; i < aCells->size(); ++i )
        BOOST_CHECK_EQUAL( ( *bCells )[i], ( *aCells )[i] );
}


BOOST_AUTO_TEST_CASE( SexprSaveLoad )
{
    auto board = std::make_unique<BOARD>();
    board->Add( new NETINFO_ITEM( board.get(), wxT( "GND" ), 1 ) );

    ZONE* zone = new ZONE( board.get() );
    zone->SetLayer( F_Cu );
    zone->SetNetCode( 1 );
    zone->Outline()->NewOutline();
    zone->Outline()->Append( 0, 0 );
    zone->Outline()->Append( pcbIUScale.mmToIU( 1 ), 0 );
    zone->Outline()->Append( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) );
    zone->Outline()->Append( 0, pcbIUScale.mmToIU( 1 ) );
    board->Add( zone );

    PCB_VIA_STITCH* stitch = new PCB_VIA_STITCH();
    board->Add( stitch );
    configureStitch( stitch );
    stitch->ExcludePosition( VECTOR2I( pcbIUScale.mmToIU( 6 ), pcbIUScale.mmToIU( 6 ) ) );

    auto path = std::filesystem::temp_directory_path() / "qa_via_stitch_roundtrip.kicad_pcb";

    ::KI_TEST::DumpBoardToFile( *board, path.string() );

    std::unique_ptr<BOARD> board2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_REQUIRE_EQUAL( board2->Generators().size(), 1 );

    PCB_VIA_STITCH* loaded = dynamic_cast<PCB_VIA_STITCH*>( board2->Generators().front() );
    BOOST_REQUIRE( loaded );

    BOOST_CHECK_EQUAL( loaded->GetPitch(), stitch->GetPitch() );
    BOOST_CHECK_EQUAL( (int) loaded->GetLayout(), (int) stitch->GetLayout() );
    BOOST_CHECK_EQUAL( (int) loaded->GetMode(), (int) stitch->GetMode() );
    BOOST_CHECK_EQUAL( loaded->GetSeed(), stitch->GetSeed() );

    // The template via travels through the (templates ...) section
    BOOST_CHECK_EQUAL( loaded->GetViaSize(), stitch->GetViaSize() );
    BOOST_CHECK_EQUAL( loaded->GetViaDrill(), stitch->GetViaDrill() );

    // Net is stored by name and re-resolved against the loaded board
    NETINFO_ITEM* gnd = board2->FindNet( wxT( "GND" ) );
    BOOST_REQUIRE( gnd );
    BOOST_CHECK_EQUAL( loaded->GetNetCode(), gnd->GetNetCode() );

    checkOutlinesEqual( stitch->Outline(), loaded->Outline() );

    auto savedCells =
            stitch->GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );
    auto loadedCells =
            loaded->GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );

    BOOST_REQUIRE( savedCells.has_value() );
    BOOST_REQUIRE( loadedCells.has_value() );
    BOOST_REQUIRE_EQUAL( loadedCells->size(), savedCells->size() );

    // (6mm, 6mm) on a 2mm staggered grid is cell (3, 3).
    BOOST_REQUIRE_EQUAL( savedCells->size(), 1 );
    BOOST_CHECK_EQUAL( savedCells->front(), VECTOR2I( 3, 3 ) );
    BOOST_CHECK_EQUAL( loadedCells->front(), VECTOR2I( 3, 3 ) );

    std::ifstream in( path );
    std::string   text( ( std::istreambuf_iterator<char>( in ) ),
                        std::istreambuf_iterator<char>() );

    BOOST_CHECK( text.find( "excluded_grid_cells" ) != std::string::npos );
    BOOST_CHECK( text.find( "(ij 3 3)" ) != std::string::npos );
}


// An exclusion identifies a via within one specific pattern, so a layout or mode change drops
// it rather than translating it.  The alternative silently relocates the user's work: the same
// cell index sits half a pitch away under STAGGERED, and means nothing at all under POISSON.
BOOST_AUTO_TEST_CASE( ExclusionsClearedOnLayoutOrModeChange )
{
    BOARD board;
    board.Add( new NETINFO_ITEM( &board, wxT( "GND" ), 1 ) );

    const VECTOR2I pos( pcbIUScale.mmToIU( 6 ), pcbIUScale.mmToIU( 6 ) );

    auto excluded = []( const PCB_VIA_STITCH& aStitch )
    {
        STRING_ANY_MAP props = aStitch.GetProperties();
        return props.get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" ).has_value()
               || props.get_opt<SHAPE_LINE_CHAIN>( "excluded_positions" ).has_value();
    };

    // PLAIN -> STAGGERED
    {
        PCB_VIA_STITCH stitch;
        stitch.SetParent( &board );
        configureStitch( &stitch );
        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
        stitch.ExcludePosition( pos );

        BOOST_REQUIRE( stitch.HasExclusions() );

        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::STAGGERED );

        BOOST_CHECK( !stitch.HasExclusions() );
        BOOST_CHECK( !excluded( stitch ) );
    }

    // PLAIN -> POISSON
    {
        PCB_VIA_STITCH stitch;
        stitch.SetParent( &board );
        configureStitch( &stitch );
        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
        stitch.ExcludePosition( pos );

        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::POISSON );

        BOOST_CHECK( !stitch.HasExclusions() );

        // The POISSON exclusion that replaces it is stored verbatim, not as an index
        stitch.ExcludePosition( pos );

        STRING_ANY_MAP props = stitch.GetProperties();
        auto           chain = props.get_opt<SHAPE_LINE_CHAIN>( "excluded_positions" );

        BOOST_CHECK( !props.get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" ).has_value() );
        BOOST_REQUIRE( chain.has_value() );
        BOOST_REQUIRE_EQUAL( chain->PointCount(), 1 );
        BOOST_CHECK_EQUAL( chain->CPoint( 0 ), pos );
    }

    // STITCH -> GUARD clears too
    {
        PCB_VIA_STITCH stitch;
        stitch.SetParent( &board );
        configureStitch( &stitch );
        stitch.ExcludePosition( pos );

        stitch.SetMode( PCB_VIA_STITCH_MODE::GUARD );

        BOOST_CHECK( !stitch.HasExclusions() );
    }

    // Re-assigning the value already in effect must not throw exclusions away
    {
        PCB_VIA_STITCH stitch;
        stitch.SetParent( &board );
        configureStitch( &stitch );
        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
        stitch.ExcludePosition( pos );

        stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
        stitch.SetMode( PCB_VIA_STITCH_MODE::STITCH );

        BOOST_CHECK( stitch.HasExclusions() );
    }

    // Loading must not run through the setters and wipe what the file carries
    {
        PCB_VIA_STITCH saved;
        saved.SetParent( &board );
        configureStitch( &saved );
        saved.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
        saved.ExcludePosition( pos );

        PCB_VIA_STITCH loaded;
        loaded.SetParent( &board );
        loaded.SetProperties( saved.GetProperties() );

        BOOST_CHECK( loaded.HasExclusions() );
    }
}


// Negative indices are ordinary: the grid is anchored to the board origin, so cells left of
// or above it are addressed with negative col/row and must survive the format unchanged.
BOOST_AUTO_TEST_CASE( NegativeGridCellsRoundTrip )
{
    BOARD board;
    board.Add( new NETINFO_ITEM( &board, wxT( "GND" ), 1 ) );

    PCB_VIA_STITCH stitch;
    stitch.SetParent( &board );
    configureStitch( &stitch );
    stitch.SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );

    stitch.ExcludePosition( VECTOR2I( pcbIUScale.mmToIU( -4 ), pcbIUScale.mmToIU( -6 ) ) );

    PCB_VIA_STITCH loaded;
    loaded.SetParent( &board );
    loaded.SetProperties( stitch.GetProperties() );

    auto cells = loaded.GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );

    BOOST_REQUIRE( cells.has_value() );
    BOOST_REQUIRE_EQUAL( cells->size(), 1 );
    BOOST_CHECK_EQUAL( cells->front(), VECTOR2I( -2, -3 ) );
}


// Build a board with a GND pour on both outer layers, filled, plus a stitch generator
// ready to Update()
struct STITCH_UPDATE_FIXTURE
{
    STITCH_UPDATE_FIXTURE()
    {
        m_board = std::make_unique<BOARD>();
        m_board->Add( new NETINFO_ITEM( m_board.get(), wxT( "GND" ), 1 ) );

        auto drcEngine =
                std::make_shared<DRC_ENGINE>( m_board.get(), &m_board->GetDesignSettings() );
        drcEngine->InitEngine( wxFileName() );
        m_board->GetDesignSettings().m_DRCEngine = drcEngine;

        ZONE* zone = new ZONE( m_board.get() );
        zone->SetLayerSet( LSET( { F_Cu, B_Cu } ) );
        zone->SetNetCode( 1 );
        zone->Outline()->NewOutline();
        zone->Outline()->Append( 0, 0 );
        zone->Outline()->Append( pcbIUScale.mmToIU( 20 ), 0 );
        zone->Outline()->Append( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) );
        zone->Outline()->Append( 0, pcbIUScale.mmToIU( 20 ) );
        m_board->Add( zone );

        KI_TEST::FillZones( m_board.get() );

        m_stitch = new PCB_VIA_STITCH();
        m_board->Add( m_stitch );
        configureStitch( m_stitch );
        m_stitch->SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );

        m_toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );
        m_dummyTool = new KI_TEST::DUMMY_TOOL();
        m_toolMgr.RegisterTool( m_dummyTool );
    }

    void regenerate()
    {
        BOARD_COMMIT commit( m_dummyTool );

        m_stitch->EditStart( nullptr, m_board.get(), &commit );
        m_stitch->Update( nullptr, m_board.get(), &commit );
        m_stitch->EditFinish( nullptr, m_board.get(), &commit );

        commit.Push( wxT( "regen" ), SKIP_UNDO | SKIP_SET_DIRTY | SKIP_CONNECTIVITY );
    }

    std::set<KIID> childViaIds() const
    {
        std::set<KIID> ids;

        for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
        {
            if( item->Type() == PCB_VIA_T )
                ids.insert( item->m_Uuid );
        }

        return ids;
    }

    std::set<VECTOR2I> childViaPositions() const
    {
        std::set<VECTOR2I> positions;

        for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
        {
            if( item->Type() == PCB_VIA_T )
                positions.insert( item->GetPosition() );
        }

        return positions;
    }

    std::unique_ptr<BOARD>  m_board;
    PCB_VIA_STITCH*         m_stitch = nullptr;
    TOOL_MANAGER            m_toolMgr;
    KI_TEST::DUMMY_TOOL*    m_dummyTool = nullptr;
};


// Round-tripping the layout through POISSON and back must land the grid exactly where it
// started.  The origin re-anchoring heuristic in Update() treats any child via that isn't on
// the grid as a user-dragged via defining a new origin — and after a POISSON pass every child
// is off-grid, so it would silently re-anchor the whole grid to an arbitrary Poisson via.
BOOST_FIXTURE_TEST_CASE( LayoutRoundTripKeepsGridAnchored, STITCH_UPDATE_FIXTURE )
{
    regenerate();

    std::set<VECTOR2I> before = childViaPositions();
    BOOST_REQUIRE_GT( before.size(), 10 );

    // Out to POISSON and back.  Exclusions are deliberately not involved: they are cleared by
    // the layout change, whereas the grid anchor must be completely unaffected by it.
    m_stitch->SetLayout( PCB_VIA_STITCH_LAYOUT::POISSON );
    regenerate();

    m_stitch->SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );
    regenerate();

    std::set<VECTOR2I> after = childViaPositions();

    BOOST_CHECK_EQUAL( after.size(), before.size() );

    for( const VECTOR2I& pos : before )
        BOOST_CHECK_EQUAL( after.count( pos ), 1 );
}


// Same root cause as the layout round trip: after a pitch change every existing via is off the
// new grid, and the re-anchoring heuristic would treat the first one as a deliberate drag.
// Changing the pitch must keep the grid anchored where it was.
BOOST_FIXTURE_TEST_CASE( PitchChangeKeepsGridAnchored, STITCH_UPDATE_FIXTURE )
{
    regenerate();
    BOOST_REQUIRE_GT( childViaPositions().size(), 10 );

    // Out to a different pitch and back
    m_stitch->SetPitch( pcbIUScale.mmToIU( 3 ) );
    regenerate();

    std::set<VECTOR2I> coarse = childViaPositions();
    BOOST_REQUIRE( !coarse.empty() );

    m_stitch->SetPitch( pcbIUScale.mmToIU( 2 ) );
    regenerate();

    // Every via must sit on the original 2mm lattice, i.e. at an exact multiple of the pitch
    for( const VECTOR2I& pos : childViaPositions() )
    {
        BOOST_CHECK_EQUAL( pos.x % pcbIUScale.mmToIU( 2 ), 0 );
        BOOST_CHECK_EQUAL( pos.y % pcbIUScale.mmToIU( 2 ), 0 );
    }
}


BOOST_FIXTURE_TEST_CASE( UpdateDeltaStability, STITCH_UPDATE_FIXTURE )
{
    regenerate();

    std::set<KIID> first = childViaIds();

    // The 16mm x 16mm outline at 2mm pitch should have 10 vias
    BOOST_REQUIRE_GT( first.size(), 10 );

    // Vias must respect the pitch
    std::vector<VECTOR2I> positions;

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
            positions.push_back( item->GetPosition() );
    }

    const int64_t minDistSq =
            (int64_t) pcbIUScale.mmToIU( 2 ) * pcbIUScale.mmToIU( 2 ) - 1;

    for( size_t i = 0; i < positions.size(); ++i )
    {
        for( size_t j = i + 1; j < positions.size(); ++j )
        {
            VECTOR2I d = positions[i] - positions[j];
            int64_t  distSq = (int64_t) d.x * d.x + (int64_t) d.y * d.y;

            BOOST_CHECK_MESSAGE( distSq >= minDistSq,
                                 "vias " << i << " and " << j << " closer than the pitch" );
        }
    }

    // Regenerating an unchanged board, none of the vias should change
    regenerate();

    std::set<KIID> second = childViaIds();

    BOOST_CHECK( first == second );
}


BOOST_FIXTURE_TEST_CASE( UpdateRespectsExclusions, STITCH_UPDATE_FIXTURE )
{
    regenerate();

    std::set<KIID> before = childViaIds();
    BOOST_REQUIRE_GT( before.size(), 10 );

    // Exclude one existing via's position: regeneration must remove exactly that via and
    // leave every other via untouched
    PCB_VIA* victim = nullptr;

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            victim = static_cast<PCB_VIA*>( item );
            break;
        }
    }

    BOOST_REQUIRE( victim );

    KIID     victimId = victim->m_Uuid;
    VECTOR2I victimPos = victim->GetPosition();

    m_stitch->ExcludePosition( victimPos );
    regenerate();

    std::set<KIID> after = childViaIds();

    BOOST_CHECK_EQUAL( after.size(), before.size() - 1 );
    BOOST_CHECK_EQUAL( after.count( victimId ), 0 );

    for( const KIID& id : after )
        BOOST_CHECK_EQUAL( before.count( id ), 1 );

    // Clearing the exclusion brings a via back at that position (with a new identity)
    m_stitch->ClearExclusion( victimPos );
    regenerate();

    std::set<KIID> restored = childViaIds();

    BOOST_CHECK_EQUAL( restored.size(), before.size() );

    bool found = false;

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T && item->GetPosition() == victimPos )
            found = true;
    }

    BOOST_CHECK( found );
}


BOOST_FIXTURE_TEST_CASE( ClearAllExclusionsRestoresEveryVia, STITCH_UPDATE_FIXTURE )
{
    regenerate();

    std::set<KIID> before = childViaIds();
    BOOST_REQUIRE_GT( before.size(), 10 );

    std::vector<VECTOR2I> victimPositions;

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        if( item->Type() == PCB_VIA_T && victimPositions.size() < 3 )
            victimPositions.push_back( item->GetPosition() );
    }

    BOOST_REQUIRE_EQUAL( victimPositions.size(), 3 );
    BOOST_CHECK( !m_stitch->HasExclusions() );

    for( const VECTOR2I& pos : victimPositions )
        m_stitch->ExcludePosition( pos );

    BOOST_CHECK( m_stitch->HasExclusions() );

    regenerate();
    BOOST_CHECK_EQUAL( childViaIds().size(), before.size() - 3 );

    m_stitch->ClearAllExclusions();
    BOOST_CHECK( !m_stitch->HasExclusions() );

    regenerate();
    BOOST_CHECK_EQUAL( childViaIds().size(), before.size() );

    // Every excluded position must be occupied again
    for( const VECTOR2I& pos : victimPositions )
    {
        bool found = false;

        for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
        {
            if( item->Type() == PCB_VIA_T && item->GetPosition() == pos )
                found = true;
        }

        BOOST_CHECK( found );
    }
}

BOOST_AUTO_TEST_SUITE_END()
