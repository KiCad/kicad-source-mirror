/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <richio.h>

#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_shape.h>
#include <zone.h>


struct KICAD_SEXPR_FIXTURE
{
    KICAD_SEXPR_FIXTURE() {}

    PCB_IO_KICAD_SEXPR kicadPlugin;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( KiCadSexprIO, KICAD_SEXPR_FIXTURE )


/**
 * Compare all footprints declared in a *.lbr file with their KiCad reference footprint
 */
BOOST_AUTO_TEST_CASE( Issue19775_ZoneLayerWildcards )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/kicad_sexpr/Issue19775_ZoneLayers/";

    BOOST_TEST_CONTEXT( "Zone layers with wildcards" )
    {
        std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

        kicadPlugin.LoadBoard( dataPath + "LayerWildcard.kicad_pcb", testBoard.get() );

        // One zone in the file
        BOOST_CHECK( testBoard->Zones().size() == 1 );

        ZONE* z = testBoard->Zones()[0];

        // On both front and back layers, with zone fill on both
        BOOST_CHECK( z->GetLayerSet().Contains( F_Cu ) && z->GetLayerSet().Contains( B_Cu ) );
        BOOST_CHECK( z->GetFilledPolysList( F_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->GetFilledPolysList( B_Cu )->TotalVertices() > 0 );
    }

    BOOST_TEST_CONTEXT( "Round trip layers" )
    {
        auto tmpBoard = std::filesystem::temp_directory_path() / "Issue19775_RoundTrip.kicad_pcb";

        // Load and save the board from above to test how we write the zones into it
        {
            std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();
            kicadPlugin.LoadBoard( dataPath + "LayerEnumerate.kicad_pcb", testBoard.get() );
            kicadPlugin.SaveBoard( tmpBoard.string(), testBoard.get() );
        }

        // Read the new board
        std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();
        kicadPlugin.LoadBoard( tmpBoard.string(), testBoard.get() );

        // One zone in the file
        BOOST_CHECK( testBoard->Zones().size() == 1 );

        ZONE* z = testBoard->Zones()[0];

        // On both front and back layers, with zone fill on both
        BOOST_CHECK( z->GetLayerSet().Contains( F_Cu ) && z->GetLayerSet().Contains( B_Cu ) );
        BOOST_CHECK( z->GetFilledPolysList( F_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->GetFilledPolysList( B_Cu )->TotalVertices() > 0 );
        BOOST_CHECK( z->LayerProperties().contains( F_Cu ) );
    }
}


/**
 * Verify that zones with no polygon outline are silently discarded during
 * loading rather than being added to the board where they would cause
 * crashes in GetPosition().
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23125
 */
BOOST_AUTO_TEST_CASE( Issue23125_EmptyZoneDiscarded )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/kicad_sexpr/Issue23125_EmptyZone/";

    std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

    kicadPlugin.LoadBoard( dataPath + "EmptyZone.kicad_pcb", testBoard.get() );

    // The file contains 3 zones: 1 valid (with polygon) and 2 empty (no polygon).
    // The 2 empty zones should have been discarded during loading.
    BOOST_CHECK_EQUAL( testBoard->Zones().size(), 1 );

    // The surviving zone should have a valid position
    ZONE* z = testBoard->Zones()[0];
    BOOST_CHECK_NO_THROW( z->GetPosition() );
    BOOST_CHECK( z->GetNumCorners() > 0 );
}


/**
 * Verify the parser still can read floating point values written in scientific notation.
 * Even though the KiCad file writter doesn't write using scientific notation anymore, at one
 * point it did, so the parser must still support reading it.
 */
BOOST_AUTO_TEST_CASE( ScientificNotationLoading )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/kicad_sexpr/";

    std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

    kicadPlugin.LoadBoard( dataPath + "ScientificNotation.kicad_pcb", testBoard.get() );

    // The file contains 1 arc with scientific notation in its coordinates
    BOOST_CHECK_EQUAL( testBoard->Drawings().size(), 1 );

    PCB_SHAPE* arc = dynamic_cast<PCB_SHAPE*>( testBoard->Drawings().front() );

