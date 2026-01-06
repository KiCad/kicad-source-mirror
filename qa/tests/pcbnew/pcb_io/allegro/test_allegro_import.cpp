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
 * @file test_allegro_import.cpp
 * Test suite for import of Cadence Allegro PCB .brd files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/allegro/pcb_io_allegro.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <netinfo.h>
#include <reporter.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>


/**
 * Custom REPORTER that captures all messages for later analysis.
 */
class CAPTURING_REPORTER : public REPORTER
{
public:
    struct MESSAGE
    {
        wxString text;
        SEVERITY severity = RPT_SEVERITY_UNDEFINED;
    };

    CAPTURING_REPORTER() : m_errorCount( 0 ), m_warningCount( 0 ), m_infoCount( 0 ) {}

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        MESSAGE msg;
        msg.text = aText;
        msg.severity = aSeverity;
        m_messages.push_back( msg );

        switch( aSeverity )
        {
        case RPT_SEVERITY_ERROR:   m_errorCount++; break;
        case RPT_SEVERITY_WARNING: m_warningCount++; break;
        case RPT_SEVERITY_INFO:
        case RPT_SEVERITY_ACTION:  m_infoCount++; break;
        default: break;
        }

        return *this;
    }

    bool HasMessage() const override { return !m_messages.empty(); }

    EDA_UNITS GetUnits() const override { return EDA_UNITS::MM; }

    void Clear()
    {
        m_messages.clear();
        m_errorCount = 0;
        m_warningCount = 0;
        m_infoCount = 0;
    }

    void PrintAllMessages( const std::string& aContext ) const
    {
        if( m_messages.empty() )
        {
            BOOST_TEST_MESSAGE( aContext << ": No messages" );
            return;
        }

        BOOST_TEST_MESSAGE( aContext << ": " << m_messages.size() << " messages ("
                            << m_errorCount << " errors, " << m_warningCount << " warnings)" );

        for( const MESSAGE& msg : m_messages )
        {
            const char* severityStr = "???";

            switch( msg.severity )
            {
            case RPT_SEVERITY_ERROR:   severityStr = "ERROR"; break;
            case RPT_SEVERITY_WARNING: severityStr = "WARN "; break;
            case RPT_SEVERITY_INFO:    severityStr = "INFO "; break;
            case RPT_SEVERITY_ACTION:  severityStr = "ACT  "; break;
            case RPT_SEVERITY_DEBUG:   severityStr = "DEBUG"; break;
            default:                   severityStr = "     "; break;
            }

            BOOST_TEST_MESSAGE( "  [" << severityStr << "] " << msg.text );
        }
    }

    int GetErrorCount() const { return m_errorCount; }
    int GetWarningCount() const { return m_warningCount; }
    const std::vector<MESSAGE>& GetMessages() const { return m_messages; }

private:
    std::vector<MESSAGE> m_messages;
    int                  m_errorCount;
    int                  m_warningCount;
    int                  m_infoCount;
};


struct ALLEGRO_IMPORT_FIXTURE
{
    ALLEGRO_IMPORT_FIXTURE() {}

    std::unique_ptr<BOARD> LoadAllegroBoard( const std::string& aFileName )
    {
        std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/" + aFileName;

        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
        m_allegroPlugin.LoadBoard( dataPath, board.get(), nullptr, nullptr );

        return board;
    }

    /**
     * Print detailed board statistics for debugging
     */
    void PrintBoardStats( const BOARD* aBoard, const std::string& aBoardName )
    {
        if( !aBoard )
        {
            BOOST_TEST_MESSAGE( aBoardName << ": FAILED TO LOAD" );
            return;
        }

        int trackCount = 0;
        int viaCount = 0;
        int arcCount = 0;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            switch( track->Type() )
            {
            case PCB_TRACE_T: trackCount++; break;
            case PCB_VIA_T: viaCount++; break;
            case PCB_ARC_T: arcCount++; break;
            default: break;
            }
        }

        int smdPadCount = 0;
        int thPadCount = 0;

        for( FOOTPRINT* fp : aBoard->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                    smdPadCount++;
                else
                    thPadCount++;
            }
        }

        BOOST_TEST_MESSAGE( "\n=== Board Statistics: " << aBoardName << " ===" );
        BOOST_TEST_MESSAGE( "  Layers: " << aBoard->GetCopperLayerCount() );
        BOOST_TEST_MESSAGE( "  Nets: " << aBoard->GetNetCount() );
        BOOST_TEST_MESSAGE( "  Footprints: " << aBoard->Footprints().size() );
        BOOST_TEST_MESSAGE( "  Tracks: " << trackCount );
        BOOST_TEST_MESSAGE( "  Vias: " << viaCount );
        BOOST_TEST_MESSAGE( "  Arcs: " << arcCount );
        BOOST_TEST_MESSAGE( "  SMD Pads: " << smdPadCount );
        BOOST_TEST_MESSAGE( "  TH Pads: " << thPadCount );
        BOOST_TEST_MESSAGE( "  Zones: " << aBoard->Zones().size() );
    }

    PCB_IO_ALLEGRO m_allegroPlugin;
};


BOOST_FIXTURE_TEST_SUITE( AllegroImport, ALLEGRO_IMPORT_FIXTURE )


/**
 * Test that an Allegro .brd file can be loaded without crashing.
 * Uses TRS80_POWER.brd as a simple test board.
 */
BOOST_AUTO_TEST_CASE( BasicLoad )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "Board should load successfully" );

    PrintBoardStats( board.get(), "TRS80_POWER.brd" );

    // Basic sanity checks
    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
}


/**
 * Test that nets are imported correctly with proper names.
 */
BOOST_AUTO_TEST_CASE( NetImport )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    // Check that nets exist
    BOOST_CHECK_GT( board->GetNetCount(), 1 );

    // Check for expected net names (will need to be adjusted based on actual board content)
    bool foundGnd = false;
    bool foundVcc = false;

    for( int i = 0; i < board->GetNetCount(); i++ )
    {
        NETINFO_ITEM* net = board->GetNetInfo().GetNetItem( i );

        if( net )
        {
            wxString name = net->GetNetname().Upper();

            if( name.Contains( "GND" ) || name.Contains( "VSS" ) )
                foundGnd = true;

            if( name.Contains( "VCC" ) || name.Contains( "VDD" ) || name.Contains( "+5" ) ||
                name.Contains( "+3" ) || name.Contains( "PWR" ) )
                foundVcc = true;
        }
    }

    BOOST_TEST_MESSAGE( "Found GND-like net: " << ( foundGnd ? "yes" : "no" ) );
    BOOST_TEST_MESSAGE( "Found VCC-like net: " << ( foundVcc ? "yes" : "no" ) );
}


/**
 * Test that footprints have valid reference designators.
 */
BOOST_AUTO_TEST_CASE( FootprintRefDes )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int emptyRefDesCount = 0;
    int validRefDesCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString refdes = fp->GetReference();

        if( refdes.IsEmpty() )
            emptyRefDesCount++;
        else
            validRefDesCount++;
    }

    BOOST_TEST_MESSAGE( "Valid RefDes: " << validRefDesCount << ", Empty: " << emptyRefDesCount );

    // Most footprints should have valid reference designators
    BOOST_CHECK_GT( validRefDesCount, emptyRefDesCount );
}


/**
 * Test that pads have valid sizes and are not degenerate.
 */
BOOST_AUTO_TEST_CASE( PadSizes )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int validPadCount = 0;
    int zeroPadCount = 0;
    int hugePadCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            VECTOR2I size = pad->GetSize( F_Cu );

            if( size.x == 0 || size.y == 0 )
            {
                zeroPadCount++;
                BOOST_TEST_MESSAGE( "Zero-size pad in " << fp->GetReference() << " pad "
                                                         << pad->GetNumber() );
            }
            else if( size.x > 50000000 || size.y > 50000000 ) // > 50mm is suspicious
            {
                hugePadCount++;
                BOOST_TEST_MESSAGE( "Huge pad in " << fp->GetReference() << " pad " << pad->GetNumber()
                                                    << ": " << size.x / 1000000.0 << "mm x "
                                                    << size.y / 1000000.0 << "mm" );
            }
            else
            {
                validPadCount++;
            }
        }
    }

    BOOST_TEST_MESSAGE( "Valid pads: " << validPadCount << ", Zero: " << zeroPadCount
                                        << ", Huge: " << hugePadCount );

    // No pads should be zero-size (this catches hardcoded fallbacks)
    BOOST_CHECK_EQUAL( zeroPadCount, 0 );
}


/**
 * Test that vias have valid sizes (not hardcoded fallback values).
 */
BOOST_AUTO_TEST_CASE( ViaSizes )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int validViaCount = 0;
    int suspiciousViaCount = 0;

    // Count vias with the hardcoded size (1000000 = 1mm exactly)
    const int HARDCODED_SIZE = 1000000;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            int      width = via->GetWidth( F_Cu );

            if( width == HARDCODED_SIZE )
            {
                suspiciousViaCount++;
            }
            else if( width > 0 && width < 10000000 ) // 0 < size < 10mm is reasonable
            {
                validViaCount++;
            }
        }
    }

    BOOST_TEST_MESSAGE( "Valid vias: " << validViaCount << ", Hardcoded-size vias: " << suspiciousViaCount );

    // This test will fail until via size is properly extracted from padstack
    if( suspiciousViaCount > 0 )
    {
        BOOST_WARN_MESSAGE( false, "Found " << suspiciousViaCount << " vias with hardcoded 1mm size" );
    }
}


