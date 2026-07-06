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
#include <pad.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <zone.h>

#include <map>
#include <set>
#include <vector>

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

    static std::vector<ZONE*> copperPours( BOARD* aBoard )
    {
        std::vector<ZONE*> pours;

        for( ZONE* zone : aBoard->Zones() )
        {
            if( !zone->GetIsRuleArea() && IsCopperLayer( zone->GetFirstLayer() ) )
                pours.push_back( zone );
        }

        return pours;
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


/**
 * Regression test for inline long-text (0x3200) records. A text string longer than
 * the 5-byte inline field is stored as an empty text record followed by a 0x3200
 * longtext record carrying the full string. The decoder once had no row for 0x3200
 * and aborted with "Unknown Eagle binary block id 0x3200"; it now folds the string
 * onto the preceding text item. Each asserted string exceeds the inline field, so it
 * can only originate from a 0x3200 record.
 */
BOOST_AUTO_TEST_CASE( LoadBinaryLongText )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/issue24612_nova_usbbox.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );

    std::set<wxString> texts;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( item ) )
            texts.insert( text->GetText() );
    }

    BOOST_CHECK( texts.count( wxS( "ASTROELEKTRONIK" ) ) );
    BOOST_CHECK( texts.count( wxS( "Nova+ USB-Box" ) ) );
}


/**
 * Regression test for an over-counted recursive subsection. This board's signal
 * subsection declares more recursive children than the stream actually holds, so
 * the count-driven block walk ran off the end of the block stream and reached the
 * trailing free-text sentinel (0x1312), which is not a block and aborted the load
 * with "Unknown Eagle binary block id". The walk now stops when it reaches the
 * free-text section instead of treating it as another block.
 */
BOOST_AUTO_TEST_CASE( LoadV3RecursiveCountOverrun )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/Sigma2_e.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GT( board->Footprints().size(), 0u );
    BOOST_CHECK_GT( board->Tracks().size(), 0u );
    BOOST_CHECK_GT( board->GetNetInfo().GetNetCount(), 1u );
}


/**
 * Regression test for the binary rotation decode. boomchak places LED1..LED16 in a
 * ring spaced 22.5 degrees apart. The original decoder routed every angle below 90
 * degrees through a v3 fallback that masked the wrong nibble, collapsing LED2, LED3
 * and LED4 onto a single 180 degree orientation; the angle is now read as the
 * full-circle / 4096 fixed-point value the format stores for all versions.
 */
BOOST_AUTO_TEST_CASE( LoadV3FootprintRotationRing )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/boomchak.brd" ) );

    if( !board )
        return;

    std::map<int, double> ledRot;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        long n = 0;

        if( fp->GetReference().StartsWith( wxS( "LED" ) ) && fp->GetReference().Mid( 3 ).ToLong( &n ) )
            ledRot[(int) n] = fp->GetOrientation().AsDegrees();
    }

    for( int n = 1; n < 16; n++ )
    {
        BOOST_REQUIRE_MESSAGE( ledRot.count( n ) && ledRot.count( n + 1 ),
                               "expected ring footprints LED" << n << " and LED" << ( n + 1 ) );

        double step = ledRot[n + 1] - ledRot[n];

        while( step > 180.0 )
            step -= 360.0;

        while( step <= -180.0 )
            step += 360.0;

        BOOST_CHECK_CLOSE( step, 22.5, 0.5 );
    }
}


/**
 * Regression test for unmapped Eagle layers. Layers with no automatic KiCad target
 * (Eagle tRestrict/bRestrict/vRestrict/Measures and similar) resolve to UNSELECTED_LAYER
 * during a headless import, where no dialog can remap them. The item loaders only
 * skipped UNDEFINED_LAYER, so graphics on those layers were stranded on the out-of-range
 * UNSELECTED_LAYER (-2). Restrict layers carry keepout shapes, which must survive as rule
 * areas even though they have no graphic layer; only the genuinely unmappable graphics
 * are dropped. boomchak and turnemoff exercise both halves.
 */
