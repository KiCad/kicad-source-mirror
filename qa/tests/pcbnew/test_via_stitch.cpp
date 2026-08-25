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

#include <algorithm>
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
#include <footprint.h>
#include <generators/pcb_via_stitch.h>
#include <geometry/shape_circle.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <zone.h>
#include <zones.h>

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
    aStitch->ViaTemplate()->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
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


BOOST_AUTO_TEST_CASE( GuardEnvelopeSampling )
{
    // Guarding previously broke down at pitches > 1.7mm
    const int pitch = pcbIUScale.mmToIU( 2 );

    // 20mm x 10mm rectangle: a 60mm perimeter, so ~30 samples before the spacing filter.
    SHAPE_POLY_SET envelope = makeRectPoly( 0, 0, pcbIUScale.mmToIU( 20 ),
                                            pcbIUScale.mmToIU( 10 ) );

    auto acceptAll = []( const VECTOR2I& ) { return true; };

    // Nothing inside the rectangle to guard, so no pair is exempt from the spacing minimum.
    SHAPE_POLY_SET noGuarded;

    std::vector<VECTOR2I> samples =
            PCB_VIA_STITCH::SampleGuardEnvelope( envelope, noGuarded, pitch, acceptAll );

    // A 60mm perimeter walked every 2mm can't produce many fewer than 30 without the
    // spacing filter having gone haywire; the corners are the only place it can bite.
    BOOST_CHECK_GE( samples.size(), 26u );
    BOOST_CHECK_LE( samples.size(), 30u );

    // Every sample lands on the envelope boundary.
    for( const VECTOR2I& pt : samples )
    {
        BOOST_CHECK_MESSAGE( envelope.Collide( pt, pcbIUScale.mmToIU( 0.001 ) ),
                             "sample " << pt.x << "," << pt.y << " left the envelope" );
    }

    // No two vias end up closer than the 0.7 * pitch minimum.
    const int64_t minDistSq = (int64_t) ( pitch * 0.7 ) * (int64_t) ( pitch * 0.7 );

    for( size_t i = 0; i < samples.size(); ++i )
    {
        for( size_t j = i + 1; j < samples.size(); ++j )
        {
            VECTOR2I d = samples[i] - samples[j];
            BOOST_CHECK_GE( (int64_t) d.x * d.x + (int64_t) d.y * d.y, minDistSq );
        }
    }

    // Positions the caller rejects are dropped rather than nudged elsewhere.
    const int midX = pcbIUScale.mmToIU( 10 );

    std::vector<VECTOR2I> leftHalf = PCB_VIA_STITCH::SampleGuardEnvelope(
            envelope, noGuarded, pitch,
            [&]( const VECTOR2I& aPt )
            {
                return aPt.x < midX;
            } );

    BOOST_CHECK( !leftHalf.empty() );
    BOOST_CHECK_LT( leftHalf.size(), samples.size() );

    for( const VECTOR2I& pt : leftHalf )
        BOOST_CHECK_LT( pt.x, midX );

    // A degenerate pitch yields nothing instead of looping forever.
    BOOST_CHECK( PCB_VIA_STITCH::SampleGuardEnvelope( envelope, noGuarded, 0, acceptAll ).empty() );
    BOOST_CHECK( PCB_VIA_STITCH::SampleGuardEnvelope( envelope, noGuarded, -1, acceptAll ).empty() );
}


