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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include <fmt/std.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <io/kicad/kicad_io_utils.h>
#include <board.h>
#include <footprint.h>
#include <settings/settings_manager.h>


struct PRETTIFIER_TEST_FIXTURE
{
    PRETTIFIER_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
};


BOOST_FIXTURE_TEST_CASE( BoardAndFootprintPrettifier, PRETTIFIER_TEST_FIXTURE )
{
    std::vector<wxString> cases = {
        "Reverb_BTDR-1V.kicad_mod",
        "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal.kicad_mod",
        "group_and_image.kicad_pcb"
    };

    std::unique_ptr<BOARD_ITEM_CONTAINER> original, prettified, golden;
    PCB_IO_KICAD_SEXPR plugin;

    std::string tempLibPath = fmt::format( "{}/prettifier.pretty",
                                           std::filesystem::temp_directory_path() );
    std::filesystem::remove_all( tempLibPath );
    std::filesystem::create_directory( tempLibPath );

    for( const wxString& testCase : cases )
    {
        std::string testCaseName = testCase.ToStdString();

        BOOST_TEST_CONTEXT( testCaseName )
        {
            std::string inPath = fmt::format( "{}prettifier/{}", KI_TEST::GetPcbnewTestDataDir(),
                                              testCaseName );

            std::ifstream inFp;
            inFp.open( inPath );
            BOOST_REQUIRE( inFp.is_open() );

            std::stringstream inBuf;
            inBuf << inFp.rdbuf();
            std::string inData = inBuf.str();

            {
                STRING_LINE_READER        reader( inData, "input file" );
                PCB_IO_KICAD_SEXPR_PARSER parser( &reader, nullptr, nullptr );

                BOOST_CHECK_NO_THROW(
                        original.reset( dynamic_cast<BOARD_ITEM_CONTAINER*>( parser.Parse() ) ) );
                BOOST_REQUIRE( original.get() );
            }

            KICAD_FORMAT::Prettify( inData );

            // For diagnosis of test failures
            std::string tempPath = fmt::format( "{}/{}", tempLibPath, testCaseName );
            std::ofstream tempFp;
            tempFp.open( tempPath );
            BOOST_REQUIRE( tempFp.is_open() );
            tempFp << inData;
            tempFp.close();

            {
                STRING_LINE_READER        reader( inData, "prettified file" );
                PCB_IO_KICAD_SEXPR_PARSER parser( &reader, nullptr, nullptr );

                BOOST_CHECK_NO_THROW(
                        prettified.reset( dynamic_cast<BOARD_ITEM_CONTAINER*>( parser.Parse() ) ) );
                BOOST_REQUIRE( prettified.get() );
            }

            // Hack around the fact that PAD::operator== compares footprint UUIDs, even though
            // these UUIDs cannot be preserved through a round-trip
            const_cast<KIID&>( prettified->m_Uuid ) = original->m_Uuid;

            // File should parse the same way
            BOOST_REQUIRE_MESSAGE( *original == *prettified,
                    "Formatted version of original board item does not parse the same way!" );

            // And the formatting should match the golden
            std::string base = testCase.BeforeLast( '.' ).ToStdString();
            std::string ext  = testCase.AfterLast( '.' ).ToStdString();

            std::string goldenPath = fmt::format( "{}prettifier/{}_formatted.{}",
                                                  KI_TEST::GetPcbnewTestDataDir(), base, ext );

            std::ifstream goldFp;
            goldFp.open( goldenPath );
            BOOST_REQUIRE( goldFp.is_open() );

            std::stringstream goldenBuf;
            goldenBuf << goldFp.rdbuf();

            BOOST_REQUIRE_MESSAGE( goldenBuf.str().compare( inData ) == 0,
                                   "Formatting result doesn't match golden!" );

             std::filesystem::remove( tempPath );
        }
    }

    std::filesystem::remove_all( tempLibPath );
}