BOOST_AUTO_TEST_CASE( LoadDropsUnmappedLayers )
{
    auto invalidLayerItems = []( BOARD* board )
    {
        auto bad = []( PCB_LAYER_ID l ) { return (int) l < 0 || (int) l >= PCB_LAYER_ID_COUNT; };

        int count = 0;

        for( BOARD_ITEM* item : board->Drawings() )
            count += bad( item->GetLayer() );

        for( PCB_TRACK* track : board->Tracks() )
            count += bad( track->GetLayer() );

        for( FOOTPRINT* fp : board->Footprints() )
        {
            for( BOARD_ITEM* item : fp->GraphicalItems() )
                count += bad( item->GetLayer() );
        }

        return count;
    };

    auto ruleAreas = []( BOARD* board )
    {
        int count = 0;

        for( ZONE* zone : board->Zones() )
            count += zone->GetIsRuleArea();

        return count;
    };

    for( const char* relPath : { "plugins/eagle_binary/boomchak.brd",
                                 "plugins/eagle_binary/turnemoff.brd" } )
    {
        std::unique_ptr<BOARD> board( loadBoard( relPath ) );

        if( !board )
            continue;

        BOOST_CHECK_EQUAL( invalidLayerItems( board.get() ), 0 );
    }

    // turnemoff carries hundreds of via-keepout rectangles on the vRestrict layer; the
    // unmapped-layer handling must keep them as rule areas, not drop them along with the
    // unmappable graphics.
    std::unique_ptr<BOARD> turnemoff( loadBoard( "plugins/eagle_binary/turnemoff.brd" ) );

    if( turnemoff )
        BOOST_CHECK_GT( ruleAreas( turnemoff.get() ), 500 );
}


/**
 * Regression test for the six-byte inline text field. Eagle stores a text-family
 * record's string in the final six bytes of its 24-byte block; the decoder read
 * only five, so the six-character ">VALUE" arrived as ">VALU". That truncated
 * string missed the ">VALUE" special case and fell through to the generic variable
 * substitution, producing the unresolvable "${VALU}" placeholder the DRC flagged.
 * blink1_b1a places ">VALUE" on several footprints, so a correct decode routes each
 * into the footprint value field and never emits the truncated placeholder.
 */
BOOST_AUTO_TEST_CASE( LoadRoutesSmashedValueText )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/blink1_b1a.brd" ) );

    if( !board )
        return;

    auto footprintTexts = [&]()
    {
        std::set<wxString> texts;

        for( FOOTPRINT* fp : board->Footprints() )
        {
            texts.insert( fp->Value().GetText() );

            for( BOARD_ITEM* item : fp->GraphicalItems() )
            {
                if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( item ) )
                    texts.insert( text->GetText() );
            }
        }

        return texts;
    }();

    BOOST_CHECK( !footprintTexts.count( wxS( "${VALU}" ) ) );
    BOOST_CHECK( !footprintTexts.count( wxS( ">VALU" ) ) );
}


/**
 * Regression test for copper pour polygons. Eagle stores a polygon outline as a chain
 * of connected wire segments, but the XML reader expects <vertex> nodes; the binary
 * decoder emitted the raw wires, so loadPolygon() saw zero vertices and dropped every
 * pour ("less than 3 vertices"). The decoder now rebuilds the vertices from the segment
 * start points. boomchak carries two signal pours on copper (Eagle layers 1 and 16).
 */
BOOST_AUTO_TEST_CASE( LoadV3CopperPourPolygons )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/boomchak.brd" ) );

    if( !board )
        return;

    std::vector<ZONE*> pours = copperPours( board.get() );

    BOOST_REQUIRE_GE( pours.size(), 2u );

    // A pour rebuilt from segment start points must yield a single closed outline with at
    // least three corners; a botched conversion would collapse it or leave it empty.
    for( ZONE* zone : pours )
    {
        BOOST_CHECK_EQUAL( zone->Outline()->OutlineCount(), 1 );
        BOOST_CHECK_GE( zone->GetNumCorners(), 3 );
    }
}