BOOST_AUTO_TEST_CASE( GuardEnvelopeCoversBothSidesAtCoarsePitch )
{
    // A guard envelope is a thin slab: the two rows of vias face each other across only
    // (trackWidth + viaSize + 2 * clearance), which a coarse pitch's 0.7 * pitch spacing
    // minimum swallows whole.  Culling on that alone leaves one side of the trace bare.
    const int trackWidth = pcbIUScale.mmToIU( 0.2 );
    const int envelopeOffset = pcbIUScale.mmToIU( 0.607 );   // via radius + clearance + slop
    const int traceLen = pcbIUScale.mmToIU( 20 );
    const int centreY = pcbIUScale.mmToIU( 10 );

    // Horizontal trace, and the slab standing off it on both sides.
    SHAPE_POLY_SET guarded = makeRectPoly( 0, centreY - trackWidth / 2, traceLen,
                                           centreY + trackWidth / 2 );
    SHAPE_POLY_SET envelope = makeRectPoly( -envelopeOffset, centreY - envelopeOffset,
                                            traceLen + envelopeOffset,
                                            centreY + envelopeOffset );

    auto acceptAll = []( const VECTOR2I& ) { return true; };

    // 2mm is the default pitch, and 0.7 * 2mm = 1.4mm overshoots the 1.214mm gap between
    // the facing rows.
    const int pitch = pcbIUScale.mmToIU( 2 );
    BOOST_REQUIRE_GT( pitch * 0.7, 2.0 * envelopeOffset );

    std::vector<VECTOR2I> samples =
            PCB_VIA_STITCH::SampleGuardEnvelope( envelope, guarded, pitch, acceptAll );

    int above = 0;
    int below = 0;

    for( const VECTOR2I& pt : samples )
    {
        if( pt.y < centreY )
            above++;
        else if( pt.y > centreY )
            below++;
    }

    // Both sides of a 20mm trace should carry a full row at 2mm pitch, not a scattering.
    BOOST_CHECK_GE( above, 8 );
    BOOST_CHECK_GE( below, 8 );

    // The exemption is for facing pairs only: along one side the pitch spacing still holds.
    std::vector<int> aboveXs;

    for( const VECTOR2I& pt : samples )
    {
        if( pt.y < centreY )
            aboveXs.push_back( pt.x );
    }

    std::sort( aboveXs.begin(), aboveXs.end() );

    for( size_t i = 1; i < aboveXs.size(); ++i )
        BOOST_CHECK_GE( aboveXs[i] - aboveXs[i - 1], (int) ( pitch * 0.7 ) );
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

    auto savedCells = stitch->GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );
    auto loadedCells = loaded->GetProperties().get_opt<std::vector<VECTOR2I>>( "excluded_grid_cells" );

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


