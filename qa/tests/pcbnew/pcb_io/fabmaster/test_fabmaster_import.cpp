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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_fabmaster_import.cpp
 * Test suite for import of Fabmaster PCB files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/fabmaster/pcb_io_fabmaster.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>
#include <zone.h>


struct FABMASTER_IMPORT_FIXTURE
{
    FABMASTER_IMPORT_FIXTURE() {}

    PCB_IO_FABMASTER m_fabmasterPlugin;
};


BOOST_FIXTURE_TEST_SUITE( FabmasterImport, FABMASTER_IMPORT_FIXTURE )


/**
 * Test that footprints with pads are properly imported when the REFDES column
 * is empty in the PIN section.
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/7955
 *
 * When a Fabmaster file is exported from a board with only components and no netlist,
 * the REFDES column in the PIN section is empty. The importer should fall back to
 * using SYM_NAME to match pins with their footprints.
 */
BOOST_AUTO_TEST_CASE( EmptyRefdesInPins )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_only_2_comp.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    // The board should have footprints
    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    // Each footprint should have pads (before the fix, pads were missing because
    // the lookup by empty REFDES failed)
    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        totalPads += fp->Pads().size();
    }

    BOOST_CHECK_MESSAGE( totalPads > 0,
                         "Footprints should have pads when REFDES is empty in PIN section" );
}


/**
 * Test that static shapes (zones with net but no matching outline) are preserved.
 * This is a regression test for https://gitlab.com/kicad/code/kicad/-/issues/7753
 *
 * In Allegro, static shapes are copper areas that don't void around other nets.
 * They appear in Fabmaster as filled areas (with net) but without a corresponding outline
 * (which would have no net). The importer must preserve these as solid filled zones.
 *
 * The test file contains static shapes on nets like "DO" and "VMBATT3-" that are
 * zero-width closed polygons with net assignments but no matching outline.
 */
BOOST_AUTO_TEST_CASE( StaticShapesPreserved )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_14066-20316FBRD.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    // Count zones - static shapes should be preserved as zones with net codes
    int zonesWithNets = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetNetCode() > 0 )
            zonesWithNets++;
    }

    // The board should have some zones with nets assigned (static shapes).
    // Before the fix, all netted zones were deleted because they were considered
    // redundant fills. With the fix, unmatched static shapes are preserved.
    BOOST_CHECK_MESSAGE( zonesWithNets > 0,
                         "Static shape zones with nets should be preserved" );
}


/**
 * Helper to find a pad by its number within a footprint.
 */
static const PAD* findPadByNumber( const FOOTPRINT* aFp, const wxString& aNumber )
{
    for( const PAD* pad : aFp->Pads() )
    {
        if( pad->GetNumber() == aNumber )
            return pad;
    }

    return nullptr;
}


/**
 * Test that pad-stacks with different shapes on front and back copper layers
 * are imported using FRONT_INNER_BACK padstack mode.
 *
 * The test file contains:
 *   DIFF_PAD  - CIRCLE on TOP (2.0mm), RECTANGLE on BOTTOM (1.5x3.0mm) with drill
 *   SAME_PAD  - RECTANGLE on both TOP and BOTTOM (1.0x2.0mm) with drill
 *   SMD_DIFF  - OVAL on TOP only (0.8x1.6mm), SMD pad
 */
BOOST_AUTO_TEST_CASE( PadStackDifferentLayers )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_padstack_test.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );
    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    const FOOTPRINT* fp = board->Footprints().front();
    BOOST_REQUIRE_EQUAL( fp->Pads().size(), 3 );

    // Pin 1 uses DIFF_PAD which has CIRCLE on TOP and RECTANGLE on BOTTOM.
    // This should use FRONT_INNER_BACK padstack mode.
    const PAD* pad1 = findPadByNumber( fp, wxT( "1" ) );
    BOOST_REQUIRE( pad1 );
    BOOST_CHECK_EQUAL( static_cast<int>( pad1->Padstack().Mode() ),
                       static_cast<int>( PADSTACK::MODE::FRONT_INNER_BACK ) );
    BOOST_CHECK_EQUAL( static_cast<int>( pad1->GetShape( F_Cu ) ),
                       static_cast<int>( PAD_SHAPE::CIRCLE ) );
    BOOST_CHECK_EQUAL( static_cast<int>( pad1->GetShape( B_Cu ) ),
                       static_cast<int>( PAD_SHAPE::RECTANGLE ) );

    // Verify front layer size (2.0mm circle)
    VECTOR2I pad1_front_size = pad1->GetSize( F_Cu );
    BOOST_CHECK_GT( pad1_front_size.x, 0 );
    BOOST_CHECK_EQUAL( pad1_front_size.x, pad1_front_size.y );

    // Verify back layer size (1.5x3.0mm rectangle, height > width)
    VECTOR2I pad1_back_size = pad1->GetSize( B_Cu );
    BOOST_CHECK_GT( pad1_back_size.x, 0 );
    BOOST_CHECK_GT( pad1_back_size.y, pad1_back_size.x );

    // Verify drill is present
    BOOST_CHECK_EQUAL( static_cast<int>( pad1->GetAttribute() ),
                       static_cast<int>( PAD_ATTRIB::PTH ) );
    BOOST_CHECK_GT( pad1->GetDrillSizeX(), 0 );
}


