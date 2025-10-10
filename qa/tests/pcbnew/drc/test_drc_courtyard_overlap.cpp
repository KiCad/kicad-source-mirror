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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew_utils/board_construction_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <widgets/ui_common.h>

#include <pcbnew_utils/board_test_utils.h>
#include "drc_test_utils.h"

/**
 * Simple definition of a rectangle, can be rounded
 */
struct RECT_DEFINITION
{
    VECTOR2I m_centre;
    VECTOR2I m_size;
    int      m_corner_rad;

    // On front or back layer (the exact layer is context-dependent)
    bool m_front;
};


/*
 * A simple mock footprint with a set of courtyard rectangles and some other information
 */
struct COURTYARD_TEST_FP
{
    std::string                  m_refdes;
    std::vector<RECT_DEFINITION> m_rects;
    VECTOR2I                     m_pos;
};


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
 * A complete courtyard overlap test case: a name, the board footprint list
 * and the expected collisions.
 */
struct COURTYARD_OVERLAP_TEST_CASE
{
    std::string                      m_case_name;
    std::vector<COURTYARD_TEST_FP>   m_fpDefs;      // The footprint in the test case
    std::vector<COURTYARD_COLLISION> m_collisions;  // The expected number of collisions
};


/**
 * Add a rectangular courtyard outline to a footprint.
 */
void AddRectCourtyard( FOOTPRINT& aFootprint, const RECT_DEFINITION& aRect )
{
    const PCB_LAYER_ID layer = aRect.m_front ? F_CrtYd : B_CrtYd;

    const int width = pcbIUScale.mmToIU( 0.1 );

    KI_TEST::DrawRect( aFootprint, aRect.m_centre, aRect.m_size, aRect.m_corner_rad, width, layer );
}


/**
 * Construct a #FOOTPRINT to use in a courtyard test from a #COURTYARD_TEST_FP definition.
 */
std::unique_ptr<FOOTPRINT> MakeCourtyardTestFP( BOARD& aBoard, const COURTYARD_TEST_FP& aFPDef )
{
    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( &aBoard );

    for( const RECT_DEFINITION& rect : aFPDef.m_rects )
        AddRectCourtyard( *footprint, rect );

    footprint->SetReference( aFPDef.m_refdes );

    // This has to go after adding the courtyards, or all the poly sets are empty when DRC'd
    footprint->SetPosition( aFPDef.m_pos );

    return footprint;
}

/**
 * Make a board for courtyard testing.
 *
 * @param aFPDefs the list of footprint definitions to add to the board
 */
std::unique_ptr<BOARD> MakeBoard( const std::vector<COURTYARD_TEST_FP>& aFPDefs )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    for( const COURTYARD_TEST_FP& fpDef : aFPDefs )
    {
        std::unique_ptr<FOOTPRINT> footprint = MakeCourtyardTestFP( *board, fpDef );

        board->Add( footprint.release() );
    }

    return board;
}


struct COURTYARD_TEST_FIXTURE
{
    const KI_TEST::BOARD_DUMPER m_dumper;
};


BOOST_FIXTURE_TEST_SUITE( DrcCourtyardOverlap, COURTYARD_TEST_FIXTURE )

// clang-format off
static std::vector<COURTYARD_OVERLAP_TEST_CASE> courtyard_cases = {
    {
        "empty board",
        {}, // no footprint
        {}, // no collisions
    },
    {
        "single empty footprint",
        {
            {
                "U1",
                {},         // no courtyard
                { 0, 0 },   // at origin
            },
        },
        {}, // no collisions
    },
    {
        // A single footprint can't overlap itself
        "single footprint, single courtyard",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
        },
        {}, // no collisions
    },
    {
        "two footprint, no overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { pcbIUScale.mmToIU( 3 ), pcbIUScale.mmToIU( 1 ) }, // One footprint is far from the other
            },
        },
        {}, // no collisions
    },
    {
        "two footprints, touching, no overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 0 ) }, // Just touching
            },
        },
        {}, // Touching means not colliding
    },
    {
        "two footprints, overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { pcbIUScale.mmToIU( 0.5 ), pcbIUScale.mmToIU( 0 ) }, // Partial overlap
            },
        },
        {
            { "U1", "U2" }, // These two collide
        },
    },
    {
        "two footprints, overlap, different sides",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        false,
                    },
                },
                { 0, 0 }, // complete overlap
            },
        },
        {}, // but on different sides
    },
    {
        "two footprints, multiple courtyards, overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                    {
                        { pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 0 ) },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        0,
                        true,
                    },
                },
                { 0, 0 }, // complete overlap with one of the others
            },
        },
        {
            { "U1", "U2" },
        },
    },
    {
        // The courtyards do not overlap, but their bounding boxes do
        "two footprints, no overlap, bbox overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        pcbIUScale.mmToIU( 0.5 ),
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { pcbIUScale.mmToIU( 0.9 ), pcbIUScale.mmToIU( 0.9 ) },
                        { pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) },
                        pcbIUScale.mmToIU( 0.5 ),
                        true,
                    },
                },
                { 0, 0 },
            },
        },
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
static void DoCourtyardOverlapTest( const COURTYARD_OVERLAP_TEST_CASE& aCase,
                                    const KI_TEST::BOARD_DUMPER& aDumper )
{
    auto board = MakeBoard( aCase.m_fpDefs );

    // Dump if env var set
    aDumper.DumpBoardToFile( *board, aCase.m_case_name );

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
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            DoCourtyardOverlapTest( c, m_dumper );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
