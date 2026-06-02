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
 * @file test_diptrace_import.cpp
 * Test suite for import of DipTrace PCB (.dip) files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/diptrace/pcb_io_diptrace.h>
#include <pcbnew/pcb_io/diptrace/diptrace_pcb_parser.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <netclass.h>
#include <project/net_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>


struct DIPTRACE_PCB_IMPORT_FIXTURE
{
    DIPTRACE_PCB_IMPORT_FIXTURE() {}

    PCB_IO_DIPTRACE m_plugin;

    std::string GetTestDataDir()
    {
        return KI_TEST::GetPcbnewTestDataDir() + "plugins/diptrace/";
    }
};


BOOST_FIXTURE_TEST_SUITE( DipTracePcbImport, DIPTRACE_PCB_IMPORT_FIXTURE )


/**
 * Test that CanReadBoard correctly identifies DipTrace .dip files by their
 * magic header bytes (0x07 "DTBOARD").
 */
BOOST_AUTO_TEST_CASE( CanReadBoard )
{
    BOOST_CHECK( m_plugin.CanReadBoard( GetTestDataDir() + "keyboard.dip" ) );
    BOOST_CHECK( m_plugin.CanReadBoard( GetTestDataDir() + "156bus_narrow.dip" ) );
    BOOST_CHECK( m_plugin.CanReadBoard( GetTestDataDir() + "z80_board.dip" ) );
    BOOST_CHECK( m_plugin.CanReadBoard( GetTestDataDir() + "logic_probe.dip" ) );
    BOOST_CHECK( m_plugin.CanReadBoard( GetTestDataDir() + "project4.dip" ) );

    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_legacy_" ) );
    wxRemoveFile( tempBase );
    wxString legacyPath = tempBase + wxS( ".dip" );

    {
        const uint8_t legacyHeader[] = {
            0x0B, 'D', 'T', 'B', 'O', 'A', 'R', 'D', '2', '.', '2', '1'
        };

        wxFFile file( legacyPath, wxS( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE_EQUAL( file.Write( legacyHeader, sizeof( legacyHeader ) ),
                             sizeof( legacyHeader ) );
    }

    BOOST_CHECK( m_plugin.CanReadBoard( legacyPath ) );
    wxRemoveFile( legacyPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentHeaderFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dip";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_pcb_comp_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dip" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_FLAGS_OFFSET = 0x491;
        const uint8_t invalidFlags[] = { 0xFF, 0x00, 0x00, 0x00 };

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_FLAGS_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidFlags, sizeof( invalidFlags ) ),
                             sizeof( invalidFlags ) );
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_CHECK_THROW( m_plugin.LoadBoard( tempPath.ToStdString(), board.get() ), IO_ERROR );

    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidRouteChainNodeCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dip";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_route_chain_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dip" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_ROUTE_CHAIN_NODE_COUNT_OFFSET = 0x1CEA7;
        const uint8_t invalidNodeCount[] = { 0x0F, 0x69, 0x51 }; // int3 value 10001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_ROUTE_CHAIN_NODE_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidNodeCount, sizeof( invalidNodeCount ) ),
                             sizeof( invalidNodeCount ) );
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_CHECK_THROW( m_plugin.LoadBoard( tempPath.ToStdString(), board.get() ), IO_ERROR );

    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidNetNameLengthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dip";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_net_name_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dip" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_NET_NAME_LENGTH_OFFSET = 0x1CE2D;
        const uint8_t invalidNameLength[] = { 0x01, 0xF5 }; // UTF-16 char count 501

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_NET_NAME_LENGTH_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidNameLength, sizeof( invalidNameLength ) ),
                             sizeof( invalidNameLength ) );
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_CHECK_THROW( m_plugin.LoadBoard( tempPath.ToStdString(), board.get() ), IO_ERROR );

    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidZoneMinWidthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dip";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_zone_width_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dip" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_ZONE_MIN_WIDTH_OFFSET = 0x382CB;
        const uint8_t zeroMinWidth[] = { 0x3B, 0x9A, 0xCA, 0x00 }; // int4 value 0

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_ZONE_MIN_WIDTH_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( zeroMinWidth, sizeof( zeroMinWidth ) ),
                             sizeof( zeroMinWidth ) );
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_CHECK_THROW( m_plugin.LoadBoard( tempPath.ToStdString(), board.get() ), IO_ERROR );

    wxRemoveFile( tempPath );
}


