/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file test_cadstar_footprints.cpp
 * Test suite for import of cadstar *.cpa footprints files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/cadstar/pcb_io_cadstar_archive.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <footprint.h>
#include <pad.h>
#include <zone.h>


struct CADSTAR_IMPORT_FIXTURE
{
    CADSTAR_IMPORT_FIXTURE() {}

    PCB_IO_CADSTAR_ARCHIVE cstarPlugin;
    PCB_IO_KICAD_SEXPR     kicadPlugin;
};


BOOST_FIXTURE_TEST_SUITE( CadstarFootprintsImport, CADSTAR_IMPORT_FIXTURE )


/**
 * Compare all footprints with their KiCad reference footprint
 * TODO: Refactor this code so it can be made common to all importers!
 * (right now this is copy/paste from EAGLE)
 */
BOOST_AUTO_TEST_CASE( CadstarFootprintImport )
{
    std::vector<std::pair<wxString, wxString>> tests = {
        { "footprint-with-thermal-pad.cpa", "footprint-with-thermal-pad.pretty" }
    };

    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/cadstar/lib/";

    for( const std::pair<wxString, wxString>& libName : tests )
    {
        wxString cstarLibraryPath = dataPath + libName.first;
        wxString kicadLibraryPath = dataPath + libName.second;

        wxArrayString cstarFootprintNames;
        wxArrayString kicadFootprintNames;

        BOOST_REQUIRE_NO_THROW(
            cstarPlugin.FootprintEnumerate( cstarFootprintNames, cstarLibraryPath, true, nullptr ) );
        BOOST_REQUIRE_NO_THROW(
            kicadPlugin.FootprintEnumerate( kicadFootprintNames, kicadLibraryPath, true, nullptr ) );

        BOOST_CHECK_EQUAL( cstarFootprintNames.GetCount(), kicadFootprintNames.GetCount() );

        for( size_t i = 0; i < cstarFootprintNames.GetCount(); i++ )
        {
            wxString footprintName = cstarFootprintNames[i];

            BOOST_TEST_CONTEXT( wxString::Format( wxT( "Import '%s' from '%s'" ),
                                                  footprintName,
                                                  libName.first ) )
            {
                FOOTPRINT* eagleFp = cstarPlugin.FootprintLoad( cstarLibraryPath, footprintName,
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
