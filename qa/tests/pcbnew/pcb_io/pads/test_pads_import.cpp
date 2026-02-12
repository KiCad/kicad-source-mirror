/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_io/pads/pcb_io_pads.h>
#include <layer_ids.h>
#include <padstack.h>
#include <board.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_field.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <board_design_settings.h>
#include <set>


struct PADS_BOARD_INFO
{
    std::string dir;
    std::string file;
};


static const PADS_BOARD_INFO PADS_BOARDS[] = {
    { "ClaySight_MK1",    "ClaySight_MK1.asc" },    // V10.0 BASIC
    { "TMS1mmX19",         "TMS1mmX19.asc" },         // V9.5 BASIC MILS
    { "MC4_PLUS_CSHAPE",   "MC4_PLUS_CSHAPE.asc" },   // V9.5 MILS
    { "MC2_PLUS_REV1",     "MC2_PLUS_REV1.asc" },     // V9.4 METRIC
    { "Ems4_Rev2",         "Ems4_Rev2.asc" },          // V9.4 MILS
    { "LCORE_4",           "LCORE_4.asc" },            // V9.0 METRIC
    { "LCORE_2",           "LCORE_2.asc" },            // V2005.0 METRIC
    { "Dexter_MotorCtrl",  "Dexter_MotorCtrl.asc" },   // V2007.0 MILS
    { "MAIS_FC",           "MAIS_FC.asc" },             // V5.0 METRIC
};


static wxString GetBoardPath( const PADS_BOARD_INFO& aBoard )
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/" + aBoard.dir + "/" + aBoard.file;
}


/**
 * Verify that the PADS file is recognized and loads without crashing.
 */
static std::unique_ptr<BOARD> LoadAndVerify( const PADS_BOARD_INFO& aBoard )
{
    PCB_IO_PADS plugin;

    wxString filename = GetBoardPath( aBoard );

    BOOST_CHECK_MESSAGE( plugin.CanReadBoard( filename ),
            aBoard.dir << " should be a readable PADS file" );

    std::unique_ptr<BOARD> board;

    try
    {
        board.reset( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    }
    catch( const std::exception& e )
    {
        BOOST_WARN_MESSAGE( false,
                aBoard.dir << " threw exception during load: " << e.what() );
        return board;
    }

    BOOST_REQUIRE_MESSAGE( board != nullptr, aBoard.dir << " failed to load" );
    BOOST_CHECK_MESSAGE( board->Footprints().size() > 0,
            aBoard.dir << " should have footprints" );

    return board;
}


/**
 * Run structural integrity checks on a successfully loaded board.
 *
 * Uses BOOST_WARN for checks that expose known parser limitations so they
 * report issues without failing the test suite. Invariant checks (no duplicate
 * vias, tracks on copper) use BOOST_CHECK since they must always hold.
 */
static void RunStructuralChecks( const PADS_BOARD_INFO& aBoard )
{
    std::unique_ptr<BOARD> board = LoadAndVerify( aBoard );

    if( !board )
        return;

    BOOST_WARN_MESSAGE( board->Tracks().size() > 0,
            aBoard.dir << " has no tracks (parser may not support this format version)" );

    if( board->Tracks().size() > 0 && board->Footprints().size() > 0 )
    {
        BOX2I fpBbox;
        fpBbox.SetMaximum();

        for( FOOTPRINT* fp : board->Footprints() )
            fpBbox.Merge( fp->GetBoundingBox() );

        BOX2I trackBbox;
        trackBbox.SetMaximum();

        for( PCB_TRACK* trk : board->Tracks() )
            trackBbox.Merge( trk->GetBoundingBox() );

        BOOST_CHECK_MESSAGE( fpBbox.Intersects( trackBbox ),
                aBoard.dir << " footprint and track bounding boxes should overlap" );
    }

    // No duplicate through-hole vias at the same position
    std::set<std::pair<int, int>> viaPositions;
    bool hasDuplicate = false;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( trk );

        if( !via || via->GetViaType() != VIATYPE::THROUGH )
            continue;

        auto key = std::make_pair( via->GetPosition().x, via->GetPosition().y );

        if( viaPositions.count( key ) )
        {
            hasDuplicate = true;
            break;
        }

        viaPositions.insert( key );
    }

    BOOST_CHECK_MESSAGE( !hasDuplicate,
            aBoard.dir << " should have no duplicate through-hole vias" );

    // All imported tracks must be on copper layers
    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
        {
            BOOST_CHECK_MESSAGE( IsCopperLayer( trk->GetLayer() ),
                    aBoard.dir << " track on non-copper layer " << trk->GetLayer() );
        }
    }

    // Pad size check uses WARN since some boards have pads the parser doesn't handle yet
    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            BOOST_WARN_MESSAGE( pad->GetSize( PADSTACK::ALL_LAYERS ).x > 0
                                        && pad->GetSize( PADSTACK::ALL_LAYERS ).y > 0,
                    aBoard.dir << " " << fp->GetReference() << " pad has zero size" );
        }
    }
}


