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

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/geda/pcb_io_geda.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <netinfo.h>
#include <zone.h>


struct GEDA_PCB_IMPORT_FIXTURE
{
    GEDA_PCB_IMPORT_FIXTURE() {}

    PCB_IO_GEDA m_plugin;
};


BOOST_FIXTURE_TEST_SUITE( GedaPcbImport, GEDA_PCB_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( CanReadBoard )
{
    std::string goodPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";
    BOOST_CHECK( m_plugin.CanReadBoard( goodPath ) );
}


BOOST_AUTO_TEST_CASE( MinimalBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // The minimal test board has 2 elements (R1, C1)
    BOOST_CHECK_EQUAL( board->Footprints().size(), 2 );

    // Check that reference designators were imported
    bool foundR1 = false;
    bool foundC1 = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == wxT( "R1" ) )
            foundR1 = true;

        if( fp->GetReference() == wxT( "C1" ) )
            foundC1 = true;
    }

    BOOST_CHECK_MESSAGE( foundR1, "R1 footprint not found" );
    BOOST_CHECK_MESSAGE( foundC1, "C1 footprint not found" );
}


BOOST_AUTO_TEST_CASE( MinimalBoardPads )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Both R1 and C1 should have 2 pads each
    for( FOOTPRINT* fp : board->Footprints() )
    {
        BOOST_CHECK_MESSAGE( fp->Pads().size() == 2,
                             wxString::Format( "%s should have 2 pads, has %zu",
                                               fp->GetReference(),
                                               fp->Pads().size() ) );
    }
}


BOOST_AUTO_TEST_CASE( MinimalBoardNetlist )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // The minimal test board defines nets in a NetList section.
    // Verify that at least some nets were imported.
    BOOST_CHECK( board->GetNetCount() > 1 );
}


BOOST_AUTO_TEST_CASE( MinimalBoardTracks )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // The minimal test has traces on the copper layer
    BOOST_CHECK( board->Tracks().size() > 0 );
}


BOOST_AUTO_TEST_CASE( MinimalBoardVias )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Count vias among the tracks
    int viaCount = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            viaCount++;
    }

    BOOST_CHECK( viaCount > 0 );
}


BOOST_AUTO_TEST_CASE( MinimalBoardCopperLayers )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Should have at least 2 copper layers
    BOOST_CHECK( board->GetCopperLayerCount() >= 2 );
}


BOOST_AUTO_TEST_CASE( CachedLibraryFootprints )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    std::vector<FOOTPRINT*> cached = m_plugin.GetImportedCachedLibraryFootprints();
    BOOST_CHECK_EQUAL( cached.size(), board->Footprints().size() );

    for( FOOTPRINT* fp : cached )
        delete fp;
}


BOOST_AUTO_TEST_CASE( RealWorldBoardLoad )
{
    // Test with a real-world gEDA board file
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/powermeter.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
    BOOST_CHECK( board->Tracks().size() > 0 );
}


// ============================================================================
// File discrimination tests
// ============================================================================

BOOST_AUTO_TEST_CASE( RejectsNonGedaPcbFile )
{
    std::string badPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/non_geda.pcb";
    BOOST_CHECK( !m_plugin.CanReadBoard( badPath ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonPcbExtension )
{
    std::string txtPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";
    wxFileName  fn( txtPath );
    fn.SetExt( wxT( "txt" ) );
    BOOST_CHECK( !m_plugin.CanReadBoard( fn.GetFullPath() ) );
}


// ============================================================================
// Element onsolder flag (bottom-side components)
// ============================================================================

BOOST_AUTO_TEST_CASE( OnsolderElementFlippedToBack )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/onsolder_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK_EQUAL( board->Footprints().size(), 2 );

    FOOTPRINT* r1 = nullptr;
    FOOTPRINT* c1 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == wxT( "R1" ) )
            r1 = fp;
        else if( fp->GetReference() == wxT( "C1" ) )
            c1 = fp;
    }

    BOOST_REQUIRE_MESSAGE( r1, "R1 footprint not found" );
    BOOST_REQUIRE_MESSAGE( c1, "C1 footprint not found" );

    // R1 has no onsolder flag, should be on front
    BOOST_CHECK_MESSAGE( !r1->IsFlipped(), "R1 should be on the front side" );

    // C1 has onsolder flag, should be flipped to back
    BOOST_CHECK_MESSAGE( c1->IsFlipped(), "C1 (onsolder) should be flipped to the back side" );
}


// ============================================================================
// Multi-layer board with through-hole components
// ============================================================================

BOOST_AUTO_TEST_CASE( MultilayerBoardCopperCount )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/multilayer_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Groups("1,c:2,s:3:4") defines 4 copper layers
    BOOST_CHECK_EQUAL( board->GetCopperLayerCount(), 4 );
}


BOOST_AUTO_TEST_CASE( MultilayerBoardThroughHolePins )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/multilayer_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() != wxT( "U1" ) )
            continue;

        BOOST_CHECK_EQUAL( fp->Pads().size(), 8 );

        for( PAD* pad : fp->Pads() )
        {
            BOOST_CHECK_MESSAGE( pad->GetAttribute() == PAD_ATTRIB::PTH,
                                 "DIP8 pad " + pad->GetNumber() + " should be PTH" );
        }

        return;
    }

    BOOST_FAIL( "U1 footprint not found" );
}


BOOST_AUTO_TEST_CASE( MultilayerBoardNetAssignment )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/multilayer_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    bool foundGND = false;
    bool foundVCC = false;

    for( int i = 0; i < board->GetNetCount(); i++ )
    {
        NETINFO_ITEM* net = board->GetNetInfo().GetNetItem( i );

        if( !net )
            continue;

        if( net->GetNetname() == wxT( "GND" ) )
            foundGND = true;

        if( net->GetNetname() == wxT( "VCC" ) )
            foundVCC = true;
    }

    BOOST_CHECK_MESSAGE( foundGND, "Net GND not found" );
    BOOST_CHECK_MESSAGE( foundVCC, "Net VCC not found" );
}


BOOST_AUTO_TEST_CASE( MultilayerBoardVias )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/multilayer_test.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    int viaCount = 0;
    int trackCount = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackCount++;
    }

    BOOST_CHECK_EQUAL( viaCount, 1 );
    BOOST_CHECK_EQUAL( trackCount, 4 );
}


// ============================================================================
// Real-world boards
// ============================================================================

BOOST_AUTO_TEST_CASE( GoodfetBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/goodfet50.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 5 );
    BOOST_CHECK( board->Tracks().size() > 10 );
}


BOOST_AUTO_TEST_CASE( Scsi2sdBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/scsi2sd.pcb";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 10 );
    BOOST_CHECK( board->Tracks().size() > 50 );
}


BOOST_AUTO_TEST_SUITE_END()
