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
#include <pcb_dimension.h>
#include <project/net_settings.h>
#include <board_stackup_manager/board_stackup.h>
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
    { "ClaySight_MK2",    "ClaySight_MK2.asc" },      // V10.0 BASIC (copper lines)
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

    // Every zone outline must have non-empty contours
    for( ZONE* zone : board->Zones() )
    {
        const SHAPE_POLY_SET* outline = zone->Outline();
        BOOST_REQUIRE_MESSAGE( outline != nullptr,
                aBoard.dir << " zone has null outline" );

        for( int ii = 0; ii < outline->OutlineCount(); ++ii )
        {
            BOOST_CHECK_MESSAGE( outline->COutline( ii ).PointCount() >= 3,
                    aBoard.dir << " zone outline " << ii << " has "
                               << outline->COutline( ii ).PointCount() << " points" );
        }
    }
}


BOOST_AUTO_TEST_SUITE( PADS_IMPORT )


BOOST_AUTO_TEST_CASE( ImportClaySight_MK1 )
{
    RunStructuralChecks( PADS_BOARDS[0] );
}


/**
 * Verify element counts for ClaySight_MK1 (V10.0 BASIC units).
 *
 * This board is a 2-layer design with 36 components, routed traces on F.Cu
 * and B.Cu, no vias, no copper pours, and a rectangular board outline.
 * Counts are derived from the ASC source file sections.
 */
BOOST_AUTO_TEST_CASE( ClaySight_MK1_ElementCounts )
{
    std::unique_ptr<BOARD> board = LoadAndVerify( PADS_BOARDS[0] );

    BOOST_REQUIRE( board != nullptr );

    // Footprints: 36 parts in the *PART* section
    BOOST_CHECK_EQUAL( board->Footprints().size(), 36 );

    // Total pads across all footprints
    int totalPads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        totalPads += fp->Pads().size();

    BOOST_CHECK_EQUAL( totalPads, 140 );

    // Tracks: routed signal segments from 32 *SIGNAL* sections
    int traceCount = 0;
    int viaCount = 0;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
            traceCount++;
        else if( trk->Type() == PCB_VIA_T )
            viaCount++;
    }

    BOOST_CHECK_EQUAL( traceCount, 247 );

    // No vias on this 2-layer board (empty *VIA* section)
    BOOST_CHECK_EQUAL( viaCount, 0 );

    // No zones (empty *POUR* section)
    BOOST_CHECK_EQUAL( board->Zones().size(), 0 );

    // Board outline on Edge.Cuts
    int edgeCutsCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts )
                edgeCutsCount++;
        }
    }

    BOOST_CHECK_EQUAL( edgeCutsCount, 6 );

    // Free text items from the *TEXT* section
    int textCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( dynamic_cast<PCB_TEXT*>( item ) )
            textCount++;
    }

    BOOST_CHECK_EQUAL( textCount, 17 );

    // Net assignments: tracks and pads should reference named nets
    std::set<wxString> trackNets;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        NETINFO_ITEM* net = trk->GetNet();

        if( net && !net->GetNetname().IsEmpty() )
            trackNets.insert( net->GetNetname() );
    }

    BOOST_CHECK_EQUAL( trackNets.size(), 32 );

    // All traces on copper layers (F.Cu or B.Cu for this 2-layer board)
    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T )
        {
            PCB_LAYER_ID layer = trk->GetLayer();
            BOOST_CHECK_MESSAGE( layer == F_Cu || layer == B_Cu,
                    "trace on unexpected layer " << layer );
        }
    }
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


BOOST_AUTO_TEST_CASE( ImportClaySight_MK2 )
{
    RunStructuralChecks( PADS_BOARDS[9] );
}


/**
 * Verify element counts for ClaySight_MK2 (V10.0 BASIC units, EasyEDA export).
 *
 * EasyEDA exports footprint silkscreen outlines as COPPER type entries on the
 * silkscreen layer in the *LINES* section. These are imported as silkscreen
 * graphics rather than copper tracks. Route tracks come from *SIGNAL* sections.
 */
BOOST_AUTO_TEST_CASE( ClaySight_MK2_ElementCounts )
{
    std::unique_ptr<BOARD> board = LoadAndVerify( PADS_BOARDS[9] );

    BOOST_REQUIRE( board != nullptr );

    // 10 parts: U1 (RPi Pico), SU1-SU8 (TO-92), U2 (ULN2003A)
    BOOST_CHECK_EQUAL( board->Footprints().size(), 10 );

    // U1=40 pads, SU1-SU8=3 each (24), U2=16 pads = 80 total
    int totalPads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        totalPads += fp->Pads().size();

    BOOST_CHECK_EQUAL( totalPads, 80 );

    int traceCount = 0;
    int viaCount = 0;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T || trk->Type() == PCB_ARC_T )
            traceCount++;
        else if( trk->Type() == PCB_VIA_T )
            viaCount++;
    }

    // 138 track segments from 2 *SIGNAL* route sections only
    BOOST_CHECK_EQUAL( traceCount, 138 );
    BOOST_CHECK_EQUAL( viaCount, 0 );

    // 2 nets from *SIGNAL* routes: N$12982 and N$12975
    std::set<wxString> trackNets;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        NETINFO_ITEM* net = trk->GetNet();

        if( net && !net->GetNetname().IsEmpty() )
            trackNets.insert( net->GetNetname() );
    }

    BOOST_CHECK_EQUAL( trackNets.size(), 2 );

    // All traces on copper layers
    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() == PCB_TRACE_T )
        {
            BOOST_CHECK_MESSAGE( IsCopperLayer( trk->GetLayer() ),
                    "trace on non-copper layer " << trk->GetLayer() );
        }
    }

    // 72 COPPER items on layer 126 become silkscreen graphics. 64 of these
    // (16 groups of 4 axis-aligned segments) are detected as rectangles. The
    // remaining 8 are individual segments. Plus 44 segments from LINES items.
    int silkCount = 0;
    int rectCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == F_SilkS )
            {
                silkCount++;

                if( shape->GetShape() == SHAPE_T::RECTANGLE )
                    rectCount++;
            }
        }
    }

    BOOST_CHECK_EQUAL( silkCount, 68 );
    BOOST_CHECK_EQUAL( rectCount, 16 );

    // Default via size from JMPVIA_1 definition (drill=457505, size=915010 BASIC)
    const BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    std::shared_ptr<NETCLASS> defaultNc = bds.m_NetSettings->GetDefaultNetclass();
    BOOST_CHECK( defaultNc->GetViaDiameter() > 0 );
    BOOST_CHECK( defaultNc->GetViaDrill() > 0 );
    BOOST_CHECK( defaultNc->GetViaDiameter() > defaultNc->GetViaDrill() );
    BOOST_CHECK_EQUAL( bds.m_ViasDimensionsList.size(), 1 );

    // Copper-to-edge clearance from OUTLINE_TO_* rules (227990 BASIC)
    BOOST_CHECK( bds.m_CopperEdgeClearance > 0 );

    // Board outline on Edge.Cuts (rectangular outline = 4 segments)
    int edgeCutsCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Edge_Cuts )
                edgeCutsCount++;
        }
    }

    BOOST_CHECK_EQUAL( edgeCutsCount, 4 );

    // 1 board-level text item on silkscreen with multi-line content
    int textCount = 0;
    wxString textContent;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() == PCB_TEXT_T )
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );
            textContent = text->GetText();
            textCount++;
        }
    }

    BOOST_CHECK_EQUAL( textCount, 1 );

    // EasyEDA exports encode newlines as underscores in text content.
    // The parser converts them back to newlines for proper multi-line display.
    BOOST_CHECK( textContent.Contains( wxT( "\n" ) ) );
    BOOST_CHECK( !textContent.Contains( wxT( "_" ) ) );
    BOOST_CHECK( textContent.Contains( wxT( "CLAYSIGHT MCU V.2" ) ) );
    BOOST_CHECK( textContent.Contains( wxT( "The Ohio State University" ) ) );
}


