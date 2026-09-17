/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file test_autotrax_import.cpp
 * Test suite for import of Protel Autotrax / Easytrax (.PCB) layout files.
 *
 * Sample files are real, openly-available boards from the PRONOM file-format
 * research corpus (glepore70/pronom-research, sample_files/p/pcb2), which the
 * UK National Archives published as reference samples for the Autotrax/Easytrax
 * format. DEMO.PCB and DEMOSMD.PCB are demo boards shipped with the Protel
 * Autotrax 1.61 freeware release. Every file carries the "PCB FILE 4" Autotrax header.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/autotrax/pcb_io_autotrax.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>

#include <reporter.h>

#include <wx/filename.h>


struct AUTOTRAX_IMPORT_FIXTURE
{
    AUTOTRAX_IMPORT_FIXTURE() {}

    PCB_IO_AUTOTRAX m_plugin;

    std::string path( const std::string& aName )
    {
        return KI_TEST::GetPcbnewTestDataDir() + "plugins/autotrax/" + aName;
    }

    bool haveSample( const std::string& aName ) { return wxFileName::FileExists( path( aName ) ); }
};


BOOST_FIXTURE_TEST_SUITE( AutotraxImport, AUTOTRAX_IMPORT_FIXTURE )


/// CanReadBoard must accept a real Autotrax file by sniffing the magic header,
/// not just the (gEDA-shared) .PCB extension.
BOOST_AUTO_TEST_CASE( SniffRecognizesPcb )
{
    if( !haveSample( "PRJ.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    BOOST_CHECK( m_plugin.CanReadBoard( path( "PRJ.PCB" ) ) );
}


/// The simplest sample is a board of through-hole components: it must yield
/// footprints with pads and some free tracks.
BOOST_AUTO_TEST_CASE( SimpleBoardStructure )
{
    if( !haveSample( "PRJ.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "PRJ.PCB" ) ) );
    BOOST_REQUIRE( board );

    BOOST_CHECK_GT( board->Footprints().size(), 0 );

    int pads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        pads += fp->Pads().size();

    BOOST_CHECK_GT( pads, 0 );
    BOOST_CHECK_GT( board->Tracks().size(), 0 );
}


/// The richest sample exercises every record family (FT/FA/FV/FP/FS plus
/// component CT/CA/CV/CP). Verify the importer produces a non-trivial board
/// with copper tracks, vias and footprints.
BOOST_AUTO_TEST_CASE( RichBoardStructure )
{
    if( !haveSample( "PRJ12.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    std::unique_ptr<BOARD> board;

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "PRJ12.PCB" ) ) );
    BOOST_REQUIRE( board );

    int tracks = 0;
    int vias = 0;

    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->Type() == PCB_VIA_T )
            vias++;
        else if( t->Type() == PCB_TRACE_T )
            tracks++;
    }

    BOOST_CHECK_GT( tracks, 0 );
    BOOST_CHECK_GT( vias, 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );

    int pads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        pads += fp->Pads().size();

    BOOST_CHECK_GT( pads, 0 );
}


/// Every sample must load without throwing and produce a positive board extent;
/// none should crash the Y-axis flip or layer mapping.
BOOST_AUTO_TEST_CASE( AllSamplesLoad )
{
    const std::vector<std::string> samples = { "PRJ.PCB", "PRJ2.PCB", "PRJ10.PCB", "PRJ12.PCB" };

    bool any = false;

    for( const std::string& name : samples )
    {
        if( !haveSample( name ) )
            continue;

        any = true;

        std::unique_ptr<BOARD> board;
        BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( name ) ) );
        BOOST_REQUIRE( board );

        size_t items = board->Tracks().size() + board->Footprints().size() + board->Drawings().size();
        BOOST_CHECK_GT( items, 0 );
    }

    if( !any )
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
}


/// NETDEF nodes must land on the pads they name.
BOOST_AUTO_TEST_CASE( NetdefAssignsPadNets )
{
    std::unique_ptr<BOARD> board;

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "DEMOSMD.PCB" ) ) );
    BOOST_REQUIRE( board );

    FOOTPRINT* u5 = board->FindFootprintByReference( wxS( "U5" ) );
    BOOST_REQUIRE( u5 );

    auto netOf = [&]( const wxString& aNumber ) -> wxString
    {
        PAD* pad = u5->FindPadByNumber( aNumber );
        BOOST_REQUIRE( pad );
        return pad->GetNetname();
    };

    BOOST_CHECK_EQUAL( netOf( wxS( "29" ) ), wxS( "GND" ) );
    BOOST_CHECK_EQUAL( netOf( wxS( "11" ) ), wxS( "VCC" ) );
    BOOST_CHECK_EQUAL( netOf( wxS( "13" ) ), wxS( "D7" ) );
}