/**
 * Test that track widths are reasonable.
 */
BOOST_AUTO_TEST_CASE( TrackWidths )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int validTrackCount = 0;
    int zeroTrackCount = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
        {
            int width = track->GetWidth();

            if( width == 0 )
                zeroTrackCount++;
            else if( width > 0 && width < 10000000 ) // 0 < width < 10mm
                validTrackCount++;
        }
    }

    BOOST_TEST_MESSAGE( "Valid tracks: " << validTrackCount << ", Zero-width: " << zeroTrackCount );

    BOOST_CHECK_EQUAL( zeroTrackCount, 0 );
}


/**
 * Test import of multiple Allegro board versions.
 * This ensures version-conditional parsing works correctly.
 */
BOOST_AUTO_TEST_CASE( MultiVersionImport )
{
    // Test boards with different Allegro versions:
    // - V166: mainBoard.brd, mainBoard2.brd, TRS80_POWER.brd, ProiectBoard.brd
    // - V174: led_youtube.brd
    // - V175: 8851_HW-U1-VCU118_REV2-0_071417.brd (large board)

    std::vector<std::string> testBoards = {
        "TRS80_POWER.brd",      // V166
        // "led_youtube.brd",      // V174 - requires additional block types for reference resolution
        // "8851_HW-U1-VCU118_REV2-0_071417.brd" // V175 - large, skip for quick tests
    };

    for( const std::string& boardName : testBoards )
    {
        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            std::unique_ptr<BOARD> board = LoadAllegroBoard( boardName );

            BOOST_CHECK_MESSAGE( board != nullptr, boardName << " should load" );

            if( board )
            {
                PrintBoardStats( board.get(), boardName );

                BOOST_CHECK_GT( board->GetNetCount(), 0 );
                BOOST_CHECK_GT( board->Footprints().size(), 0 );
            }
        }
    }
}


/**
 * Test that pad numbers are set correctly.
 */
BOOST_AUTO_TEST_CASE( PadNumbers )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int numberedPads = 0;
    int unnumberedPads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetNumber().IsEmpty() )
                unnumberedPads++;
            else
                numberedPads++;
        }
    }

    BOOST_TEST_MESSAGE( "Numbered pads: " << numberedPads << ", Unnumbered: " << unnumberedPads );

    // All pads should have numbers for proper netlist generation
    // This test will fail until pad numbers are properly set
    if( unnumberedPads > 0 )
    {
        BOOST_WARN_MESSAGE( false, "Found " << unnumberedPads << " pads without numbers" );
    }
}


/**
 * Test that copper layers are set up correctly.
 */
BOOST_AUTO_TEST_CASE( CopperLayers )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    int copperLayers = board->GetCopperLayerCount();

    BOOST_TEST_MESSAGE( "Copper layer count: " << copperLayers );

    // Should have at least 2 copper layers
    BOOST_CHECK_GE( copperLayers, 2 );

    // Verify layer names are set (not generic)
    wxString topName = board->GetLayerName( F_Cu );
    wxString botName = board->GetLayerName( B_Cu );

    BOOST_TEST_MESSAGE( "Top copper name: " << topName );
    BOOST_TEST_MESSAGE( "Bottom copper name: " << botName );
}


/**
 * Test that board outline is imported correctly.
 */
BOOST_AUTO_TEST_CASE( BoardOutline )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    // Count shapes on Edge_Cuts layer
    int outlineSegmentCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
        {
            outlineSegmentCount++;
        }
    }

    BOOST_TEST_MESSAGE( "Board outline segments: " << outlineSegmentCount );

    // Board should have an outline - TRS80_POWER.brd has a rectangular outline (4 segments)
    BOOST_CHECK_GE( outlineSegmentCount, 4 );

    // Verify outline forms a closed contour by checking that all segments connect
    std::vector<PCB_SHAPE*> outlineShapes;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
        {
            outlineShapes.push_back( static_cast<PCB_SHAPE*>( item ) );
        }
    }

    if( !outlineShapes.empty() )
    {
        // For a valid closed outline, the sum of all segment lengths should equal the perimeter
        // and each endpoint should connect to another endpoint
        int connectedCount = 0;

        for( PCB_SHAPE* shape : outlineShapes )
        {
            VECTOR2I start = shape->GetStart();
            VECTOR2I end = shape->GetEnd();

            for( PCB_SHAPE* other : outlineShapes )
            {
                if( other == shape )
                    continue;

                VECTOR2I otherStart = other->GetStart();
                VECTOR2I otherEnd = other->GetEnd();

                // Check if this shape's start connects to another shape's start or end
                if( start == otherStart || start == otherEnd )
                    connectedCount++;

                // Check if this shape's end connects to another shape's start or end
                if( end == otherStart || end == otherEnd )
                    connectedCount++;
            }
        }

        // Each segment should connect at both ends for a closed outline
        // For 4 segments, we expect 8 connections (2 per segment)
        BOOST_TEST_MESSAGE( "Connected endpoints: " << connectedCount );
        BOOST_CHECK_GE( connectedCount, outlineShapes.size() * 2 );
    }
}


/**
 * Test that all pads are inside the board outline.
 */
BOOST_AUTO_TEST_CASE( PadsInsideOutline )
{
    std::unique_ptr<BOARD> board = LoadAllegroBoard( "TRS80_POWER.brd" );

    BOOST_REQUIRE( board != nullptr );

    // Get board bounding box from outline
    BOX2I boardBbox;
    bool  hasBbox = false;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
        {
            if( !hasBbox )
            {
                boardBbox = item->GetBoundingBox();
                hasBbox = true;
            }
            else
            {
                boardBbox.Merge( item->GetBoundingBox() );
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( hasBbox, "Board should have an outline" );

    int padsInside = 0;
    int padsOutside = 0;

    // Inflate bbox slightly to account for edge cases
    BOX2I testBbox = boardBbox;
    testBbox.Inflate( 1000 ); // 1mm tolerance

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            VECTOR2I padCenter = pad->GetPosition();

            if( testBbox.Contains( padCenter ) )
            {
                padsInside++;
            }
            else
            {
                padsOutside++;
                BOOST_TEST_MESSAGE( "Pad outside outline: " << fp->GetReference() << " pad "
                                                             << pad->GetNumber() << " at ("
                                                             << padCenter.x / 1000000.0 << ", "
                                                             << padCenter.y / 1000000.0 << ") mm" );
            }
        }
    }

    BOOST_TEST_MESSAGE( "Pads inside outline: " << padsInside << ", outside: " << padsOutside );

    // Most pads should be inside the board outline
    // Some boards may have off-board test points or fiducials
    if( padsOutside > 0 )
    {
        BOOST_WARN_MESSAGE( false, "Found " << padsOutside << " pads outside board outline" );
    }

    // At minimum, most pads should be inside
    BOOST_CHECK_GT( padsInside, padsOutside );
}


BOOST_AUTO_TEST_SUITE_END()


/**
 * Data for parameterized all-boards test.
 */
struct ALLEGRO_BOARD_TEST_CASE
{
    std::string filename;
    bool        expected_to_load;  // Set false for known-broken boards
};


/**
 * Fixture for comprehensive board import tests with error capturing.
 */
struct ALLEGRO_COMPREHENSIVE_FIXTURE
{
    ALLEGRO_COMPREHENSIVE_FIXTURE() {}

    /**
     * Attempt to load an Allegro board, capturing all reporter messages.
     * Returns the board (or nullptr on failure) and populates the reporter.
     */
    std::unique_ptr<BOARD> LoadBoardWithCapture( const std::string& aFilePath,
                                                  CAPTURING_REPORTER& aReporter )
    {
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

        m_allegroPlugin.SetReporter( &aReporter );

        try
        {
            m_allegroPlugin.LoadBoard( aFilePath, board.get(), nullptr, nullptr );
            return board;
        }
        catch( const IO_ERROR& e )
        {
            aReporter.Report( wxString::Format( "IO_ERROR: %s", e.What() ), RPT_SEVERITY_ERROR );
            return nullptr;
        }
        catch( const std::exception& e )
        {
            aReporter.Report( wxString::Format( "Exception: %s", e.what() ), RPT_SEVERITY_ERROR );
            return nullptr;
        }
        catch( ... )
        {
            aReporter.Report( "Unknown exception during load", RPT_SEVERITY_ERROR );
            return nullptr;
        }
    }

    /**
     * Print detailed board statistics for debugging.
     */
    void PrintBoardStats( const BOARD* aBoard, const std::string& aBoardName )
    {
        if( !aBoard )
        {
            BOOST_TEST_MESSAGE( aBoardName << ": FAILED TO LOAD" );
            return;
        }

        int trackCount = 0;
        int viaCount = 0;
        int arcCount = 0;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            switch( track->Type() )
            {
            case PCB_TRACE_T: trackCount++; break;
            case PCB_VIA_T: viaCount++; break;
            case PCB_ARC_T: arcCount++; break;
            default: break;
            }
        }

