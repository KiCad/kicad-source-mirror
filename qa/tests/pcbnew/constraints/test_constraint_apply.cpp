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

/**
 * @file
 * Tests ApplyConstraintImmediately (issue #2329 UI): creating a constraint solves its cluster on
 * the spot, pinning the first member and moving the rest so the geometry satisfies the relation.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>

#include <board.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


BOOST_AUTO_TEST_SUITE( ConstraintApply )


BOOST_AUTO_TEST_CASE( CoincidentSnapsSecondPointToFirst )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 11 * MM, 1 * MM }, { 20 * MM, 0 } );

    // Bind a's END to b's START; pinning a's END should pull b's START onto it.
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                       { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                                         { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( a->GetEnd().x, 10 * MM );   // pinned, unmoved
    BOOST_CHECK_EQUAL( a->GetEnd().y, 0 );
    BOOST_CHECK_EQUAL( b->GetStart().x, 10 * MM ); // pulled onto a's END
    BOOST_CHECK_EQUAL( b->GetStart().y, 0 );
}


BOOST_AUTO_TEST_CASE( SingleShapeFixedLengthStagesThePinnedShape )
{
    BOARD board;

    // A 12mm segment driven to 8mm: pinning START keeps it put, END moves in. The pinned shape is
    // the only shape, so it must be reported as modified (else the move would not be committed).
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 12 * MM, 0 } );
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                       { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } },
                                       8.0 * MM );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_TEST( ( std::find( modified.begin(), modified.end(), seg ) != modified.end() ) );
    BOOST_CHECK_EQUAL( seg->GetStart().x, 0 );
    BOOST_CHECK_EQUAL( seg->GetEnd().x, 8 * MM );
}


BOOST_AUTO_TEST_CASE( CircleFirstMemberPinsCenterAndSolves )
{
    BOARD board;

    // Concentric circles authored as WHOLE members: WHOLE must pin the circle's CENTER (its only
    // anchor), not a non-existent START, or the solve silently does nothing.
    PCB_SHAPE* a = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* b = addCircle( board, { 3 * MM, 0 }, 2 * MM );
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::CONCENTRIC,
                                       { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                                         { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( a->GetCenter().x, 0 );       // pinned
    BOOST_CHECK_EQUAL( b->GetCenter().x, 0 );       // moved concentric with a
    BOOST_CHECK_EQUAL( b->GetCenter().y, 0 );
}


// Issue 25049: creating "point on line" moved the line to the point.  The picker always takes the
// point first, so the line is member 1 and ConstraintReferenceShapes names it the frozen reference.
BOOST_AUTO_TEST_CASE( PointOnLineMovesThePoint )
{
    BOARD board;

    // Oblique, so a stretch and a travel land the far end in different places.
    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg = addSegment( board, { 5 * MM, 3 * MM }, { 8 * MM, 7 * MM } );

    PCB_CONSTRAINT* c =
            addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                           { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag =
            ApplyConstraintImmediately( &board, c, &modified, {}, ConstraintReferenceShapes( &board, c ) );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( line->GetStart(), VECTOR2I( 0, 0 ) ); // frozen, unmoved
    BOOST_CHECK_EQUAL( line->GetEnd(), VECTOR2I( 10 * MM, 0 ) );
    // The whole segment travels: same angle, same length, not stretched to reach.
    BOOST_CHECK_LE( ( seg->GetStart() - VECTOR2I( 5 * MM, 0 ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 8 * MM, 4 * MM ) ).EuclideanNorm(), 1000 );

    BOOST_TEST( ( std::find( modified.begin(), modified.end(), seg ) != modified.end() ) );
    BOOST_TEST( ( std::find( modified.begin(), modified.end(), line ) == modified.end() ) );
}


// Issue 25049, the same for "midpoint": the segment used to move so its midpoint met the point.
BOOST_AUTO_TEST_CASE( MidpointMovesThePoint )
{
    BOARD board;

    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg = addSegment( board, { 2 * MM, 4 * MM }, { 2 * MM, 9 * MM } );

    PCB_CONSTRAINT* c =
            addConstraint( board, PCB_CONSTRAINT_TYPE::MIDPOINT,
                           { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag =
            ApplyConstraintImmediately( &board, c, &modified, {}, ConstraintReferenceShapes( &board, c ) );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( line->GetStart(), VECTOR2I( 0, 0 ) ); // frozen, unmoved
    BOOST_CHECK_EQUAL( line->GetEnd(), VECTOR2I( 10 * MM, 0 ) );

    // The midpoint is solver-derived, so allow the same slack the sibling adapter tests use.
    BOOST_CHECK_LE( ( seg->GetStart() - VECTOR2I( 5 * MM, 0 ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 5 * MM, 5 * MM ) ).EuclideanNorm(), 1000 );

    BOOST_TEST( ( std::find( modified.begin(), modified.end(), line ) == modified.end() ) );
}


// A circle reference freezes centre and radius, so the point lands on the circumference.
BOOST_AUTO_TEST_CASE( PointOnCircleMovesThePoint )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* seg = addSegment( board, { 8 * MM, 0 }, { 12 * MM, 0 } );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
            { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag =
            ApplyConstraintImmediately( &board, c, &modified, {}, ConstraintReferenceShapes( &board, c ) );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( circle->GetCenter(), VECTOR2I( 0, 0 ) ); // frozen, unmoved
    BOOST_CHECK_EQUAL( circle->GetRadius(), 5 * MM );
    BOOST_CHECK_LE( std::abs( ( seg->GetStart() - circle->GetCenter() ).EuclideanNorm() - 5.0 * MM ), 1000.0 );

    // Travelled whole, so it kept its 4mm length.
    BOOST_CHECK_LE( std::abs( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() - 4.0 * MM ), 1000.0 );
}


// A corridor pin looks just like an authored constraint, but its line is the segment just drawn.
// The drawing tool names no reference, so the plain apply still pins member 0 and pulls the drawn
// line onto the existing anchor.
BOOST_AUTO_TEST_CASE( AutoBindingKeepsPinningTheFirstMember )
{
    BOARD board;

    PCB_SHAPE* existing = addSegment( board, { 5 * MM, 0 }, { 5 * MM, 10 * MM } );
    PCB_SHAPE* drawn = addSegment( board, { 0, 1 * MM }, { 10 * MM, 1 * MM } );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
            { { existing->m_Uuid, CONSTRAINT_ANCHOR::START }, { drawn->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    // Soft-pinned rather than frozen, so allow the solver's slack here.
    BOOST_TEST( diag.solved );
    BOOST_CHECK_LE( ( existing->GetStart() - VECTOR2I( 5 * MM, 0 ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( ( existing->GetEnd() - VECTOR2I( 5 * MM, 10 * MM ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( std::abs( drawn->GetStart().y ), 1000 ); // drawn line pulled onto it
    BOOST_CHECK_LE( std::abs( drawn->GetEnd().y ), 1000 );
}


// Holding the moved shape whole pushes the give onto a neighbour tied to it, which stretches.
BOOST_AUTO_TEST_CASE( CoincidentNeighbourTakesTheStretch )
{
    BOARD board;

    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* seg = addSegment( board, { 5 * MM, 3 * MM }, { 5 * MM, 8 * MM } );
    PCB_SHAPE* neighbour = addSegment( board, { 5 * MM, 8 * MM }, { 9 * MM, 8 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { neighbour->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* c =
            addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                           { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    CONSTRAINT_DIAGNOSIS diag =
            ApplyConstraintImmediately( &board, c, nullptr, {}, ConstraintReferenceShapes( &board, c ) );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_LE( ( seg->GetStart() - VECTOR2I( 5 * MM, 0 ) ).EuclideanNorm(), 1000 );

    // seg kept its length, so the corner came down with it and the neighbour follows.
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 5 * MM, 5 * MM ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( ( neighbour->GetStart() - seg->GetEnd() ).EuclideanNorm(), 1000 );
}


// Freezing the line would leave nothing able to move, so these keep the old behaviour instead.
BOOST_AUTO_TEST_CASE( NoReferenceWhenThePointCannotMove )
{
    BOARD board;

    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* locked = addSegment( board, { 5 * MM, 3 * MM }, { 5 * MM, 8 * MM } );
    PCB_SHAPE* pinned = addSegment( board, { 2 * MM, 3 * MM }, { 2 * MM, 8 * MM } );

    locked->SetLocked( true );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION, { { pinned->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* onLocked = addConstraint(
            board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
            { { locked->m_Uuid, CONSTRAINT_ANCHOR::START }, { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* onPinned = addConstraint(
            board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
            { { pinned->m_Uuid, CONSTRAINT_ANCHOR::START }, { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOOST_TEST( ConstraintReferenceShapes( &board, onLocked ).empty() );
    BOOST_TEST( ConstraintReferenceShapes( &board, onPinned ).empty() );
}


// Every other constraint keeps pinning its first member, and so does a self-referencing one.
BOOST_AUTO_TEST_CASE( NoReferenceForOtherConstraints )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* parallel =
            addConstraint( board, PCB_CONSTRAINT_TYPE::PARALLEL,
                           { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* oneMember =
            addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE, { { a->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    PCB_CONSTRAINT* itself =
            addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                           { { a->m_Uuid, CONSTRAINT_ANCHOR::START }, { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOOST_TEST( ConstraintReferenceShapes( &board, parallel ).empty() );
    BOOST_TEST( ConstraintReferenceShapes( &board, oneMember ).empty() );
    BOOST_TEST( ConstraintReferenceShapes( &board, itself ).empty() );
}


BOOST_AUTO_TEST_CASE( FailedOrEmptyClusterLeavesGeometry )
{
    BOARD board;

    // A constraint with no members cannot pin or solve anything.
    PCB_CONSTRAINT empty( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, &empty, &modified );

    BOOST_TEST( !diag.solved );
    BOOST_TEST( modified.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
