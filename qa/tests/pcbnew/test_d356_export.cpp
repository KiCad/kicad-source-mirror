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

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <board.h>
#include <exporters/export_d356.h>
#include <pcb_track.h>
#include <pcbnew_utils/board_file_utils.h>

#include <wx/string.h>


namespace
{
std::string trim( const std::string& aStr )
{
    const size_t begin = aStr.find_first_not_of( ' ' );

    if( begin == std::string::npos )
        return std::string();

    const size_t end = aStr.find_last_not_of( ' ' );

    return aStr.substr( begin, end - begin + 1 );
}
} // namespace


/*
 * IPC-D-356A column 32 carries the midpoint flag ('M' = a mid-net access point,
 * blank = an end-net point / component terminal). Vias and unnamed copper features
 * are mid-net; named component pins are end-net. The fixture exercises all three.
 */
BOOST_AUTO_TEST_CASE( ExportD356MidpointFlag )
{
    const std::filesystem::path boardPath =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / "issue3812.kicad_pcb";

    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( boardPath.string() );
    BOOST_REQUIRE( board );

    const std::filesystem::path outputPath =
            std::filesystem::temp_directory_path() / "kicad_d356_midpoint_test.d356";

    IPC356D_WRITER writer( board.get() );
    BOOST_REQUIRE( writer.Write( wxString::FromUTF8( outputPath.string().c_str() ) ) );

    std::ifstream in( outputPath );
    BOOST_REQUIRE( in.is_open() );

    // The soldermask field encodes which sides are tented; derive the expectation from the
    // board's vias (the fixture tents them uniformly) rather than hardcoding a value.
    int expectedViaMask = -1;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );
        const int mask = ( via->IsTented( F_Mask ) ? 1 : 0 ) | ( via->IsTented( B_Mask ) ? 2 : 0 );

        if( expectedViaMask < 0 )
            expectedViaMask = mask;
        else
            BOOST_REQUIRE_EQUAL( mask, expectedViaMask );
    }

    std::string line;
    int         namedPads = 0;
    int         unnamedPads = 0;
    int         vias = 0;

    while( std::getline( in, line ) )
    {
        // Only 3xx test-point records carry the midpoint flag in column 32
        if( line.size() < 32 || line[0] != '3' )
            continue;

        const std::string refdes = trim( line.substr( 20, 6 ) );
        const std::string pin = trim( line.substr( 27, 4 ) );
        const char        midpoint = line[31];

        if( refdes == "VIA" )
        {
            ++vias;
            BOOST_CHECK_EQUAL( midpoint, 'M' );

            // 'S' cannot occur in the fixed-format fields after column 32, so the last one
            // starts the soldermask field
            const size_t sPos = line.rfind( 'S' );
            BOOST_REQUIRE( sPos != std::string::npos && sPos > 31 );
            BOOST_CHECK_EQUAL( std::stoi( line.substr( sPos + 1 ) ), expectedViaMask );
        }
        else if( pin.empty() )
        {
            ++unnamedPads;
            BOOST_CHECK_EQUAL( midpoint, 'M' );
        }
        else
        {
            ++namedPads;
            BOOST_CHECK_EQUAL( midpoint, ' ' );
        }
    }

    // Guard against a silently trivial pass by requiring each record class
    BOOST_CHECK_GT( namedPads, 0 );
    BOOST_CHECK_GT( unnamedPads, 0 );
    BOOST_CHECK_GT( vias, 0 );

    std::filesystem::remove( outputPath );
}