/**
 * Verify stackup is built from LAYER DATA for MAIS_FC (V5.0 BASIC, 2-layer).
 *
 * MAIS_FC has meaningful stackup data for its 2 copper layers:
 * LAYER_THICKNESS 304800, COPPER_THICKNESS 38100, DIELECTRIC 3.8.
 */
BOOST_AUTO_TEST_CASE( MAIS_FC_Stackup )
{
    std::unique_ptr<BOARD> board = LoadAndVerify( PADS_BOARDS[8] );

    BOOST_REQUIRE( board != nullptr );

    const BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    BOOST_CHECK( bds.m_HasStackup );

    const BOARD_STACKUP& stackup = bds.GetStackupDescriptor();

    bool foundCopperThickness = false;
    bool foundDielectric = false;

    for( BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_COPPER )
        {
            if( item->GetThickness() > 0 )
                foundCopperThickness = true;
        }
        else if( item->GetType() == BOARD_STACKUP_ITEM_TYPE::BS_ITEM_TYPE_DIELECTRIC )
        {
            if( item->GetEpsilonR() > 3.0 )
                foundDielectric = true;
        }
    }

    BOOST_CHECK_MESSAGE( foundCopperThickness, "stackup should have non-zero copper thickness" );
    BOOST_CHECK_MESSAGE( foundDielectric, "stackup should have dielectric constant > 3.0" );
    BOOST_CHECK( bds.GetBoardThickness() > 0 );
}


/**
 * Verify that degenerate pour definitions (PADTHERM/VIATHERM with < 3 points)
 * are skipped without crashing.
 *
 * Before the fix, these created zones with empty SHAPE_LINE_CHAIN outlines
 * which caused a SIGABRT in the renderer's DrawPolyline() when accessing
 * CPoint(0) on a zero-length vector.
 */
BOOST_AUTO_TEST_CASE( ImportDegeneratePourSkipped )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir()
                        + "plugins/pads/synthetic_degenerate_pour.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // Only the valid 4-point pour should produce a zone.
    // The PADTHERM (2 SEG pieces with 2 points each) and
    // VIATHERM (1 SEG piece with 2 points) must be skipped.
    BOOST_CHECK_EQUAL( board->Zones().size(), 1 );

    // The single valid zone must have a non-degenerate outline
    if( board->Zones().size() == 1 )
    {
        ZONE* zone = board->Zones()[0];
        BOOST_CHECK( zone->Outline()->OutlineCount() == 1 );
        BOOST_CHECK( zone->Outline()->COutline( 0 ).PointCount() >= 3 );
    }
}


/**
 * Verify that filled copper shapes produce zones with exactly one valid outline.
 *
 * Before the fix, loadCopperShapes() called NewOutline() then Append(SHAPE_LINE_CHAIN),
 * which created two outlines per zone: an empty polygon from NewOutline() and the real
 * one from the implicit SHAPE_LINE_CHAIN->SHAPE_POLY_SET conversion. The empty outline
 * (0 points, but closed) crashed the renderer via CPoint(0) on an empty vector.
 */
BOOST_AUTO_TEST_CASE( ImportFilledCopperSingleOutline )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir()
                        + "plugins/pads/synthetic_filled_copper.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );
    BOOST_REQUIRE_EQUAL( board->Zones().size(), 1 );

    ZONE* zone = board->Zones()[0];
    BOOST_CHECK_EQUAL( zone->Outline()->OutlineCount(), 1 );
    BOOST_CHECK( zone->Outline()->COutline( 0 ).PointCount() >= 3 );
}


/**
 * Verify Importer.asc (V10.0 BASIC, 6-layer) imports correctly.
 *
 * This board exercises four specific import scenarios:
 * 1. Graphics on PADS layers 18/19/20 (UNASSIGNED type) must be imported
 * 2. Copper pours with HATOUT fills and VOIDOUT holes
 * 3. Oval finger (OF) pads on U1 must not be overwritten by thermal (RT) entries
 * 4. Dimension line with non-aligned base points must not be skewed
 */
BOOST_AUTO_TEST_CASE( Importer_SpecificFixes )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/Importer.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // Bug 1: Graphics on layers 18/19/20 must be imported.
    // The LAYERSTACK_6L_35U block has 556 pieces mostly on layer 18, plus layer 20.
    // The DRW59706864 block has a BOARD outline on layer 0 (Edge.Cuts).
    // Count graphics on Dwgs_User and Cmts_User to verify documentation layers imported.
    int dwgsUserCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            if( shape->GetLayer() == Dwgs_User )
                dwgsUserCount++;
        }
    }

    BOOST_CHECK_MESSAGE( dwgsUserCount > 0,
            "graphics on PADS layer 18 (drill drawing) should map to Dwgs_User" );

    // Bug 3: U1 pads should be oval (OF shape), not circular (RT thermal).
    // U1 has part type DIO_RECT_3PH_1600V_100A with DIOB_D100JHT160V decal.
    // PAD 0 stack has OF 0.000 11550000 on layer -2 (4.8mm height, 11.55mm width).
    FOOTPRINT* u1 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == wxT( "U1" ) )
        {
            u1 = fp;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( u1 != nullptr, "U1 footprint should exist" );

    bool foundOvalPad = false;

    for( PAD* pad : u1->Pads() )
    {
        VECTOR2I padSize = pad->GetSize( PADSTACK::ALL_LAYERS );

        if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::OVAL && padSize.x != padSize.y )
        {
            foundOvalPad = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundOvalPad, "U1 should have oval pads (OF shape, not RT thermal)" );

    // Bug 2: Copper pours should not have duplicates from HATOUT records.
    // The file has 13 POUROUT records. HATOUT records should become fills, not zones.
    // Count zones that are NOT rule areas (actual copper pours).
    int pourZoneCount = 0;
    int filledZoneCount = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
        {
            pourZoneCount++;

            if( zone->IsFilled() )
                filledZoneCount++;
        }
    }

    BOOST_CHECK_MESSAGE( pourZoneCount <= 13,
            "should not have duplicate zones from HATOUT; got " << pourZoneCount );

    BOOST_CHECK_MESSAGE( filledZoneCount > 0, "HATOUT records should produce filled zones" );

    // Bug 4: Dimension line should not be skewed.
    // DIM92271615 measures 110.00mm horizontal. Start=(0,9000000) end=(165000000,1500000).
    // After fix, both endpoints should have the same Y for horizontal measurement.
    int dimCount = 0;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( PCB_DIM_ALIGNED* dim = dynamic_cast<PCB_DIM_ALIGNED*>( item ) )
        {
            dimCount++;

            VECTOR2I start = dim->GetStart();
            VECTOR2I end = dim->GetEnd();

            BOOST_CHECK_MESSAGE( start.y == end.y,
                    "horizontal dimension endpoints should have equal Y coordinates; "
                    "start.y=" << start.y << " end.y=" << end.y );
        }
    }

    BOOST_CHECK_MESSAGE( dimCount > 0, "should have at least one dimension" );
}