    // The arc's midpoint should be located at (4.17, -4.5e-05) in file units
    BOOST_REQUIRE( arc );
    BOOST_TEST( arc->GetArcMid().x == 4170000 );
    BOOST_TEST( arc->GetArcMid().y == -45 );
}


/**
 * Verify that a corrupted file with thousands of stackup items does not crash
 * KiCad.  The parser should silently cap the stackup item count.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23625
 */
BOOST_AUTO_TEST_CASE( Issue23625_CorruptedStackupCapped )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/kicad_sexpr/Issue23625_CorruptedStackup/";

    std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

    BOOST_CHECK_NO_THROW( kicadPlugin.LoadBoard( dataPath + "corrupted_stackup.kicad_pcb",
                                                 testBoard.get() ) );

    const BOARD_STACKUP& stackup =
            testBoard->GetDesignSettings().GetStackupDescriptor();

    // The test file has 200 dielectric layers (plus copper, silk, mask), well
    // beyond the parser's 128-item cap.  After loading, the count must be
    // clamped and the board must still be usable.
    BOOST_CHECK_LE( stackup.GetCount(), 128 );
    BOOST_CHECK_GT( stackup.GetCount(), 0 );
}


/**
 * Verify that a file whose stackup was duplicated (the whole layer sequence repeated several
 * times, as could happen after a board append in older versions) is reconciled on load so the
 * board thickness is not inflated by the number of copies.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24133
 */
BOOST_AUTO_TEST_CASE( Issue24133_DuplicatedStackupNotInflated )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/kicad_sexpr/Issue24133_DuplicatedStackup/";

    std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

    BOOST_CHECK_NO_THROW( kicadPlugin.LoadBoard( dataPath + "duplicated_stackup.kicad_pcb",
                                                 testBoard.get() ) );

    const BOARD_STACKUP& stackup = testBoard->GetDesignSettings().GetStackupDescriptor();

    // The file repeats the 9-item layer sequence three times.  Only the first copy must be kept.
    BOOST_CHECK_EQUAL( stackup.GetCount(), 9 );

    // A single copy is a standard 1.6 mm two-layer board.  Without dedup the three copies would
    // sum to 4.8 mm.
    BOOST_CHECK_EQUAL( stackup.BuildBoardThicknessFromStackup(), pcbIUScale.mmToIU( 1.6 ) );
}


/**
 * Verify that append-board preserves the destination stackup while still
 * growing it to match a source board with more copper layers.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23752
 */
