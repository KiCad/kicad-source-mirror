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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <widgets/ui_common.h>

#include "drc_test_utils.h"


/*
 * Struct holding information about a courtyard collision
 */
struct COURTYARD_COLLISION
{
    // The two colliding parts
    std::string m_refdes_a;
    std::string m_refdes_b;
};


std::ostream& operator<<( std::ostream& os, const COURTYARD_COLLISION& aColl )
{
    os << "COURTYARD_COLLISION[ " << aColl.m_refdes_a << " -> " << aColl.m_refdes_b << "]";
    return os;
}


/**
 * A complete courtyard overlap test case: the board fixture to load and the
 * expected collisions.
 */
struct COURTYARD_OVERLAP_TEST_CASE
{
    std::string                      m_board_name;  // Fixture under drc_courtyard/overlap/
    std::vector<COURTYARD_COLLISION> m_collisions;  // The expected collisions
};


BOOST_AUTO_TEST_SUITE( DrcCourtyardOverlap )

// clang-format off
static std::vector<COURTYARD_OVERLAP_TEST_CASE> courtyard_cases = {
    {
        "empty_board",
        {}, // no collisions
    },
    {
        "single_empty_footprint",
        {}, // no collisions
    },
    {
        // A single footprint can't overlap itself
        "single_footprint_single_courtyard",
        {}, // no collisions
    },
    {
        "two_footprints_no_overlap",
        {}, // no collisions
    },
    {
        "two_footprints_touching",
        {}, // Touching means not colliding
    },
    {
        "two_footprints_overlap",
        {
            { "U1", "U2" }, // These two collide
        },
    },
    {
        "two_footprints_overlap_different_sides",
        {}, // but on different sides
    },
    {
        "two_footprints_multiple_courtyards_overlap",
        {
            { "U1", "U2" },
        },
    },
    {
        // The courtyards do not overlap, but their bounding boxes do
        "two_footprints_bbox_overlap_only",
        {},
    },
};
// clang-format on


/**
 * Check if a #PCB_MARKER is described by a particular #COURTYARD_COLLISION object.
 */
static bool CollisionMatchesExpected( BOARD& aBoard, const PCB_MARKER& aMarker,
                                      const COURTYARD_COLLISION& aCollision )
{
    auto reporter = std::static_pointer_cast<DRC_ITEM>( aMarker.GetRCItem() );

    const FOOTPRINT* item_a = dynamic_cast<FOOTPRINT*>( aBoard.ResolveItem( reporter->GetMainItemID(), true ) );
    const FOOTPRINT* item_b = dynamic_cast<FOOTPRINT*>( aBoard.ResolveItem( reporter->GetAuxItemID(), true ) );

    // can't find the items!
    if( !item_a || !item_b )
        return false;

    const bool ref_match_aa_bb = ( item_a->GetReference() == aCollision.m_refdes_a )
                                 && ( item_b->GetReference() == aCollision.m_refdes_b );

    const bool ref_match_ab_ba = ( item_a->GetReference() == aCollision.m_refdes_b )
                                 && ( item_b->GetReference() == aCollision.m_refdes_a );

    // Doesn't matter which way around it is, but both have to match somehow
    return ref_match_aa_bb || ref_match_ab_ba;
}


/**
 * Check that the produced markers match the expected. This does NOT check ordering,
 * as that is not part of the contract of the DRC function.
 *
 * @param aMarkers    list of markers produced by the DRC
 * @param aCollisions list of expected collisions
 */
static void CheckCollisionsMatchExpected( BOARD& aBoard,
                                          const std::vector<std::unique_ptr<PCB_MARKER>>& aMarkers,
                                          const std::vector<COURTYARD_COLLISION>& aExpCollisions )
{
    for( const auto& marker : aMarkers )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsDrcMarkerOfType, ( *marker )( DRCE_OVERLAPPING_FOOTPRINTS ) );
    }

    KI_TEST::CheckUnorderedMatches( aExpCollisions, aMarkers,
            [&]( const COURTYARD_COLLISION& aColl, const std::unique_ptr<PCB_MARKER>& aMarker )
            {
                return CollisionMatchesExpected( aBoard, *aMarker, aColl );
            } );
}


/**
 * Run a single courtyard overlap testcase
 * @param aCase The testcase to run.
 */
static void DoCourtyardOverlapTest( const COURTYARD_OVERLAP_TEST_CASE& aCase )
{
    const std::string path = KI_TEST::GetPcbnewTestDataDir() + "drc_courtyard/overlap/"
                             + aCase.m_board_name + ".kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( path );

    BOOST_REQUIRE( board );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    bds.m_DRCSeverities[ DRCE_OVERLAPPING_FOOTPRINTS ] = RPT_SEVERITY_ERROR;

    // we might not always have courtyards - that's a separate test
    bds.m_DRCSeverities[ DRCE_MISSING_COURTYARD ] = RPT_SEVERITY_IGNORE;

    // list of markers to collect
    std::vector<std::unique_ptr<PCB_MARKER>> markers;

    DRC_ENGINE drcEngine( board.get(), &board->GetDesignSettings() );

    drcEngine.InitEngine( wxFileName() );

    drcEngine.SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if(    aItem->GetErrorCode() == DRCE_OVERLAPPING_FOOTPRINTS
                    || aItem->GetErrorCode() == DRCE_MALFORMED_COURTYARD
                    || aItem->GetErrorCode() == DRCE_MISSING_COURTYARD )
                {
                    markers.push_back( std::make_unique<PCB_MARKER>( aItem, aPos ) );
                }
            } );

    drcEngine.RunTests( EDA_UNITS::MM, true, false );

    CheckCollisionsMatchExpected( *board, markers, aCase.m_collisions );
}


BOOST_AUTO_TEST_CASE( OverlapCases )
{
    for( const auto& c : courtyard_cases )
    {
        BOOST_TEST_CONTEXT( c.m_board_name )
        {
            DoCourtyardOverlapTest( c );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