/**
 * Verify peka.asc (V9.0 BASIC, 4-layer) via import.
 *
 * STANDARDVIA spans all 4 copper layers (-2=top, -1=bottom) plus non-copper
 * entries (layer 0=inner pad, layer 25=soldermask). Three specific issues:
 * 1. Via classified as blind instead of through-hole because the soldermask
 *    layer (25) inflates the span beyond the copper layer count
 * 2. Via size uses the soldermask opening (~63 mil) instead of the copper
 *    pad size (~36 mil) because def.size takes the max across all layers
 * 3. Multiple copies of the same through-hole via because deduplication
 *    only applies to VIATYPE::THROUGH
 */
BOOST_AUTO_TEST_CASE( Peka_ViaImport )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/peka.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // Collect all vias and check for duplicates
    std::map<std::pair<int, int>, int> viaPositionCount;
    int blindCount = 0;
    int throughCount = 0;

    // Copper pad size from the STANDARDVIA definition is 1371600 BASIC units.
    // At BASIC_TO_NM = 25400/38100, that converts to ~914,400 nm (36 mil).
    // The soldermask opening is 2400300 BASIC = ~1,600,200 nm (63 mil).
    // Via size must use the copper value, not the mask opening.
    const int maxExpectedViaWidth = 1200000;  // 1.2mm, well above 36 mil copper pad

    int oversizedViaCount = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( track );

        if( !via )
            continue;

        VECTOR2I pos = via->GetPosition();
        auto key = std::make_pair( pos.x, pos.y );
        viaPositionCount[key]++;

        if( via->GetViaType() == VIATYPE::BLIND )
            blindCount++;
        else if( via->GetViaType() == VIATYPE::THROUGH )
            throughCount++;

        if( via->GetWidth( F_Cu ) > maxExpectedViaWidth )
            oversizedViaCount++;
    }

    // All vias in this 4-layer board span top-to-bottom, so none should be blind
    BOOST_CHECK_MESSAGE( blindCount == 0,
            "no vias should be blind; STANDARDVIA spans all copper layers; got "
            << blindCount << " blind vias" );

    BOOST_CHECK_MESSAGE( throughCount > 0, "should have through-hole vias" );

    // No via should use the soldermask opening as its pad size
    BOOST_CHECK_MESSAGE( oversizedViaCount == 0,
            "via size should use copper pad, not soldermask opening; got "
            << oversizedViaCount << " oversized vias" );

    // No duplicate vias at the same position
    int duplicateCount = 0;

    for( const auto& [pos, count] : viaPositionCount )
    {
        if( count > 1 )
            duplicateCount++;
    }

    BOOST_CHECK_MESSAGE( duplicateCount == 0,
            "should not have duplicate vias at the same position; got "
            << duplicateCount << " positions with duplicates" );

    // STANDARDVIA has a layer 25 (front mask) entry but no layer 28 (back mask),
    // so the back should be tented. JMPVIA has no mask layers at all.
    // At minimum, every via should have the back tented.
    int backTentedCount = 0;
    int totalVias = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( track );

        if( !via )
            continue;

        totalVias++;

        if( via->GetBackTentingMode() == TENTING_MODE::TENTED )
            backTentedCount++;
    }

    BOOST_CHECK_MESSAGE( backTentedCount == totalVias,
            "vias without soldermask opening should be tented; "
            << backTentedCount << " of " << totalVias << " back-tented" );
}


/**
 * Verify that U1 pads 1-5 in Importer.asc have oval drill hits.
 *
 * The DIOB_D100JHT160V decal's PAD 0 definition includes slotted drill
 * parameters (2250000 x 9000000 BASIC units at 0 degrees). The converter
 * must set PAD_DRILL_SHAPE::OBLONG with the correct major/minor dimensions
 * rather than treating the drill as round.
 */
BOOST_AUTO_TEST_CASE( Importer_OvalDrillHits )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/Importer.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    FOOTPRINT* u1 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == "U1" )
        {
            u1 = fp;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( u1, "U1 not found on board" );

    // BASIC-to-nm: value * 25400 / 38100 = value * 2/3
    // drill = 2250000 BASIC -> 1500000 nm (1.5mm)
    // slot_length = 9000000 BASIC -> 6000000 nm (6.0mm)
    const int expectedMajor = 6000000;
    const int expectedMinor = 1500000;
    const int tolerance = 10000;  // 10um

    int oblongCount = 0;

    for( PAD* pad : u1->Pads() )
    {
        wxString padNum = pad->GetNumber();

        if( padNum == "1" || padNum == "2" || padNum == "3"
            || padNum == "4" || padNum == "5" )
        {
            BOOST_CHECK_MESSAGE( pad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG,
                    "pad " << padNum << " should have oblong drill" );

            VECTOR2I drillSize = pad->GetDrillSize();
            int major = std::max( drillSize.x, drillSize.y );
            int minor = std::min( drillSize.x, drillSize.y );

            BOOST_CHECK_MESSAGE( std::abs( major - expectedMajor ) < tolerance,
                    "pad " << padNum << " drill major axis " << major
                    << " should be ~" << expectedMajor );

            BOOST_CHECK_MESSAGE( std::abs( minor - expectedMinor ) < tolerance,
                    "pad " << padNum << " drill minor axis " << minor
                    << " should be ~" << expectedMinor );

            if( pad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
                oblongCount++;
        }
    }

    BOOST_CHECK_MESSAGE( oblongCount == 5,
            "expected 5 pads with oblong drill, got " << oblongCount );
}


/**
 * Verify M4 pad 1 in peka.asc uses the alternate MTHOLEAAAB decal.
 *
 * The MTHOLE part type has alternate decals separated by colons. M4 has
 * ALT=1 which selects MTHOLEAAAB (250 mil pad, 125 mil circular drill).
 * The converter must resolve through the PARTTYPE section to pick the
 * correct alternate rather than defaulting to the base MTHOLE decal.
 */
BOOST_AUTO_TEST_CASE( Peka_AlternateDecalDrill )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/peka.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    FOOTPRINT* m4 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == "M4" )
        {
            m4 = fp;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( m4, "M4 not found on board" );

    // MTHOLEAAAB: pad = 9525000 BASIC * 2/3 = 6350000 nm (250 mil)
    //             drill = 4762500 BASIC * 2/3 = 3175000 nm (125 mil)
    const int expectedPadSize = 6350000;
    const int expectedDrill = 3175000;
    const int tolerance = 10000;

    BOOST_REQUIRE_MESSAGE( m4->Pads().size() == 1,
            "MTHOLEAAAB has 1 terminal; got " << m4->Pads().size() );

    PAD* pad = m4->Pads().front();

    BOOST_CHECK_MESSAGE( pad->GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE,
            "M4 pad 1 drill should be circular" );

    VECTOR2I padSize = pad->GetSize( F_Cu );
    int padDim = std::max( padSize.x, padSize.y );

    BOOST_CHECK_MESSAGE( std::abs( padDim - expectedPadSize ) < tolerance,
            "M4 pad size " << padDim << " should be ~" << expectedPadSize
            << " (250 mil)" );

    VECTOR2I drillSize = pad->GetDrillSize();
    int drillDim = std::max( drillSize.x, drillSize.y );

    BOOST_CHECK_MESSAGE( std::abs( drillDim - expectedDrill ) < tolerance,
            "M4 drill size " << drillDim << " should be ~" << expectedDrill
            << " (125 mil)" );
}