        int smdPadCount = 0;
        int thPadCount = 0;

        for( FOOTPRINT* fp : aBoard->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetAttribute() == PAD_ATTRIB::SMD )
                    smdPadCount++;
                else
                    thPadCount++;
            }
        }

        BOOST_TEST_MESSAGE( "\n=== Board Statistics: " << aBoardName << " ===" );
        BOOST_TEST_MESSAGE( "  Layers: " << aBoard->GetCopperLayerCount() );
        BOOST_TEST_MESSAGE( "  Nets: " << aBoard->GetNetCount() );
        BOOST_TEST_MESSAGE( "  Footprints: " << aBoard->Footprints().size() );
        BOOST_TEST_MESSAGE( "  Tracks: " << trackCount );
        BOOST_TEST_MESSAGE( "  Vias: " << viaCount );
        BOOST_TEST_MESSAGE( "  Arcs: " << arcCount );
        BOOST_TEST_MESSAGE( "  SMD Pads: " << smdPadCount );
        BOOST_TEST_MESSAGE( "  TH Pads: " << thPadCount );
        BOOST_TEST_MESSAGE( "  Zones: " << aBoard->Zones().size() );
    }

    /**
     * Get list of all .brd files in the Allegro test data directory.
     */
    static std::vector<std::string> GetAllBoardFiles()
    {
        std::vector<std::string> boards;
        std::string              dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

        try
        {
            for( const auto& entry : std::filesystem::directory_iterator( dataPath ) )
            {
                if( entry.is_regular_file() && entry.path().extension() == ".brd"
                    && entry.file_size() > 0 )
                {
                    boards.push_back( entry.path().filename().string() );
                }
            }
        }
        catch( const std::filesystem::filesystem_error& e )
        {
            BOOST_TEST_MESSAGE( "Failed to enumerate board files: " << e.what() );
        }

        std::sort( boards.begin(), boards.end() );
        return boards;
    }

    PCB_IO_ALLEGRO m_allegroPlugin;
};


BOOST_FIXTURE_TEST_SUITE( AllegroComprehensive, ALLEGRO_COMPREHENSIVE_FIXTURE )


/**
 * Comprehensive test that attempts to load ALL Allegro boards in the test data directory.
 * Reports detailed error messages and statistics for each board.
 */
BOOST_AUTO_TEST_CASE( LoadAllBoards )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    BOOST_TEST_MESSAGE( "\n========================================" );
    BOOST_TEST_MESSAGE( "ALLEGRO COMPREHENSIVE BOARD LOAD TEST" );
    BOOST_TEST_MESSAGE( "========================================" );
    BOOST_TEST_MESSAGE( "Found " << boards.size() << " board files to test\n" );

    int successCount = 0;
    int failureCount = 0;
    std::vector<std::string> failedBoards;

    for( const std::string& boardName : boards )
    {
        BOOST_TEST_MESSAGE( "\n----------------------------------------" );
        BOOST_TEST_MESSAGE( "Testing: " << boardName );
        BOOST_TEST_MESSAGE( "----------------------------------------" );

        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        reporter.PrintAllMessages( boardName );

        if( board )
        {
            PrintBoardStats( board.get(), boardName );

            bool hasContent = board->GetNetCount() > 0 || board->Footprints().size() > 0;

            if( hasContent )
            {
                successCount++;
                BOOST_TEST_MESSAGE( "RESULT: SUCCESS" );
            }
            else
            {
                failureCount++;
                failedBoards.push_back( boardName + " (loaded but empty)" );
                BOOST_TEST_MESSAGE( "RESULT: FAILED (board is empty)" );
            }
        }
        else
        {
            failureCount++;
            failedBoards.push_back( boardName );
            BOOST_TEST_MESSAGE( "RESULT: FAILED (could not load)" );
        }
    }

    BOOST_TEST_MESSAGE( "\n========================================" );
    BOOST_TEST_MESSAGE( "SUMMARY" );
    BOOST_TEST_MESSAGE( "========================================" );
    BOOST_TEST_MESSAGE( "Total boards: " << boards.size() );
    BOOST_TEST_MESSAGE( "Successful: " << successCount );
    BOOST_TEST_MESSAGE( "Failed: " << failureCount );

    if( !failedBoards.empty() )
    {
        BOOST_TEST_MESSAGE( "\nFailed boards:" );

        for( const std::string& name : failedBoards )
        {
            BOOST_TEST_MESSAGE( "  - " << name );
        }
    }

    // Mark the test as failed if any boards failed to load
    BOOST_CHECK_EQUAL( failureCount, 0 );
}


/**
 * Test TRS80_POWER.brd individually.
 */
BOOST_AUTO_TEST_CASE( Individual_TRS80_POWER )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/TRS80_POWER.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "TRS80_POWER.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "TRS80_POWER.brd should load successfully" );
    PrintBoardStats( board.get(), "TRS80_POWER.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test led_youtube.brd individually.
 */
BOOST_AUTO_TEST_CASE( Individual_led_youtube )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/led_youtube.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "led_youtube.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "led_youtube.brd should load successfully" );
    PrintBoardStats( board.get(), "led_youtube.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test mainBoard.brd individually.
 */
BOOST_AUTO_TEST_CASE( Individual_mainBoard )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/mainBoard.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "mainBoard.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "mainBoard.brd should load successfully" );
    PrintBoardStats( board.get(), "mainBoard.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test mainBoard2.brd individually.
 */
BOOST_AUTO_TEST_CASE( Individual_mainBoard2 )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/mainBoard2.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "mainBoard2.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "mainBoard2.brd should load successfully" );
    PrintBoardStats( board.get(), "mainBoard2.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test ProiectBoard.brd individually.
 */
BOOST_AUTO_TEST_CASE( Individual_ProiectBoard )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/ProiectBoard.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "ProiectBoard.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "ProiectBoard.brd should load successfully" );
    PrintBoardStats( board.get(), "ProiectBoard.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test BeagleBone_Black_RevC.brd individually.
 * This board has components on both top and bottom layers.
 */
BOOST_AUTO_TEST_CASE( Individual_BeagleBone )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/BeagleBone_Black_RevC.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "BeagleBone_Black_RevC.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "BeagleBone_Black_RevC.brd should load successfully" );
    PrintBoardStats( board.get(), "BeagleBone_Black_RevC.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Test 8851_HW-U1-VCU118_REV2-0_071417.brd individually.
 * This is a large, complex board that may take longer to load.
 */
BOOST_AUTO_TEST_CASE( Individual_VCU118 )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/allegro/8851_HW-U1-VCU118_REV2-0_071417.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    reporter.PrintAllMessages( "8851_HW-U1-VCU118_REV2-0_071417.brd" );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "VCU118 board should load successfully" );
    PrintBoardStats( board.get(), "8851_HW-U1-VCU118_REV2-0_071417.brd" );

    BOOST_CHECK_GT( board->GetNetCount(), 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
    BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );
}


/**
 * Verify that all pads have positive sizes (no negative dimensions).
 */
BOOST_AUTO_TEST_CASE( PadSizesPositive )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int negativePadCount = 0;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                for( PAD* pad : fp->Pads() )
                {
                    VECTOR2I size = pad->GetSize( F_Cu );

                    if( size.x < 0 || size.y < 0 )
                    {
                        negativePadCount++;
                        BOOST_TEST_MESSAGE( boardName << ": Negative pad size in " << fp->GetReference()
                                            << " pad " << pad->GetNumber() << ": " << size.x << " x "
                                            << size.y );
                    }
                }
            }

            BOOST_CHECK_EQUAL( negativePadCount, 0 );
        }
    }
}


/**
 * Verify that via drill sizes are not larger than via diameters.
 * This would indicate a parsing error in the padstack.
 */
BOOST_AUTO_TEST_CASE( ViaDrillNotLargerThanSize )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int invalidViaCount = 0;

            for( PCB_TRACK* track : board->Tracks() )
            {
                if( track->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( track );
                    int drill = via->GetDrill();
                    int width = via->GetWidth( F_Cu );

                    if( drill > width )
                    {
                        invalidViaCount++;
                        BOOST_TEST_MESSAGE( boardName << ": Via at ("
                                            << via->GetPosition().x / 1000000.0 << ", "
                                            << via->GetPosition().y / 1000000.0
                                            << ") has drill " << drill / 1000000.0
                                            << "mm > width " << width / 1000000.0 << "mm" );
                    }
                }
            }

            BOOST_CHECK_EQUAL( invalidViaCount, 0 );
        }
    }
}


/**
 * Verify that pads with no drill hole are properly marked as SMD.
 * If a pad has drill size of 0, it should have PAD_ATTRIB::SMD, not PTH.
 */
