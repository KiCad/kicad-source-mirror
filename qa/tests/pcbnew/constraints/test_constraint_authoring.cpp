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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <map>
#include <memory>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


BOOST_AUTO_TEST_SUITE( ConstraintSolverAuthoring )


BOOST_AUTO_TEST_CASE( BuildParallelFromTwoSegments )
{
    BOARD board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::PARALLEL, { a, b } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE_EQUAL( c->GetMembers().size(), 2 );
    BOOST_CHECK( c->GetMembers()[0].m_item == a->m_Uuid );
    BOOST_CHECK( c->GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::WHOLE );
    BOOST_CHECK( c->GetMembers()[1].m_item == b->m_Uuid );
}


BOOST_AUTO_TEST_CASE( BuildFixedLengthMeasuresGeometry )
{
    BOARD board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 7 * MM, 0 } );

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH, { seg } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE( c->HasValue() );
    BOOST_CHECK_CLOSE( *c->GetValue(), 7.0 * MM, 1e-6 );
}


BOOST_AUTO_TEST_CASE( BuildAngularMeasuresAngle )
{
    BOARD board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // along +x
    PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 0, 10 * MM } );        // along +y (90 deg)

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE_EQUAL( c->GetMembers().size(), 2 );
    BOOST_REQUIRE( c->HasValue() );
    BOOST_CHECK_CLOSE( *c->GetValue(), 90.0, 1e-6 );   // degrees
}


BOOST_AUTO_TEST_CASE( BuildAngularIsUndirectedCorner )
{
    BOARD board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // ray +x
    PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 0, -10 * MM } );       // ray -y

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE( c->HasValue() );

    // The corner is undirected: the same 90 degree opening regardless of segment orientation.
    BOOST_CHECK_CLOSE( *c->GetValue(), 90.0, 1e-3 );
}


// A 120 degree corner authored in all four endpoint permutations and both member orders stores 120,
// never folded to 60 (Seth: obtuse outline corners are preserved).
BOOST_AUTO_TEST_CASE( BuildAngularObtuseCornerInvariant )
{
    const VECTOR2I vertex{ 0, 0 };
    const VECTOR2I aFar{ 10 * MM, 0 };          // ray at 0 deg
    const VECTOR2I bFar{ -5 * MM, 8660254 };    // ray at 120 deg (10 mm at 120)

    for( bool swapA : { false, true } )
    {
        for( bool swapB : { false, true } )
        {
            for( bool swapOrder : { false, true } )
            {
                BOARD      board;
                PCB_SHAPE* a = swapA ? addSegment( board, aFar, vertex )
                                     : addSegment( board, vertex, aFar );
                PCB_SHAPE* b = swapB ? addSegment( board, bFar, vertex )
                                     : addSegment( board, vertex, bFar );

                std::vector<BOARD_ITEM*> items = swapOrder ? std::vector<BOARD_ITEM*>{ b, a }
                                                           : std::vector<BOARD_ITEM*>{ a, b };

                std::unique_ptr<PCB_CONSTRAINT> c = BuildConstraintFromItems(
                        &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, items );

                BOOST_REQUIRE( c );
                BOOST_REQUIRE( c->HasValue() );
                BOOST_CHECK_CLOSE( *c->GetValue(), 120.0, 0.05 );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( BuildAngularDomainEndpoints )
{
    {
        // Both rays leave the vertex the same way: a 0 degree corner.
        BOARD      board;
        PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
        PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 20 * MM, 0 } );
        auto c = BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );
        BOOST_REQUIRE( c && c->HasValue() );
        BOOST_CHECK_SMALL( *c->GetValue(), 1e-3 );
    }
    {
        // Rays leave the vertex in opposite directions: a straight 180 degree corner.
        BOARD      board;
        PCB_SHAPE* a = addSegment( board, { 0, 0 }, { -10 * MM, 0 } );
        PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
        auto c = BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );
        BOOST_REQUIRE( c && c->HasValue() );
        BOOST_CHECK_CLOSE( *c->GetValue(), 180.0, 1e-3 );
    }
}


// Two segments that do not touch still get a well-defined corner from nearest-endpoint pairing,
// with no dependence on SEG::IntersectLines.
BOOST_AUTO_TEST_CASE( BuildAngularDisconnectedLines )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );          // ray +x
    PCB_SHAPE* b = addSegment( board, { 0, 2 * MM }, { 0, 12 * MM } );     // ray +y, 2 mm gap

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );

    BOOST_REQUIRE( c && c->HasValue() );

    // SEG::Angle uses each segment's true direction, so the gap does not skew the measurement.
    BOOST_CHECK_CLOSE( *c->GetValue(), 90.0, 1e-3 );
}


BOOST_AUTO_TEST_CASE( WrongSelectionYieldsNoConstraint )
{
    BOARD board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    // Parallel needs two segments.
    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::PARALLEL, { a } ) );

    // Concentric needs circles, not a segment.
    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::CONCENTRIC, { a, a } ) );

    // Coincident needs point anchors, not whole shapes.
    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::COINCIDENT, { a, a } ) );
}


BOOST_AUTO_TEST_CASE( AngularRejectsZeroLengthSegment )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* zero = addSegment( board, { 5 * MM, 5 * MM }, { 5 * MM, 5 * MM } );   // no direction

    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION,
                                            { a, zero } ) );
}


// The nearest-anchor picker snaps a click to the closest shape endpoint/center.
BOOST_AUTO_TEST_CASE( NearestAnchorSnapsToEndpoint )
{
    BOARD board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    // Close to the end (10mm,0): snaps to that segment's END.
    auto hit = NearestConstraintAnchor( &board, { 10 * MM + 100, 200 }, 1 * MM );
    BOOST_REQUIRE( hit.has_value() );
    BOOST_CHECK( hit->m_item == seg->m_Uuid );
    BOOST_CHECK( hit->m_anchor == CONSTRAINT_ANCHOR::END );

    // Far from any anchor: nothing within tolerance.
    BOOST_CHECK( !NearestConstraintAnchor( &board, { 5 * MM, 5 * MM }, 1 * MM ).has_value() );
}