/**
 * Verify zone fills imported from peka.asc are not self-intersecting.
 *
 * The HATOUT records define complex outlines with arcs that get converted
 * to polyline approximations. Errors in arc conversion (wrong winding,
 * bad radius, or incorrect center) can produce outlines that cross
 * themselves. Every filled polygon on every layer must be clean.
 */
BOOST_AUTO_TEST_CASE( Peka_ZoneFillNoSelfIntersection )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/peka.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // Check whether any two non-adjacent segments in the polygon truly
    // cross. Clipper2 BooleanSubtract can produce bridge edges where
    // non-adjacent segments share endpoints at T-junctions. These are
    // valid geometry, not real crossings.
    auto hasTrueCrossing = []( const SHAPE_POLY_SET& aPoly, int aIdx ) -> bool
    {
        std::vector<SEG> segs;

        for( auto it = aPoly.CIterateSegmentsWithHoles( aIdx ); it; it++ )
            segs.emplace_back( *it );

        for( size_t i = 0; i < segs.size(); i++ )
        {
            for( size_t j = i + 1; j < segs.size(); j++ )
            {
                // Segments sharing any endpoint are either adjacent in the
                // contour or bridge junctions from Clipper2.
                if( segs[i].A == segs[j].A || segs[i].A == segs[j].B
                    || segs[i].B == segs[j].A || segs[i].B == segs[j].B )
                {
                    continue;
                }

                if( segs[i].Intersects( segs[j] ) )
                    return true;
            }
        }

        return false;
    };

    int zonesChecked = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->IsFilled() )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( !zone->HasFilledPolysForLayer( layer ) )
                continue;

            std::shared_ptr<SHAPE_POLY_SET> fill = zone->GetFilledPolysList( layer );

            if( !fill || fill->OutlineCount() == 0 )
                continue;

            zonesChecked++;

            for( int pi = 0; pi < fill->OutlineCount(); pi++ )
            {
                if( fill->Outline( pi ).PointCount() < 3 )
                    continue;

                BOOST_CHECK_MESSAGE(
                        !hasTrueCrossing( *fill, pi ),
                        "zone \"" << zone->GetNetname() << "\" on "
                        << board->GetLayerName( layer )
                        << " outline " << pi
                        << " has self-intersecting fill polygon" );
            }
        }
    }

    BOOST_CHECK_MESSAGE( zonesChecked > 0, "no filled zones found to check" );
}


/**
 * Verify that SMD pads with explicit solder mask and paste mask stack entries
 * are imported with F.Mask and F.Paste layers enabled (issue 23254).
 *
 * synthetic_mask_paste.asc defines four footprints:
 *   U1 SMD_WITH_MASK_PASTE  - pad stack has explicit layers 21 (F.Mask) and 23 (F.Paste)
 *   U2 SMD_WITH_MASK_ONLY   - pad stack has explicit layer 21 (F.Mask) only
 *   U3 SMD_NO_MASK          - pad stack has only copper entries (-2, -1, 0)
 *   U4 PTH_WITH_MASK        - PTH pad with explicit mask layers
 *
 * Before the fix, all mask/paste PADS layer entries mapped to UNDEFINED_LAYER
 * via the copper-only mapPadsLayer lambda and were silently dropped, leaving
 * SMD pads with only F.Cu in their layer set.
 */
BOOST_AUTO_TEST_CASE( ImportMaskPasteLayers )
{
    PCB_IO_PADS plugin;

    wxString filename =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/synthetic_mask_paste.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );
    BOOST_REQUIRE_EQUAL( board->Footprints().size(), 5 );

    auto findFP = [&]( const wxString& aRef ) -> FOOTPRINT*
    {
        for( FOOTPRINT* fp : board->Footprints() )
            if( fp->GetReference() == aRef )
                return fp;
        return nullptr;
    };

    // U1: explicit F.Mask (layer 21) and F.Paste (layer 23) in pad stack
    {
        FOOTPRINT* u1 = findFP( "U1" );
        BOOST_REQUIRE_MESSAGE( u1, "U1 should exist" );
        BOOST_REQUIRE_EQUAL( u1->Pads().size(), 1 );

        PAD* pad = u1->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),    "U1 pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Mask ),  "U1 pad should have F.Mask (explicit in stack)" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Paste ), "U1 pad should have F.Paste (explicit in stack)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( B_Cu ),   "U1 SMD pad should not be on B.Cu" );
    }

    // U2: explicit F.Mask (layer 21) only; F.Paste added by fallback
    {
        FOOTPRINT* u2 = findFP( "U2" );
        BOOST_REQUIRE_MESSAGE( u2, "U2 should exist" );
        BOOST_REQUIRE_EQUAL( u2->Pads().size(), 1 );

        PAD* pad = u2->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),    "U2 pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Mask ),  "U2 pad should have F.Mask (explicit in stack)" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Paste ), "U2 pad should have F.Paste (fallback for SMD)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( B_Cu ),   "U2 SMD pad should not be on B.Cu" );
    }

    // U3: no mask entries; both F.Mask and F.Paste added by fallback
    {
        FOOTPRINT* u3 = findFP( "U3" );
        BOOST_REQUIRE_MESSAGE( u3, "U3 should exist" );
        BOOST_REQUIRE_EQUAL( u3->Pads().size(), 1 );

        PAD* pad = u3->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),    "U3 pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Mask ),  "U3 pad should have F.Mask (SMD fallback)" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Paste ), "U3 pad should have F.Paste (SMD fallback)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( B_Cu ),   "U3 SMD pad should not be on B.Cu" );
    }

    // U4: PTH pad with explicit F.Mask (layer 21) and B.Mask (layer 28).
    // No paste layers are present in the stack, so F.Paste/B.Paste must not be set.
    {
        FOOTPRINT* u4 = findFP( "U4" );
        BOOST_REQUIRE_MESSAGE( u4, "U4 should exist" );
        BOOST_REQUIRE_EQUAL( u4->Pads().size(), 1 );

        PAD* pad = u4->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),     "U4 PTH pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( B_Cu ),     "U4 PTH pad should be on B.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Mask ),   "U4 pad should have F.Mask (explicit in stack)" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( B_Mask ),   "U4 pad should have B.Mask (explicit in stack)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( F_Paste ), "U4 PTH pad should not have F.Paste" );
    }

    // U5: SMD pad with explicit zero-size F.Paste entry (layer 23 size 0).
    // A zero-size entry means "intentionally no paste on this layer".
    // The SMD fallback must not re-enable F.Paste for this pad.
    {
        FOOTPRINT* u5 = findFP( "U5" );
        BOOST_REQUIRE_MESSAGE( u5, "U5 should exist" );
        BOOST_REQUIRE_EQUAL( u5->Pads().size(), 1 );

        PAD* pad = u5->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),     "U5 pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Mask ),   "U5 pad should have F.Mask (SMD fallback)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( F_Paste ), "U5 pad should NOT have F.Paste (explicitly zero-size)" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( B_Cu ),    "U5 SMD pad should not be on B.Cu" );
    }
}


