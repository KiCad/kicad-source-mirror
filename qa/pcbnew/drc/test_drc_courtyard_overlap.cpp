/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <unit_test_utils/unit_test_utils.h>

#include <pcbnew_utils/board_construction_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <class_module.h>
#include <drc.h>

#include <drc/courtyard_overlap.h>

#include "../board_test_utils.h"
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
 * A simple mock module with a set of courtyard rectangles and some other
 * information
 */
struct COURTYARD_TEST_MODULE
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
 * A complete courtyard overlap test case: a name, the board modules list
 * and the expected collisions.
 */
struct COURTYARD_OVERLAP_TEST_CASE
{
    std::string m_case_name;

    // The modules in the test case
    std::vector<COURTYARD_TEST_MODULE> m_mods;

    // The expected number of collisions
    std::vector<COURTYARD_COLLISION> m_collisions;
};


/**
 * Add a rectangular courtyard outline to a module.
 */
void AddRectCourtyard( MODULE& aMod, const RECT_DEFINITION& aRect )
{
    const PCB_LAYER_ID layer = aRect.m_front ? F_CrtYd : B_CrtYd;

    const int width = Millimeter2iu( 0.1 );

    KI_TEST::DrawRect( aMod, aRect.m_centre, aRect.m_size, aRect.m_corner_rad, width, layer );
}


/**
 * Construct a #MODULE to use in a courtyard test from a #COURTYARD_TEST_MODULE
 * definition.
 */
std::unique_ptr<MODULE> MakeCourtyardTestModule( BOARD& aBoard, const COURTYARD_TEST_MODULE& aMod )
{
    auto module = std::make_unique<MODULE>( &aBoard );

    for( const auto& rect : aMod.m_rects )
    {
        AddRectCourtyard( *module, rect );
    }

    module->SetReference( aMod.m_refdes );

    // As of 2019-01-17, this has to go after adding the courtyards,
    // or all the poly sets are empty when DRC'd
    module->SetPosition( (wxPoint) aMod.m_pos );

    return module;
}

/**
 * Make a board for courtyard testing.
 *
 * @param aMods the list of module definitions to add to the board
 */
std::unique_ptr<BOARD> MakeBoard( const std::vector<COURTYARD_TEST_MODULE>& aMods )
{
    auto board = std::make_unique<BOARD>();

    for( const auto& mod : aMods )
    {
        auto module = MakeCourtyardTestModule( *board, mod );

        board->Add( module.release() );
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
        {}, // no modules
        {}, // no collisions
    },
    {
        "single empty mod",
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
        // A single module can't overlap itself
        "single mod, single courtyard",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
        "two modules, no overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        0,
                        true,
                    },
                },
                { Millimeter2iu( 3 ), Millimeter2iu( 1 ) }, // One module is far from the other
            },
        },
        {}, // no collisions
    },
    {
        "two modules, touching, no overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        0,
                        true,
                    },
                },
                { Millimeter2iu( 1 ), Millimeter2iu( 0 ) }, // Just touching
            },
        },
        {}, // Touching means not colliding
    },
    {
        "two modules, overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        0,
                        true,
                    },
                },
                { Millimeter2iu( 0.5 ), Millimeter2iu( 0 ) }, // Partial overlap
            },
        },
        {
            { "U1", "U2" }, // These two collide
        },
    },
    {
        "two modules, overlap, different sides",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
        "two modules, multiple courtyards, overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        0,
                        true,
                    },
                    {
                        { Millimeter2iu( 2 ), Millimeter2iu( 0 ) },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
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
        "two modules, no overlap, bbox overlap",
        {
            {
                "U1",
                {
                    {
                        { 0, 0 },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        Millimeter2iu( 0.5 ),
                        true,
                    },
                },
                { 0, 0 },
            },
            {
                "U2",
                {
                    {
                        { Millimeter2iu( 0.9 ), Millimeter2iu( 0.9 ) },
                        { Millimeter2iu( 1 ), Millimeter2iu( 1 ) },
                        Millimeter2iu( 0.5 ),
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
 * Check if a #MARKER_PCB is described by a particular #COURTYARD_COLLISION
 * object.
 */
static bool CollisionMatchesExpected(
        BOARD& aBoard, const MARKER_PCB& aMarker, const COURTYARD_COLLISION& aCollision )
{
    const DRC_ITEM& reporter = aMarker.GetReporter();

    const MODULE* item_a = dynamic_cast<MODULE*>( reporter.GetMainItem( &aBoard ) );
    const MODULE* item_b = dynamic_cast<MODULE*>( reporter.GetAuxiliaryItem( &aBoard ) );

    // cant' find the items!
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
 * Check that the produced markers match the expected. This does NOT
 * check ordering, as that is not part of the contract of the DRC function.
 *
 * @param aMarkers    list of markers produced by the DRC
 * @param aCollisions list of expected collisions
 */
static void CheckCollisionsMatchExpected( BOARD&        aBoard,
        const std::vector<std::unique_ptr<MARKER_PCB>>& aMarkers,
        const std::vector<COURTYARD_COLLISION>&         aExpCollisions )
{
    for( const auto& marker : aMarkers )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsDrcMarkerOfType, ( *marker )( DRCE_OVERLAPPING_FOOTPRINTS ) );
    }

    KI_TEST::CheckUnorderedMatches( aExpCollisions, aMarkers,
            [&]( const COURTYARD_COLLISION& aColl, const std::unique_ptr<MARKER_PCB>& aMarker ) {
                return CollisionMatchesExpected( aBoard, *aMarker, aColl );
            } );
}


/**
 * Get a #BOARD_DESIGN_SETTINGS object that will cause DRC to
 * check for courtyard overlaps
 */
static BOARD_DESIGN_SETTINGS GetOverlapCheckDesignSettings()
{
    BOARD_DESIGN_SETTINGS des_settings;
    des_settings.m_ProhibitOverlappingCourtyards = true;

    // we might not always have courtyards - that's a separate test
    des_settings.m_RequireCourtyards = false;

    return des_settings;
}


/**
 * Run a single courtyard overlap testcase
 * @param aCase The testcase to run.
 */
static void DoCourtyardOverlapTest(
        const COURTYARD_OVERLAP_TEST_CASE& aCase, const KI_TEST::BOARD_DUMPER& aDumper )
{
    DRC_MARKER_FACTORY marker_factory;

    auto board = MakeBoard( aCase.m_mods );

    // Dump if env var set
    aDumper.DumpBoardToFile( *board, aCase.m_case_name );

    board->SetDesignSettings( GetOverlapCheckDesignSettings() );

    // list of markers to collect
    std::vector<std::unique_ptr<MARKER_PCB>> markers;

    DRC_COURTYARD_OVERLAP drc_overlap( marker_factory, [&]( MARKER_PCB* aMarker ) {
        markers.push_back( std::unique_ptr<MARKER_PCB>( aMarker ) );
    } );

    drc_overlap.RunDRC( *board );

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