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

#include <pcbnew/pcb_io/sprint_layout/pcb_io_sprint_layout.h>
#include <pcbnew/pcb_io/sprint_layout/sprint_layout_parser.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <netinfo.h>
#include <zone.h>


struct SPRINT_LAYOUT_IMPORT_FIXTURE
{
    SPRINT_LAYOUT_IMPORT_FIXTURE() {}

    PCB_IO_SPRINT_LAYOUT m_plugin;
};


BOOST_FIXTURE_TEST_SUITE( SprintLayoutImport, SPRINT_LAYOUT_IMPORT_FIXTURE )


// ============================================================================
// File discrimination tests
// ============================================================================

BOOST_AUTO_TEST_CASE( CanReadLay6File )
{
    std::string path = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";
    BOOST_CHECK( m_plugin.CanReadBoard( path ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonSprintLayoutFile )
{
    std::string kicadPath = KI_TEST::GetPcbnewTestDataDir() + "/io/geda/minimal_test.pcb";
    BOOST_CHECK( !m_plugin.CanReadBoard( kicadPath ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonExistentFile )
{
    BOOST_CHECK( !m_plugin.CanReadBoard( wxT( "/nonexistent/path/file.lay6" ) ) );
}


// ============================================================================
// Basic board loading tests
// ============================================================================

BOOST_AUTO_TEST_CASE( Gpio2nescBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( ReedDoorbellBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/cacazi-a8-zigbee_cr2032_1.2mm.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( MdbRs232BoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mdb-rs232.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( LoadBoardAppendToExisting )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    // Load first into a fresh board
    std::unique_ptr<BOARD> baseBoard( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( baseBoard );

    size_t originalFootprints = baseBoard->Footprints().size();
    size_t originalDrawings = baseBoard->Drawings().size();

    BOOST_REQUIRE( originalFootprints > 0 );

    // Load again, appending into the existing board
    PCB_IO_SPRINT_LAYOUT plugin2;
    BOARD* result = plugin2.LoadBoard( dataPath, baseBoard.get() );

    BOOST_CHECK_EQUAL( result, baseBoard.get() );
    BOOST_CHECK( baseBoard->Footprints().size() >= originalFootprints * 2 );
    BOOST_CHECK( baseBoard->Drawings().size() >= originalDrawings * 2 );
}


// ============================================================================
// Board outline tests
// ============================================================================

BOOST_AUTO_TEST_CASE( BoardHasOutline )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    int edgeCutsCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( shape && shape->GetLayer() == Edge_Cuts )
            edgeCutsCount++;
    }

    BOOST_CHECK_MESSAGE( edgeCutsCount > 0, "Board should have Edge.Cuts outline shapes" );
}


// ============================================================================
// Pad tests
// ============================================================================

BOOST_AUTO_TEST_CASE( PadsInsideBoardOutline )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Get the board bounding box from Edge.Cuts shapes
    BOX2I boardBox;
    bool  first = true;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( !shape || shape->GetLayer() != Edge_Cuts )
            continue;

        BOX2I shapeBox = shape->GetBoundingBox();

        if( first )
        {
            boardBox = shapeBox;
            first = false;
        }
        else
        {
            boardBox.Merge( shapeBox );
        }
    }

    if( first )
    {
        BOOST_TEST_MESSAGE( "No Edge.Cuts found, skipping pad containment test" );
        return;
    }

    // Expand for tolerance -- edge connector pads can extend past the board boundary
    boardBox.Inflate( pcbIUScale.mmToIU( 2.0 ) );

    int outsideCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !boardBox.Contains( pad->GetPosition() ) )
                outsideCount++;
        }
    }

    BOOST_CHECK_MESSAGE( outsideCount == 0,
                         wxString::Format( "%d pads found outside board outline", outsideCount ) );
}


// ============================================================================
// Component and layer tests
// ============================================================================

BOOST_AUTO_TEST_CASE( BoardHasCopperLayers )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->GetCopperLayerCount() >= 2 );
}