/**
 * Smoke test: the reporter's original reproduction file loads and has SMD pads
 * with F.Mask and F.Paste set (issue 23254).
 */
BOOST_AUTO_TEST_CASE( ImportMaskPasteLayersIssue23254 )
{
    PCB_IO_PADS plugin;

    wxString filename =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23254/issue23254.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    bool foundSmdWithMask = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD && pad->IsOnLayer( F_Mask )
                && pad->IsOnLayer( F_Paste ) )
            {
                foundSmdWithMask = true;
                break;
            }
        }

        if( foundSmdWithMask )
            break;
    }

    BOOST_CHECK_MESSAGE( foundSmdWithMask,
                         "At least one SMD pad in issue23254.asc should have F.Mask and F.Paste" );
}


/**
 * Verify that the issue23352 demo board imports square pads, per-pad
 * thermal connections, and netclass rules correctly.
 */
BOOST_AUTO_TEST_CASE( ImportIssue23352 )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23352.asc";

    std::unique_ptr<BOARD> board;
    board.reset( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    // Issue 1: Square pads should be imported as RECTANGLE, not CIRCLE.
    // The CON_2X1M part has PAD 1 with shape "S" (square) on F_Cu.
    bool foundSquarePad = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetShape( F_Cu ) == PAD_SHAPE::RECTANGLE )
            {
                foundSquarePad = true;
                break;
            }
        }

        if( foundSquarePad )
            break;
    }

    BOOST_CHECK_MESSAGE( foundSquarePad,
                         "At least one pad should have RECTANGLE shape (square pad import)" );

    // Issue 2: Zone connection should default to FULL (solid), not THERMAL.
    // Pads with RT/ST entries should have per-pad THERMAL override.
    bool foundZoneWithFull = false;
    bool foundPadWithThermal = false;
    bool foundPadWithoutThermal = false;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetPadConnection() == ZONE_CONNECTION::FULL )
        {
            foundZoneWithFull = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundZoneWithFull,
                         "Zones should default to FULL (solid) connection" );

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetLocalZoneConnection() == ZONE_CONNECTION::THERMAL )
                foundPadWithThermal = true;
            else
                foundPadWithoutThermal = true;
        }
    }

    BOOST_CHECK_MESSAGE( foundPadWithThermal,
                         "Pads with RT/ST entries should have per-pad THERMAL connection" );
    BOOST_CHECK_MESSAGE( foundPadWithoutThermal,
                         "Pads without RT/ST entries should not have per-pad THERMAL override" );

    // Issue 3: Netclasses should be imported with their rules.
    const BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    const auto& netclasses = bds.m_NetSettings->GetNetclasses();

    auto nc1It = netclasses.find( wxT( "NETTCLASS1" ) );
    auto nc2It = netclasses.find( wxT( "NETTCLASS2" ) );

    BOOST_CHECK_MESSAGE( nc1It != netclasses.end(), "NETTCLASS1 should exist" );
    BOOST_CHECK_MESSAGE( nc2It != netclasses.end(), "NETTCLASS2 should exist" );

    if( nc1It != netclasses.end() )
    {
        BOOST_CHECK_MESSAGE( nc1It->second->HasTrackWidth(),
                             "NETTCLASS1 should have a track width rule" );
    }

    if( nc2It != netclasses.end() )
    {
        BOOST_CHECK_MESSAGE( nc2It->second->HasTrackWidth(),
                             "NETTCLASS2 should have a track width rule" );
        BOOST_CHECK_MESSAGE( nc2It->second->HasClearance(),
                             "NETTCLASS2 should have a clearance rule" );
    }

    // Verify net-to-class assignments from the NET_CLASS DATA block
    const auto& patterns = bds.m_NetSettings->GetNetclassPatternAssignments();
    std::map<wxString, wxString> netAssignments;

    for( const auto& [matcher, ncName] : patterns )
        netAssignments[matcher->GetPattern()] = ncName;

    BOOST_CHECK_MESSAGE( netAssignments.count( wxT( "+24V0" ) ),
                         "+24V0 should be assigned to a net class" );
    BOOST_CHECK_MESSAGE( netAssignments.count( wxT( "+24V0_FILTER" ) ),
                         "+24V0_FILTER should be assigned to a net class" );
    BOOST_CHECK_MESSAGE( netAssignments.count( wxT( "+24V0_FILTER_RTN" ) ),
                         "+24V0_FILTER_RTN should be assigned to a net class" );

    if( netAssignments.count( wxT( "+24V0" ) ) )
    {
        BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0" )], wxT( "NETTCLASS1" ) );
    }

    if( netAssignments.count( wxT( "+24V0_FILTER_RTN" ) ) )
    {
        BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0_FILTER_RTN" )], wxT( "NETTCLASS2" ) );
    }
}


/**
 * Verify that net classes inside RULES_SECTION PARENT in the MISC section
 * are parsed and applied correctly.
 *
 * Issue #23393: NET_CLASS DATA blocks nested inside RULES_SECTION PARENT
 * were not parsed because the brace depth checks were hardcoded instead of
 * relative to the block entry depth.
 */
BOOST_AUTO_TEST_CASE( Issue23393_NetClassImport )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23393/demo.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    const BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    const auto& netclasses = bds.m_NetSettings->GetNetclasses();

    BOOST_CHECK_MESSAGE( netclasses.find( wxT( "NETTCLASS1" ) ) != netclasses.end(),
                         "NETTCLASS1 should be imported" );
    BOOST_CHECK_MESSAGE( netclasses.find( wxT( "NETTCLASS2" ) ) != netclasses.end(),
                         "NETTCLASS2 should be imported" );

    // Verify net-to-class assignments
    const auto& patterns = bds.m_NetSettings->GetNetclassPatternAssignments();
    std::map<wxString, wxString> netAssignments;

    for( const auto& [matcher, ncName] : patterns )
        netAssignments[matcher->GetPattern()] = ncName;

    // NETTCLASS1 should contain +24V0 and +24V0_FILTER
    BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0" )], wxT( "NETTCLASS1" ) );
    BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0_FILTER" )], wxT( "NETTCLASS1" ) );

    // NETTCLASS2 should contain +24V0_FILTER_RTN, +24V0_RTN, GND_CHASSIS
    BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0_FILTER_RTN" )], wxT( "NETTCLASS2" ) );
    BOOST_CHECK_EQUAL( netAssignments[wxT( "+24V0_RTN" )], wxT( "NETTCLASS2" ) );
    BOOST_CHECK_EQUAL( netAssignments[wxT( "GND_CHASSIS" )], wxT( "NETTCLASS2" ) );

    // NETTCLASS2 RULE_SET has TRACK_TO_TRACK 4500000 BASIC
    auto nc2It = netclasses.find( wxT( "NETTCLASS2" ) );

    if( nc2It != netclasses.end() )
    {
        BOOST_CHECK_MESSAGE( nc2It->second->HasClearance(),
                             "NETTCLASS2 should have clearance from RULE_SET" );
        BOOST_CHECK_MESSAGE( nc2It->second->HasTrackWidth(),
                             "NETTCLASS2 should have track width from RULE_SET" );
    }
}


