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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/ipc2581/pcb_io_ipc2581.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <lset.h>
#include <base_units.h>
#include <core/mirror.h>

#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( Ipc2581Io )


/**
 * Test that flipped footprint rotations are exported to IPC-2581 with the board-top
 * adjustment applied.
 *
 * Per IPC-2581 the Component rotation is viewed from the board top while mirror="true"
 * accounts for the bottom placement, so a flipped footprint at board orientation A is
 * exported as ( -A - 180 ) normalized into [0, 360). A footprint flipped with no rotation
 * therefore exports rotation="180.0", and one at 30 degrees exports rotation="150.0".
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/18013. A 30 degree
 * angle is used deliberately because 90 and 270 are fixed points of the transform and so
 * cannot distinguish the corrected formula from the unadjusted one.
 */
BOOST_AUTO_TEST_CASE( Issue18013_FlippedFootprintRotation )
{
    BOARD board;

    auto addFootprint =
            [&]( const wxString& aRef, const VECTOR2I& aPos, bool aFlip ) -> FOOTPRINT*
            {
                FOOTPRINT* fp = new FOOTPRINT( &board );
                fp->SetReference( aRef );
                fp->SetPosition( aPos );
                board.Add( fp );

                PAD* pad = new PAD( fp );
                pad->SetNumber( wxT( "1" ) );
                pad->SetAttribute( PAD_ATTRIB::SMD );
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
                pad->SetSize( PADSTACK::ALL_LAYERS,
                              VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
                pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
                fp->Add( pad );

                // Flip first, then set the absolute board orientation so GetOrientation()
                // is deterministic regardless of any rotation the flip itself applies.
                if( aFlip )
                    fp->Flip( fp->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

                fp->SetOrientationDegrees( 30 );

                return fp;
            };

    FOOTPRINT* flipped = addFootprint( wxT( "U1" ),
                                       VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ),
                                       true );
    FOOTPRINT* top = addFootprint( wxT( "U2" ),
                                   VECTOR2I( pcbIUScale.mmToIU( 60 ), pcbIUScale.mmToIU( 60 ) ),
                                   false );

    BOOST_REQUIRE( flipped->IsFlipped() );
    BOOST_REQUIRE( !top->IsFlipped() );

    PCB_IO_IPC2581 ipc2581Plugin;

    wxString tempPath = wxFileName::CreateTempFileName( wxT( "kicad_ipc2581_test" ) );

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    BOOST_REQUIRE_NO_THROW( ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::ifstream xmlFile( tempPath.ToStdString() );
    BOOST_REQUIRE( xmlFile.is_open() );

    std::string xml( ( std::istreambuf_iterator<char>( xmlFile ) ),
                     std::istreambuf_iterator<char>() );
    xmlFile.close();

    // Return the <Component>...</Component> block for a given refDes so the checks below
    // cannot bleed across into a sibling component.
    auto componentRegion =
            [&]( const std::string& aRef ) -> std::string
            {
                size_t start = xml.find( "refDes=\"" + aRef + "\"" );

                if( start == std::string::npos )
                    return std::string();

                size_t end = xml.find( "</Component>", start );

                return xml.substr( start, end == std::string::npos ? std::string::npos
                                                                    : end - start );
            };

    std::string u1 = componentRegion( "U1" );
    BOOST_REQUIRE_MESSAGE( !u1.empty(), "Flipped component U1 should be exported" );

    BOOST_CHECK_MESSAGE( u1.find( "mirror=\"true\"" ) != std::string::npos,
                         "Flipped component must carry mirror=\"true\". Region: " + u1 );
    BOOST_CHECK_MESSAGE( u1.find( "rotation=\"150.0\"" ) != std::string::npos,
                         "Flipped 30 degree component must export rotation=\"150.0\". Region: "
                                 + u1 );

    // Guard against the two known incorrect formulas so the test fails on regression.
    BOOST_CHECK_MESSAGE( u1.find( "rotation=\"330.0\"" ) == std::string::npos,
                         "Flipped rotation must not be a bare Invert() (330.0). Region: " + u1 );
    BOOST_CHECK_MESSAGE( u1.find( "rotation=\"30.0\"" ) == std::string::npos,
                         "Flipped rotation must not be the unadjusted orientation (30.0). Region: "
                                 + u1 );

    std::string u2 = componentRegion( "U2" );
    BOOST_REQUIRE_MESSAGE( !u2.empty(), "Top component U2 should be exported" );

    BOOST_CHECK_MESSAGE( u2.find( "mirror=\"true\"" ) == std::string::npos,
                         "Top component must not be mirrored. Region: " + u2 );
    BOOST_CHECK_MESSAGE( u2.find( "rotation=\"30.0\"" ) != std::string::npos,
                         "Top 30 degree component must export rotation=\"30.0\". Region: " + u2 );

    std::filesystem::remove( std::filesystem::path( tempPath.ToStdString() ) );
}


BOOST_AUTO_TEST_SUITE_END()