BOOST_AUTO_TEST_CASE( CachedLibraryFootprints )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    std::vector<FOOTPRINT*> cached = m_plugin.GetImportedCachedLibraryFootprints();

    // We should have at least some cached footprints
    BOOST_CHECK( cached.size() > 0 );

    for( FOOTPRINT* fp : cached )
        delete fp;
}


BOOST_AUTO_TEST_CASE( PadsHaveAttributes )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    int thtCount = 0;
    int smdCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                thtCount++;
            else if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                smdCount++;
        }
    }

    // This board should have at least some through-hole pads
    BOOST_CHECK_MESSAGE( thtCount > 0 || smdCount > 0,
                         "Board should have at least some pads" );
}


// ============================================================================
// Format-specific feature tests
// ============================================================================

BOOST_AUTO_TEST_CASE( PadPositionsHavePositiveY )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // After Y-flip conversion, all pads should have non-negative Y coordinates
    int negativeCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetPosition().y < 0 )
                negativeCount++;
        }
    }

    BOOST_CHECK_MESSAGE( negativeCount == 0,
                         wxString::Format( "%d pads have negative Y (Y-flip error)", negativeCount ) );
}


BOOST_AUTO_TEST_CASE( DrawingsExistOnCopperAndSilk )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "/io/sprint_layout/gpio2nesc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    int copperDrawings = 0;
    int silkDrawings = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_LAYER_ID layer = item->GetLayer();

        if( layer == F_Cu || layer == B_Cu || layer == In1_Cu || layer == In2_Cu )
            copperDrawings++;
        else if( layer == F_SilkS || layer == B_SilkS )
            silkDrawings++;
    }

    BOOST_CHECK_MESSAGE( copperDrawings > 0 || silkDrawings > 0,
                         "Board should have drawings on copper or silkscreen layers" );
}


// ============================================================================
// Multi-file consistency
// ============================================================================

BOOST_AUTO_TEST_CASE( AllTestFilesLoadWithoutCrash )
{
    std::vector<std::string> files = {
        "/io/sprint_layout/gpio2nesc.lay6",
        "/io/sprint_layout/cacazi-a8-zigbee_cr2032_1.2mm.lay6",
        "/io/sprint_layout/mdb-rs232.lay6",
        "/io/sprint_layout/mdb-master-rev2a.lay6",
        "/io/sprint_layout/smalldualrgb-withmask.lay6",
        "/io/sprint_layout/amiga2000-remake.lay6",
        "/io/sprint_layout/karpaty-rx-pcb1-bpf-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb2-rfamp-1st-mixer-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb3-vfo-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb5-buffer-freq-doubler-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb6-mainboard-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb7-power-supply-orig.lay6",
        "/io/sprint_layout/ku14194revb.lay6",
        "/io/sprint_layout/pcb100x40_v5.lay6",
        "/io/sprint_layout/tfcc.lay6",
        "/io/sprint_layout/12F629_SM.lay6",
    };

    for( const auto& file : files )
    {
        std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + file;

        BOOST_TEST_CONTEXT( "Loading " << file )
        {
            std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );
            BOOST_CHECK( board != nullptr );
        }
    }
}


// ============================================================================
// Complex board tests
// ============================================================================

BOOST_AUTO_TEST_CASE( MmJoy2BoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mmjoy2-74hc165.lay6";

    std::map<std::string, UTF8> props;
    props["pcb_id"] = "0";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr, &props ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );

    int padCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        padCount += static_cast<int>( fp->Pads().size() );

    BOOST_CHECK_MESSAGE( padCount > 0, "MMJoy2 board should have pads" );
}