BOOST_AUTO_TEST_CASE( Issue23752_AppendBoardPreservesStackupAndGrowsToSixCopperLayers )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir();

    // four layer board
    std::string destinationPath = dataPath + "issue3812.kicad_pcb";
    // six layer board
    std::string                 sourcePath = dataPath + "issue18142.kicad_pcb";
    std::map<std::string, UTF8> props;

    // basically, make sure that we start with a four layer board, append a six layer
    // board like this is a design block apply-layout with six layers, and make
    // sure we end up with a six layer board and a six layer stackup and not a
    // ten layer board
    //
    // we also need to test the stackup properties are preserved, because the increase
    // in copper layers should cause a stackup growth from from four to six layers,
    // but not replace the stackup with the source board's stackup, which has different properties (e.g. finish type)
    props[PCB_IO_LOAD_PROPERTIES::APPEND_PRESERVE_DESTINATION_STACKUP] = "";

    auto countCopperLayers = []( const BOARD_STACKUP& aStackup )
    {
        int copperLayerCount = 0;

        for( BOARD_STACKUP_ITEM* item : aStackup.GetList() )
        {
            if( item->GetType() == BS_ITEM_TYPE_COPPER )
                ++copperLayerCount;
        }

        return copperLayerCount;
    };

    auto findFirstDielectric = []( const BOARD_STACKUP& aStackup ) -> const BOARD_STACKUP_ITEM*
    {
        for( BOARD_STACKUP_ITEM* item : aStackup.GetList() )
        {
            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                return item;
        }

        return nullptr;
    };

    std::unique_ptr<BOARD> testBoard = std::make_unique<BOARD>();

    kicadPlugin.LoadBoard( destinationPath, testBoard.get() );

    const BOARD_STACKUP&      initialStackup = testBoard->GetDesignSettings().GetStackupDescriptor();
    const BOARD_STACKUP_ITEM* initialFirstDielectric = findFirstDielectric( initialStackup );
    const int  initialCopperLayerCount = testBoard->GetCopperLayerCount();
    const LSET initialEnabledLayers = testBoard->GetEnabledLayers();
    const wxString            initialFinishType = initialStackup.m_FinishType;

    BOOST_REQUIRE_EQUAL( initialCopperLayerCount, 4 );
    BOOST_REQUIRE_EQUAL( countCopperLayers( initialStackup ), 4 );
    BOOST_REQUIRE( initialFirstDielectric );
    BOOST_REQUIRE_EQUAL( initialFinishType, wxS( "ENIG" ) );
    const wxString initialFirstDielectricMaterial = initialFirstDielectric->GetMaterial();
    const int      initialFirstDielectricThickness = initialFirstDielectric->GetThickness();

    kicadPlugin.LoadBoard( sourcePath, testBoard.get(), &props );

    const int appendedCopperLayerCount = testBoard->GetCopperLayerCount();

    if( appendedCopperLayerCount > initialCopperLayerCount )
        testBoard->SetCopperLayerCount( appendedCopperLayerCount );

    LSET enabledLayers = testBoard->GetEnabledLayers();
    enabledLayers |= initialEnabledLayers;
    testBoard->SetEnabledLayers( enabledLayers );
    testBoard->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard( &testBoard->GetDesignSettings() );

    const BOARD_STACKUP&      finalStackup = testBoard->GetDesignSettings().GetStackupDescriptor();
    const BOARD_STACKUP_ITEM* finalFirstDielectric = findFirstDielectric( finalStackup );

    BOOST_CHECK_EQUAL( testBoard->GetCopperLayerCount(), 6 );
    BOOST_CHECK_EQUAL( countCopperLayers( finalStackup ), 6 );
    BOOST_REQUIRE( finalFirstDielectric );
    BOOST_CHECK_EQUAL( finalStackup.m_FinishType, initialFinishType );
    BOOST_CHECK_EQUAL( finalFirstDielectric->GetMaterial(), initialFirstDielectricMaterial );
    BOOST_CHECK_EQUAL( finalFirstDielectric->GetThickness(), initialFirstDielectricThickness );
}


/**
 * Regression for the footprint-save SIGSEGV observed in KiCad 10.0.0 (introduced by
 * b335ce6e2c "Don't save netcodes to files", which switched PCB_SHAPE/PCB_TRACK/ZONE
 * serialization from writing the netcode to writing the netname).  If a footprint's
 * descendants carry m_netinfo pointers that belong to a different (possibly destroyed)
 * board, the library serializer reads through those pointers and can SEGV inside
 * BOARD_CONNECTED_ITEM::GetNetname().
 *
 * The library-save path must (a) skip writing (net ...) tokens for pads/shapes/tracks/zones
 * when CTL_OMIT_PAD_NETS is set (as it is under CTL_FOR_LIBRARY) and (b) orphan every
 * BOARD_CONNECTED_ITEM descendant on the cloned footprint before serialization, so that
 * any downstream code reading m_netinfo lands on the board-independent ORPHANED singleton.
 *
 * This test exercises the library-save path with a footprint whose pad, copper shape and
 * copper zone all reference a real net on a locally-built board.  The emitted .kicad_mod
 * must contain no "(net " tokens, and ClearAllNets() must orphan every connected item.
 */