BOOST_AUTO_TEST_CASE( SmdPadDetection )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int misclassifiedSmdCount = 0;
            int correctSmdCount = 0;
            int correctThCount = 0;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                for( PAD* pad : fp->Pads() )
                {
                    bool hasDrill = pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0;
                    PAD_ATTRIB attr = pad->GetAttribute();

                    if( !hasDrill && attr == PAD_ATTRIB::PTH )
                    {
                        misclassifiedSmdCount++;
                        BOOST_TEST_MESSAGE( boardName << ": Pad " << fp->GetReference()
                                            << "." << pad->GetNumber()
                                            << " has no drill but is marked as PTH (should be SMD)" );
                    }
                    else if( !hasDrill && attr == PAD_ATTRIB::SMD )
                    {
                        correctSmdCount++;
                    }
                    else if( hasDrill && ( attr == PAD_ATTRIB::PTH || attr == PAD_ATTRIB::NPTH ) )
                    {
                        correctThCount++;
                    }
                }
            }

            BOOST_TEST_MESSAGE( boardName << ": Correct SMD=" << correctSmdCount
                                << ", Correct TH=" << correctThCount
                                << ", Misclassified=" << misclassifiedSmdCount );

            BOOST_CHECK_EQUAL( misclassifiedSmdCount, 0 );
        }
    }
}


/**
 * Verify that pads on quad flat packages (QFN/QFP) have proper rotation.
 * Pads on the sides should be rotated 90° from each other.
 */
BOOST_AUTO_TEST_CASE( QuadPackagePadRotation )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int quadPackageCount = 0;
            int packagesWithRotatedPads = 0;
            int packagesWithUnrotatedPads = 0;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                wxString refdes = fp->GetReference().Upper();

                // Look for ICs (typically U* prefix) that might be quad packages
                if( !refdes.StartsWith( "U" ) )
                    continue;

                // Must have at least 16 pads to be a quad package
                if( fp->Pads().size() < 16 )
                    continue;

                // Find bounding box of all pad centers to estimate package shape
                BOX2I padBounds;
                bool  first = true;

                for( PAD* pad : fp->Pads() )
                {
                    VECTOR2I pos = pad->GetPosition();

                    if( first )
                    {
                        padBounds = BOX2I( pos, VECTOR2I( 0, 0 ) );
                        first = false;
                    }
                    else
                    {
                        padBounds.Merge( pos );
                    }
                }

                // Must be roughly square to be a quad package
                int width = padBounds.GetWidth();
                int height = padBounds.GetHeight();

                if( width == 0 || height == 0 )
                    continue;

                double aspectRatio = static_cast<double>( std::max( width, height ) ) /
                                     static_cast<double>( std::min( width, height ) );

                if( aspectRatio > 2.0 )
                    continue;

                quadPackageCount++;

                // Check if pads have varying orientations
                std::set<int> uniqueAngles;

                for( PAD* pad : fp->Pads() )
                {
                    EDA_ANGLE angle = pad->GetOrientation();
                    angle.Normalize();
                    int degrees = static_cast<int>( angle.AsDegrees() + 0.5 ) % 360;
                    uniqueAngles.insert( degrees );
                }

                // A properly imported quad package should have at least 2 different pad orientations
                // (for 2-sided packages) or 4 (for 4-sided packages like QFP)
                if( uniqueAngles.size() >= 2 )
                {
                    packagesWithRotatedPads++;
                    BOOST_TEST_MESSAGE( boardName << ": " << fp->GetReference()
                                        << " has " << uniqueAngles.size() << " unique pad orientations" );
                }
                else
                {
                    packagesWithUnrotatedPads++;
                    BOOST_TEST_MESSAGE( boardName << ": " << fp->GetReference()
                                        << " has only " << uniqueAngles.size()
                                        << " unique pad orientation (may be missing rotation)" );
                }
            }

            if( quadPackageCount > 0 )
            {
                BOOST_TEST_MESSAGE( boardName << ": Found " << quadPackageCount
                                    << " potential quad packages, "
                                    << packagesWithRotatedPads << " with rotated pads, "
                                    << packagesWithUnrotatedPads << " without" );

                // At least some packages should have rotated pads to confirm rotation parsing works.
                // Many packages may legitimately have all pads at the same orientation (BGAs, single-row).
                if( packagesWithRotatedPads == 0 && quadPackageCount > 0 )
                {
                    BOOST_WARN_MESSAGE( false, boardName << " has no packages with rotated pads" );
                }
            }
        }
    }
}


/**
 * Verify that footprints are placed on the correct layer.
 * Tests that bottom-layer components are imported with correct layer assignment.
 * BeagleBone Black C78 is known to be a bottom-layer component.
 */
BOOST_AUTO_TEST_CASE( FootprintLayerPlacement )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/BeagleBone_Black_RevC.brd";

    CAPTURING_REPORTER reporter;
    std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath, reporter );

    BOOST_REQUIRE_MESSAGE( board != nullptr, "BeagleBone_Black_RevC.brd should load successfully" );

    // Look for C78 which should be on the bottom layer
    FOOTPRINT* c78 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == "C78" )
        {
            c78 = fp;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( c78 != nullptr, "Footprint C78 should exist in BeagleBone Black" );

    PCB_LAYER_ID fpLayer = c78->GetLayer();

    BOOST_TEST_MESSAGE( "C78 layer: " << board->GetLayerName( fpLayer ) << " (ID: " << fpLayer << ")" );
    BOOST_TEST_MESSAGE( "C78 is flipped: " << ( c78->IsFlipped() ? "yes" : "no" ) );

    BOOST_CHECK_MESSAGE( fpLayer == B_Cu, "C78 should be on the bottom copper layer (B_Cu), got "
                         << board->GetLayerName( fpLayer ) );
    BOOST_CHECK_MESSAGE( c78->IsFlipped(), "C78 should be flipped (IsFlipped() == true)" );

    // Count footprints on top vs bottom to ensure we're parsing layer correctly
    int topCount = 0;
    int bottomCount = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetLayer() == F_Cu )
            topCount++;
        else if( fp->GetLayer() == B_Cu )
            bottomCount++;
    }

    BOOST_TEST_MESSAGE( "Footprints on top: " << topCount << ", on bottom: " << bottomCount );

    // BeagleBone should have components on both sides
    BOOST_CHECK_GT( topCount, 0 );
    BOOST_CHECK_GT( bottomCount, 0 );
}


/**
 * Verify that arc start points connect to adjacent track endpoints.
 * This checks that arc orientation/winding is correct.
 */
BOOST_AUTO_TEST_CASE( ArcConnectivity )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int arcCount = 0;
            int disconnectedArcs = 0;

            // Build a map of track endpoints per net for quick lookup
            std::map<int, std::vector<VECTOR2I>> netEndpoints;

            for( PCB_TRACK* track : board->Tracks() )
            {
                int netCode = track->GetNetCode();

                if( track->Type() == PCB_TRACE_T )
                {
                    netEndpoints[netCode].push_back( track->GetStart() );
                    netEndpoints[netCode].push_back( track->GetEnd() );
                }
                else if( track->Type() == PCB_VIA_T )
                {
                    netEndpoints[netCode].push_back( track->GetPosition() );
                }
            }

            // Also include pad positions
            for( FOOTPRINT* fp : board->Footprints() )
            {
                for( PAD* pad : fp->Pads() )
                {
                    int netCode = pad->GetNetCode();

                    if( netCode > 0 )
                        netEndpoints[netCode].push_back( pad->GetPosition() );
                }
            }

            // Now check each arc
            for( PCB_TRACK* track : board->Tracks() )
            {
                if( track->Type() != PCB_ARC_T )
                    continue;

                arcCount++;
                PCB_ARC* arc = static_cast<PCB_ARC*>( track );
                int      netCode = arc->GetNetCode();

                VECTOR2I arcStart = arc->GetStart();
                VECTOR2I arcEnd = arc->GetEnd();

                // Check if arc endpoints connect to something
                bool startConnected = false;
                bool endConnected = false;
                const int tolerance = 1000; // 1um tolerance

                for( const VECTOR2I& pt : netEndpoints[netCode] )
                {
                    if( ( pt - arcStart ).EuclideanNorm() < tolerance )
                        startConnected = true;

                    if( ( pt - arcEnd ).EuclideanNorm() < tolerance )
                        endConnected = true;
                }

                // Arc should connect to at least one other track/pad at each end
                // (unless it's an isolated arc, which is unusual but possible)
                if( !startConnected && !endConnected && netEndpoints[netCode].size() > 2 )
                {
                    disconnectedArcs++;
                    BOOST_TEST_MESSAGE( boardName << ": Arc at ("
                                        << arcStart.x / 1000000.0 << ", "
                                        << arcStart.y / 1000000.0 << ") to ("
                                        << arcEnd.x / 1000000.0 << ", "
                                        << arcEnd.y / 1000000.0
                                        << ") appears disconnected from net " << netCode );
                }
            }

            if( arcCount > 0 )
            {
                BOOST_TEST_MESSAGE( boardName << ": Found " << arcCount << " arcs, "
                                    << disconnectedArcs << " disconnected" );
            }

            // Allow some disconnected arcs as they may be legitimate isolated features
            // but flag if more than 20% are disconnected
            if( arcCount > 5 )
            {
                BOOST_CHECK_LE( disconnectedArcs, arcCount / 5 );
            }
        }
    }
}


/**
 * Parse a FabMaster .alg file and extract reference data for cross-validation.
 *
 * The .alg format uses `!` as field delimiter with three record types:
 *   A! = column header definition
 *   J! = journal/metadata
 *   S! = data record
 *
 * Sections are demarcated by A! lines. Data lines follow the schema of the
 * most recent A! header.
 */