BOOST_AUTO_TEST_CASE( SmallDualRgbBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/smalldualrgb-withmask.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( MdbMasterRev2aBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mdb-master-rev2a.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
    BOOST_CHECK( board->GetCopperLayerCount() >= 2 );
}


// ============================================================================
// Karpaty-RX HAM receiver boards (6-board set from github.com/UT8IFG/karpaty-rx)
// ============================================================================

BOOST_AUTO_TEST_CASE( KarpatyBpfBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb1-bpf-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( KarpatyRfAmpBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb2-rfamp-1st-mixer-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( KarpatyVfoBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb3-vfo-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( KarpatyBufferBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb5-buffer-freq-doubler-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( KarpatyMainboardBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb6-mainboard-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( KarpatyPowerSupplyBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/karpaty-rx-pcb7-power-supply-orig.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


// ============================================================================
// Additional real-world boards
// ============================================================================

BOOST_AUTO_TEST_CASE( Ku14194RevBBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/ku14194revb.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( AntennaSwitchBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/pcb100x40_v5.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


BOOST_AUTO_TEST_CASE( TfccBoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/tfcc.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


// ============================================================================
// Large board stress test (Amiga 2000 -- full motherboard recreation, ~4.3MB)
// ============================================================================

BOOST_AUTO_TEST_CASE( Amiga2000BoardLoad )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/amiga2000-remake.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    BOOST_CHECK_MESSAGE( board->Footprints().size() > 50,
                         wxString::Format( "Amiga 2000 should have many footprints, got %zu",
                                           board->Footprints().size() ) );

    BOOST_CHECK( board->GetCopperLayerCount() >= 2 );
}


// ============================================================================
// Cross-board consistency checks
// ============================================================================

BOOST_AUTO_TEST_CASE( AllBoardsHaveConsistentPadCoordinates )
{
    std::vector<std::string> files = {
        "/io/sprint_layout/gpio2nesc.lay6",
        "/io/sprint_layout/mdb-rs232.lay6",
        "/io/sprint_layout/mdb-master-rev2a.lay6",
        "/io/sprint_layout/smalldualrgb-withmask.lay6",
        "/io/sprint_layout/amiga2000-remake.lay6",
        "/io/sprint_layout/karpaty-rx-pcb1-bpf-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb2-rfamp-1st-mixer-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb3-vfo-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb5-buffer-freq-doubler-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb6-mainboard-orig.lay6",
        "/io/sprint_layout/karpaty-rx-pcb7-power-supply-orig.lay6",
        "/io/sprint_layout/ku14194revb.lay6",
        "/io/sprint_layout/pcb100x40_v5.lay6",
        "/io/sprint_layout/tfcc.lay6",
        "/io/sprint_layout/12F629_SM.lay6",
    };

    for( const auto& file : files )
    {
        std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + file;

        BOOST_TEST_CONTEXT( "Checking coordinate consistency in " << file )
        {
            std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

            BOOST_REQUIRE( board );

            // All pad coordinates should be finite and within a reasonable range
            // (no overflow, no NaN from bad float conversion)
            const int MAX_COORD = pcbIUScale.mmToIU( 1000.0 );
            int       badCount = 0;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                for( PAD* pad : fp->Pads() )
                {
                    VECTOR2I pos = pad->GetPosition();

                    if( std::abs( pos.x ) > MAX_COORD || std::abs( pos.y ) > MAX_COORD )
                        badCount++;
                }
            }

            BOOST_CHECK_MESSAGE( badCount == 0,
                                 wxString::Format( "%d pads have coordinates outside +-10m in %s",
                                                    badCount, wxString::FromUTF8( file ) ) );
        }
    }
}


// ============================================================================
// Multi-board selection tests
// ============================================================================

BOOST_AUTO_TEST_CASE( MultiBoardFileHasFiveBoards )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mmjoy2-74hc165.lay6";

    SPRINT_LAYOUT_PARSER parser;
    BOOST_REQUIRE( parser.ParseBoard( dataPath ) );

    const auto& fileData = parser.GetFileData();
    BOOST_CHECK_EQUAL( fileData.boards.size(), 5 );
}


BOOST_AUTO_TEST_CASE( MultiBoardSelectByIndex )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mmjoy2-74hc165.lay6";

    SPRINT_LAYOUT_PARSER parser;
    BOOST_REQUIRE( parser.ParseBoard( dataPath ) );

    const auto& fileData = parser.GetFileData();
    BOOST_REQUIRE( fileData.boards.size() == 5 );

    for( size_t i = 0; i < fileData.boards.size(); i++ )
    {
        BOOST_TEST_CONTEXT( "Board index " << i )
        {
            std::map<std::string, UTF8> props;
            props["pcb_id"] = std::to_string( i );

            PCB_IO_SPRINT_LAYOUT plugin;
            std::unique_ptr<BOARD> board( plugin.LoadBoard( dataPath, nullptr, &props ) );

            BOOST_REQUIRE( board );
        }
    }
}


BOOST_AUTO_TEST_CASE( MultiBoardCallbackInvoked )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mmjoy2-74hc165.lay6";

    PCB_IO_SPRINT_LAYOUT plugin;
    bool                 callbackInvoked = false;
    size_t               optionCount = 0;

    plugin.RegisterCallback(
            [&]( const std::vector<IMPORT_PROJECT_DESC>& aOptions )
            {
                callbackInvoked = true;
                optionCount = aOptions.size();

                // Select the second board
                std::vector<IMPORT_PROJECT_DESC> chosen;
                chosen.push_back( aOptions[1] );
                return chosen;
            } );

    std::unique_ptr<BOARD> board( plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_CHECK( callbackInvoked );
    BOOST_CHECK_EQUAL( optionCount, 5 );
    BOOST_REQUIRE( board );
}


BOOST_AUTO_TEST_CASE( MultiBoardCallbackCancelReturnsNull )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/mmjoy2-74hc165.lay6";

    PCB_IO_SPRINT_LAYOUT plugin;

    plugin.RegisterCallback(
            [&]( const std::vector<IMPORT_PROJECT_DESC>& aOptions )
            {
                return std::vector<IMPORT_PROJECT_DESC>();
            } );

    std::unique_ptr<BOARD> board( plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_CHECK( board == nullptr );
}


BOOST_AUTO_TEST_CASE( SingleBoardFileSkipsCallback )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/gpio2nesc.lay6";

    PCB_IO_SPRINT_LAYOUT plugin;
    bool                 callbackInvoked = false;

    plugin.RegisterCallback(
            [&]( const std::vector<IMPORT_PROJECT_DESC>& aOptions )
            {
                callbackInvoked = true;
                return aOptions;
            } );

    std::unique_ptr<BOARD> board( plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_CHECK( !callbackInvoked );
    BOOST_REQUIRE( board );
}


// ============================================================================
// Regression tests
// ============================================================================

BOOST_AUTO_TEST_CASE( Pic12F629SmdPadPositions )
{
    // GitLab #23538: SMD pad x,y fields in some Sprint Layout files store
    // component-relative offsets rather than absolute positions, causing
    // all SMD pads to pile up near (0,0).
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "/io/sprint_layout/12F629_SM.lay6";

    std::unique_ptr<BOARD> board( m_plugin.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // The PIC12F629 is an 8-pin SOIC. After import, its pads should be
    // spread across the board, not clustered at the origin.
    BOX2I boardBox;
    bool  first = true;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( shape && shape->GetLayer() == Edge_Cuts )
        {
            if( first )
            {
                boardBox = shape->GetBoundingBox();
                first = false;
            }
            else
            {
                boardBox.Merge( shape->GetBoundingBox() );
            }
        }
    }

    BOOST_REQUIRE( !first );

    boardBox.Inflate( pcbIUScale.mmToIU( 2.0 ) );

    int outsideCount = 0;
    int smdCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD )
            {
                smdCount++;

                if( !boardBox.Contains( pad->GetPosition() ) )
                    outsideCount++;
            }
        }
    }

    BOOST_CHECK_MESSAGE( smdCount > 0,
                         "PIC12F629 board should have SMD pads" );

    BOOST_CHECK_MESSAGE( outsideCount == 0,
                         wxString::Format( "%d of %d SMD pads outside board outline",
                                           outsideCount, smdCount ) );
}


BOOST_AUTO_TEST_SUITE_END()
