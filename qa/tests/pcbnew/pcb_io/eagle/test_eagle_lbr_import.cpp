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

/**
 * @file test_eagle_lbr_import.cpp
 * Test suite for import of *.lbr libraries
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/eagle/pcb_io_eagle.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <footprint.h>
#include <pad.h>
#include <zone.h>


struct EAGLE_LBR_IMPORT_FIXTURE
{
    EAGLE_LBR_IMPORT_FIXTURE() {}

    PCB_IO_EAGLE eaglePlugin;
    PCB_IO_KICAD_SEXPR   kicadPlugin;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( EagleLbrLibImport, EAGLE_LBR_IMPORT_FIXTURE )


/**
 * Compare all footprints declared in a *.lbr file with their KiCad reference footprint
 */
BOOST_AUTO_TEST_CASE( EagleLbrLibImport )
{
    // clang-format off
    std::vector<std::pair<wxString, wxString>> tests = {
        { "SparkFun-GPS.lbr", "SparkFun-GPS.pretty" }
    };
    // clang-format on

    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/eagle/lbr/";

    for( const std::pair<wxString, wxString>& libName : tests )
    {
        wxString eagleLibraryPath = dataPath + libName.first;
        wxString kicadLibraryPath = dataPath + libName.second;

        wxArrayString eagleFootprintNames;
        wxArrayString kicadFootprintNames;

        eaglePlugin.FootprintEnumerate( eagleFootprintNames, eagleLibraryPath, true, nullptr );
        kicadPlugin.FootprintEnumerate( kicadFootprintNames, kicadLibraryPath, true, nullptr );

        BOOST_CHECK_EQUAL( eagleFootprintNames.GetCount(), kicadFootprintNames.GetCount() );

        for( size_t i = 0; i < eagleFootprintNames.GetCount(); i++ )
        {
            wxString footprintName = eagleFootprintNames[i];

            BOOST_TEST_CONTEXT( wxString::Format( wxT( "Import '%s' from '%s'" ), footprintName,
                                                  libName.first ) )
            {
                FOOTPRINT* eagleFp = eaglePlugin.FootprintLoad( eagleLibraryPath, footprintName,
                                                                false, nullptr );
                BOOST_CHECK( eagleFp );

                BOOST_CHECK_EQUAL( "REF**", eagleFp->GetReference() );
                BOOST_CHECK_EQUAL( footprintName, eagleFp->GetValue() );

                FOOTPRINT* kicadFp = kicadPlugin.FootprintLoad( kicadLibraryPath, footprintName,
                                                                true, nullptr );
                BOOST_CHECK( kicadFp );

                KI_TEST::CheckFootprint( kicadFp, eagleFp );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