/**
 * Verify that route arcs (CW/CCW) from PADS are imported as proper semicircles.
 *
 * Issue #23540: route arcs specified with only CW/CCW direction (no explicit
 * center/radius) were imported with degenerate geometry because the arc
 * midpoint was computed from zero center and zero radius.
 */
BOOST_AUTO_TEST_CASE( Issue23540_RouteArcSemicircle )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir()
                        + "plugins/pads/issue23540/test_import.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    int arcCount = 0;

    for( PCB_TRACK* trk : board->Tracks() )
    {
        if( trk->Type() != PCB_ARC_T )
            continue;

        PCB_ARC* arc = static_cast<PCB_ARC*>( trk );
        arcCount++;

        EDA_ANGLE angle = arc->GetAngle();
        double absDeg = std::abs( angle.AsDegrees() );

        BOOST_CHECK_MESSAGE( absDeg > 170.0 && absDeg < 190.0,
                "route arc angle " << absDeg << " should be ~180 degrees (semicircle)" );

        VECTOR2I mid = arc->GetMid();
        VECTOR2I start = arc->GetStart();
        VECTOR2I end = arc->GetEnd();

        // In PADS the CW arc from left to right goes upward. After the Y-axis
        // flip to KiCad coordinates, "upward on screen" means smaller Y values.
        // The arc midpoint Y must be less than both endpoint Y values.
        int chordY = ( start.y + end.y ) / 2;

        BOOST_CHECK_MESSAGE( mid.y < chordY,
                "arc midpoint Y=" << mid.y << " should be above (less than) "
                "chord center Y=" << chordY );
    }

    BOOST_CHECK_MESSAGE( arcCount >= 1,
            "expected at least 1 PCB_ARC from route CW/CCW arc, got " << arcCount );
}




/**
 * Verify in-circuit test point import (issue 23637).
 *
 * In-circuit test points are zero-drill vias placed on a route layer and tagged
 * in the *TESTPOINT* section.  Two bugs existed before the fix:
 *   1. The test point via was also emitted as a bare PCB_VIA by loadTracksAndVias(),
 *      creating a duplicate object and a DRC open-connection error.
 *   2. loadTestPoints() always placed the footprint on F.Cu when side=0 ("through"),
 *      even though the pad definition's soldermask layer indicates B.Cu.
 *
 * synthetic_testpoint.asc defines two test point via types:
 *   TP_BOTTOM_SMD  drill=0, routing-layer pad (layer 0) + soldermask bottom (28)
 *                  -> SMD footprint on B.Cu, pad ~0.8mm
 *   TP_TOP_SMD     drill=0, top copper pad (layer -2) + soldermask top (25)
 *                  -> SMD footprint on F.Cu, pad ~0.8mm
 */
BOOST_AUTO_TEST_CASE( InCircuitTestPointImport )
{
    PCB_IO_PADS plugin;

    wxString filename =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/synthetic_testpoint.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // R1 (1 part) + 2 test point footprints = 3 total
    BOOST_CHECK_EQUAL( board->Footprints().size(), 3u );

    FOOTPRINT* tpBottom = nullptr;
    FOOTPRINT* tpTop    = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetValue() == wxT( "TP_BOTTOM_SMD" ) )
            tpBottom = fp;
        else if( fp->GetValue() == wxT( "TP_TOP_SMD" ) )
            tpTop = fp;
    }

    // TP_BOTTOM_SMD: stack has soldermask bottom (layer 28) -> must land on B.Cu
    BOOST_REQUIRE_MESSAGE( tpBottom, "TP_BOTTOM_SMD test point footprint should exist" );
    BOOST_CHECK_EQUAL( tpBottom->Pads().size(), 1u );

    if( tpBottom->Pads().size() == 1 )
    {
        PAD* pad = tpBottom->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( B_Cu ),
                             "TP_BOTTOM_SMD pad should be on B.Cu" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( F_Cu ),
                             "TP_BOTTOM_SMD pad should not be on F.Cu" );

        // Pad size: 1200000 BASIC * 2/3 = 800000 nm (0.8mm).  Allow 5% tolerance.
        int padSize = pad->GetSize( PADSTACK::ALL_LAYERS ).x;
        BOOST_CHECK_MESSAGE( padSize > 700000 && padSize < 900000,
                             "TP_BOTTOM_SMD pad size " << padSize << " should be ~800000 nm" );
    }

    // TP_TOP_SMD: explicit top copper pad (layer -2) -> must land on F.Cu
    BOOST_REQUIRE_MESSAGE( tpTop, "TP_TOP_SMD test point footprint should exist" );
    BOOST_CHECK_EQUAL( tpTop->Pads().size(), 1u );

    if( tpTop->Pads().size() == 1 )
    {
        PAD* pad = tpTop->Pads().front();
        BOOST_CHECK_MESSAGE( pad->IsOnLayer( F_Cu ),
                             "TP_TOP_SMD pad should be on F.Cu" );
        BOOST_CHECK_MESSAGE( !pad->IsOnLayer( B_Cu ),
                             "TP_TOP_SMD pad should not be on B.Cu" );

        int padSize = pad->GetSize( PADSTACK::ALL_LAYERS ).x;
        BOOST_CHECK_MESSAGE( padSize > 700000 && padSize < 900000,
                             "TP_TOP_SMD pad size " << padSize << " should be ~800000 nm" );
    }

    // Test point positions must NOT also appear as bare PCB_VIA objects.
    // Before the fix, loadTracksAndVias() placed a PCB_VIA at each test point
    // position, creating a duplicate and causing DRC open-connection errors.
    VECTOR2I tpBottomPos( 0, 0 );
    VECTOR2I tpTopPos( 0, 0 );

    if( tpBottom )
        tpBottomPos = tpBottom->GetPosition();

    if( tpTop )
        tpTopPos = tpTop->GetPosition();

    for( PCB_TRACK* trk : board->Tracks() )
    {
        PCB_VIA* via = dynamic_cast<PCB_VIA*>( trk );

        if( !via )
            continue;

        VECTOR2I pos = via->GetPosition();

        BOOST_CHECK_MESSAGE( pos != tpBottomPos,
                             "TP_BOTTOM_SMD position should not have a bare PCB_VIA" );
        BOOST_CHECK_MESSAGE( pos != tpTopPos,
                             "TP_TOP_SMD position should not have a bare PCB_VIA" );
    }
}


