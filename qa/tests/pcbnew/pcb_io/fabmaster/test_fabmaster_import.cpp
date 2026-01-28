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


BOOST_AUTO_TEST_SUITE_END()
