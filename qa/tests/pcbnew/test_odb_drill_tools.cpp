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

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/file_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <boost/test/unit_test.hpp>

#include <board.h>
#include <pad.h>
#include <settings/settings_manager.h>

#include "pcb_io/odbpp/odb_test_utils.h"

namespace fs = std::filesystem;

/**
 * One TOOLS record from an ODB++ drill layer "tools" file.
 */
struct ODB_TOOL_RECORD
{
    int    m_num = -1;
    double m_finishSize = 0.0;
    double m_drillSize = 0.0;
};


/**
 * Parse the TOOLS records out of an ODB++ "tools" file.
 *
 * The format is a flat sequence of "TOOLS {" blocks of KEY=VALUE lines.  Only the keys the
 * drill tool table is asserted on are read; the file's leading UNITS/THICKNESS/USER_PARAMS
 * lines share none of those names, so they fall through untouched.
 */
static std::vector<ODB_TOOL_RECORD> parseOdbTools( const fs::path& aToolsFile )
{
    std::vector<ODB_TOOL_RECORD> tools;
    std::ifstream                stream( aToolsFile );
    std::string                  line;

    auto value = []( const std::string& aLine ) -> std::string
    {
        return aLine.substr( aLine.find( '=' ) + 1 );
    };

    while( std::getline( stream, line ) )
    {
        const size_t start = line.find_first_not_of( " \t" );

        if( start == std::string::npos )
            continue;

        line = line.substr( start );

        if( line.rfind( "TOOLS", 0 ) == 0 )
            tools.emplace_back();
        else if( tools.empty() )
            continue;
        else if( line.rfind( "NUM=", 0 ) == 0 )
            tools.back().m_num = std::stoi( value( line ) );
        else if( line.rfind( "FINISH_SIZE=", 0 ) == 0 )
            tools.back().m_finishSize = std::stod( value( line ) );
        else if( line.rfind( "DRILL_SIZE=", 0 ) == 0 )
            tools.back().m_drillSize = std::stod( value( line ) );
    }

    return tools;
}


// The Tag-Connect footprint has repeated non-plated holes at three sizes
// This checks size conversion and duplicate bit numbering
static void checkNonPlatedDrillTools( const char* aUnits, double aOutputScale )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream(
            KI_TEST::GetPcbnewTestDataDir() + "issue14130.kicad_pcb" );

    BOOST_REQUIRE( board );

    std::set<double> expectedSizes;
    int              npthHoleCount = 0;

    for( const PAD* pad : board->GetPads() )
    {
        if( pad->GetAttribute() != PAD_ATTRIB::NPTH || !pad->HasHole() )
            continue;

        expectedSizes.insert( pcbIUScale.IUTomm( std::min( pad->GetDrillSizeX(), pad->GetDrillSizeY() ) )
                              * aOutputScale );
        npthHoleCount++;
    }

    // A fixture whose holes were all one size would not show a mis-numbered tool table
    BOOST_REQUIRE_GT( npthHoleCount, (int) expectedSizes.size() );
    BOOST_REQUIRE_GT( expectedSizes.size(), 1 );

    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad_qa_odb_drill_tools" ) );

    BOOST_REQUIRE_NO_THROW( ExportOdb( *board, tempDir.Path(), aUnits ) );

    fs::path toolsFile;

    for( const fs::directory_entry& entry : fs::recursive_directory_iterator( tempDir.Path() ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == "tools"
            && entry.path().parent_path().filename().string().find( "non-plated" )
                       != std::string::npos )
        {
            toolsFile = entry.path();
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( !toolsFile.empty(),
                           "ODB++ export produced no non-plated drill tools file" );

    std::ifstream unitsStream( toolsFile );
    std::string   unitsLine;
    BOOST_REQUIRE( static_cast<bool>( std::getline( unitsStream, unitsLine ) ) );
    BOOST_CHECK_EQUAL( unitsLine, std::string( "UNITS=" ) + ( std::string( aUnits ) == "mm" ? "MM" : "INCH" ) );

    const std::vector<ODB_TOOL_RECORD> tools = parseOdbTools( toolsFile );

    std::set<int>    seenNums;
    std::set<double> seenSizes;

    for( const ODB_TOOL_RECORD& tool : tools )
    {
        BOOST_CHECK_MESSAGE( seenNums.insert( tool.m_num ).second,
                             "Drill tool number " << tool.m_num << " is used twice" );

        BOOST_CHECK_EQUAL( tool.m_finishSize, tool.m_drillSize );
        seenSizes.insert( tool.m_drillSize );
    }

    BOOST_CHECK_EQUAL( tools.size(), expectedSizes.size() );
    BOOST_REQUIRE_EQUAL( seenSizes.size(), expectedSizes.size() );

    auto actual = seenSizes.begin();
    auto expected = expectedSizes.begin();

    while( actual != seenSizes.end() )
    {
        BOOST_CHECK_SMALL( std::abs( *actual - *expected ), 0.00001 );
        ++actual;
        ++expected;
    }
}


BOOST_AUTO_TEST_CASE( OdbNonPlatedDrillTools )
{
    checkNonPlatedDrillTools( "mm", 1000.0 );
}
