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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_cadstar_footprints.cpp
 * Test suite for import of cadstar *.cpa footprints and board files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/cadstar/pcb_io_cadstar_archive.h>
#include <pcbnew/pcb_io/cadstar/cadstar_pcb_archive_parser.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <wx/log.h>


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


/**
 * Test that CADSTAR Revision 7 format files without ROUTEWIDTH nodes can be imported.
 * This tests the fix for GitLab issue #17783.
 */
BOOST_AUTO_TEST_CASE( CadstarRevision7FormatImport )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/cadstar/route_offset/";
    wxString    filePath = dataPath + "revision7_format_no_routewidth.cpa";

    BOARD* board = nullptr;

    BOOST_CHECK_NO_THROW( board = cstarPlugin.LoadBoard( filePath, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // The test file has 5 nets with routes
    std::vector<PCB_TRACK*> tracks;

    for( PCB_TRACK* track : board->Tracks() )
        tracks.push_back( track );

    // Should have imported some tracks
    BOOST_CHECK( tracks.size() > 0 );

    // All nets use route code W1 (OptimalWidth 100000 hundredth-micron = 1.0 mm). Track widths
    // are derived from that route code since the routes have no explicit ROUTEWIDTH nodes.
    const int expectedWidth = pcbIUScale.mmToIU( 1.0 );
    bool      foundRouteCodeWidth = false;

    for( PCB_TRACK* track : tracks )
    {
        // No track should be left at zero width by the missing-ROUTEWIDTH fallback
        BOOST_CHECK( track->GetWidth() > 0 );

        if( track->GetWidth() == expectedWidth )
            foundRouteCodeWidth = true;
    }

    // At least one track must take the route-code-derived width, proving the fallback was used
    BOOST_CHECK( foundRouteCodeWidth );

    delete board;
}


/**
 * A PADREASSIGN/VIAREASSIGN whose shape node is unknown (e.g. from a newer CADSTAR version)
 * must warn and continue rather than throwing, and must not report a (default-circle) shape so
 * the caller can skip the bogus reassignment instead of corrupting the pad geometry.
 */
BOOST_AUTO_TEST_CASE( UnknownReassignShapeIsSkipped )
{
    wxLogNull suppress;

    XNODE padReassign( wxXML_ELEMENT_NODE, wxT( "PADREASSIGN" ) );
    padReassign.AddAttribute( wxT( "attr0" ), wxT( "TOP" ) );
    padReassign.AddChild( new XNODE( wxXML_ELEMENT_NODE, wxT( "FUTURE_SHAPE" ) ) );

    CADSTAR_PCB_ARCHIVE_PARSER::PADREASSIGN          padParser;
    CADSTAR_PCB_ARCHIVE_PARSER::PARSER_CONTEXT       ctx;

    BOOST_CHECK_NO_THROW( padParser.Parse( &padReassign, &ctx ) );
    BOOST_CHECK( !padParser.HasShape );

    XNODE viaReassign( wxXML_ELEMENT_NODE, wxT( "VIAREASSIGN" ) );
    viaReassign.AddAttribute( wxT( "attr0" ), wxT( "TOP" ) );
    viaReassign.AddChild( new XNODE( wxXML_ELEMENT_NODE, wxT( "FUTURE_SHAPE" ) ) );

    CADSTAR_PCB_ARCHIVE_PARSER::VIAREASSIGN viaParser;

    BOOST_CHECK_NO_THROW( viaParser.Parse( &viaReassign, &ctx ) );
    BOOST_CHECK( !viaParser.HasShape );

    // An empty reassignment (no shape child at all) must not crash either
    XNODE emptyReassign( wxXML_ELEMENT_NODE, wxT( "PADREASSIGN" ) );
    emptyReassign.AddAttribute( wxT( "attr0" ), wxT( "TOP" ) );

    CADSTAR_PCB_ARCHIVE_PARSER::PADREASSIGN emptyParser;

    BOOST_CHECK_NO_THROW( emptyParser.Parse( &emptyReassign, &ctx ) );
    BOOST_CHECK( !emptyParser.HasShape );
}


BOOST_AUTO_TEST_SUITE_END()