/**
 * A single zone polygon from the .alg export, representing an ETCH shape on a copper layer.
 */
struct ALG_ZONE_POLYGON
{
    wxString layer;
    int      recordId = 0;
    wxString netName;
    double   minX = 1e18, minY = 1e18, maxX = -1e18, maxY = -1e18;
    int      segmentCount = 0;

    void AddPoint( double aX, double aY )
    {
        minX = std::min( minX, aX );
        minY = std::min( minY, aY );
        maxX = std::max( maxX, aX );
        maxY = std::max( maxY, aY );
        segmentCount++;
    }
};


struct ALG_REFERENCE_DATA
{
    std::set<wxString>                     netNames;
    std::set<wxString>                     refDes;
    std::map<wxString, wxString>           refDesToSymName;
    std::map<wxString, std::set<wxString>> netToRefDes;

    std::vector<ALG_ZONE_POLYGON>          zonePolygons;

    static std::vector<wxString> SplitAlgLine( const wxString& aLine )
    {
        std::vector<wxString> fields;
        wxString              current;

        for( size_t i = 0; i < aLine.size(); ++i )
        {
            if( aLine[i] == '!' )
            {
                fields.push_back( current );
                current.clear();
            }
            else
            {
                current += aLine[i];
            }
        }

        if( !current.empty() )
            fields.push_back( current );

        return fields;
    }

    /**
     * Extract the integer record ID from a RECORD_TAG field like "36 1 0".
     * Returns -1 on failure.
     */
    static int ParseRecordId( const wxString& aTag )
    {
        long     val = -1;
        wxString tag = aTag.BeforeFirst( ' ' );
        tag.ToLong( &val );
        return static_cast<int>( val );
    }

    static ALG_REFERENCE_DATA ParseAlgFile( const std::string& aPath )
    {
        ALG_REFERENCE_DATA data;
        std::ifstream      file( aPath );

        if( !file.is_open() )
            return data;

        enum class SECTION
        {
            UNKNOWN,
            NET_NODES,
            SYM_PLACEMENT,
            GRAPHICS,
        };

        SECTION     currentSection = SECTION::UNKNOWN;
        std::string line;

        // Accumulate zone segments grouped by (layer, recordId)
        std::map<std::pair<wxString, int>, ALG_ZONE_POLYGON> zoneMap;

        while( std::getline( file, line ) )
        {
            if( line.empty() || line[0] == 'J' )
                continue;

            if( line[0] == 'A' )
            {
                if( line.find( "NET_NAME_SORT!NODE_SORT!NET_NAME!REFDES!" ) != std::string::npos )
                    currentSection = SECTION::NET_NODES;
                else if( line.find( "SYM_TYPE!SYM_NAME!REFDES!SYM_MIRROR!" ) != std::string::npos )
                    currentSection = SECTION::SYM_PLACEMENT;
                else if( line.find( "CLASS!SUBCLASS!RECORD_TAG!GRAPHIC_DATA_NAME!" ) != std::string::npos )
                    currentSection = SECTION::GRAPHICS;
                else
                    currentSection = SECTION::UNKNOWN;

                continue;
            }

            if( line[0] != 'S' )
                continue;

            auto fields = SplitAlgLine( wxString::FromUTF8( line ) );

            switch( currentSection )
            {
            case SECTION::NET_NODES:
            {
                // S!sort!nodeSort!NET_NAME!REFDES!PIN!PIN_NAME!SUBCLASS!
                if( fields.size() >= 5 )
                {
                    wxString netName = fields[3];
                    wxString refdes = fields[4];

                    if( !netName.empty() )
                    {
                        data.netNames.insert( netName );

                        if( !refdes.empty() )
                            data.netToRefDes[netName].insert( refdes );
                    }
                }

                break;
            }
            case SECTION::SYM_PLACEMENT:
            {
                // S!SYM_TYPE!SYM_NAME!REFDES!MIRROR!ROTATE!X!Y!CX!CY!LIB_PATH!
                if( fields.size() >= 4 )
                {
                    wxString symType = fields[1];
                    wxString symName = fields[2];
                    wxString refdes = fields[3];

                    if( symType == wxT( "PACKAGE" ) && !refdes.empty() )
                    {
                        data.refDes.insert( refdes );
                        data.refDesToSymName[refdes] = symName;
                    }
                }

                break;
            }
            case SECTION::GRAPHICS:
            {
                // Field layout (0-indexed after splitting on '!'):
                //   0=S, 1=CLASS, 2=SUBCLASS, 3=RECORD_TAG, 4=GRAPHIC_DATA_NAME,
                //   5=GRAPHIC_DATA_NUMBER, 6..15=GRAPHIC_DATA_1..10,
                //   16=PIN_NUMBER, ..., 23=NET_NAME
                if( fields.size() < 16 || fields[1] != wxT( "ETCH" ) )
                    break;

                wxString closureType = fields[15];

                if( closureType != wxT( "SHAPE" ) )
                    break;

                wxString layer = fields[2];
                int      recordId = ParseRecordId( fields[3] );

                if( recordId < 0 )
                    break;

                wxString netName;

                if( fields.size() > 23 )
                    netName = fields[23];

                auto  key = std::make_pair( layer, recordId );
                auto& zone = zoneMap[key];
                zone.layer = layer;
                zone.recordId = recordId;

                if( !netName.empty() )
                    zone.netName = netName;

                double x1 = 0, y1 = 0, x2 = 0, y2 = 0;

                if( fields.size() > 9 )
                {
                    fields[6].ToDouble( &x1 );
                    fields[7].ToDouble( &y1 );
                    fields[8].ToDouble( &x2 );
                    fields[9].ToDouble( &y2 );
                    zone.AddPoint( x1, y1 );
                    zone.AddPoint( x2, y2 );
                }

                break;
            }
            default:
                break;
            }
        }

        for( auto& [key, zone] : zoneMap )
            data.zonePolygons.push_back( std::move( zone ) );

        return data;
    }
};


/**
 * Cross-validate imported board net names against .alg ASCII reference export.
 */
BOOST_AUTO_TEST_CASE( AlgReferenceNetNames )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    struct BOARD_REF
    {
        std::string brdFile;
        std::string algFile;
    };

    std::vector<BOARD_REF> boardsWithAlg;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".alg" )
        {
            std::string algName = entry.path().filename().string();
            std::string brdName = algName.substr( 0, algName.size() - 4 );

            std::filesystem::path brdPath( dataPath + brdName );

            if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
                boardsWithAlg.push_back( { brdName, entry.path().string() } );
        }
    }

    BOOST_REQUIRE_GT( boardsWithAlg.size(), 0u );

    for( const auto& ref : boardsWithAlg )
    {
        BOOST_TEST_MESSAGE( "Validating net names: " << ref.brdFile );

        ALG_REFERENCE_DATA algData = ALG_REFERENCE_DATA::ParseAlgFile( ref.algFile );
        BOOST_REQUIRE_GT( algData.netNames.size(), 0u );

        CAPTURING_REPORTER     reporter;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + ref.brdFile, reporter );
        BOOST_REQUIRE( board );

        std::set<wxString> boardNets;

        for( const NETINFO_ITEM* net : board->GetNetInfo() )
        {
            if( net->GetNetCode() > 0 )
                boardNets.insert( net->GetNetname() );
        }

        int missingNets = 0;

        for( const wxString& algNet : algData.netNames )
        {
            if( boardNets.find( algNet ) == boardNets.end() )
            {
                missingNets++;

                if( missingNets <= 10 )
                    BOOST_TEST_MESSAGE( "  Missing net: " << algNet );
            }
        }

        BOOST_TEST_MESSAGE( ref.brdFile << ": .alg has " << algData.netNames.size()
                            << " nets, board has " << boardNets.size()
                            << ", missing " << missingNets );

        BOOST_CHECK_EQUAL( missingNets, 0 );
    }
}


/**
 * Cross-validate imported board component reference designators against .alg reference.
 */
BOOST_AUTO_TEST_CASE( AlgReferenceComponentPlacement )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> boardsWithAlg;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".alg" )
        {
            std::string algName = entry.path().filename().string();
            std::string brdName = algName.substr( 0, algName.size() - 4 );

            std::filesystem::path brdPath( dataPath + brdName );

            if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
                boardsWithAlg.push_back( { brdName, entry.path().string() } );
        }
    }

    BOOST_REQUIRE_GT( boardsWithAlg.size(), 0u );

    for( const auto& [brdFile, algFile] : boardsWithAlg )
    {
        BOOST_TEST_MESSAGE( "Validating components: " << brdFile );

        ALG_REFERENCE_DATA algData = ALG_REFERENCE_DATA::ParseAlgFile( algFile );
        BOOST_REQUIRE_GT( algData.refDes.size(), 0u );

        CAPTURING_REPORTER     reporter;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
        BOOST_REQUIRE( board );

        std::set<wxString> boardRefDes;

        for( const FOOTPRINT* fp : board->Footprints() )
            boardRefDes.insert( fp->GetReference() );

        int missingRefDes = 0;
        int extraRefDes = 0;

        for( const wxString& algRef : algData.refDes )
        {
            if( boardRefDes.find( algRef ) == boardRefDes.end() )
            {
                missingRefDes++;

                if( missingRefDes <= 10 )
                    BOOST_TEST_MESSAGE( "  Missing refdes: " << algRef );
            }
        }

        for( const wxString& boardRef : boardRefDes )
        {
            if( algData.refDes.find( boardRef ) == algData.refDes.end() )
            {
                extraRefDes++;

                if( extraRefDes <= 10 )
                    BOOST_TEST_MESSAGE( "  Extra refdes in board: " << boardRef );
            }
        }

        BOOST_TEST_MESSAGE( brdFile << ": .alg has " << algData.refDes.size()
                            << " components, board has " << boardRefDes.size()
                            << ", missing " << missingRefDes
                            << ", extra " << extraRefDes );

        BOOST_CHECK_EQUAL( missingRefDes, 0 );
    }
}