/**
 * Load keyboard.dip (DipTrace format v54, complex board with ~103 components).
 * Verifies that the board loads without assertions and has a reasonable number
 * of footprints.
 */
BOOST_AUTO_TEST_CASE( LoadKeyboard )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "keyboard.dip", board.get() );

    BOOST_REQUIRE( board );

    // keyboard.dip has approximately 103 components
    BOOST_CHECK_GT( board->Footprints().size(), 50 );

    // keyboard.dip has ~70 nets (key matrix: col1-6, row1-8, diode nets, etc.)
    BOOST_CHECK_GT( board->GetNetCount(), 20 );
}


/**
 * Determinism gate for the DipTrace PCB importer.
 *
 * Every component, pad, shape, mount hole, and post-component section must be located
 * by a field-derived offset, not by byte-pattern scanning. PCB_PARSER::ScanLocatorUseCount()
 * counts the scan-based locates; a fully deterministic decode reports zero. This mirrors the
 * schematic importer's ComponentBoundaryScanCount() == 0 invariant. The remaining pattern
 * scanners are retained only as recovery fallbacks.
 *
 * This is the gate for the staged determinism work (see
 * ~/Developer/AGENT/plans/2026-06-01-diptrace-pcb-deterministic-import.md). It currently fails
 * because object/section location is still scan-based; it is driven to zero category-by-category.
 */
BOOST_AUTO_TEST_CASE( ObjectsAreFieldLocatedNotScanned )
{
    const std::vector<std::string> files = {
        "keyboard.dip", "156bus_narrow.dip", "logic_probe.dip", "project4.dip", "z80_board.dip"
    };

    for( const std::string& name : files )
    {
        const std::string path = GetTestDataDir() + name;
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
        board->SetFileName( wxString::FromUTF8( path ) );

        DIPTRACE::PCB_PARSER parser( wxString::FromUTF8( path ), board.get() );
        parser.Parse();

        // Per-category regression guards for the categories already converted to a
        // deterministic field-walk. Pad records are field-located from the component
        // header; mount holes carry no scan in this corpus.
        BOOST_CHECK_MESSAGE( parser.PadLocatorScans() == 0,
                             name + ": pad located by scan (" + std::to_string( parser.PadLocatorScans() )
                                     + ")" );
        BOOST_CHECK_MESSAGE( parser.MountHoleLocatorScans() == 0,
                             name + ": mount hole located by scan ("
                                     + std::to_string( parser.MountHoleLocatorScans() ) + ")" );
        BOOST_CHECK_MESSAGE( parser.ShapeLocatorScans() == 0,
                             name + ": shape located by scan ("
                                     + std::to_string( parser.ShapeLocatorScans() ) + ")" );
        BOOST_CHECK_MESSAGE( parser.ComponentLocatorScans() == 0,
                             name + ": component located by scan ("
                                     + std::to_string( parser.ComponentLocatorScans() ) + ")" );
        BOOST_CHECK_MESSAGE( parser.SectionLocatorScans() == 0,
                             name + ": section located by scan ("
                                     + std::to_string( parser.SectionLocatorScans() ) + ")" );

        BOOST_CHECK_MESSAGE(
                parser.ScanLocatorUseCount() == 0,
                name + ": object/section located by byte-pattern scan (total="
                        + std::to_string( parser.ScanLocatorUseCount() )
                        + " components=" + std::to_string( parser.ComponentLocatorScans() )
                        + " pads=" + std::to_string( parser.PadLocatorScans() )
                        + " shapes=" + std::to_string( parser.ShapeLocatorScans() )
                        + " holes=" + std::to_string( parser.MountHoleLocatorScans() )
                        + " sections=" + std::to_string( parser.SectionLocatorScans() ) + ")" );
    }
}


/**
 * Load 156bus_narrow.dip (DipTrace format v41, has board outline with arcs
 * and text objects).  Verifies board outline is present on Edge.Cuts.
 */