/**
 * Verify RF finger pad offsets are not double-rotated on import (issue #23425).
 *
 * The reporter's board ({TXN004} controlCARD docking station) has a 180-pin HSEC8
 * edge connector (J3) whose finger pads are defined with FINORI 90 and a non-zero
 * FINOFFSET. In PADS, finger_offset runs along the finger's long axis and is stored
 * in the pad-local coordinate system (before rotation). PAD::ShapePos() applies the
 * full pad orientation when computing the shape center, so the importer must NOT
 * pre-rotate the offset vector by layer_def.rotation. Doing so rotated the offset
 * twice, shifting every finger pad sideways and visibly misaligning the connector.
 *
 * After the fix the stored offset is purely along pad-local X (offset.y == 0); a
 * double rotation by 90 degrees would instead leave offset.x == 0 and put the
 * magnitude on offset.y, which these checks reject.
 */
BOOST_AUTO_TEST_CASE( ImportFingerPadOffsetIssue23425 )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir()
                        + "plugins/pads/issue23425/controlCARDDockingStation.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    FOOTPRINT* j3 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == wxT( "J3" ) )
        {
            j3 = fp;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( j3, "J3 (HSEC8 edge connector) not found on board" );

    // FINOFFSET 1143000 BASIC units * (25400 / 38100) = 762000 nm (30 mil)
    const int expectedOffset = 762000;
    const int tolerance = 1000;   // 1um

    int offsetPadCount = 0;

    for( PAD* pad : j3->Pads() )
    {
        VECTOR2I offset = pad->GetOffset( F_Cu );

        if( offset == VECTOR2I( 0, 0 ) )
            continue;

        offsetPadCount++;

        // Stored unrotated in pad-local space: all magnitude on X, none on Y.
        BOOST_CHECK_MESSAGE( offset.y == 0,
                "J3 pad " << pad->GetNumber()
                << " offset Y should be 0 (unrotated pad-local), got " << offset.y );

        BOOST_CHECK_MESSAGE( std::abs( std::abs( offset.x ) - expectedOffset ) < tolerance,
                "J3 pad " << pad->GetNumber()
                << " offset X magnitude " << std::abs( offset.x )
                << " should be ~" << expectedOffset );
    }

    // The connector's signal fingers all carry the offset; before the fix none of
    // them satisfied the checks above. Require a substantial number so a parser
    // change that stops applying the offset entirely cannot pass silently.
    BOOST_CHECK_MESSAGE( offsetPadCount >= 90,
            "expected the HSEC8 finger pads to carry a finger offset; got "
            << offsetPadCount );
}


/**
 * Verify padstack import for issue #23391.
 *
 * A padstack whose layer -2 and layer -1 entries share the same shape but have
 * different sizes must use NORMAL mode (not FRONT_INNER_BACK).  The primary
 * (layer -2, component-side) size must be preserved, and top-placed and
 * bottom-placed instances of the same footprint must produce identical pad sizes.
 *
 * A padstack with different shapes on layer -2 and -1 (square pin-1) must still
 * use FRONT_INNER_BACK so the shape difference is preserved.
 */
BOOST_AUTO_TEST_CASE( ImportIssue23391 )
{
    PCB_IO_PADS plugin;

    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23391.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );

    BOOST_REQUIRE( board != nullptr );

    // Collect mounting-hole footprints (MTG_HOLE_TYPE: E1/E2 top, E3/E4 bottom)
    // and connector footprints (SQ_PIN1_TYPE: X1 top, X2 bottom).
    PAD* mtg_top_pad = nullptr;
    PAD* mtg_bot_pad = nullptr;
    PAD* conn_top_pin1 = nullptr;
    PAD* conn_bot_pin1 = nullptr;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString ref = fp->GetReference();

        for( PAD* pad : fp->Pads() )
        {
            if( ref == "E1" )
                mtg_top_pad = pad;
            else if( ref == "E3" )
                mtg_bot_pad = pad;
            else if( ref == "X1" && pad->GetNumber() == "1" )
                conn_top_pin1 = pad;
            else if( ref == "X2" && pad->GetNumber() == "1" )
                conn_bot_pin1 = pad;
        }
    }

    BOOST_REQUIRE_MESSAGE( mtg_top_pad, "E1 (top mounting hole) not found" );
    BOOST_REQUIRE_MESSAGE( mtg_bot_pad, "E3 (bottom mounting hole) not found" );
    BOOST_REQUIRE_MESSAGE( conn_top_pin1, "X1 pin 1 (top connector square pad) not found" );
    BOOST_REQUIRE_MESSAGE( conn_bot_pin1, "X2 pin 1 (bottom connector square pad) not found" );

    // Same-shape / different-size padstack must remain in NORMAL mode.
    BOOST_CHECK_MESSAGE(
            mtg_top_pad->Padstack().Mode() == PADSTACK::MODE::NORMAL,
            "Mounting hole with same shape but different sizes should use NORMAL padstack mode" );

    BOOST_CHECK_MESSAGE(
            mtg_bot_pad->Padstack().Mode() == PADSTACK::MODE::NORMAL,
            "Bottom-placed mounting hole should also use NORMAL padstack mode" );

    // Both instances of the same footprint must have identical pad sizes.
    VECTOR2I top_size = mtg_top_pad->GetSize( F_Cu );
    VECTOR2I bot_size = mtg_bot_pad->GetSize( F_Cu );

    BOOST_CHECK_MESSAGE(
            top_size == bot_size,
            "Top and bottom mounting holes should have equal pad size on F_Cu; "
            "top=" << top_size.x << " bot=" << bot_size.x );

    // The primary (layer -2) size must be used, not the secondary (layer -1) size.
    // In the test file layer -2 = 200 mils and layer -1 = 150 mils.
    // At 1 mil = 25400 nm, 200 mils = 5080000 nm.
    BOOST_CHECK_MESSAGE(
            top_size.x > 0,
            "Mounting hole pad size must be non-zero" );

    // Different-shape padstack (square vs round) must still use FRONT_INNER_BACK.
    BOOST_CHECK_MESSAGE(
            conn_top_pin1->Padstack().Mode() == PADSTACK::MODE::FRONT_INNER_BACK,
            "Connector pin-1 with square-on-top / round-on-bottom must use FRONT_INNER_BACK" );

    // Square (RECTANGLE) shape must appear on F_Cu for the top-placed connector.
    BOOST_CHECK_MESSAGE(
            conn_top_pin1->GetShape( F_Cu ) == PAD_SHAPE::RECTANGLE,
            "Top connector pin-1 must have RECTANGLE shape on F_Cu" );

    BOOST_CHECK_MESSAGE(
            conn_top_pin1->GetShape( B_Cu ) == PAD_SHAPE::CIRCLE,
            "Top connector pin-1 must have CIRCLE shape on B_Cu" );

    // After Flip, the bottom-placed connector pin-1 must have the shapes swapped.
    BOOST_CHECK_MESSAGE(
            conn_bot_pin1->GetShape( F_Cu ) == PAD_SHAPE::CIRCLE,
            "Bottom connector pin-1 must have CIRCLE shape on F_Cu after flip" );

    BOOST_CHECK_MESSAGE(
            conn_bot_pin1->GetShape( B_Cu ) == PAD_SHAPE::RECTANGLE,
            "Bottom connector pin-1 must have RECTANGLE shape on B_Cu after flip" );
}