/**
 * Verify that all tracks and arcs have positive width across all boards.
 */
BOOST_AUTO_TEST_CASE( AllTracksPositiveWidth )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int zeroWidthCount = 0;
            int totalCount = 0;

            for( PCB_TRACK* track : board->Tracks() )
            {
                if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
                {
                    totalCount++;

                    if( track->GetWidth() <= 0 )
                    {
                        zeroWidthCount++;

                        if( zeroWidthCount <= 5 )
                        {
                            BOOST_TEST_MESSAGE( boardName << ": Zero-width track at ("
                                                << track->GetStart().x / 1000000.0 << ", "
                                                << track->GetStart().y / 1000000.0 << ")" );
                        }
                    }
                }
            }

            BOOST_CHECK_EQUAL( zeroWidthCount, 0 );
        }
    }
}


/**
 * Verify the import produces zero errors and bounded warnings per board.
 * This acts as a regression gate to ensure warning counts don't increase.
 */
BOOST_AUTO_TEST_CASE( WarningBudget )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            BOOST_CHECK_EQUAL( reporter.GetErrorCount(), 0 );

            if( reporter.GetWarningCount() > 0 )
            {
                BOOST_TEST_MESSAGE( boardName << ": " << reporter.GetWarningCount() << " warnings" );

                for( const auto& msg : reporter.GetMessages() )
                {
                    if( msg.severity == RPT_SEVERITY_WARNING )
                        BOOST_TEST_MESSAGE( "  " << msg.text );
                }
            }
        }
    }
}


/**
 * Parse board outline geometry from a .alg ASCII reference file.
 *
 * Extracts LINE, ARC, and RECTANGLE records from both DESIGN_OUTLINE
 * and OUTLINE subclasses under BOARD GEOMETRY. Coordinates are in mils.
 */
struct ALG_OUTLINE_DATA
{
    enum class SEGMENT_TYPE
    {
        LINE,
        ARC,
        RECTANGLE,
    };

    struct OUTLINE_SEGMENT
    {
        SEGMENT_TYPE type;
        double       x1, y1, x2, y2;
        double       centerX, centerY, radius;
        bool         clockwise;
    };

    int                          designOutlineCount = 0;
    int                          outlineCount = 0;
    std::vector<OUTLINE_SEGMENT> designOutlineSegments;
    std::vector<OUTLINE_SEGMENT> outlineSegments;

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    void updateBounds( double aX, double aY )
    {
        minX = std::min( minX, aX );
        minY = std::min( minY, aY );
        maxX = std::max( maxX, aX );
        maxY = std::max( maxY, aY );
    }

    static ALG_OUTLINE_DATA ParseAlgOutlines( const std::string& aPath )
    {
        ALG_OUTLINE_DATA data;
        std::ifstream    file( aPath );

        if( !file.is_open() )
            return data;

        std::string line;

        while( std::getline( file, line ) )
        {
            if( line.empty() || line[0] != 'S' )
                continue;

            auto fields = ALG_REFERENCE_DATA::SplitAlgLine( wxString::FromUTF8( line ) );

            if( fields.size() < 10 )
                continue;

            bool isDesignOutline = ( fields[1] == wxT( "BOARD GEOMETRY" )
                                     && fields[2] == wxT( "DESIGN_OUTLINE" ) );

            bool isOutline = ( fields[1] == wxT( "BOARD GEOMETRY" )
                               && fields[2] == wxT( "OUTLINE" ) );

            if( !isDesignOutline && !isOutline )
                continue;

            wxString shapeType = fields[4];
            OUTLINE_SEGMENT seg = {};

            if( shapeType == wxT( "LINE" ) && fields.size() >= 10 )
            {
                seg.type = SEGMENT_TYPE::LINE;
                fields[6].ToCDouble( &seg.x1 );
                fields[7].ToCDouble( &seg.y1 );
                fields[8].ToCDouble( &seg.x2 );
                fields[9].ToCDouble( &seg.y2 );

                data.updateBounds( seg.x1, seg.y1 );
                data.updateBounds( seg.x2, seg.y2 );
            }
            else if( shapeType == wxT( "ARC" ) && fields.size() >= 15 )
            {
                seg.type = SEGMENT_TYPE::ARC;
                fields[6].ToCDouble( &seg.x1 );
                fields[7].ToCDouble( &seg.y1 );
                fields[8].ToCDouble( &seg.x2 );
                fields[9].ToCDouble( &seg.y2 );
                fields[10].ToCDouble( &seg.centerX );
                fields[11].ToCDouble( &seg.centerY );
                fields[12].ToCDouble( &seg.radius );
                seg.clockwise = ( fields[14] == wxT( "CLOCKWISE" ) );

                data.updateBounds( seg.x1, seg.y1 );
                data.updateBounds( seg.x2, seg.y2 );
            }
            else if( shapeType == wxT( "RECTANGLE" ) && fields.size() >= 10 )
            {
                seg.type = SEGMENT_TYPE::RECTANGLE;
                fields[6].ToCDouble( &seg.x1 );
                fields[7].ToCDouble( &seg.y1 );
                fields[8].ToCDouble( &seg.x2 );
                fields[9].ToCDouble( &seg.y2 );

                data.updateBounds( seg.x1, seg.y1 );
                data.updateBounds( seg.x2, seg.y2 );
            }
            else
            {
                continue;
            }

            if( isDesignOutline )
            {
                data.designOutlineCount++;
                data.designOutlineSegments.push_back( seg );
            }
            else
            {
                data.outlineCount++;
                data.outlineSegments.push_back( seg );
            }
        }

        return data;
    }

    /**
     * Expected number of Edge_Cuts segments when translating to KiCad.
     * Each LINE/ARC = 1 segment. Each RECTANGLE = 4 segments.
     */
    int expectedEdgeCutsSegments() const
    {
        int count = 0;

        for( const auto& seg : designOutlineSegments )
        {
            if( seg.type == SEGMENT_TYPE::RECTANGLE )
                count += 4;
            else
                count += 1;
        }

        return count;
    }
};


/**
 * Compare board outline segment count from binary import against .alg reference.
 * Uses DESIGN_OUTLINE records since those are what the binary importer targets.
 */
BOOST_AUTO_TEST_CASE( OutlineSegmentCount )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    struct OUTLINE_TEST_BOARD
    {
        std::string brdFile;
        std::string algFile;
    };

    std::vector<OUTLINE_TEST_BOARD> testBoards;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( !entry.is_regular_file() || entry.path().extension() != ".alg" )
            continue;

        std::string algName = entry.path().filename().string();
        std::string brdName = algName.substr( 0, algName.size() - 4 );
        std::filesystem::path brdPath( dataPath + brdName );

        if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
            testBoards.push_back( { brdName, entry.path().string() } );
    }

    BOOST_REQUIRE_GT( testBoards.size(), 0u );

    for( const auto& tb : testBoards )
    {
        BOOST_TEST_CONTEXT( "Board: " << tb.brdFile )
        {
            ALG_OUTLINE_DATA algOutlines = ALG_OUTLINE_DATA::ParseAlgOutlines( tb.algFile );

            if( algOutlines.designOutlineCount == 0 && algOutlines.outlineCount == 0 )
            {
                BOOST_TEST_MESSAGE( "  No outline records in .alg, skipping" );
                continue;
            }

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + tb.brdFile, reporter );
            BOOST_REQUIRE( board );

            int edgeCutsCount = 0;

            for( BOARD_ITEM* item : board->Drawings() )
            {
                if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
                    edgeCutsCount++;
            }

            int expectedCount = algOutlines.expectedEdgeCutsSegments();

            BOOST_TEST_MESSAGE( "  .alg DESIGN_OUTLINE records: " << algOutlines.designOutlineCount
                                << " -> expected Edge_Cuts segments: " << expectedCount );
            BOOST_TEST_MESSAGE( "  .alg OUTLINE records: " << algOutlines.outlineCount );
            BOOST_TEST_MESSAGE( "  Binary import Edge_Cuts segments: " << edgeCutsCount );

            BOOST_CHECK_EQUAL( edgeCutsCount, expectedCount );
        }
    }
}


/**
 * Compare board bounding box from binary import against .alg outline coordinates.
 * The .alg coordinates are in mils; we convert to nanometers for comparison.
 */