BOOST_AUTO_TEST_CASE( Load156Bus )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "156bus_narrow.dip", board.get() );

    BOOST_REQUIRE( board );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );

    // 156bus_narrow has a board outline with 12 vertices including arcs
    bool hasOutline = false;

    for( BOARD_ITEM* drawing : board->Drawings() )
    {
        if( drawing->GetLayer() == Edge_Cuts )
        {
            hasOutline = true;
            break;
        }
    }

    BOOST_CHECK( hasOutline );
}


/**
 * Load project4.dip (DipTrace format v37, oldest supported format using
 * legacy ASCII string encoding).  Ensures backward compatibility.
 */
BOOST_AUTO_TEST_CASE( LoadV37 )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "project4.dip", board.get() );

    BOOST_REQUIRE( board );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
}


/**
 * Load logic_probe.dip and verify basic import succeeds.
 */
BOOST_AUTO_TEST_CASE( LoadLogicProbe )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "logic_probe.dip", board.get() );

    BOOST_REQUIRE( board );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );
}


/**
 * Load Z80 board and verify component, net, and pad import.
 * The Z80 board is a paired test with a matching schematic.
 */
BOOST_AUTO_TEST_CASE( LoadZ80Board )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    // Z80 computer board should have a healthy number of components
    BOOST_CHECK_GT( board->Footprints().size(), 10 );

    // Verify reference designators are present on imported footprints
    int refsFound = 0;
    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        if( !fp->GetReference().IsEmpty() )
            refsFound++;

        totalPads += static_cast<int>( fp->Pads().size() );
    }

    BOOST_CHECK_GT( refsFound, 10 );
    BOOST_CHECK_GT( totalPads, 0 );

    // Z80 board has ~97 nets (address bus A0-A15, data bus D0-D7, control, etc.)
    BOOST_CHECK_GT( board->GetNetCount(), 20 );
}


/**
 * Load project4.dip (v37) and verify pad parsing works for the oldest supported format.
 */
BOOST_AUTO_TEST_CASE( LoadV37Pads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "project4.dip", board.get() );

    BOOST_REQUIRE( board );

    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
        totalPads += static_cast<int>( fp->Pads().size() );

    // project4.dip has 27 components, mostly 2-pin through-hole parts
    BOOST_CHECK_GT( totalPads, 0 );
}


/**
 * Load keyboard.dip (v54) and verify pad parsing works for the modern format.
 */
BOOST_AUTO_TEST_CASE( LoadKeyboardPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "keyboard.dip", board.get() );

    BOOST_REQUIRE( board );

    int totalPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
        totalPads += static_cast<int>( fp->Pads().size() );

    BOOST_CHECK_GT( totalPads, 0 );
}


/**
 * Load keyboard.dip (v54) and verify that footprint graphics are parsed from
 * the per-layer font block format introduced in v46. Before the fix, the v45
 * fixed-record shape parser found zero shapes in v46+ files because the binary
 * format changed to per-layer Tahoma font blocks containing line coordinates.
 */
BOOST_AUTO_TEST_CASE( KeyboardFootprintGraphics )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "keyboard.dip", board.get() );

    BOOST_REQUIRE( board );

    int totalGraphics = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                totalGraphics++;
        }
    }

    // keyboard.dip has 123 components; keyboard switches have 20 line segments each,
    // capacitors have 14 each. A healthy import should produce many hundreds of shapes.
    BOOST_CHECK_GT( totalGraphics, 100 );
}


/**
 * Load logic_probe.dip (v46) and verify footprint graphics. This is the earliest
 * version using font block shapes.
 */
BOOST_AUTO_TEST_CASE( LogicProbeFootprintGraphics )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "logic_probe.dip", board.get() );

    BOOST_REQUIRE( board );

    int totalGraphics = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                totalGraphics++;
        }
    }

    BOOST_CHECK_GT( totalGraphics, 50 );
}


/**
 * Load z80_board.dip (v45) and verify footprint graphics still work with the
 * original fixed-record parser path.
 */
BOOST_AUTO_TEST_CASE( Z80FootprintGraphics )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    int totalGraphics = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                totalGraphics++;
        }
    }

    // v45 uses fixed-record shapes; the Z80 board has fewer shape-bearing footprints
    // than the keyboard/logic_probe boards since many components are simple 2-pin DIP.
    BOOST_CHECK_GT( totalGraphics, 10 );
}


