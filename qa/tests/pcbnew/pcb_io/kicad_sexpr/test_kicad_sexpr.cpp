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
#include <string>

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
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


BOOST_AUTO_TEST_SUITE_END()