/// COMP headers carry the value placement, then the designator placement, then their display flags.
BOOST_AUTO_TEST_CASE( CompHeaderPlacesFields )
{
    std::unique_ptr<BOARD> board;

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "DEMOSMD.PCB" ) ) );
    BOOST_REQUIRE( board );

    FOOTPRINT* r6 = board->FindFootprintByReference( wxS( "R6" ) );
    BOOST_REQUIRE( r6 );

    BOOST_CHECK_EQUAL( r6->Reference().GetPosition().x, pcbIUScale.MilsToIU( 559 ) );
    BOOST_CHECK_EQUAL( r6->Value().GetPosition().x, pcbIUScale.MilsToIU( 584 ) );
    BOOST_CHECK( r6->Reference().IsVisible() );
    BOOST_CHECK( !r6->Value().IsVisible() );
}


/// Rotation codes turn counterclockwise, and codes 16..19 mirror the text after rotating it.
BOOST_AUTO_TEST_CASE( TextRotationAndMirror )
{
    auto angleOf = []( const PCB_TEXT& aText )
    {
        EDA_ANGLE angle = aText.GetTextAngle();
        return angle.Normalize().AsDegrees();
    };

    std::unique_ptr<BOARD> smd;
    BOOST_REQUIRE_NO_THROW( smd = m_plugin.LoadBoard( path( "DEMOSMD.PCB" ) ) );
    FOOTPRINT* r6 = smd->FindFootprintByReference( wxS( "R6" ) );
    BOOST_REQUIRE( r6 );

    BOOST_CHECK_EQUAL( angleOf( r6->Reference() ), 90.0 );
    BOOST_CHECK( !r6->Reference().IsMirrored() );

    std::unique_ptr<BOARD> prj;
    BOOST_REQUIRE_NO_THROW( prj = m_plugin.LoadBoard( path( "PRJ12.PCB" ) ) );
    FOOTPRINT* a32 = prj->FindFootprintByReference( wxS( "A32" ) );
    BOOST_REQUIRE( a32 );

    BOOST_CHECK_EQUAL( angleOf( a32->Reference() ), 90.0 );
    BOOST_CHECK( a32->Reference().IsMirrored() );
    BOOST_CHECK( a32->Reference().GetLayer() == F_SilkS );
}


/// Arc quadrant bits follow a Y-up frame. U7's pin 1 notch sits on the left edge and must bulge right.
BOOST_AUTO_TEST_CASE( ArcQuadrantsBulgeIntoPart )
{
    std::unique_ptr<BOARD> board;

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "PRJ.PCB" ) ) );
    FOOTPRINT* u7 = board->FindFootprintByReference( wxS( "U7" ) );
    BOOST_REQUIRE( u7 );

    PCB_SHAPE* notch = nullptr;

    for( BOARD_ITEM* item : u7->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::ARC )
            notch = static_cast<PCB_SHAPE*>( item );
    }

    BOOST_REQUIRE( notch );
    BOOST_CHECK_GT( notch->GetArcMid().x, notch->GetCenter().x );
}


/// DEMOSMD draws its board edge only on the keepout layer, around every pad.
BOOST_AUTO_TEST_CASE( KeepoutBecomesBoardOutline )
{
    std::unique_ptr<BOARD> board;

    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "DEMOSMD.PCB" ) ) );

    int edges = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->GetLayer() == Edge_Cuts )
            edges++;
    }

    BOOST_CHECK_EQUAL( edges, 7 );
}


/// DEMO.PCB has pads wired to the ground and power planes, which KiCad cannot represent.
BOOST_AUTO_TEST_CASE( PlaneConnectionsAreReported )
{
    WX_STRING_REPORTER     reporter;
    std::unique_ptr<BOARD> board;

    m_plugin.SetReporter( &reporter );
    BOOST_REQUIRE_NO_THROW( board = m_plugin.LoadBoard( path( "DEMO.PCB" ) ) );

    BOOST_CHECK( reporter.GetMessages().Contains( wxS( "87 pad(s) ask for a ground or power plane" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