/**
 * Load logic_probe.dip (v46) and verify text positioning fields are parsed from
 * the 37-byte component tail. The refdes and value text should have non-zero Y
 * offsets for components with visible text.
 */
BOOST_AUTO_TEST_CASE( LogicProbeTextPositioning )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "logic_probe.dip", board.get() );

    BOOST_REQUIRE( board );

    int nonZeroRefY = 0;
    int nonZeroValY = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        VECTOR2I refPos = fp->Reference().GetPosition();
        VECTOR2I valPos = fp->Value().GetPosition();
        VECTOR2I fpPos = fp->GetPosition();

        if( refPos.y != fpPos.y )
            nonZeroRefY++;

        if( valPos.y != fpPos.y )
            nonZeroValY++;
    }

    // logic_probe.dip has 113 components; the component tail parser finds text
    // offsets for components where the 37-byte tail is intact at the expected position.
    BOOST_CHECK_GT( nonZeroRefY, 30 );
    BOOST_CHECK_GT( nonZeroValY, 3 );
}


/**
 * Load z80_board.dip (v45) and verify text positioning is parsed.
 */
BOOST_AUTO_TEST_CASE( Z80TextPositioning )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    int nonZeroRefY = 0;
    int nonZeroValY = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        VECTOR2I refPos = fp->Reference().GetPosition();
        VECTOR2I valPos = fp->Value().GetPosition();
        VECTOR2I fpPos = fp->GetPosition();

        if( refPos.y != fpPos.y )
            nonZeroRefY++;

        if( valPos.y != fpPos.y )
            nonZeroValY++;
    }

    BOOST_CHECK_GT( nonZeroRefY, 1 );
    BOOST_CHECK_GT( nonZeroValY, 1 );
}


/**
 * Load keyboard.dip (v54) and verify text positioning for the modern format.
 */
BOOST_AUTO_TEST_CASE( KeyboardTextPositioning )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "keyboard.dip", board.get() );

    BOOST_REQUIRE( board );

    int nonZeroRefY = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        VECTOR2I refPos = fp->Reference().GetPosition();
        VECTOR2I fpPos = fp->GetPosition();

        if( refPos.y != fpPos.y )
            nonZeroRefY++;
    }

    BOOST_CHECK_GT( nonZeroRefY, 30 );
}


/**
 * Load project4.dip (v37) and verify text positioning for the oldest supported format.
 */
BOOST_AUTO_TEST_CASE( V37TextPositioning )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "project4.dip", board.get() );

    BOOST_REQUIRE( board );

    int nonZeroValY = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        VECTOR2I valPos = fp->Value().GetPosition();
        VECTOR2I fpPos = fp->GetPosition();

        if( valPos.y != fpPos.y )
            nonZeroValY++;
    }

    // project4.dip v37 has 27 components; at least some should have value Y offsets
    BOOST_CHECK_GT( nonZeroValY, 5 );
}


/**
 * Load logic_probe.dip and keyboard.dip, count footprint graphics by shape type, and verify
 * that curved shapes (CIRCLE or ARC) are present alongside line segments.
 */
BOOST_AUTO_TEST_CASE( FootprintGraphicShapeTypes )
{
    std::vector<std::string> files = { "logic_probe.dip", "keyboard.dip" };

    int totalSegments = 0;
    int totalCircles  = 0;
    int totalArcs     = 0;

    for( const std::string& file : files )
    {
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

        m_plugin.LoadBoard( GetTestDataDir() + file, board.get() );

        BOOST_REQUIRE( board );

        int segments = 0;
        int circles  = 0;
        int arcs     = 0;

        for( const FOOTPRINT* fp : board->Footprints() )
        {
            for( const BOARD_ITEM* item : fp->GraphicalItems() )
            {
                if( item->Type() != PCB_SHAPE_T )
                    continue;

                const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

                switch( shape->GetShape() )
                {
                case SHAPE_T::SEGMENT: segments++; break;
                case SHAPE_T::CIRCLE:  circles++;  break;
                case SHAPE_T::ARC:     arcs++;     break;
                default:                            break;
                }
            }
        }

        BOOST_TEST_MESSAGE( file << ": SEGMENT=" << segments
                            << " CIRCLE=" << circles << " ARC=" << arcs );

        BOOST_CHECK_GT( segments + circles + arcs, 0 );

        totalSegments += segments;
        totalCircles  += circles;
        totalArcs     += arcs;
    }

    // Across both boards, verify we see a healthy mix of shape types and not just segments
    BOOST_CHECK_GT( totalSegments, 200 );
    BOOST_CHECK_GT( totalCircles + totalArcs, 0 );
}


