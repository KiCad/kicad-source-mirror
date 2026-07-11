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


BOOST_AUTO_TEST_CASE( BuildAngularPreservesSign )
{
    BOARD board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // along +x (0 deg)
    PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 0, -10 * MM } );       // clockwise from a

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION, { a, b } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE( c->HasValue() );

    // The directed angle member[0] -> member[1] is negative; storing abs() would snap the lines to
    // the mirror configuration when the constraint solves on creation.
    BOOST_CHECK_CLOSE( *c->GetValue(), -90.0, 1e-6 );
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


BOOST_AUTO_TEST_SUITE_END()