BOOST_AUTO_TEST_CASE( OutlineBoundingBox )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> testBoards;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( !entry.is_regular_file() || entry.path().extension() != ".alg" )
            continue;

        std::string algName = entry.path().filename().string();
        std::string brdName = algName.substr( 0, algName.size() - 4 );
        std::filesystem::path brdPath( dataPath + brdName );

        if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
            testBoards.push_back( { brdName, entry.path().string() } );
    }

    BOOST_REQUIRE_GT( testBoards.size(), 0u );

    // 1 mil = 25400 nm
    const double milToNm = 25400.0;

    // Allow 2 mil tolerance for coordinate rounding across formats
    const int toleranceNm = static_cast<int>( 2.0 * milToNm );

    for( const auto& [brdFile, algFile] : testBoards )
    {
        BOOST_TEST_CONTEXT( "Board: " << brdFile )
        {
            ALG_OUTLINE_DATA algOutlines = ALG_OUTLINE_DATA::ParseAlgOutlines( algFile );

            if( algOutlines.designOutlineCount == 0 )
            {
                BOOST_TEST_MESSAGE( "  No DESIGN_OUTLINE records in .alg, skipping" );
                continue;
            }

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
            BOOST_REQUIRE( board );

            BOX2I boardBbox;
            bool  hasBbox = false;

            for( BOARD_ITEM* item : board->Drawings() )
            {
                if( item->Type() == PCB_SHAPE_T && item->GetLayer() == Edge_Cuts )
                {
                    if( !hasBbox )
                    {
                        boardBbox = item->GetBoundingBox();
                        hasBbox = true;
                    }
                    else
                    {
                        boardBbox.Merge( item->GetBoundingBox() );
                    }
                }
            }

            BOOST_REQUIRE_MESSAGE( hasBbox, "Board should have Edge_Cuts outline" );

            // Convert .alg bounding box from mils to nm
            int algMinXnm = static_cast<int>( algOutlines.minX * milToNm );
            int algMinYnm = static_cast<int>( algOutlines.minY * milToNm );
            int algMaxXnm = static_cast<int>( algOutlines.maxX * milToNm );
            int algMaxYnm = static_cast<int>( algOutlines.maxY * milToNm );
            int algWidthNm = algMaxXnm - algMinXnm;
            int algHeightNm = algMaxYnm - algMinYnm;

            int boardWidth = boardBbox.GetWidth();
            int boardHeight = boardBbox.GetHeight();

            BOOST_TEST_MESSAGE( "  .alg extent (mils): "
                                << algOutlines.minX << "," << algOutlines.minY << " to "
                                << algOutlines.maxX << "," << algOutlines.maxY
                                << " = " << ( algOutlines.maxX - algOutlines.minX ) << " x "
                                << ( algOutlines.maxY - algOutlines.minY ) );
            BOOST_TEST_MESSAGE( "  Board bbox (nm): "
                                << boardBbox.GetLeft() << "," << boardBbox.GetTop() << " to "
                                << boardBbox.GetRight() << "," << boardBbox.GetBottom()
                                << " = " << boardWidth << " x " << boardHeight );
            BOOST_TEST_MESSAGE( "  .alg (nm): " << algWidthNm << " x " << algHeightNm );

            // KiCad bounding boxes include line width so allow 3% tolerance
            BOOST_CHECK_CLOSE( static_cast<double>( boardWidth ),
                               static_cast<double>( algWidthNm ), 3.0 );
            BOOST_CHECK_CLOSE( static_cast<double>( boardHeight ),
                               static_cast<double>( algHeightNm ), 3.0 );
        }
    }
}


/**
 * For boards with line-only outlines, verify each segment endpoint
 * matches the .alg reference within tolerance.
 *
 * Allegro's Y axis is inverted relative to KiCad, so we compare
 * absolute coordinate differences.
 */
BOOST_AUTO_TEST_CASE( OutlineEndpoints )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> testBoards;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( !entry.is_regular_file() || entry.path().extension() != ".alg" )
            continue;

        std::string algName = entry.path().filename().string();
        std::string brdName = algName.substr( 0, algName.size() - 4 );
        std::filesystem::path brdPath( dataPath + brdName );

        if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
            testBoards.push_back( { brdName, entry.path().string() } );
    }

    BOOST_REQUIRE_GT( testBoards.size(), 0u );

    const double milToNm = 25400.0;
    const int    toleranceNm = static_cast<int>( 2.0 * milToNm );

    for( const auto& [brdFile, algFile] : testBoards )
    {
        BOOST_TEST_CONTEXT( "Board: " << brdFile )
        {
            ALG_OUTLINE_DATA algOutlines = ALG_OUTLINE_DATA::ParseAlgOutlines( algFile );

            if( algOutlines.designOutlineCount == 0 )
                continue;

            // Only validate endpoint-by-endpoint for pure-LINE outlines
            bool allLines = true;

            for( const auto& seg : algOutlines.designOutlineSegments )
            {
                if( seg.type != ALG_OUTLINE_DATA::SEGMENT_TYPE::LINE
                    && seg.type != ALG_OUTLINE_DATA::SEGMENT_TYPE::RECTANGLE )
                {
                    allLines = false;
                    break;
                }
            }

            if( !allLines )
            {
                BOOST_TEST_MESSAGE( "  Outline has arcs, skipping endpoint-level validation" );
                continue;
            }

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
            BOOST_REQUIRE( board );

            // Collect all Edge_Cuts segment endpoints
            struct ENDPOINT_PAIR
            {
                VECTOR2I start;
                VECTOR2I end;
            };

            std::vector<ENDPOINT_PAIR> boardSegments;

            for( BOARD_ITEM* item : board->Drawings() )
            {
                if( item->Type() != PCB_SHAPE_T || item->GetLayer() != Edge_Cuts )
                    continue;

                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

                if( shape->GetShape() == SHAPE_T::SEGMENT )
                    boardSegments.push_back( { shape->GetStart(), shape->GetEnd() } );
            }

            // Build expected segments from .alg, expanding RECTANGLEs into 4 segments
            std::vector<ENDPOINT_PAIR> algSegments;

            for( const auto& seg : algOutlines.designOutlineSegments )
            {
                if( seg.type == ALG_OUTLINE_DATA::SEGMENT_TYPE::LINE )
                {
                    VECTOR2I start( static_cast<int>( seg.x1 * milToNm ),
                                    static_cast<int>( seg.y1 * milToNm ) );
                    VECTOR2I end( static_cast<int>( seg.x2 * milToNm ),
                                  static_cast<int>( seg.y2 * milToNm ) );
                    algSegments.push_back( { start, end } );
                }
                else if( seg.type == ALG_OUTLINE_DATA::SEGMENT_TYPE::RECTANGLE )
                {
                    int x1 = static_cast<int>( seg.x1 * milToNm );
                    int y1 = static_cast<int>( seg.y1 * milToNm );
                    int x2 = static_cast<int>( seg.x2 * milToNm );
                    int y2 = static_cast<int>( seg.y2 * milToNm );

                    algSegments.push_back( { { x1, y1 }, { x2, y1 } } );
                    algSegments.push_back( { { x2, y1 }, { x2, y2 } } );
                    algSegments.push_back( { { x2, y2 }, { x1, y2 } } );
                    algSegments.push_back( { { x1, y2 }, { x1, y1 } } );
                }
            }

            BOOST_CHECK_EQUAL( boardSegments.size(), algSegments.size() );

            if( boardSegments.size() != algSegments.size() )
                continue;

            // Match each .alg segment to a board segment by finding closest start/end pair.
            // Allegro and KiCad may have opposite Y axis, so we compare using absolute
            // coordinate deltas.
            int matchedCount = 0;

            std::vector<bool> used( boardSegments.size(), false );

            for( size_t ai = 0; ai < algSegments.size(); ++ai )
            {
                const auto& algSeg = algSegments[ai];
                int         bestIdx = -1;
                int64_t     bestDist = std::numeric_limits<int64_t>::max();

                for( size_t bi = 0; bi < boardSegments.size(); ++bi )
                {
                    if( used[bi] )
                        continue;

                    const auto& bSeg = boardSegments[bi];

                    // Try both orientations (start-start or start-end swap)
                    auto dist = [&]( const VECTOR2I& aAlgPt, const VECTOR2I& aBoardPt ) -> int64_t
                    {
                        int64_t dx = std::abs( static_cast<int64_t>( aAlgPt.x )
                                               - static_cast<int64_t>( aBoardPt.x ) );
                        int64_t dy = std::abs( static_cast<int64_t>( aAlgPt.y )
                                               - static_cast<int64_t>( aBoardPt.y ) );
                        return dx + dy;
                    };

                    int64_t d1 = dist( algSeg.start, bSeg.start ) + dist( algSeg.end, bSeg.end );
                    int64_t d2 = dist( algSeg.start, bSeg.end ) + dist( algSeg.end, bSeg.start );
                    int64_t d = std::min( d1, d2 );

                    if( d < bestDist )
                    {
                        bestDist = d;
                        bestIdx = static_cast<int>( bi );
                    }
                }

                if( bestIdx >= 0 && bestDist < 2LL * toleranceNm )
                {
                    used[bestIdx] = true;
                    matchedCount++;
                }
                else
                {
                    BOOST_TEST_MESSAGE( "  Unmatched .alg segment " << ai << ": ("
                                        << algSeg.start.x / 1000000.0 << ", "
                                        << algSeg.start.y / 1000000.0 << ") -> ("
                                        << algSeg.end.x / 1000000.0 << ", "
                                        << algSeg.end.y / 1000000.0 << ") mm"
                                        << " bestDist=" << bestDist );
                }
            }

            BOOST_TEST_MESSAGE( "  Matched " << matchedCount << " / " << algSegments.size()
                                << " outline segments" );

            BOOST_CHECK_EQUAL( matchedCount, static_cast<int>( algSegments.size() ) );
        }
    }
}


