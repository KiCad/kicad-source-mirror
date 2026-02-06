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
 * @file test_diptrace_cross_link.cpp
 * Cross-linking validation for DipTrace board (.dip) imports.
 *
 * Loads the Z80 Board project PCB and verifies that the imported board has
 * the reference designators, nets, and pad-net assignments that would be
 * needed for successful cross-linking with the matching schematic.
 *
 * The companion schematic file (z80_board.dch) is verified to have a valid
 * DipTrace schematic header, confirming the paired test data is present.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/diptrace/pcb_io_diptrace.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <netinfo.h>

#include <cstring>
#include <fstream>
#include <set>


struct DIPTRACE_CROSS_LINK_FIXTURE
{
    DIPTRACE_CROSS_LINK_FIXTURE() {}

    PCB_IO_DIPTRACE m_pcbPlugin;

    std::string GetPcbTestDataDir()
    {
        return KI_TEST::GetPcbnewTestDataDir() + "plugins/diptrace/";
    }

    /**
     * Check the first 8 bytes of a file for the DipTrace schematic magic:
     * 0x07 "DTSCHEM"
     */
    bool HasDipTraceSchematicHeader( const std::string& aFilePath )
    {
        std::ifstream file( aFilePath, std::ios::binary );

        if( !file.is_open() )
            return false;

        uint8_t header[8];
        file.read( reinterpret_cast<char*>( header ), 8 );

        if( file.gcount() < 8 )
            return false;

        static const uint8_t expected[] = { 0x07, 'D', 'T', 'S', 'C', 'H', 'E', 'M' };

        return std::memcmp( header, expected, 8 ) == 0;
    }
};


BOOST_FIXTURE_TEST_SUITE( DipTraceCrossLink, DIPTRACE_CROSS_LINK_FIXTURE )


/**
 * Verify that the paired Z80 Board schematic file (.dch) has a valid DipTrace
 * schematic header.  This confirms the companion test data is present and is
 * a genuine DipTrace schematic file.
 */
BOOST_AUTO_TEST_CASE( SchematicHeaderValid )
{
    // The paired schematic lives in the eeschema test data tree
    std::string schPath = KI_TEST::GetPcbnewTestDataDir()
                          + "../eeschema/plugins/diptrace/z80_board.dch";

    BOOST_CHECK( HasDipTraceSchematicHeader( schPath ) );
}


/**
 * Load the Z80 Board PCB and verify that reference designators were imported.
 * A board with reference designators is a prerequisite for board-schematic
 * cross-linking.
 */
BOOST_AUTO_TEST_CASE( BoardRefDesPresent )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_pcbPlugin.LoadBoard( GetPcbTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    std::set<wxString> refDesSet;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        wxString ref = fp->GetReference();

        if( !ref.IsEmpty() )
            refDesSet.insert( ref );
    }

    // The Z80 board should have a substantial number of unique reference designators
    BOOST_CHECK_GT( refDesSet.size(), 10 );

    // Verify that reference designators follow expected patterns (Uxx, Rxx, Cxx, etc.)
    bool hasIC = false;

    for( const wxString& ref : refDesSet )
    {
        if( ref.StartsWith( "U" ) || ref.StartsWith( "IC" ) || ref.StartsWith( "DD" ) )
        {
            hasIC = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( hasIC,
                         "Z80 board should have at least one IC/U-prefixed reference" );
}


/**
 * Load the Z80 Board PCB and verify that nets were imported with meaningful
 * names.  Named nets indicate that the netlist data from DipTrace was
 * successfully parsed.
 */
BOOST_AUTO_TEST_CASE( BoardNetsPresent )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_pcbPlugin.LoadBoard( GetPcbTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    std::set<wxString> netNames;

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net->GetNetCode() > 0 )
            netNames.insert( net->GetNetname() );
    }

    // Z80 board has ~97 named nets (address bus, data bus, control signals, etc.)
    BOOST_CHECK_GT( netNames.size(), 20 );

    // Verify that known Z80-board net names are present
    BOOST_CHECK( netNames.count( wxT( "GND" ) ) > 0 );
    BOOST_CHECK( netNames.count( wxT( "A0" ) ) > 0 );
    BOOST_CHECK( netNames.count( wxT( "D0" ) ) > 0 );
}


/**
 * Load the Z80 Board PCB and verify that pads were created on footprints.
 * The Z80 board contains ICs with 14-40 pins as well as discrete 2-pin parts.
 */
BOOST_AUTO_TEST_CASE( PadsParsed )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_pcbPlugin.LoadBoard( GetPcbTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );
    BOOST_CHECK_GT( board->Footprints().size(), 10 );

    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
        totalPads += static_cast<int>( fp->Pads().size() );

    BOOST_CHECK_GT( totalPads, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
