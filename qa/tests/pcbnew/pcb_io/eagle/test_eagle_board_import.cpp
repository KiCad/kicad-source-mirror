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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_eagle_board_import.cpp
 * Test suite for import of Eagle *.brd board files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/eagle/pcb_io_eagle.h>

#include <board.h>
#include <footprint.h>
#include <netinfo.h>
#include <pcb_text.h>
#include <pcb_track.h>

#include <map>
#include <wx/filename.h>


struct EAGLE_BOARD_IMPORT_FIXTURE
{
    EAGLE_BOARD_IMPORT_FIXTURE() {}
};


BOOST_FIXTURE_TEST_SUITE( EagleBoardImport, EAGLE_BOARD_IMPORT_FIXTURE )


/**
 * Verify that vias imported from an Eagle board are assigned to the correct nets.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/21243 and
 * https://gitlab.com/kicad/code/kicad/-/issues/23016
 *
 * The Adafruit AHT20 board has vias on SDA, SCL, GND, VCC, and VDD nets. A previous bug
 * caused netCode to not increment for signals with only polygons (no pads), which shifted
 * all subsequent net assignments.
 */
BOOST_AUTO_TEST_CASE( ViaNetAssignment )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/eagle/Adafruit-AHT20-PCB/"
                             "Adafruit AHT20 Temperature & Humidity.brd";

    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( dataPath ),
                           "Test board file not found: " + dataPath );

    PCB_IO_EAGLE eaglePlugin;
    BOARD*       rawBoard = nullptr;

    try
    {
        rawBoard = eaglePlugin.LoadBoard( dataPath, nullptr, nullptr );
    }
    catch( const IO_ERROR& e )
    {
        BOOST_FAIL( "IO_ERROR loading Eagle board: " + e.What().ToStdString() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( std::string( "Exception loading Eagle board: " ) + e.what() );
    }

    std::unique_ptr<BOARD> board( rawBoard );

    BOOST_REQUIRE( board );

    // Collect vias grouped by net name
    std::map<wxString, int> viasPerNet;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            NETINFO_ITEM* net = track->GetNet();
            BOOST_REQUIRE( net );

            wxString netName = net->GetNetname();
            viasPerNet[netName]++;
        }
    }

    // The Eagle board has vias on these nets (verified from the XML source)
    BOOST_CHECK_GT( viasPerNet[wxT( "SDA" )], 0 );
    BOOST_CHECK_GT( viasPerNet[wxT( "SCL" )], 0 );
    BOOST_CHECK_GT( viasPerNet[wxT( "GND" )], 0 );
    BOOST_CHECK_GT( viasPerNet[wxT( "VCC" )], 0 );
    BOOST_CHECK_GT( viasPerNet[wxT( "VDD" )], 0 );

    // Verify exact counts from the Eagle XML
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "SDA" )], 2 );
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "SCL" )], 2 );
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "GND" )], 3 );
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "VCC" )], 2 );
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "VDD" )], 2 );

    // No via should be unassigned (net code 0)
    BOOST_CHECK_EQUAL( viasPerNet[wxT( "" )], 0 );
}


/**
 * Verify that text justification is correct after importing an Eagle board with rotated
 * footprints and silkscreen text annotations.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23016
 *
 * The test board has 4-pin connectors (I2C0, I2C1) rotated R90 with pin label text
 * "G  SDA SCL V+" on the top silkscreen. A bug in orientFPText() had dead code
 * (abs(degrees) <= -180, which can never be true) that prevented text justification
 * from being flipped for footprints rotated 180 or 270 degrees. The board also
 * exercises the layer enable fix by having unmapped Eagle layers (53, 54, 100+)
 * whose sentinel values previously caused std::bad_alloc in LSET::set().
 */
BOOST_AUTO_TEST_CASE( TextJustification )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/eagle/test_eagle_23016/test_eagle.brd";

    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( dataPath ),
                           "Test board file not found: " + dataPath );

    PCB_IO_EAGLE eaglePlugin;
    BOARD*       rawBoard = nullptr;

    try
    {
        rawBoard = eaglePlugin.LoadBoard( dataPath, nullptr, nullptr );
    }
    catch( const IO_ERROR& e )
    {
        BOOST_FAIL( "IO_ERROR loading Eagle board: " + e.What().ToStdString() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( std::string( "Exception loading Eagle board: " ) + e.what() );
    }

    std::unique_ptr<BOARD> board( rawBoard );

    BOOST_REQUIRE( board );

    // Board should have 13 elements from the Eagle file
    BOOST_CHECK_EQUAL( board->Footprints().size(), 13 );

    // Find the I2C connector footprints and verify reference text justification.
    // I2C0 and I2C1 are smashed but have no NAME attribute, so their Reference text
    // goes through the non-smashed orientFPText() code path. With the footprint at R90
    // and the package text at 0 degrees, the combined angle is 180, triggering the
    // justification flip to RIGHT/TOP. Before the fix, the dead code branch
    // (abs(degrees) <= -180) would never fire and justification stayed at LEFT/BOTTOM.
    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString ref = fp->GetReference();

        if( ref == wxT( "I2C0" ) || ref == wxT( "I2C1" ) )
        {
            PCB_TEXT& refText = fp->Reference();

            BOOST_CHECK_MESSAGE(
                    refText.GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT,
                    ref + " reference horizontal justification should be RIGHT, got "
                            + std::to_string( refText.GetHorizJustify() ) );

            BOOST_CHECK_MESSAGE(
                    refText.GetVertJustify() == GR_TEXT_V_ALIGN_TOP,
                    ref + " reference vertical justification should be TOP, got "
                            + std::to_string( refText.GetVertJustify() ) );
        }
    }

    // Verify the plain text pin labels have correct justification.
    // "G  SDA SCL V+" on F.SilkS (Eagle layer 21) should have LEFT/BOTTOM justification
    // with 90-degree rotation so the text reads bottom-to-top matching the pin order.
    int topPinLabelCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() != PCB_TEXT_T )
            continue;

        PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

        if( text->GetText() != wxT( "G  SDA SCL V+" ) )
            continue;

        if( text->GetLayer() != F_SilkS )
            continue;

        topPinLabelCount++;

        BOOST_CHECK_MESSAGE(
                text->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT,
                "Pin label text horizontal justification should be LEFT" );

        BOOST_CHECK_MESSAGE(
                text->GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM,
                "Pin label text vertical justification should be BOTTOM" );

        BOOST_CHECK_MESSAGE(
                text->GetTextAngle() == EDA_ANGLE( 90, DEGREES_T ),
                "Pin label text angle should be 90 degrees" );
    }

    // Two instances of "G  SDA SCL V+" on F.SilkS (one per connector)
    BOOST_CHECK_EQUAL( topPinLabelCount, 2 );
}


BOOST_AUTO_TEST_SUITE_END()