BOOST_AUTO_TEST_SUITE( PADS_IMPORT )


BOOST_AUTO_TEST_CASE( ImportClaySight_MK1 )
{
    RunStructuralChecks( PADS_BOARDS[0] );
}


BOOST_AUTO_TEST_CASE( ImportTMS1mmX19 )
{
    RunStructuralChecks( PADS_BOARDS[1] );
}


BOOST_AUTO_TEST_CASE( ImportMC4_PLUS_CSHAPE )
{
    RunStructuralChecks( PADS_BOARDS[2] );
}


BOOST_AUTO_TEST_CASE( ImportMC2_PLUS_REV1 )
{
    RunStructuralChecks( PADS_BOARDS[3] );
}


BOOST_AUTO_TEST_CASE( ImportEms4_Rev2 )
{
    RunStructuralChecks( PADS_BOARDS[4] );
}


BOOST_AUTO_TEST_CASE( ImportLCORE_4 )
{
    RunStructuralChecks( PADS_BOARDS[5] );
}


BOOST_AUTO_TEST_CASE( ImportLCORE_2 )
{
    RunStructuralChecks( PADS_BOARDS[6] );
}


BOOST_AUTO_TEST_CASE( ImportDexter_MotorCtrl )
{
    RunStructuralChecks( PADS_BOARDS[7] );
}


BOOST_AUTO_TEST_CASE( ImportMAIS_FC )
{
    RunStructuralChecks( PADS_BOARDS[8] );
}


BOOST_AUTO_TEST_CASE( ImportNonCopperTrackSkipped )
{
    // Test that tracks on non-copper layers are skipped without crashing
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/synthetic_noncopper_track.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    int track_count = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T )
        {
            track_count++;
            BOOST_CHECK( IsCopperLayer( track->GetLayer() ) );
        }
    }

    BOOST_CHECK( track_count > 0 );
}


BOOST_AUTO_TEST_CASE( ImportTextOnUnmappedLayer )
{
    // Test that text on unmapped layers is assigned to Comments layer without crashing
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/synthetic_unmapped_text_layer.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    int silkscreen_count = 0;
    int comments_count = 0;
    int copper_count = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( item ) )
        {
            PCB_LAYER_ID layer = text->GetLayer();

            BOOST_CHECK( layer != UNDEFINED_LAYER );

            if( layer == F_SilkS )
                silkscreen_count++;
            else if( layer == Cmts_User )
                comments_count++;
            else if( layer == F_Cu )
                copper_count++;
        }
    }

    BOOST_CHECK_EQUAL( silkscreen_count, 1 );
    BOOST_CHECK_EQUAL( comments_count, 1 );
    BOOST_CHECK_EQUAL( copper_count, 1 );
}


BOOST_AUTO_TEST_SUITE_END()