// Shared plumbing for driving a stitch generator through Update().  Derived fixtures supply
// the board and point m_stitch at the generator sitting on it.
struct STITCH_FIXTURE_BASE
{
    /// The BOARD_COMMITs in regenerate() need a tool to hang off.
    void attachToolManager()
    {
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


// An empty board with a GND net and a live DRC engine.  Derived fixtures pour the copper they
// need, then call finishSetup() to fill it and attach a stitch generator.
struct STITCH_SYNTHETIC_FIXTURE : public STITCH_FIXTURE_BASE
{
    STITCH_SYNTHETIC_FIXTURE()
    {
        m_board = std::make_unique<BOARD>();
        m_board->Add( new NETINFO_ITEM( m_board.get(), wxT( "GND" ), 1 ) );

        auto drcEngine =
                std::make_shared<DRC_ENGINE>( m_board.get(), &m_board->GetDesignSettings() );
        drcEngine->InitEngine( wxFileName() );
        m_board->GetDesignSettings().m_DRCEngine = drcEngine;
    }

    /// Add a full-height GND pour on aLayers, spanning x from aLeft to aRight.
    void addGndZone( const LSET& aLayers, int aLeft, int aRight )
    {
        ZONE* zone = new ZONE( m_board.get() );
        zone->SetLayerSet( aLayers );
        zone->SetNetCode( 1 );
        zone->Outline()->NewOutline();
        zone->Outline()->Append( aLeft, 0 );
        zone->Outline()->Append( aRight, 0 );
        zone->Outline()->Append( aRight, pcbIUScale.mmToIU( 20 ) );
        zone->Outline()->Append( aLeft, pcbIUScale.mmToIU( 20 ) );
        m_board->Add( zone );
    }

    /// Fill the poured zones and attach a stitch generator ready to Update().
    void finishSetup()
    {
        KI_TEST::FillZones( m_board.get() );

        m_stitch = new PCB_VIA_STITCH();
        m_board->Add( m_stitch );
        configureStitch( m_stitch );
        m_stitch->SetLayout( PCB_VIA_STITCH_LAYOUT::PLAIN );

        attachToolManager();
    }
};


// A GND pour on both outer layers, covering the whole board
struct STITCH_UPDATE_FIXTURE : public STITCH_SYNTHETIC_FIXTURE
{
    STITCH_UPDATE_FIXTURE()
    {
        addGndZone( LSET( { F_Cu, B_Cu } ), 0, pcbIUScale.mmToIU( 20 ) );
        finishSetup();
    }
};


// A 4-layer board whose two inner planes cover everything, with an F_Cu pour that only
// reaches the left half of them
struct STITCH_PARTIAL_LAYER_FIXTURE : public STITCH_SYNTHETIC_FIXTURE
{
    STITCH_PARTIAL_LAYER_FIXTURE()
    {
        m_board->SetCopperLayerCount( 4 );
        m_board->GetDesignSettings().SetCopperLayerCount( 4 );
        m_board->SetEnabledLayers( m_board->GetEnabledLayers() | LSET::AllCuMask( 4 ) );

        addGndZone( LSET( { In1_Cu, In2_Cu } ), 0, pcbIUScale.mmToIU( 20 ) );
        addGndZone( LSET( { F_Cu } ), 0, pcbIUScale.mmToIU( 10 ) );

        finishSetup();
    }
};


// Loads a saved board out of qa/data/pcbnew and picks up the via-stitch generator on it, so a
// real design's zones, netclasses and design settings drive the placement.
struct STITCH_BOARD_FIXTURE : public STITCH_FIXTURE_BASE
{
    ~STITCH_BOARD_FIXTURE()
    {
        // The board borrows the project owned by m_settingsManager, which is destroyed right
        // after this body runs.
        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    void loadBoard( const wxString& aRelPath )
    {
        KI_TEST::LoadBoard( m_settingsManager, aRelPath, m_board );
        BOOST_REQUIRE( m_board );

        KI_TEST::FillZones( m_board.get() );

        for( PCB_GENERATOR* generator : m_board->Generators() )
        {
            if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( generator ) )
            {
                BOOST_REQUIRE_MESSAGE( !m_stitch, "more than one stitch generator on the board" );
                m_stitch = stitch;
            }
        }

        BOOST_REQUIRE_MESSAGE( m_stitch, "no via-stitch generator on the board" );

        attachToolManager();
    }

    SETTINGS_MANAGER m_settingsManager;
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

// A via needs same-net copper on two of the layers it spans, not on every one of them.  The
// two inner planes overlap across the whole stitch outline and must be stitched to each other
// everywhere, including the right half that the F_Cu pour doesn't reach.
BOOST_FIXTURE_TEST_CASE( PartialOuterPourStillStitchesInnerPlanes, STITCH_PARTIAL_LAYER_FIXTURE )
{
    regenerate();

    std::set<VECTOR2I> positions = childViaPositions();
    BOOST_REQUIRE_GT( positions.size(), 10 );

    bool overThreeLayers = false;   // F_Cu plus both planes
    bool overTwoLayers = false;     // the planes alone

    for( const VECTOR2I& pos : positions )
    {
        if( pos.x < pcbIUScale.mmToIU( 8 ) )
            overThreeLayers = true;
        else if( pos.x > pcbIUScale.mmToIU( 12 ) )
            overTwoLayers = true;
    }

    BOOST_CHECK( overThreeLayers );
    BOOST_CHECK_MESSAGE( overTwoLayers,
                         "no vias placed where only the two inner planes overlap" );
}


// The same partial-coverage case on a real board (issue 25303): GND on both inner planes
// across the whole area, a small GND pour on F.Cu, and a stitch outline running well past the
// right edge of that pour.  The stitching has to carry on across the inner planes alone.
BOOST_FIXTURE_TEST_CASE( PartialTopPourStillStitchesInnerPlanes, STITCH_BOARD_FIXTURE )
{
    loadBoard( wxT( "issue25303" ) );

    // The saved board carries the generator but none of its vias
    BOOST_REQUIRE( childViaPositions().empty() );

    regenerate();

    std::set<VECTOR2I> positions = childViaPositions();
    BOOST_REQUIRE_GT( positions.size(), 100 );

    // The F.Cu pour runs stops at about x = 107mm; the inner planes and the stitch outline both
    // carry on past x = 124mm.
    const int pourEdge = pcbIUScale.mmToIU( 107 );

    int underPour = 0;
    int beyondPour = 0;
    int maxX = INT_MIN;

    for( const VECTOR2I& pos : positions )
    {
        if( pos.x < pourEdge )
            underPour++;
        else
            beyondPour++;

        maxX = std::max( maxX, pos.x );
    }

    BOOST_CHECK_GT( underPour, 20 );
    BOOST_CHECK_MESSAGE( beyondPour > 20, "stitching stopped at the edge of the F.Cu pour" );

    // ...and it has to reach the far side of the outline, not just spill over the pour edge
    BOOST_CHECK_GT( maxX, pcbIUScale.mmToIU( 122 ) );

    // Every via belongs to the stitch net and sits inside the outline
    const SHAPE_POLY_SET& outline = m_stitch->Outline();

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        BOOST_REQUIRE_EQUAL( item->Type(), PCB_VIA_T );

        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        BOOST_CHECK_EQUAL( via->GetNetCode(), m_stitch->GetNetCode() );
        BOOST_CHECK( outline.Contains( via->GetPosition() ) );
    }
}

// Pads have to be avoided in their own right, not just when they happen to be cross-net copper.
// The GND pour on this board (issue 25265) connects to pads solidly instead of through thermal
// reliefs, so it floods straight over its own pads -- and a stitch position sitting on one of
// them looks like any other patch of same-net zone fill.  Vias used to land right on top.
BOOST_FIXTURE_TEST_CASE( PadsBlockStitchingWithSolidZoneConnections, STITCH_BOARD_FIXTURE )
{
    loadBoard( wxT( "issue25265" ) );

    // The saved board carries the generator but none of its vias
    BOOST_REQUIRE( childViaPositions().empty() );

    // Guard the premise: the pour has to be solid-connected, and it has to have pads of its own
    // sitting under the stitch outline, or the case being tested isn't on the board any more.
    for( ZONE* zone : m_board->Zones() )
        BOOST_REQUIRE_EQUAL( (int) zone->GetPadConnection(), (int) ZONE_CONNECTION::FULL );

    const std::vector<PAD*> pads = m_board->GetPads();
    int                     sameNetPadsInside = 0;

    for( PAD* pad : pads )
    {
        if( pad->GetNetCode() == m_stitch->GetNetCode()
                && m_stitch->Outline().Contains( pad->GetPosition() ) )
        {
            sameNetPadsInside++;
        }
    }

    BOOST_REQUIRE_MESSAGE( sameNetPadsInside > 0,
                           "no pads on the stitch net sit inside the stitch outline" );

    regenerate();

    BOOST_REQUIRE_GT( childViaPositions().size(), 100 );

    for( BOARD_ITEM* item : m_stitch->GetBoardItems() )
    {
        BOOST_REQUIRE_EQUAL( item->Type(), PCB_VIA_T );

        PCB_VIA* via = static_cast<PCB_VIA*>( item );

        for( PCB_LAYER_ID layer : { F_Cu, B_Cu } )
        {
            SHAPE_CIRCLE viaCopper( via->GetPosition(), via->GetWidth( layer ) / 2 );

            for( PAD* pad : pads )
            {
                if( !pad->FlashLayer( layer ) )
                    continue;

                BOOST_CHECK_MESSAGE(
                        !pad->GetEffectiveShape( layer )->Collide( &viaCopper ),
                        wxString::Format( "Via at (%.3f, %.3f) sits on pad %s of %s (net %s)",
                                          pcbIUScale.IUTomm( via->GetPosition().x ),
                                          pcbIUScale.IUTomm( via->GetPosition().y ),
                                          pad->GetNumber(),
                                          pad->GetParentFootprint()->GetReferenceAsString(),
                                          pad->GetNetname() ) );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
