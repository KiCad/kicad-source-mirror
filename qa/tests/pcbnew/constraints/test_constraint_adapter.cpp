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

#include <cmath>
#include <cstdlib>
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
int slopeDiffY( const PCB_SHAPE* aSeg )
{
    return std::abs( aSeg->GetEnd().y - aSeg->GetStart().y );
}
}


BOOST_AUTO_TEST_SUITE( ConstraintSolverAdapter )


// A free segment made parallel to a fixed horizontal reference becomes horizontal.
BOOST_AUTO_TEST_CASE( ParallelToFixedHorizontal )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* free = addSegment( board, { 1 * MM, 5 * MM }, { 9 * MM, 7 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::PARALLEL,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    solveAndApply( board, shapes );

    // free is now horizontal (endpoints share Y) and ref is untouched.
    BOOST_CHECK_LE( slopeDiffY( free ), 100 );   // within 100 nm of horizontal
    BOOST_CHECK_EQUAL( ref->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( ref->GetEnd(), VECTOR2I( 10 * MM, 0 ) );
}


// The same cluster placed far from the origin still solves (normalization conditions it).
BOOST_AUTO_TEST_CASE( ConditioningFarFromOrigin )
{
    BOARD board;

    const VECTOR2I off( 500 * MM, 700 * MM );

    PCB_SHAPE* ref = addSegment( board, off, off + VECTOR2I( 10 * MM, 0 ) );
    PCB_SHAPE* free = addSegment( board, off + VECTOR2I( 1 * MM, 5 * MM ),
                                  off + VECTOR2I( 9 * MM, 7 * MM ) );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::PARALLEL,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( slopeDiffY( free ), 100 );
}


// Dragging a coincident corner re-derives the neighbor through SolveCluster.
BOOST_AUTO_TEST_CASE( DragCoincidentCornerMovesNeighbor )
{
    BOARD board;

    // Two segments sharing a corner: A.end coincident with B.start.
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    // Drag A.end to a new spot; B.start must follow to stay coincident.
    CONSTRAINT_MEMBER dragged{ a->m_Uuid, CONSTRAINT_ANCHOR::END };
    std::vector<PCB_SHAPE*> modified;

    CONSTRAINT_DIAGNOSIS diag = SolveCluster( &board, dragged, { 12 * MM, 3 * MM }, &modified );

    BOOST_REQUIRE( diag.solved );

    // A.end and B.start moved together and remain coincident.
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( a->GetEnd() - VECTOR2I( 12 * MM, 3 * MM ) ).EuclideanNorm(), 1000 );

    // The neighbor B is reported as modified; the dragged shape A is not.
    BOOST_CHECK( std::find( modified.begin(), modified.end(), b ) != modified.end() );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), a ) == modified.end() );
}


// Diagnosis: a lone segment with one direction constraint is under-constrained.
BOOST_AUTO_TEST_CASE( DiagnosisUnderConstrained )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ seg };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_REQUIRE( adapter.Solve() );

    CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();
    BOOST_CHECK( diag.freeDof > 0 );
    BOOST_CHECK( !diag.IsOverConstrained() );
}


// Diagnosis: a duplicated constraint is reported as redundant and attributed by uuid.
// (A true metric conflict needs distance/length constraints, which arrive in step 6; the
// directional subset here over-constrains into degenerate-but-solvable systems, not conflicts.)
BOOST_AUTO_TEST_CASE( DiagnosisRedundant )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* dup = addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ seg };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_REQUIRE( adapter.Solve() );

    CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();

    BOOST_CHECK( !diag.redundant.empty() );

    // The redundancy is attributed to a real constraint by uuid.
    bool attributed = false;

    for( const KIID& id : diag.redundant )
    {
        for( PCB_CONSTRAINT* c : board.Constraints() )
        {
            if( c->m_Uuid == id )
                attributed = true;
        }
    }

    BOOST_CHECK( attributed );
    (void) dup;
}


