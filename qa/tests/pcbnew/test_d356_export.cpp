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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

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


std::vector<std::string> exportD356( const std::string& aBoardFile )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream(
            ( std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / aBoardFile ).string() );
    BOOST_REQUIRE( board );

    const std::filesystem::path outputPath = std::filesystem::temp_directory_path() / ( aBoardFile + ".d356" );

    IPC356D_WRITER writer( board.get() );
    BOOST_REQUIRE( writer.Write( wxString::FromUTF8( outputPath.string().c_str() ) ) );

    std::ifstream            in( outputPath );
    std::vector<std::string> lines;

    for( std::string line; std::getline( in, line ); )
        lines.push_back( line );

    std::filesystem::remove( outputPath );
    return lines;
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


// A blind via is a 307 record with its layer span in columns 75-80 plus a 027 surface pad record
// The fixture's blind vias run from F.Cu to In2.Cu
BOOST_AUTO_TEST_CASE( ExportD356BlindVias )
{
    const std::vector<std::string> lines = exportD356( "issue10697.kicad_pcb" );

    BOOST_CHECK( std::any_of( lines.begin(), lines.end(),
                              []( const std::string& aLine )
                              {
                                  return aLine.rfind( "P  VER   IPC-D-356A ", 0 ) == 0;
                              } ) );

    int blindVias = 0;

    for( size_t ii = 0; ii < lines.size(); ++ii )
    {
        if( lines[ii].rfind( "307", 0 ) != 0 )
            continue;

        ++blindVias;
        BOOST_REQUIRE_EQUAL( lines[ii].size(), 80u );
        BOOST_CHECK_EQUAL( lines[ii].substr( 38, 3 ), "A01" );
        BOOST_CHECK_EQUAL( lines[ii].substr( 71, 2 ), " S" );
        BOOST_CHECK_EQUAL( lines[ii].substr( 74 ), "L01L03" );

        // 0.3175 mm surface pad is 125 decimils
        BOOST_REQUIRE_LT( ii + 1, lines.size() );
        BOOST_REQUIRE_EQUAL( lines[ii + 1].size(), 80u );
        BOOST_CHECK_EQUAL( lines[ii + 1].substr( 0, 17 ), "027" + lines[ii].substr( 3, 14 ) );
        BOOST_CHECK_EQUAL( lines[ii + 1].substr( 38, 3 ), "A01" );
        BOOST_CHECK_EQUAL( lines[ii + 1].substr( 57, 5 ), "X0125" );
    }

    BOOST_CHECK_EQUAL( blindVias, 7 );

    // Records are sorted by net, and names past 14 characters go through an NNAME alias
    const std::vector<std::string> longNames = exportD356( "issue3812.kicad_pcb" );
    std::vector<std::string>       nets;
    std::string                    alias;

    for( const std::string& line : longNames )
    {
        if( line[0] == '3' )
            nets.push_back( line.substr( 3, 14 ) );

        if( line.rfind( "P  NNAME", 0 ) == 0 && trim( line.substr( 14 ) ) == "/inout_user/CAN_H" )
            alias = line.substr( 8, 5 );
    }

    BOOST_CHECK( std::is_sorted( nets.begin(), nets.end() ) );

    BOOST_REQUIRE_EQUAL( alias.size(), 5u );
    BOOST_CHECK( std::any_of( longNames.begin(), longNames.end(),
                              [&]( const std::string& aLine )
                              {
                                  return aLine[0] == '3' && trim( aLine.substr( 3, 14 ) ) == alias;
                              } ) );
}