BOOST_AUTO_TEST_CASE( FootprintSave_OmitsNetsOnAllBoardConnectedItems )
{
    auto tmpLib = std::filesystem::temp_directory_path() / "qa_fp_save_netinfo.pretty";
    std::filesystem::remove_all( tmpLib );
    std::filesystem::create_directories( tmpLib );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxT( "TestNet" ), 1 );
    board->Add( net );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    board->Add( fp );
    fp->SetFPID( LIB_ID( wxT( "scratch" ), wxT( "test_fp_save_netinfo" ) ) );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    pad->SetNet( net );
    fp->Add( pad );

    PCB_SHAPE* shape = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    shape->SetLayer( F_Cu );
    shape->SetStart( VECTOR2I( 0, 0 ) );
    shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
    shape->SetNet( net );
    fp->Add( shape );

    ZONE* zone = new ZONE( fp );
    zone->SetLayer( F_Cu );
    zone->SetNet( net );
    fp->Add( zone );

    BOOST_REQUIRE_EQUAL( pad->GetNet(), net );
    BOOST_REQUIRE_EQUAL( shape->GetNet(), net );
    BOOST_REQUIRE_EQUAL( zone->GetNet(), net );

    // Save into the scratch library via the public library-save API.  Must not throw.
    BOOST_REQUIRE_NO_THROW( kicadPlugin.FootprintSave( tmpLib.string(), fp ) );

    auto savedFile = tmpLib / "test_fp_save_netinfo.kicad_mod";
    BOOST_REQUIRE( std::filesystem::exists( savedFile ) );

    std::ifstream in( savedFile );
    BOOST_REQUIRE_MESSAGE( in.is_open(),
                           "Failed to open serialized footprint: " << savedFile.string() );

    std::stringstream ss;
    ss << in.rdbuf();
    BOOST_REQUIRE( !in.bad() );

    const std::string contents = ss.str();
    BOOST_REQUIRE( !contents.empty() );

    BOOST_CHECK_MESSAGE( contents.find( "(net " ) == std::string::npos,
                         "Saved footprint library file must not contain (net ...) tokens:\n"
                                 << contents );

    // Verify the second defense directly on the save-path's transient state: clone the
    // footprint, detach it from its parent board exactly as FootprintSave does, invoke
    // ClearAllNets(), and assert every BOARD_CONNECTED_ITEM descendant is orphaned.  This
    // catches a regression where FootprintSave stops calling ClearAllNets() even if the
    // serializer guards keep the file contents looking correct.
    std::unique_ptr<FOOTPRINT> detached( static_cast<FOOTPRINT*>( fp->Clone() ) );
    detached->SetParent( nullptr );
    detached->SetParentGroup( nullptr );
    detached->ClearAllNets();

    BOARD_CONNECTED_ITEM* detachedPad = nullptr;
    BOARD_CONNECTED_ITEM* detachedShape = nullptr;
    BOARD_CONNECTED_ITEM* detachedZone = nullptr;

    detached->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                switch( aItem->Type() )
                {
                case PCB_PAD_T:   detachedPad   = static_cast<BOARD_CONNECTED_ITEM*>( aItem ); break;
                case PCB_SHAPE_T: detachedShape = static_cast<BOARD_CONNECTED_ITEM*>( aItem ); break;
                case PCB_ZONE_T:  detachedZone  = static_cast<BOARD_CONNECTED_ITEM*>( aItem ); break;
                default: break;
                }
            },
            RECURSE_MODE::RECURSE );

    BOOST_REQUIRE( detachedPad );
    BOOST_REQUIRE( detachedShape );
    BOOST_REQUIRE( detachedZone );
    BOOST_CHECK_EQUAL( detachedPad->GetNet(), NETINFO_LIST::OrphanedItem() );
    BOOST_CHECK_EQUAL( detachedShape->GetNet(), NETINFO_LIST::OrphanedItem() );
    BOOST_CHECK_EQUAL( detachedZone->GetNet(), NETINFO_LIST::OrphanedItem() );

    std::filesystem::remove_all( tmpLib );
}


/**
 * Verify that a copper-thieving zone round-trips through the kicad_sexpr writer
 * and parser, preserving every THIEVING_SETTINGS field plus the netless invariant.
 *
 * Tests the writer's `(mode thieving)` emission, the `(thieving ...)` sub-block
 * format, and the parser's case for both.  Independently exercises each pattern
 * value to catch keyword-table omissions.
 */
