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
 * @file test_easyedapro_import.cpp
 * Test suite for import of EasyEDA Pro footprints
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/easyedapro/pcb_io_easyedapro.h>

#include <footprint.h>
#include <pad.h>


struct EASYEDAPRO_IMPORT_FIXTURE
{
    EASYEDAPRO_IMPORT_FIXTURE() {}

    PCB_IO_EASYEDAPRO plugin;
};


BOOST_FIXTURE_TEST_SUITE( EasyedaproImport, EASYEDAPRO_IMPORT_FIXTURE )


/**
 * Test importing a footprint with POLYGON-shaped pads (issue #20348)
 * EasyEDA Pro uses POLYGON shape type for complex custom pad shapes,
 * which was previously not handled.
 */
BOOST_AUTO_TEST_CASE( PolygonPadImport )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/easyedapro/PDFN-8_L3.2-W3.1-P0.65-LS3.4-BL-EP2.efoo";

    wxString fpName = wxS( "PDFN-8_L3.2-W3.1-P0.65-LS3.4-BL-EP2" );
    FOOTPRINT* fp = plugin.FootprintLoad( dataPath, fpName, false, nullptr );

    BOOST_REQUIRE( fp );

    // The footprint should have 10 pads: 8 numbered pads plus 2 polygon-shaped pads (9 and 10)
    BOOST_CHECK_EQUAL( fp->Pads().size(), 10 );

    // Find the polygon pads (pads 9 and 10 in the test file)
    PAD* pad9 = fp->FindPadByNumber( "9" );
    PAD* pad10 = fp->FindPadByNumber( "10" );

    BOOST_REQUIRE( pad9 );
    BOOST_REQUIRE( pad10 );

    // These pads should be custom shape (POLYGON in EasyEDA Pro becomes CUSTOM in KiCad)
    BOOST_CHECK_EQUAL( static_cast<int>( pad9->GetShape( PADSTACK::ALL_LAYERS ) ),
                       static_cast<int>( PAD_SHAPE::CUSTOM ) );
    BOOST_CHECK_EQUAL( static_cast<int>( pad10->GetShape( PADSTACK::ALL_LAYERS ) ),
                       static_cast<int>( PAD_SHAPE::CUSTOM ) );

    // Check that primitives were added to the custom pads
    BOOST_CHECK( !pad9->GetPrimitives( PADSTACK::ALL_LAYERS ).empty() );
    BOOST_CHECK( !pad10->GetPrimitives( PADSTACK::ALL_LAYERS ).empty() );

    delete fp;
}


BOOST_AUTO_TEST_SUITE_END()