/**
 * Load z80_board.dip (v45) and verify that polygon/custom pad shapes are imported.
 * The Z80 board contains pads with C=3 pad style (custom polygon outlines), such as
 * oblong THT pads and circular QFP pads encoded as vertex polygons.
 */
BOOST_AUTO_TEST_CASE( Z80PolygonPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    int customPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CUSTOM )
                customPads++;
        }
    }

    BOOST_CHECK_GT( customPads, 0 );
}


/**
 * Load z80_board.dip (v45) and verify that rectangular pads (padStyleC=2) are imported
 * with PAD_SHAPE::RECTANGLE instead of being treated as OVAL.
 */
BOOST_AUTO_TEST_CASE( Z80RectangularPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    int rectPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::RECTANGLE )
                rectPads++;
        }
    }

    // The Z80 board has SMD components with rectangular pads (padStyleC=2)
    BOOST_CHECK_GT( rectPads, 0 );
}


/**
 * Load keyboard.dip (v54) and verify pad shape differentiation. The keyboard board
 * has both circular THT pads and rectangular SMD pads.
 */
BOOST_AUTO_TEST_CASE( KeyboardPadShapes )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "keyboard.dip", board.get() );

    BOOST_REQUIRE( board );

    int circlePads = 0;
    int ovalPads = 0;
    int rectPads = 0;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            switch( pad->GetShape( PADSTACK::ALL_LAYERS ) )
            {
            case PAD_SHAPE::CIRCLE:    circlePads++; break;
            case PAD_SHAPE::OVAL:      ovalPads++;   break;
            case PAD_SHAPE::RECTANGLE: rectPads++;   break;
            default:                                  break;
            }
        }
    }

    BOOST_TEST_MESSAGE( "keyboard.dip pads: CIRCLE=" << circlePads
                        << " OVAL=" << ovalPads << " RECT=" << rectPads );

    // Should have at least some circular pads (THT switch pins)
    BOOST_CHECK_GT( circlePads, 0 );
}


/**
 * Load logic_probe.dip (v46) and verify that board design settings are applied.
 * This file has ViaStyles and design rules that should be mapped to the board defaults.
 */
BOOST_AUTO_TEST_CASE( LogicProbeBoardSettings )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "logic_probe.dip", board.get() );

    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    // Board should have at least 2 copper layers
    BOOST_CHECK_GE( board->GetCopperLayerCount(), 2 );

    // Default track width and clearance should be non-zero (from design rules)
    std::shared_ptr<NETCLASS> defNc = bds.m_NetSettings->GetDefaultNetclass();
    BOOST_CHECK_GT( defNc->GetTrackWidth(), 0 );
    BOOST_CHECK_GT( defNc->GetClearance(), 0 );

    // logic_probe.dip has ViaStyles, so via diameter should be set
    BOOST_CHECK_GT( defNc->GetViaDiameter(), 0 );
    BOOST_CHECK_GT( defNc->GetViaDrill(), 0 );
}


/**
 * Load z80_board.dip (v45) and verify board copper layer count.
 */
BOOST_AUTO_TEST_CASE( Z80BoardSettings )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_plugin.LoadBoard( GetTestDataDir() + "z80_board.dip", board.get() );

    BOOST_REQUIRE( board );

    // Z80 board is a 2-layer board
    BOOST_CHECK_EQUAL( board->GetCopperLayerCount(), 2 );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    std::shared_ptr<NETCLASS> defNc = bds.m_NetSettings->GetDefaultNetclass();

    // Default track width should have been set from design rules
    BOOST_CHECK_GT( defNc->GetTrackWidth(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