/**
 * Regression test for issue 24812. This 5.12 Professional board's two copper pours were
 * reported missing after import because their outline wire segments were never rebuilt as
 * vertices. The board is not license-clean and is not committed, so this loads only when
 * the sample is present locally.
 */
BOOST_AUTO_TEST_CASE( LoadIssue24812CopperPours )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/issue24812_vertice_error.brd" ) );

    if( !board )
        return;

    BOOST_CHECK_GE( copperPours( board.get() ).size(), 2u );
}


/**
 * Regression test for issue 24827. Eagle 4.x pad and SMD records carry the same
 * rotation word and offset-19 name as the 5.x layout, but their signature clears
 * the bits that distinguished the two layouts, so every pad bound the Eagle 3.x
 * short row and read an empty name. Contactrefs resolve to a pad by name, so all
 * of an element's pads collapsed onto whichever signal was written last, wiring
 * every multi-pin part to a single net. brenner57e is a 4.x board; a correct
 * decode names each pad and keeps its signals distinct. The board is not
 * license-clean and is not committed, so this loads only when present locally.
 */
BOOST_AUTO_TEST_CASE( LoadIssue24827PadNamesAndSignals )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/issue24827_brenner57e.brd" ) );

    if( !board )
        return;

    BOOST_REQUIRE_GT( board->Footprints().size(), 0u );

    int padsWithNumber = 0;
    int padsWithoutNumber = 0;
    int multiPinPartsWithDistinctNets = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        std::set<int> nets;

        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetNumber().IsEmpty() )
                padsWithoutNumber++;
            else
                padsWithNumber++;

            nets.insert( pad->GetNetCode() );
        }

        if( fp->Pads().size() > 1 && nets.size() > 1 )
            multiPinPartsWithDistinctNets++;
    }

    // The pad-name decode is the whole fix: every through-hole pad in this board
    // carries a numeric name, so an empty-name pad means the short row still won.
    BOOST_CHECK_GT( padsWithNumber, 0 );
    BOOST_CHECK_EQUAL( padsWithoutNumber, 0 );

    // With names restored the contactref keys no longer collide, so multi-pin parts
    // spread across several signals instead of collapsing onto one.
    BOOST_CHECK_GT( multiPinPartsWithDistinctNets, 0 );
}


/**
 * Regression test for the pad shapes in issue 24827. The binary stores a pad's shape
 * as an ordinal, but the shared XML reader matches it by name and falls back to round
 * for anything else, so every through-hole pad imported as a circle. The ordinal maps
 * one-to-one to the reader's shape names: brenner57e's 0207 resistors carry octagon
 * pads (imported as chamfered rectangles) and its TO-92 transistors oblong pads
 * (imported as ovals), which also pins down the mapping direction. Local-only, like
 * the sibling test.
 */
BOOST_AUTO_TEST_CASE( LoadIssue24827PadShapes )
{
    std::unique_ptr<BOARD> board( loadBoard( "plugins/eagle_binary/issue24827_brenner57e.brd" ) );

    if( !board )
        return;

    auto shapeOf = [&]( const wxString& aRef ) -> PAD_SHAPE
    {
        for( FOOTPRINT* fp : board->Footprints() )
        {
            if( fp->GetReference() == aRef && !fp->Pads().empty() )
                return fp->Pads().front()->GetShape( PADSTACK::ALL_LAYERS );
        }

        return PAD_SHAPE::CIRCLE;
    };

    // R10 is an 0207 resistor (Eagle octagon pads); Q2 is a TO-92 transistor (oblong).
    // Both decoded to circles while the shape ordinal was ignored, and swapping the
    // mapping would trade their shapes.
    BOOST_CHECK_EQUAL( static_cast<int>( shapeOf( wxS( "R10" ) ) ),
                       static_cast<int>( PAD_SHAPE::CHAMFERED_RECT ) );
    BOOST_CHECK_EQUAL( static_cast<int>( shapeOf( wxS( "Q2" ) ) ),
                       static_cast<int>( PAD_SHAPE::OVAL ) );
}


BOOST_AUTO_TEST_SUITE_END()