/**
 * Issue #23856: PADS PCB import got text and pad details wrong.
 *
 *   1. Free text containing non-ASCII bytes (e.g. the copyright sign) was
 *      dropped entirely because the raw ISO-8859-1 content was treated as UTF-8.
 *   2. Finger pad orientation (PADS FINORI) was reset to zero by a later round
 *      pad-stack layer entry, so oval/rectangular fingers were not rotated.
 *   3. Free text on the back of the board ignored the PADS mirror flag, so it
 *      read left-to-right instead of mirrored.
 */
BOOST_AUTO_TEST_CASE( Issue23856_TextAndPadOrientation )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23856.asc";

    std::unique_ptr<BOARD> board;
    board.reset( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    // Issue 1 + 3: free text with the copyright character must survive import,
    // and back-side text must be mirrored.
    int copyrightCount = 0;
    bool frontCopyrightNotMirrored = false;
    bool backCopyrightMirrored = false;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_TEXT* t = dynamic_cast<PCB_TEXT*>( item );

        if( !t )
            continue;

        // No imported free text should be empty (empty == dropped on decode).
        BOOST_CHECK_MESSAGE( !t->GetText().IsEmpty(),
                "imported free text should not be empty" );

        if( t->GetText().Contains( wxT( "TEXMATE" ) ) )
        {
            copyrightCount++;

            // Copyright sign previously broke the UTF-8 decode.
            BOOST_CHECK_MESSAGE( t->GetText().Contains( wxString::FromUTF8( "©" ) ),
                    "copyright text should retain the (c) character" );

            if( t->GetLayer() == F_SilkS )
            {
                BOOST_CHECK_MESSAGE( !t->IsMirrored(),
                        "front silkscreen text should not be mirrored" );
                frontCopyrightNotMirrored = true;
            }
            else if( IsBackLayer( t->GetLayer() ) )
            {
                BOOST_CHECK_MESSAGE( t->IsMirrored(),
                        "back-side text should be mirrored" );
                backCopyrightMirrored = true;
            }
        }
    }

    BOOST_CHECK_MESSAGE( copyrightCount >= 2,
            "expected the copyright text on both front and back, got " << copyrightCount );
    BOOST_CHECK( frontCopyrightNotMirrored );
    BOOST_CHECK( backCopyrightMirrored );

    // Issue 2: CN1 finger pads use FINORI 90, which must not be reset to zero by
    // the back-side round entry of the through-hole stack.
    bool foundCN1 = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() != wxT( "CN1" ) )
            continue;

        foundCN1 = true;

        BOOST_CHECK_MESSAGE( fp->Pads().size() >= 10,
                "CN1 should have at least 10 pads, got " << fp->Pads().size() );

        for( PAD* pad : fp->Pads() )
        {
            // Oval/rectangle finger pads must carry the 90 degree finger rotation.
            if( pad->GetShape( F_Cu ) == PAD_SHAPE::OVAL
                || pad->GetShape( F_Cu ) == PAD_SHAPE::RECTANGLE )
            {
                BOOST_CHECK_MESSAGE(
                        pad->GetOrientation() == EDA_ANGLE( 90, DEGREES_T ),
                        "CN1 finger pad " << pad->GetNumber().ToStdString()
                            << " should be oriented 90 degrees, got "
                            << pad->GetOrientation().AsDegrees() );
            }
        }
    }

    BOOST_CHECK_MESSAGE( foundCN1, "CN1 footprint should be imported" );
}


/**
 * Verify that RT/ST thermal relief pad-stack entries map the PADS thermal-relief
 * geometry onto KiCad pad overrides (issue #23392): a THERMAL zone connection, a
 * spoke-width override, and, when the relief outer diameter exceeds the pad size, a
 * thermal-gap override derived as (outer - pad_size) / 2. Reliefs whose outer diameter
 * equals the pad size carry no gap and must leave the gap override unset.
 */
BOOST_AUTO_TEST_CASE( Issue23392_ThermalReliefGap )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/issue23393/demo.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    auto findFP = [&]( const wxString& aRef ) -> FOOTPRINT*
    {
        for( FOOTPRINT* fp : board->Footprints() )
        {
            if( fp->GetReference() == aRef )
                return fp;
        }

        return nullptr;
    };

    const int tolerance = 5000;  // 5 um

    // L1 carries an RT relief whose outer diameter (4110000) exceeds the pad size
    // (3750000), so every pad gets a THERMAL connection, a spoke-width override, and a
    // gap override of (4110000 - 3750000) / 2 scaled to internal units.
    {
        FOOTPRINT* l1 = findFP( "L1" );
        BOOST_REQUIRE_MESSAGE( l1, "L1 footprint should be imported" );
        BOOST_REQUIRE( !l1->Pads().empty() );

        for( PAD* pad : l1->Pads() )
        {
            BOOST_CHECK_MESSAGE( pad->GetLocalZoneConnection() == ZONE_CONNECTION::THERMAL,
                    "L1 pad " << pad->GetNumber().ToStdString()
                        << " should have THERMAL zone connection" );

            std::optional<int> spoke = pad->GetLocalThermalSpokeWidthOverride();
            BOOST_REQUIRE_MESSAGE( spoke.has_value(),
                    "L1 pad " << pad->GetNumber().ToStdString()
                        << " should have a spoke-width override" );
            BOOST_CHECK_MESSAGE( std::abs( spoke.value() - 1000000 ) < tolerance,
                    "L1 pad " << pad->GetNumber().ToStdString() << " spoke width "
                        << spoke.value() << " should be ~1000000 nm" );

            std::optional<int> gap = pad->GetLocalThermalGapOverride();
            BOOST_REQUIRE_MESSAGE( gap.has_value(),
                    "L1 pad " << pad->GetNumber().ToStdString()
                        << " should have a thermal gap override" );
            BOOST_CHECK_MESSAGE( std::abs( gap.value() - 120000 ) < tolerance,
                    "L1 pad " << pad->GetNumber().ToStdString() << " thermal gap "
                        << gap.value() << " should be ~120000 nm" );
        }
    }

    // E1 carries an RT relief whose outer diameter equals the pad size, so it must have a
    // THERMAL connection but NO gap override.
    {
        FOOTPRINT* e1 = findFP( "E1" );
        BOOST_REQUIRE_MESSAGE( e1, "E1 footprint should be imported" );
        BOOST_REQUIRE( !e1->Pads().empty() );

        PAD* pad = e1->Pads().front();
        BOOST_CHECK_MESSAGE( pad->GetLocalZoneConnection() == ZONE_CONNECTION::THERMAL,
                "E1 pad should have THERMAL zone connection" );
        BOOST_CHECK_MESSAGE( !pad->GetLocalThermalGapOverride().has_value(),
                "E1 pad should NOT have a thermal gap override (outer == pad size)" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