BOOST_AUTO_TEST_CASE( CopperThievingZone_RoundTrip )
{
    std::unique_ptr<BOARD> writeBoard = std::make_unique<BOARD>();

    ZONE* zone = new ZONE( writeBoard.get() );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), -1 );
    zone->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 10 ) ), -1 );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );

    THIEVING_SETTINGS thieving;
    thieving.pattern      = THIEVING_PATTERN::HATCH;
    thieving.element_size = pcbIUScale.mmToIU( 0.42 );
    thieving.gap        = pcbIUScale.mmToIU( 1.27 );
    thieving.line_width   = pcbIUScale.mmToIU( 0.35 );
    thieving.stagger      = true;
    thieving.orientation     = EDA_ANGLE( 30.0, DEGREES_T );
    zone->SetThievingSettings( thieving );

    writeBoard->Add( zone );

    std::filesystem::path tmpPath = std::filesystem::temp_directory_path()
                                    / "copper_thieving_roundtrip.kicad_pcb";

    PCB_IO_KICAD_SEXPR writer;
    writer.SaveBoard( tmpPath.string(), writeBoard.get() );

    std::unique_ptr<BOARD> readBoard = std::make_unique<BOARD>();
    PCB_IO_KICAD_SEXPR    reader;
    reader.LoadBoard( tmpPath.string(), readBoard.get() );

    BOOST_REQUIRE_EQUAL( readBoard->Zones().size(), 1u );

    ZONE* loaded = readBoard->Zones()[0];

    BOOST_CHECK( loaded->GetFillMode() == ZONE_FILL_MODE::COPPER_THIEVING );
    BOOST_CHECK( loaded->IsCopperThieving() );
    BOOST_CHECK_EQUAL( loaded->GetNetCode(), 0 );

    const THIEVING_SETTINGS& loadedSettings = loaded->GetThievingSettings();
    BOOST_CHECK( loadedSettings.pattern == THIEVING_PATTERN::HATCH );
    BOOST_CHECK_EQUAL( loadedSettings.element_size, thieving.element_size );
    BOOST_CHECK_EQUAL( loadedSettings.gap, thieving.gap );
    BOOST_CHECK_EQUAL( loadedSettings.line_width, thieving.line_width );
    BOOST_CHECK_EQUAL( loadedSettings.stagger, true );
    BOOST_CHECK( loadedSettings.orientation == EDA_ANGLE( 30.0, DEGREES_T ) );

    std::filesystem::remove( tmpPath );
}


/**
 * Each THIEVING_PATTERN enum value should round-trip through the file format.
 * Catches token-table mistakes that only affect a specific pattern.
 */
BOOST_AUTO_TEST_CASE( CopperThievingZone_AllPatternsRoundTrip )
{
    const std::array<THIEVING_PATTERN, 3> patterns = {
        THIEVING_PATTERN::DOTS,
        THIEVING_PATTERN::SQUARES,
        THIEVING_PATTERN::HATCH,
    };

    for( THIEVING_PATTERN pattern : patterns )
    {
        BOOST_TEST_CONTEXT( "pattern enum value " << static_cast<int>( pattern ) )
        {
            std::unique_ptr<BOARD> writeBoard = std::make_unique<BOARD>();

            ZONE* zone = new ZONE( writeBoard.get() );
            zone->SetLayer( F_Cu );
            zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
            zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ), -1 );
            zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ), -1 );
            zone->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 5 ) ), -1 );
            zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );

            THIEVING_SETTINGS thieving = zone->GetThievingSettings();
            thieving.pattern = pattern;
            zone->SetThievingSettings( thieving );

            writeBoard->Add( zone );

            std::filesystem::path tmpPath = std::filesystem::temp_directory_path()
                                            / "copper_thieving_pattern.kicad_pcb";
            PCB_IO_KICAD_SEXPR writer;
            writer.SaveBoard( tmpPath.string(), writeBoard.get() );

            std::unique_ptr<BOARD> readBoard = std::make_unique<BOARD>();
            PCB_IO_KICAD_SEXPR    reader;
            reader.LoadBoard( tmpPath.string(), readBoard.get() );

            BOOST_REQUIRE_EQUAL( readBoard->Zones().size(), 1u );
            BOOST_CHECK( readBoard->Zones()[0]->GetThievingSettings().pattern == pattern );

            std::filesystem::remove( tmpPath );
        }
    }
}


