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
 * @file test_altium_pcblib_import.cpp
 * Test suite for import of *.PcbLib libraries
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <pcbnew/pcb_io/altium/pcb_io_altium_designer.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>


struct ALTIUM_PCBLIB_IMPORT_FIXTURE
{
    ALTIUM_PCBLIB_IMPORT_FIXTURE() {}

    PCB_IO_ALTIUM_DESIGNER altiumPlugin;
    PCB_IO_KICAD_SEXPR     kicadPlugin;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumPcbLibImport, ALTIUM_PCBLIB_IMPORT_FIXTURE )

static const std::vector<std::tuple<wxString, wxString>> altium_to_kicad_footprint_property = {
    { "Tracks.v5.PcbLib", "Tracks.pretty" },
    { "Tracks.v6.PcbLib", "Tracks.pretty" },
    { "Espressif ESP32-WROOM-32.PcbLib", "Espressif ESP32-WROOM-32.pretty" }
};

/**
 * Compare all footprints declared in a *.PcbLib file with their KiCad reference footprint
 */
BOOST_DATA_TEST_CASE( AltiumPcbLibImport2,
                      boost::unit_test::data::make(altium_to_kicad_footprint_property),
                      altiumLibraryName,
                      kicadLibraryName )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/pcblib/";

    wxString altiumLibraryPath = dataPath + altiumLibraryName;
    wxString kicadLibraryPath = dataPath + kicadLibraryName;

    wxArrayString altiumFootprintNames;
    wxArrayString kicadFootprintNames;

    altiumPlugin.FootprintEnumerate( altiumFootprintNames, altiumLibraryPath, true, nullptr );
    kicadPlugin.FootprintEnumerate( kicadFootprintNames, kicadLibraryPath, true, nullptr );

    BOOST_CHECK_EQUAL( altiumFootprintNames.GetCount(), kicadFootprintNames.GetCount() );

    for( size_t i = 0; i < altiumFootprintNames.GetCount(); i++ )
    {
        wxString footprintName = altiumFootprintNames[i];

        BOOST_TEST_CONTEXT( wxString::Format( wxT( "Import '%s' from '%s'" ), footprintName,
                                              altiumLibraryName ) )
        {
            FOOTPRINT*  altiumFp = altiumPlugin.FootprintLoad( altiumLibraryPath, footprintName,
                                                              false, nullptr );
            BOOST_CHECK( altiumFp );

            BOOST_CHECK_EQUAL( "REF**", altiumFp->GetReference() );
            BOOST_CHECK_EQUAL( footprintName, altiumFp->GetValue() );

            FOOTPRINT* kicadFp = kicadPlugin.FootprintLoad( kicadLibraryPath, footprintName,
                                                            true, nullptr );
            BOOST_CHECK( kicadFp );

            if( !kicadFp )
                continue;

            KI_TEST::CheckFootprint( kicadFp, altiumFp );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
