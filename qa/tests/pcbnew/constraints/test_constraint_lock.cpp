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

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


namespace
{
double segLength( const PCB_SHAPE* aSeg )
{
    return ( aSeg->GetEnd() - aSeg->GetStart() ).EuclideanNorm();
}
} // namespace


BOOST_AUTO_TEST_SUITE( ConstraintSolverLock )


// A locked segment is an immovable reference: an equal-length constraint stretches only the free
// segment, leaving the locked one untouched.  Without lock-awareness the solver splits the change
// across both and the locked segment moves, so this fails on the pre-fix code.
BOOST_AUTO_TEST_CASE( LockedSegmentIsNotMoved )
{
    BOARD board;

    PCB_SHAPE* locked = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // length 10 mm
    PCB_SHAPE* free = addSegment( board, { 0, 5 * MM }, { 4 * MM, 5 * MM } ); // length 4 mm

    locked->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_LENGTH,
                   { { locked->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ locked, free };
    solveAndApply( board, shapes );

    // The locked reference is byte-for-byte unchanged...
    BOOST_CHECK_EQUAL( locked->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( locked->GetEnd(), VECTOR2I( 10 * MM, 0 ) );

    // ...and the free segment took the locked segment's length.
    BOOST_CHECK_LE( std::abs( segLength( free ) - 10.0 * MM ), 5000.0 );
}


// The same guarantee for circles: a locked circle keeps its radius under equal-radius, so the free
// circle grows to it.  The locked circle is ordered second, where a lock-unaware solve would move
// it (the solver keeps the first-added radius), which is what makes this a regression.
BOOST_AUTO_TEST_CASE( LockedCircleIsNotMoved )
{
    BOARD board;

    PCB_SHAPE* free = addCircle( board, { 20 * MM, 0 }, 2 * MM );
    PCB_SHAPE* locked = addCircle( board, { 0, 0 }, 5 * MM );

    locked->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS,
                   { { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { locked->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ free, locked };
    solveAndApply( board, shapes );

    BOOST_CHECK_EQUAL( locked->GetCenter(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( locked->GetRadius(), 5 * MM );

    BOOST_CHECK_LE( std::abs( free->GetRadius() - 5 * MM ), 5000 );
}


// The interactive create path (ApplyConstraintImmediately) must honour the lock even when the
// locked shape is not the constraint's first member: it is neither moved nor reported as modified.
BOOST_AUTO_TEST_CASE( LockedMemberNotMovedOnApply )
{
    BOARD board;

    PCB_SHAPE* free = addSegment( board, { 0, 5 * MM }, { 4 * MM, 5 * MM } ); // member 0, length 4 mm
    PCB_SHAPE* locked = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // member 1, length 10 mm

    locked->SetLocked( true );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::EQUAL_LENGTH,
            { { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { locked->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, c, &modified,
                                []( PCB_SHAPE* )
                                {
                                } );

    // The locked segment did not move and is not staged for undo.
    BOOST_CHECK_EQUAL( locked->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( locked->GetEnd(), VECTOR2I( 10 * MM, 0 ) );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), locked ) == modified.end() );

    // The free segment took the locked segment's length.
    BOOST_CHECK_LE( std::abs( segLength( free ) - 10.0 * MM ), 5000.0 );
}


// Authoring an angle constraint against a locked reference must rotate the free segment, not shrink
// it to a point.  On the pre-fix solver the free segment collapsed to zero length and vanished, so
// this fails without the stabilize pins and the collapse floor.
BOOST_AUTO_TEST_CASE( PerpendicularDoesNotCollapseFreeSegment )
{
    BOARD board;

    PCB_SHAPE* locked = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // horizontal reference
    PCB_SHAPE* free = addSegment( board, { 0, 5 * MM }, { 4 * MM, 6 * MM } ); // angled, length ~4.1 mm

    locked->SetLocked( true );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::PERPENDICULAR,
            { { locked->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, c, &modified,
                                []( PCB_SHAPE* )
                                {
                                } );

    // The free segment kept a real length instead of collapsing...
    BOOST_CHECK_GT( segLength( free ), 1 * MM );

    // ...and turned perpendicular to the horizontal reference (vertical: endpoints share X).
    BOOST_CHECK_LE( std::abs( free->GetEnd().x - free->GetStart().x ), 1000 );
}


// Parallel and perpendicular on the same two lines contradict, so both are flagged and the geometry
// is left alone.
BOOST_AUTO_TEST_CASE( ContradictoryConstraintsAreFlagged )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );          // horizontal
    PCB_SHAPE* b = addSegment( board, { 0, 5 * MM }, { 4 * MM, 9 * MM } ); // 45 degrees

    PCB_CONSTRAINT* par =
            addConstraint( board, PCB_CONSTRAINT_TYPE::PARALLEL,
                           { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* perp =
            addConstraint( board, PCB_CONSTRAINT_TYPE::PERPENDICULAR,
                           { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOARD_CONSTRAINT_DIAGNOSTICS d = DiagnoseBoardConstraints( &board );
    std::set<KIID>               conflicting( d.conflicting.begin(), d.conflicting.end() );

    BOOST_CHECK( conflicting.count( par->m_Uuid ) );
    BOOST_CHECK( conflicting.count( perp->m_Uuid ) );

    // Diagnosis does not move geometry.
    BOOST_CHECK_EQUAL( b->GetStart(), VECTOR2I( 0, 5 * MM ) );
    BOOST_CHECK_EQUAL( b->GetEnd(), VECTOR2I( 4 * MM, 9 * MM ) );
}


BOOST_AUTO_TEST_SUITE_END()
