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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_eagle_binary_import.cpp
 * Test suite for import of pre-v6 binary Eagle *.brd board files.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/eagle/pcb_io_eagle.h>

#include <board.h>
#include <footprint.h>
#include <netinfo.h>
#include <pcb_track.h>

#include <wx/filename.h>


struct EAGLE_BINARY_IMPORT_FIXTURE
{
    EAGLE_BINARY_IMPORT_FIXTURE() {}

    BOARD* loadBoard( const std::string& aRelPath )
    {
        std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + aRelPath;

        if( !wxFileName::FileExists( dataPath ) )
        {
            BOOST_TEST_MESSAGE( "no real binary Eagle sample available at " + dataPath + "; load test skipped" );
            return nullptr;
        }

        PCB_IO_EAGLE eaglePlugin;

        // The binary format is identified by content, never by extension.
        BOOST_CHECK( eaglePlugin.CanReadBoard( dataPath ) );

        BOARD* board = nullptr;

        try
        {
            board = eaglePlugin.LoadBoard( dataPath, nullptr, nullptr );
        }
        catch( const IO_ERROR& e )
        {
            BOOST_FAIL( "IO_ERROR loading binary Eagle board: " + e.What().ToStdString() );
        }
        catch( const std::exception& e )
        {
            BOOST_FAIL( std::string( "Exception loading binary Eagle board: " ) + e.what() );
        }

        return board;
    }
};


BOOST_FIXTURE_TEST_SUITE( EagleBinaryImport, EAGLE_BINARY_IMPORT_FIXTURE )


/**
 * Load a v4/v5 binary board (magic 0x10 0x00) which also carries the trailing
 * free-text and DRC sections. This load is the smoke test: the binary stream is
 * decoded into a synthesized XML DOM and then walked by the shared XML loader.
 */
BOOST_AUTO_TEST_CASE( LoadBinaryV4V5 )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/blink1_b1a.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
    BOOST_CHECK_GT( board->GetNetInfo().GetNetCount(), 1u );
}


/**
 * Load a v3 binary board (magic 0x10 0x80). v3 files have no DRC or free-text
 * sections, so this exercises the graceful-fallback path where the trailing
 * sections are absent.
 */
BOOST_AUTO_TEST_CASE( LoadBinaryV3 )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/blink1_v1a.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
}


/**
 * Regression test for custom element attributes. The binary attribute record has
 * no name field, so the decoder once emitted nameless <attribute> nodes that the
 * shared XML reader rejected ("required attribute name is missing"). The decoder
 * now drops those unrecoverable nodes, so the board loads.
 */
BOOST_AUTO_TEST_CASE( LoadV3CustomAttributes )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/rocketgps.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
    BOOST_CHECK_GT( board->GetNetInfo().GetNetCount(), 1u );
}


/**
 * Regression test for unnamed (auto-generated) signals. Their empty net name
 * collided with the reserved unconnected net, so the net code requested for every
 * item routed on them was never registered, tripping the m_netinfo assertion in
 * BOARD_CONNECTED_ITEM::SetNetCode(). Unnamed nets now get a unique fallback name
 * and items take the net code the board actually assigned.
 */
BOOST_AUTO_TEST_CASE( LoadV3UnnamedSignals )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/boomchak.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
    BOOST_CHECK_GT( board->GetNetInfo().GetNetCount(), 1u );

    // Every routed item must resolve to a real net; an orphaned net code would
    // have aborted the load before this point.
    for( PCB_TRACK* track : board->Tracks() )
        BOOST_CHECK( track->GetNet() != nullptr );
}


/**
 * Regression test for degenerate (vertex-less) polygons. A package polygon with
 * no vertices dereferenced an empty vertex list in packagePolygon(); the loader
 * now skips polygons with fewer than three vertices, matching loadPolygon().
 */
BOOST_AUTO_TEST_CASE( LoadV4V5DegeneratePolygons )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/turnemoff.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