// Excluding a handle skips only that shape and anchor coincident endpoint elsewhere stays reachable
BOOST_AUTO_TEST_CASE( NearestAnchorExcludesPickedHandleNotCoincident )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 20 * MM, 0 } );

    const VECTOR2I shared{ 10 * MM, 0 };   // a's END and b's START coincide here

    std::optional<CONSTRAINT_MEMBER> first = NearestConstraintAnchor( &board, shared, 1 * MM );
    BOOST_REQUIRE( first.has_value() );

    // Exclude first handle still finds coincident endpoint as distinct member on b
    std::optional<CONSTRAINT_MEMBER> second =
            NearestConstraintAnchor( &board, shared, 1 * MM, { *first } );
    BOOST_REQUIRE( second.has_value() );
    BOOST_CHECK( *second != *first );
    BOOST_CHECK( second->m_item != first->m_item );
    BOOST_CHECK( ConstraintAnchorPosition( &board, *first )
                 == ConstraintAnchorPosition( &board, *second ) );

    // Two handles are shape a and shape b own endpoints at the shared point
    BOOST_CHECK( ( first->m_item == a->m_Uuid && second->m_item == b->m_Uuid )
                 || ( first->m_item == b->m_Uuid && second->m_item == a->m_Uuid ) );

    // Both handles excluded nothing else within tolerance
    BOOST_CHECK( !NearestConstraintAnchor( &board, shared, 1 * MM, { *first, *second } ).has_value() );
}


// Applying an authored constraint through a commit is a single undoable step.
BOOST_AUTO_TEST_CASE( ApplyIsOneUndoStep )
{
    BOARD                board;
    TOOL_MANAGER         mgr;
    KI_TEST::DUMMY_TOOL* tool = new KI_TEST::DUMMY_TOOL();
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    mgr.RegisterTool( tool );

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::PARALLEL, { a, b } );
    BOOST_REQUIRE( c );

    BOARD_COMMIT commit( tool );
    commit.Add( c.release() );
    commit.Push( wxT( "add constraint" ) );

    BOOST_CHECK_EQUAL( board.Constraints().size(), 1 );

    // Authoring then reverting (here a subsequent removal) returns to no constraints in one step.
    PCB_CONSTRAINT* added = board.Constraints().front();
    BOARD_COMMIT removeCommit( tool );
    removeCommit.Remove( added );
    removeCommit.Push( wxT( "remove constraint" ) );

    BOOST_CHECK( board.Constraints().empty() );
}


// The radial constraints accept arcs, and concentric also accepts ellipses, matching the solver.
BOOST_AUTO_TEST_CASE( BuildRadialFromArcsAndEllipses )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* arc = addArc( board, { 10 * MM, 0 }, { 12 * MM, 2 * MM }, { 14 * MM, 0 } );
    PCB_SHAPE* ellipse = addEllipse( board, { 30 * MM, 0 }, 8 * MM, 4 * MM, EDA_ANGLE( 0.0, DEGREES_T ) );

    // Fixed radius and equal radius work on arcs.
    BOOST_CHECK( BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS, { arc } ) );
    BOOST_CHECK( BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS, { circle, arc } ) );

    // Concentric additionally works on ellipses.
    BOOST_CHECK( BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::CONCENTRIC, { circle, ellipse } ) );
    BOOST_CHECK( BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::CONCENTRIC, { arc, ellipse } ) );

    // Equal radius still rejects an ellipse (no single radius).
    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS, { circle, ellipse } ) );
}


// Nothing remembered dialog opens on measured geometry
BOOST_AUTO_TEST_CASE( InitialValueDefaultsToMeasured )
{
    std::map<PCB_CONSTRAINT_TYPE, double> remembered;

    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::FIXED_LENGTH, 7.0 * MM, remembered ),
                       7.0 * MM, 1e-6 );
    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::ARC_ANGLE, 90.0, remembered ), 90.0,
                       1e-6 );
}


// Once set remembered value overrides measurement
BOOST_AUTO_TEST_CASE( InitialValueRemembersLastUsed )
{
    std::map<PCB_CONSTRAINT_TYPE, double> remembered;
    remembered[PCB_CONSTRAINT_TYPE::FIXED_LENGTH] = 5.0 * MM;

    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::FIXED_LENGTH, 7.0 * MM, remembered ),
                       5.0 * MM, 1e-6 );
}


// Keyed by type length value never bleeds into angle default
BOOST_AUTO_TEST_CASE( InitialValueDoesNotMixTypes )
{
    std::map<PCB_CONSTRAINT_TYPE, double> remembered;
    remembered[PCB_CONSTRAINT_TYPE::FIXED_LENGTH] = 5.0 * MM;

    // ARC_ANGLE unremembered opens on measured 90 degrees
    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::ARC_ANGLE, 90.0, remembered ), 90.0,
                       1e-6 );

    // Recording an angle leaves the earlier length value untouched.
    remembered[PCB_CONSTRAINT_TYPE::ARC_ANGLE] = 60.0;

    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::FIXED_LENGTH, 7.0 * MM, remembered ),
                       5.0 * MM, 1e-6 );
    BOOST_CHECK_CLOSE( InitialConstraintValue( PCB_CONSTRAINT_TYPE::ARC_ANGLE, 90.0, remembered ), 60.0,
                       1e-6 );
}


BOOST_AUTO_TEST_SUITE_END()