/**
 * Loading a .kicad_pcb that declares an old format version must reject
 * (mode thieving) and (thieving ...) tokens.  Otherwise a misversioned file
 * could load on this build but silently corrupt on an older reader because
 * the format never went through a proper transition.
 */
BOOST_AUTO_TEST_CASE( CopperThievingZone_RejectedInOldFileVersion )
{
    std::filesystem::path tmpPath = std::filesystem::temp_directory_path()
                                    / "copper_thieving_old_version.kicad_pcb";
    std::ofstream out( tmpPath );
    // 20260512 is the Net Chains release; thieving needs >= 20260513.  Using the
    // immediately-prior version exercises the boundary check.
    out << "(kicad_pcb (version 20260512) (generator \"test\") (generator_version \"test\")"
        << " (general (thickness 1.6)) (paper \"A4\")"
        << " (layers (0 \"F.Cu\" signal) (31 \"B.Cu\" signal))"
        << " (zone (net 0) (net_name \"\") (layer \"F.Cu\") (uuid \"00000000-0000-0000-0000-000000000001\")"
        << "       (hatch edge 0.5)"
        << "       (connect_pads (clearance 0))"
        << "       (min_thickness 0.25) (filled_areas_thickness no)"
        << "       (fill yes (mode thieving) (thermal_gap 0.5) (thermal_bridge_width 0.5)"
        << "             (island_removal_mode 0))"
        << "       (polygon (pts (xy 0 0) (xy 5 0) (xy 5 5) (xy 0 5))))"
        << ")";
    out.close();

    std::unique_ptr<BOARD> readBoard = std::make_unique<BOARD>();
    PCB_IO_KICAD_SEXPR    reader;
    BOOST_CHECK_THROW( reader.LoadBoard( tmpPath.string(), readBoard.get() ), IO_ERROR );

    std::filesystem::remove( tmpPath );
}


/**
 * The parser must reject non-positive size / gap / width values inline so a
 * hand-edited or corrupted file cannot leave the zone in a state that would
 * deadlock the filler.  Verifies the inline clamp keeps the constructor
 * defaults for any malformed field.
 */
BOOST_AUTO_TEST_CASE( CopperThievingZone_RejectsMalformedGeometry )
{
    std::filesystem::path tmpPath = std::filesystem::temp_directory_path()
                                    / "copper_thieving_malformed.kicad_pcb";
    std::ofstream out( tmpPath );
    out << "(kicad_pcb (version 20260513) (generator \"test\") (generator_version \"test\")"
        << " (general (thickness 1.6)) (paper \"A4\")"
        << " (layers (0 \"F.Cu\" signal) (31 \"B.Cu\" signal))"
        << " (zone (net 0) (net_name \"\") (layer \"F.Cu\") (uuid \"00000000-0000-0000-0000-000000000002\")"
        << "       (hatch edge 0.5)"
        << "       (connect_pads (clearance 0))"
        << "       (min_thickness 0.25) (filled_areas_thickness no)"
        << "       (fill yes (mode thieving)"
        << "             (thermal_gap 0.5) (thermal_bridge_width 0.5)"
        << "             (island_removal_mode 0)"
        << "             (thieving (type dots) (size -1) (gap 0)"
        << "                       (width -5) (stagger no) (orientation 0)))"
        << "       (polygon (pts (xy 0 0) (xy 5 0) (xy 5 5) (xy 0 5))))"
        << ")";
    out.close();

    std::unique_ptr<BOARD> readBoard = std::make_unique<BOARD>();
    PCB_IO_KICAD_SEXPR    reader;
    reader.LoadBoard( tmpPath.string(), readBoard.get() );

    BOOST_REQUIRE_EQUAL( readBoard->Zones().size(), 1u );

    const THIEVING_SETTINGS& loaded = readBoard->Zones()[0]->GetThievingSettings();
    BOOST_CHECK_GT( loaded.element_size, 0 );
    BOOST_CHECK_GT( loaded.gap, 0 );
    BOOST_CHECK_GT( loaded.line_width, 0 );

    std::filesystem::remove( tmpPath );
}


BOOST_AUTO_TEST_SUITE_END()