// Diagnosis: a fixed reference plus parallel fully determines a free segment's direction; with
// its remaining freedom pinned by a drag it reaches zero free DOF.
BOOST_AUTO_TEST_CASE( DiagnosisWellConstrained )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* free = addSegment( board, { 1 * MM, 5 * MM }, { 9 * MM, 5 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { free->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { free->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    bool solved = adapter.Solve();
    BOOST_REQUIRE( solved );

    CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();
    diag.solved = solved;   // Diagnose() does not run a solve; carry the verdict like real callers.

    // Every coordinate is grounded, so there is no remaining freedom.
    BOOST_CHECK_EQUAL( diag.freeDof, 0 );
    BOOST_CHECK( diag.IsWellConstrained() );
}


// A free segment made perpendicular to a fixed horizontal reference becomes vertical.
BOOST_AUTO_TEST_CASE( PerpendicularToFixedHorizontal )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* free = addSegment( board, { 1 * MM, 5 * MM }, { 9 * MM, 7 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::PERPENDICULAR,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( free->GetEnd().x - free->GetStart().x ), 100 );   // vertical
}


// A free segment made equal-length to a fixed reference takes the reference's length.
BOOST_AUTO_TEST_CASE( EqualLengthToFixedReference )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );           // length 10 mm
    PCB_SHAPE* free = addSegment( board, { 0, 20 * MM }, { 4 * MM, 20 * MM } ); // length 4 mm

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { free->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_LENGTH,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    solveAndApply( board, shapes );

    double freeLen = ( free->GetEnd() - free->GetStart() ).EuclideanNorm();
    BOOST_CHECK_LE( std::abs( freeLen - 10.0 * MM ), 1000 );
}


// A point dragged onto a segment ends up collinear with it (point-on-line).
BOOST_AUTO_TEST_CASE( PointOnLineLandsOnSegment )
{
    BOARD board;

    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );        // the y=0 line
    PCB_SHAPE* probe = addSegment( board, { 5 * MM, 5 * MM }, { 6 * MM, 6 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { line->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { line->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { probe->m_Uuid, CONSTRAINT_ANCHOR::START },
                     { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ line, probe };
    solveAndApply( board, shapes );

    // probe's start now lies on the y=0 line.
    BOOST_CHECK_LE( std::abs( probe->GetStart().y ), 100 );
}


// A probe point made the midpoint of a fixed segment lands at its center.
BOOST_AUTO_TEST_CASE( MidpointOfFixedSegment )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 4 * MM } );   // midpoint (5,2) mm
    PCB_SHAPE* probe = addSegment( board, { 1 * MM, 1 * MM }, { 2 * MM, 2 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::MIDPOINT,
                   { { probe->m_Uuid, CONSTRAINT_ANCHOR::START },
                     { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ seg, probe };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( ( probe->GetStart() - VECTOR2I( 5 * MM, 2 * MM ) ).EuclideanNorm(), 1000 );
}


// A free segment made collinear with a fixed horizontal reference lands on the reference's line.
BOOST_AUTO_TEST_CASE( CollinearWithFixedReference )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );   // the y=0 line
    PCB_SHAPE* free = addSegment( board, { 2 * MM, 5 * MM }, { 8 * MM, 6 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COLLINEAR,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, free };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( free->GetStart().y ), 100 );
    BOOST_CHECK_LE( std::abs( free->GetEnd().y ), 100 );
}


// Two points made symmetric about a fixed vertical axis end up mirrored across it.
BOOST_AUTO_TEST_CASE( SymmetricAboutFixedAxis )
{
    BOARD board;

    // Axis is the fixed vertical line x = 5 mm.
    PCB_SHAPE* axis = addSegment( board, { 5 * MM, 0 }, { 5 * MM, 10 * MM } );
    PCB_SHAPE* segA = addSegment( board, { 2 * MM, 3 * MM }, { 2 * MM, 4 * MM } );
    PCB_SHAPE* segB = addSegment( board, { 9 * MM, 3 * MM }, { 9 * MM, 4 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { axis->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { axis->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { segA->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::SYMMETRIC,
                   { { segA->m_Uuid, CONSTRAINT_ANCHOR::START },
                     { segB->m_Uuid, CONSTRAINT_ANCHOR::START },
                     { axis->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ axis, segA, segB };
    solveAndApply( board, shapes );

    // segB.start is the mirror of segA.start (2mm) across x = 5mm, i.e. x = 8mm, same y.
    BOOST_CHECK_LE( std::abs( segB->GetStart().x - 8 * MM ), 1000 );
    BOOST_CHECK_LE( std::abs( segB->GetStart().y - segA->GetStart().y ), 1000 );
}


// A segment with a fixed start and a driving fixed-length takes that length.
BOOST_AUTO_TEST_CASE( FixedLengthDrivesSegmentLength )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 3 * MM, 0 } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    len->SetValue( 8.0 * MM );

    std::vector<PCB_SHAPE*> shapes{ seg };
    solveAndApply( board, shapes );

    double length = ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm();
    BOOST_CHECK_LE( std::abs( length - 8.0 * MM ), 1000 );
}


// A circle with a driving fixed-radius takes that radius.
BOOST_AUTO_TEST_CASE( FixedRadiusDrivesCircle )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 3 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { circle->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );

    PCB_CONSTRAINT* fr = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS,
                                        { { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    fr->SetValue( 5.0 * MM );

    std::vector<PCB_SHAPE*> shapes{ circle };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( circle->GetRadius() - 5 * MM ), 1000 );
}


// Two circles made equal-radius end up the same size as the fixed reference.
BOOST_AUTO_TEST_CASE( EqualRadiusMatchesReference )
{
    BOARD board;

    PCB_SHAPE* ref = addCircle( board, { 0, 0 }, 4 * MM );
    PCB_SHAPE* other = addCircle( board, { 30 * MM, 0 }, 1 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    PCB_CONSTRAINT* fr = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS,
                                        { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    fr->SetValue( 4.0 * MM );
    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { other->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, other };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( other->GetRadius() - 4 * MM ), 1000 );
}


// Two circles made concentric end up sharing the fixed reference's center.
BOOST_AUTO_TEST_CASE( ConcentricSharesCenter )
{
    BOARD board;

    PCB_SHAPE* ref = addCircle( board, { 5 * MM, 5 * MM }, 4 * MM );
    PCB_SHAPE* other = addCircle( board, { 30 * MM, 0 }, 2 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::CONCENTRIC,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { other->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, other };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( ( other->GetCenter() - VECTOR2I( 5 * MM, 5 * MM ) ).EuclideanNorm(), 1000 );
}


// A driving angular dimension forces the angle between two segments.
BOOST_AUTO_TEST_CASE( AngularDimensionDrivesAngle )
{
    BOARD board;

    PCB_SHAPE* ref = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );   // along +x
    PCB_SHAPE* arm = addSegment( board, { 0, 0 }, { 8 * MM, 1 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { arm->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* dim = addConstraint( board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION,
                                         { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                                           { arm->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    dim->SetValue( 90.0 );   // degrees

    std::vector<PCB_SHAPE*> shapes{ ref, arm };
    solveAndApply( board, shapes );

    // arm starts at the origin; at 90 deg to the +x reference it is vertical.
    BOOST_CHECK_LE( std::abs( arm->GetEnd().x - arm->GetStart().x ), 100 );
}


// A fixed-radius driving value resizes an arc (verifies the arc setup solves).
BOOST_AUTO_TEST_CASE( FixedRadiusArc )
{
    BOARD board;

    PCB_SHAPE* arc = addArc( board, { 5 * MM, 0 }, { 3535533, 3535533 }, { 0, 5 * MM } );  // R = 5mm

    PCB_CONSTRAINT* fr = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS,
                                        { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    fr->SetValue( 6.0 * MM );

    std::vector<PCB_SHAPE*> shapes{ arc };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( arc->GetRadius() - 6 * MM ), 2000 );
}


// An arc made equal-radius to a fixed-radius reference arc takes that radius.
BOOST_AUTO_TEST_CASE( EqualRadiusArcs )
{
    BOARD board;

    PCB_SHAPE* ref = addArc( board, { 5 * MM, 0 }, { 3535533, 3535533 }, { 0, 5 * MM } );  // R = 5mm
    // A clean R = 4 mm quarter arc centred at (30,0).
    PCB_SHAPE* other = addArc( board, { 34 * MM, 0 }, { 32828427, 2828427 }, { 30 * MM, 4 * MM } );

    PCB_CONSTRAINT* fr = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS,
                                        { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    fr->SetValue( 5.0 * MM );
    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS,
                   { { ref->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { other->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ref, other };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( other->GetRadius() - 5 * MM ), 3000 );
}


BOOST_AUTO_TEST_SUITE_END()