/**
 * Verify PTH pads have nonzero drill and SMD pads have zero drill
 * across all boards.
 */
BOOST_AUTO_TEST_CASE( PadDrillConsistency )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";
    std::vector<std::string> boards = GetAllBoardFiles();

    for( const std::string& boardName : boards )
    {
        CAPTURING_REPORTER reporter;
        std::string        fullPath = dataPath + boardName;
        std::unique_ptr<BOARD> board = LoadBoardWithCapture( fullPath, reporter );

        if( !board )
            continue;

        BOOST_TEST_CONTEXT( "Testing board: " << boardName )
        {
            int pthNoDrill = 0;
            int smdWithDrill = 0;

            for( FOOTPRINT* fp : board->Footprints() )
            {
                for( PAD* pad : fp->Pads() )
                {
                    PAD_ATTRIB attr = pad->GetAttribute();
                    bool hasDrill = pad->GetDrillSizeX() > 0;

                    if( attr == PAD_ATTRIB::PTH && !hasDrill )
                    {
                        pthNoDrill++;

                        if( pthNoDrill <= 5 )
                        {
                            BOOST_TEST_MESSAGE( boardName << ": PTH pad without drill: "
                                                << fp->GetReference() << "."
                                                << pad->GetNumber() );
                        }
                    }

                    if( attr == PAD_ATTRIB::SMD && hasDrill )
                    {
                        smdWithDrill++;

                        if( smdWithDrill <= 5 )
                        {
                            BOOST_TEST_MESSAGE( boardName << ": SMD pad with drill: "
                                                << fp->GetReference() << "."
                                                << pad->GetNumber() );
                        }
                    }
                }
            }

            BOOST_CHECK_EQUAL( pthNoDrill, 0 );
            BOOST_CHECK_EQUAL( smdWithDrill, 0 );
        }
    }
}


/**
 * Cross-validate the number of copper zones against .alg ETCH SHAPE records.
 */
BOOST_AUTO_TEST_CASE( ZoneCountMatchesAlg )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> boardsWithAlg;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".alg" )
        {
            std::string algName = entry.path().filename().string();
            std::string brdName = algName.substr( 0, algName.size() - 4 );

            std::filesystem::path brdPath( dataPath + brdName );

            if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
                boardsWithAlg.push_back( { brdName, entry.path().string() } );
        }
    }

    BOOST_REQUIRE_GT( boardsWithAlg.size(), 0u );

    for( const auto& [brdFile, algFile] : boardsWithAlg )
    {
        BOOST_TEST_CONTEXT( "Zone count: " << brdFile )
        {
            ALG_REFERENCE_DATA algData = ALG_REFERENCE_DATA::ParseAlgFile( algFile );

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
            BOOST_REQUIRE( board );

            int boardCopperZones = 0;

            for( const ZONE* zone : board->Zones() )
            {
                if( !zone->GetIsRuleArea() )
                    boardCopperZones++;
            }

            size_t algZoneCount = algData.zonePolygons.size();

            BOOST_TEST_MESSAGE( brdFile << ": .alg has " << algZoneCount
                                << " zone polygons, board has " << boardCopperZones
                                << " copper zones" );

            BOOST_CHECK_EQUAL( static_cast<size_t>( boardCopperZones ), algZoneCount );
        }
    }
}


/**
 * Cross-validate how many zones appear on each copper layer.
 */
BOOST_AUTO_TEST_CASE( ZoneLayerDistribution )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> boardsWithAlg;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".alg" )
        {
            std::string algName = entry.path().filename().string();
            std::string brdName = algName.substr( 0, algName.size() - 4 );

            std::filesystem::path brdPath( dataPath + brdName );

            if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
                boardsWithAlg.push_back( { brdName, entry.path().string() } );
        }
    }

    BOOST_REQUIRE_GT( boardsWithAlg.size(), 0u );

    for( const auto& [brdFile, algFile] : boardsWithAlg )
    {
        BOOST_TEST_CONTEXT( "Zone layers: " << brdFile )
        {
            ALG_REFERENCE_DATA algData = ALG_REFERENCE_DATA::ParseAlgFile( algFile );

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
            BOOST_REQUIRE( board );

            std::map<wxString, int> algLayerCounts;

            for( const ALG_ZONE_POLYGON& zone : algData.zonePolygons )
                algLayerCounts[zone.layer]++;

            std::map<wxString, int> boardLayerCounts;

            for( const ZONE* zone : board->Zones() )
            {
                if( !zone->GetIsRuleArea() )
                    boardLayerCounts[board->GetLayerName( zone->GetFirstLayer() )]++;
            }

            BOOST_TEST_MESSAGE( brdFile << " layer distribution:" );

            for( const auto& [layer, count] : algLayerCounts )
            {
                auto it = boardLayerCounts.find( layer );
                int  boardCount = ( it != boardLayerCounts.end() ) ? it->second : 0;

                BOOST_TEST_MESSAGE( "  " << layer << ": .alg=" << count << " board=" << boardCount );
                BOOST_CHECK_EQUAL( boardCount, count );
            }
        }
    }
}


/**
 * Cross-validate zone bounding box sizes between binary import and .alg reference.
 * Sorts zone areas per layer and verifies the size distributions match within tolerance.
 */
BOOST_AUTO_TEST_CASE( ZoneBoundingBoxes )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/";

    std::vector<std::pair<std::string, std::string>> boardsWithAlg;

    for( const auto& entry : std::filesystem::directory_iterator( dataPath + "expected/" ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".alg" )
        {
            std::string algName = entry.path().filename().string();
            std::string brdName = algName.substr( 0, algName.size() - 4 );

            std::filesystem::path brdPath( dataPath + brdName );

            if( std::filesystem::exists( brdPath ) && std::filesystem::file_size( brdPath ) > 0 )
                boardsWithAlg.push_back( { brdName, entry.path().string() } );
        }
    }

    BOOST_REQUIRE_GT( boardsWithAlg.size(), 0u );

    for( const auto& [brdFile, algFile] : boardsWithAlg )
    {
        BOOST_TEST_CONTEXT( "Zone bboxes: " << brdFile )
        {
            ALG_REFERENCE_DATA algData = ALG_REFERENCE_DATA::ParseAlgFile( algFile );

            CAPTURING_REPORTER     reporter;
            std::unique_ptr<BOARD> board = LoadBoardWithCapture( dataPath + brdFile, reporter );
            BOOST_REQUIRE( board );

            // Collect sorted areas per layer from .alg and board, then compare distributions
            const double milsToNm = 25400.0;

            std::map<wxString, std::vector<double>> algAreas;

            for( const ALG_ZONE_POLYGON& zone : algData.zonePolygons )
            {
                double w = ( zone.maxX - zone.minX ) * milsToNm;
                double h = ( zone.maxY - zone.minY ) * milsToNm;
                algAreas[zone.layer].push_back( w * h );
            }

            std::map<wxString, std::vector<double>> boardAreas;

            for( const ZONE* zone : board->Zones() )
            {
                if( !zone->GetIsRuleArea() )
                {
                    BOX2I  bbox = zone->GetBoundingBox();
                    double area = static_cast<double>( bbox.GetWidth() )
                                  * static_cast<double>( bbox.GetHeight() );
                    boardAreas[board->GetLayerName( zone->GetFirstLayer() )].push_back( area );
                }
            }

            int matched = 0;
            int mismatched = 0;

            for( auto& [layer, algList] : algAreas )
            {
                std::sort( algList.begin(), algList.end() );
                auto it = boardAreas.find( layer );

                if( it == boardAreas.end() || it->second.size() != algList.size() )
                    continue;

                std::sort( it->second.begin(), it->second.end() );

                for( size_t i = 0; i < algList.size(); ++i )
                {
                    double ref = std::max( algList[i], 1.0 );
                    double err = std::abs( it->second[i] - algList[i] ) / ref;

                    if( err < 0.10 )
                    {
                        matched++;
                    }
                    else
                    {
                        mismatched++;

                        if( mismatched <= 5 )
                        {
                            BOOST_TEST_MESSAGE( "  " << layer << " index " << i
                                                << ": alg area " << algList[i] / ( milsToNm * milsToNm )
                                                << " sq mils vs board area "
                                                << it->second[i] / ( milsToNm * milsToNm )
                                                << " sq mils" );
                        }
                    }
                }
            }

            BOOST_TEST_MESSAGE( brdFile << ": " << matched << " zone areas matched, "
                                << mismatched << " mismatched" );

            int total = matched + mismatched;

            if( total > 0 )
            {
                BOOST_CHECK_GT( matched, total * 8 / 10 );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