/**
 * Test that pad-stacks with the same shape on all layers remain in NORMAL mode.
 */
BOOST_AUTO_TEST_CASE( PadStackSameLayers )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_padstack_test.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );
    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    const FOOTPRINT* fp = board->Footprints().front();

    // Pin 2 uses SAME_PAD which has RECTANGLE 1.0x2.0 on both TOP and BOTTOM.
    // This should stay in NORMAL mode since shapes are identical.
    const PAD* pad2 = findPadByNumber( fp, wxT( "2" ) );
    BOOST_REQUIRE( pad2 );
    BOOST_CHECK_EQUAL( static_cast<int>( pad2->Padstack().Mode() ),
                       static_cast<int>( PADSTACK::MODE::NORMAL ) );
    BOOST_CHECK_EQUAL( static_cast<int>( pad2->GetShape( F_Cu ) ),
                       static_cast<int>( PAD_SHAPE::RECTANGLE ) );
}


/**
 * Test SMD pads (top-only, no drill) are imported correctly.
 */
BOOST_AUTO_TEST_CASE( PadStackSmdPad )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_padstack_test.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );
    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    const FOOTPRINT* fp = board->Footprints().front();

    // Pin 3 uses SMD_DIFF which is an OVAL on TOP only (no BOTTOM pad, no drill).
    const PAD* pad3 = findPadByNumber( fp, wxT( "3" ) );
    BOOST_REQUIRE( pad3 );
    BOOST_CHECK_EQUAL( static_cast<int>( pad3->GetAttribute() ),
                       static_cast<int>( PAD_ATTRIB::SMD ) );
    BOOST_CHECK_EQUAL( static_cast<int>( pad3->GetShape( F_Cu ) ),
                       static_cast<int>( PAD_SHAPE::OVAL ) );
}


/**
 * Test that Allegro area "shapes" (keepouts, keepins and constraint regions) are imported
 * as KiCad rule areas rather than unconnected copper fill zones.
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/7732
 *
 * In Allegro these areas carry no net.  Importing them as copper fill zones silently
 * creates shorts.  Each Allegro area class maps to a distinct rule-area restriction:
 *   ROUTE KEEPOUT   -> no tracks, vias, pads or zone fills
 *   VIA KEEPOUT     -> no vias
 *   PACKAGE KEEPOUT -> no footprints
 *   ROUTE/PACKAGE KEEPIN and CONSTRAINT REGION -> named marker with no restrictions
 *     (KiCad has no equivalent, so the shape is preserved without producing copper)
 *
 * The test file mirrors the reporter's board and contains 5 route keepouts, 5 via keepouts,
 * 3 package keepouts, 1 route keepin, 1 package keepin and 8 constraint regions.
 */
BOOST_AUTO_TEST_CASE( AreaClassesImportedAsRuleAreas )
{
    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/fabmaster/cds2f_issue7732_shape_pads2.txt";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_fabmasterPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    int routeKeepouts = 0;
    int viaKeepouts = 0;
    int footprintKeepouts = 0;
    int markerAreas = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;

        if( zone->GetDoNotAllowFootprints() && !zone->GetDoNotAllowTracks()
            && !zone->GetDoNotAllowVias() )
        {
            footprintKeepouts++;
        }
        else if( zone->GetDoNotAllowTracks() && zone->GetDoNotAllowVias()
                 && zone->GetDoNotAllowPads() && zone->GetDoNotAllowZoneFills() )
        {
            routeKeepouts++;
        }
        else if( zone->GetDoNotAllowVias() && !zone->GetDoNotAllowTracks()
                 && !zone->GetDoNotAllowFootprints() )
        {
            viaKeepouts++;
        }
        else if( !zone->GetDoNotAllowTracks() && !zone->GetDoNotAllowVias()
                 && !zone->GetDoNotAllowPads() && !zone->GetDoNotAllowFootprints()
                 && !zone->GetDoNotAllowZoneFills() )
        {
            markerAreas++;
        }
    }

    BOOST_CHECK_EQUAL( routeKeepouts, 5 );
    BOOST_CHECK_EQUAL( viaKeepouts, 5 );
    BOOST_CHECK_EQUAL( footprintKeepouts, 3 );

    // 1 route keepin + 1 package keepin + 8 constraint regions
    BOOST_CHECK_EQUAL( markerAreas, 10 );
}


BOOST_AUTO_TEST_SUITE_END()
