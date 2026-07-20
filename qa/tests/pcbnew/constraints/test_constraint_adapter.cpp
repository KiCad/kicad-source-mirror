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
#include <constraints/constraint_builder.h>

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

    // Two segments share a corner A end coincident with B start
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


// Lone segment with one direction constraint is under constrained
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


// Duplicated constraint reported redundant and attributed by uuid
// True conflicts need distance and length constraints arriving in step 6 so directional subset here is only degenerate
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


// Fixed reference plus parallel fixes direction of free segment
// Drag pins remaining freedom reaching zero free DOF
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


// A point-on-line constraint with a circle target lands the point on the circumference.
BOOST_AUTO_TEST_CASE( PointOnLineLandsOnCircle )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 10 * MM, 10 * MM }, 5 * MM );
    PCB_SHAPE* probe = addSegment( board, { 0, 0 }, { 2 * MM, 1 * MM } );

    circle->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { probe->m_Uuid, CONSTRAINT_ANCHOR::END }, { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ circle, probe };
    solveAndApply( board, shapes );

    double distToCenter = ( probe->GetEnd() - circle->GetCenter() ).EuclideanNorm();

    BOOST_CHECK_LE( std::abs( distToCenter - 5.0 * MM ), 5000.0 );
    BOOST_CHECK_EQUAL( circle->GetCenter(), VECTOR2I( 10 * MM, 10 * MM ) );
    BOOST_CHECK_EQUAL( circle->GetRadius(), 5 * MM );
}


// A point-on-line constraint with an ellipse target lands the point on the outline.
BOOST_AUTO_TEST_CASE( PointOnLineLandsOnEllipse )
{
    BOARD board;

    PCB_SHAPE* ellipse = addEllipse( board, { 10 * MM, 10 * MM }, 8 * MM, 4 * MM, EDA_ANGLE( 30.0, DEGREES_T ) );
    PCB_SHAPE* probe = addSegment( board, { 0, 0 }, { 2 * MM, 1 * MM } );

    ellipse->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { probe->m_Uuid, CONSTRAINT_ANCHOR::END }, { ellipse->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ ellipse, probe };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( ellipseEquationAt( ellipse, probe->GetEnd() ) - 1.0 ), 1e-3 );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseCenter(), VECTOR2I( 10 * MM, 10 * MM ) );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMajorRadius(), 8 * MM );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMinorRadius(), 4 * MM );
}


// Same but target is elliptical arc point lands on its underlying ellipse
BOOST_AUTO_TEST_CASE( PointOnLineLandsOnEllipticalArc )
{
    BOARD board;

    PCB_SHAPE* arc = addEllipseArc( board, { 10 * MM, 10 * MM }, 8 * MM, 4 * MM, EDA_ANGLE( 30.0, DEGREES_T ),
                                    EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 120.0, DEGREES_T ) );
    PCB_SHAPE* probe = addSegment( board, { 0, 0 }, { 2 * MM, 1 * MM } );

    arc->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { probe->m_Uuid, CONSTRAINT_ANCHOR::END }, { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ arc, probe };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( std::abs( ellipseEquationAt( arc, probe->GetEnd() ) - 1.0 ), 1e-3 );
}


// A free segment made tangent to a locked circle ends up with the center at exactly one radius
// from its supporting line, on the side the circle started on.
BOOST_AUTO_TEST_CASE( TangentLineToCircle )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 10 * MM, 10 * MM }, 5 * MM );
    PCB_SHAPE* line = addSegment( board, { 0, 0 }, { 20 * MM, 0 } );

    circle->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::TANGENT,
                   { { line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ circle, line };
    solveAndApply( board, shapes );

    VECTOR2D d = VECTOR2D( line->GetEnd() - line->GetStart() );
    VECTOR2D toCenter = VECTOR2D( circle->GetCenter() - line->GetStart() );
    double   dist = std::abs( d.Cross( toCenter ) ) / d.EuclideanNorm();

    BOOST_CHECK_LE( std::abs( dist - 5.0 * MM ), 5000.0 );
    BOOST_CHECK_EQUAL( circle->GetCenter(), VECTOR2I( 10 * MM, 10 * MM ) );
}


// A free circle made tangent to a locked circle moves until the two touch externally.
BOOST_AUTO_TEST_CASE( TangentCircleToCircle )
{
    BOARD board;

    PCB_SHAPE* fixed = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* free = addCircle( board, { 20 * MM, 0 }, 3 * MM );

    fixed->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::TANGENT,
                   { { fixed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { free->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ fixed, free };
    solveAndApply( board, shapes );

    // The free circle's radius is also a solver variable, so assert the tangency itself.
    double centerDist = ( free->GetCenter() - fixed->GetCenter() ).EuclideanNorm();
    double radiusSum = 5.0 * MM + free->GetRadius();

    BOOST_CHECK_LE( std::abs( centerDist - radiusSum ), 5000.0 );
    BOOST_CHECK_GT( free->GetRadius(), 0 );
    BOOST_CHECK_EQUAL( fixed->GetCenter(), VECTOR2I( 0, 0 ) );
}


// Resizing one of two tangent circles re-solves the other so they still touch. The neighbor moves,
// the resized circle stays put. This is the radius-edit path that a plain re-solve would miss.
BOOST_AUTO_TEST_CASE( ResizeTangentCircleMovesNeighbor )
{
    BOARD board;

    PCB_SHAPE* resized = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* neighbor = addCircle( board, { 8 * MM, 0 }, 3 * MM ); // externally tangent 5 + 3 = 8

    addConstraint( board, PCB_CONSTRAINT_TYPE::TANGENT,
                   { { resized->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { neighbor->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // Grow the first circle, as the radius handle would.
    resized->SetRadius( 7 * MM );

    std::vector<PCB_SHAPE*> modified;
    ReSolveAfterShapeResize( &board, resized, &modified );

    // They still touch, the resized circle kept its new radius and center, and the neighbor moved.
    double centerDist = ( neighbor->GetCenter() - resized->GetCenter() ).EuclideanNorm();

    BOOST_CHECK_LE( std::abs( centerDist - ( 7.0 * MM + neighbor->GetRadius() ) ), 5000.0 );
    BOOST_CHECK_EQUAL( resized->GetRadius(), 7 * MM );
    BOOST_CHECK_EQUAL( resized->GetCenter(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), neighbor ) != modified.end() );

    // The neighbor translates to stay tangent, keeping its own radius, not distorting it.
    BOOST_CHECK_EQUAL( neighbor->GetRadius(), 3 * MM );
    BOOST_CHECK_LE( std::abs( centerDist - 10 * MM ), 5000.0 );
}


// Resizing circle tangent to LOCKED circle
// Locked one cannot move so resized circle keeps new radius and translates to stay tangent
BOOST_AUTO_TEST_CASE( ResizeTangentToLockedTranslates )
{
    BOARD board;

    PCB_SHAPE* locked = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* resized = addCircle( board, { 8 * MM, 0 }, 3 * MM ); // tangent 5 + 3 = 8

    locked->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::TANGENT,
                   { { resized->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { locked->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    resized->SetRadius( 4 * MM ); // grow it, breaking tangency

    std::vector<PCB_SHAPE*> modified;
    ReSolveAfterShapeResize( &board, resized, &modified );

    double centerDist = ( resized->GetCenter() - locked->GetCenter() ).EuclideanNorm();

    // Still tangent 5 + 4 = 9 resized circle kept new radius and locked one held
    BOOST_CHECK_LE( std::abs( centerDist - 9 * MM ), 5000.0 );
    BOOST_CHECK_EQUAL( resized->GetRadius(), 4 * MM );
    BOOST_CHECK_EQUAL( locked->GetCenter(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( locked->GetRadius(), 5 * MM );
}


// Radius hold yields to real radius constraint
// Resizing one equal radius circle resizes other to match
BOOST_AUTO_TEST_CASE( ResizeEqualRadiusStillPropagates )
{
    BOARD board;

    PCB_SHAPE* resized = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* neighbor = addCircle( board, { 20 * MM, 0 }, 5 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS,
                   { { resized->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { neighbor->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    resized->SetRadius( 8 * MM );

    ReSolveAfterShapeResize( &board, resized, nullptr );

    BOOST_CHECK_LE( std::abs( neighbor->GetRadius() - 8 * MM ), 5000 );
}


// Properties edit is authoritative hold edited circle fixed keep typed radius exact resize neighbor to match
// Plain move semantics would leave radius free and solve it back instead
BOOST_AUTO_TEST_CASE( HoldingEditedCircleKeepsTypedRadius )
{
    BOARD board;

    PCB_SHAPE* edited = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* neighbor = addCircle( board, { 20 * MM, 0 }, 5 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS,
                   { { edited->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { neighbor->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // The user typed a new radius on the edited circle through the properties panel.
    edited->SetRadius( 8 * MM );

    ReSolveShapeClustersHoldingEdited( &board, { edited }, nullptr );

    BOOST_CHECK_EQUAL( edited->GetRadius(), 8 * MM );                    // typed value survives exactly
    BOOST_CHECK_LE( std::abs( neighbor->GetRadius() - 8 * MM ), 5000 ); // neighbor adjusted to match
}


// A properties edit of a polygon's geometry is authoritative, so the hold-edited re-solve keeps the
// edited outline exactly, the vertex-bound segment follows, and the unbound vertices hold.
BOOST_AUTO_TEST_CASE( HoldingEditedPolygonResizeMovesBoundNeighbor )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );
    PCB_SHAPE*            seg = addSegment( board, pts[1], { 40 * MM, 0 } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } } );

    // Move the 1-2 edge 3 mm right, as a typed geometry edit would.
    std::vector<VECTOR2I> dragged = pts;
    dragged[1] += VECTOR2I( 3 * MM, 0 );
    dragged[2] += VECTOR2I( 3 * MM, 0 );
    poly->SetPolyPoints( dragged );

    std::vector<PCB_SHAPE*> modified;
    ReSolveShapeClustersHoldingEdited( &board, { poly }, &modified );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    BOOST_CHECK_LE( ( outline.CPoint( 1 ) - dragged[1] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( outline.CPoint( 2 ) - dragged[2] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( seg->GetStart() - dragged[1] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), seg ) != modified.end() );

    for( int i : { 0, 3, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - pts[i] ).EuclideanNorm(), 5000.0 );
}


// A resize solve holds the resized ellipse's minor radius; with the focus offset fixed at Build,
// that pins the whole shape, so the user's new size survives the re-solve undistorted.
BOOST_AUTO_TEST_CASE( ResizeEllipseKeepsShape )
{
    BOARD board;

    PCB_SHAPE* resized = addEllipse( board, { 0, 0 }, 6 * MM, 3 * MM, EDA_ANGLE( 30.0, DEGREES_T ) );
    PCB_SHAPE* neighbor = addCircle( board, { 20 * MM, 0 }, 2 * MM );

    addConstraint( board, PCB_CONSTRAINT_TYPE::CONCENTRIC,
                   { { resized->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { neighbor->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // Shrink the minor radius, as the resize handle would.
    resized->SetEllipseMinorRadius( 2 * MM );

    ReSolveAfterShapeResize( &board, resized, nullptr );

    BOOST_CHECK_LE( std::abs( resized->GetEllipseMajorRadius() - 6 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( resized->GetEllipseMinorRadius() - 2 * MM ), 5000 );
    BOOST_CHECK_LE(
            std::abs( ( resized->GetEllipseRotation() - EDA_ANGLE( 30.0, DEGREES_T ) ).Normalize180().AsDegrees() ),
            0.01 );

    // The circle came to the ellipse's center; the resized shape stayed put.
    BOOST_CHECK_LE( ( resized->GetEllipseCenter() - VECTOR2I( 0, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( neighbor->GetCenter() - resized->GetEllipseCenter() ).EuclideanNorm(), 5000.0 );
}


// Free ellipse concentric with locked circle moves center without distorting
// Focus follows center so major and minor radius and rotation stay preserved
BOOST_AUTO_TEST_CASE( ConcentricEllipseKeepsShape )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 20 * MM, 15 * MM }, 3 * MM );
    PCB_SHAPE* ellipse = addEllipse( board, { 0, 0 }, 8 * MM, 4 * MM, EDA_ANGLE( 30.0, DEGREES_T ) );

    circle->SetLocked( true );

    addConstraint( board, PCB_CONSTRAINT_TYPE::CONCENTRIC,
                   { { ellipse->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> shapes{ circle, ellipse };
    solveAndApply( board, shapes );

    BOOST_CHECK_LE( ( ellipse->GetEllipseCenter() - VECTOR2I( 20 * MM, 15 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( std::abs( ellipse->GetEllipseMajorRadius() - 8 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( ellipse->GetEllipseMinorRadius() - 4 * MM ), 5000 );
    BOOST_CHECK_LE(
            std::abs( ( ellipse->GetEllipseRotation() - EDA_ANGLE( 30.0, DEGREES_T ) ).Normalize180().AsDegrees() ),
            0.01 );
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


// Driving fixed length over two point anchors not a whole segment drives distance between them
// Matches aligned dimension Driving mode binding its own START and END
BOOST_AUTO_TEST_CASE( FixedLengthTwoPointDrivesDistance )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 3 * MM, 0 } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                           { seg->m_Uuid, CONSTRAINT_ANCHOR::END } } );
    len->SetValue( 8.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*> shapes{ seg };
    solveAndApply( board, shapes );

    double length = ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm();
    BOOST_CHECK_LE( std::abs( length - 8.0 * MM ), 1000 );
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 8 * MM, 0 ) ).EuclideanNorm(), 5000 );
}


// A rectangle's corners are aliases over its two stored corners, so a driving fixed length between
// two adjacent corners resizes the rectangle while it stays a rectangle by construction.
BOOST_AUTO_TEST_CASE( RectCornerFixedLengthDrivesWidth )
{
    BOARD board;

    PCB_SHAPE* rect = addRect( board, { 10 * MM, 10 * MM }, { 50 * MM, 40 * MM } );

    // VERTEX 0 and 1 are the top-left and top-right corners, so this drives the width.
    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ rect };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );
    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    int width = std::abs( rect->GetEnd().x - rect->GetStart().x );
    int height = std::abs( rect->GetEnd().y - rect->GetStart().y );

    BOOST_CHECK_LE( std::abs( width - 30 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( height - 30 * MM ), 5000 );

    // The corners kept their min/max relationship, so the rect did not fold through itself.
    BOOST_CHECK_LT( rect->GetStart().x, rect->GetEnd().x );
    BOOST_CHECK_LT( rect->GetStart().y, rect->GetEnd().y );
}


// Same physical rectangle stored with start and end swapped
// VERTEX indices canonicalized from initial geometry so 0 and 3 still name left edge corners
BOOST_AUTO_TEST_CASE( RectSwappedStorageBindsSamePhysicalCorners )
{
    BOARD board;

    // Stored start is the bottom-right corner; the physical rect is (10,10)-(50,40).
    PCB_SHAPE* rect = addRect( board, { 50 * MM, 40 * MM }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } },
                                         20.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ rect };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );
    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    int width = std::abs( rect->GetEnd().x - rect->GetStart().x );
    int height = std::abs( rect->GetEnd().y - rect->GetStart().y );

    BOOST_CHECK_LE( std::abs( height - 20 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( width - 40 * MM ), 5000 );
}


// An out-of-range corner index cannot map; Build still succeeds and the geometry is untouched.
BOOST_AUTO_TEST_CASE( RectInvalidVertexIndexUnmapped )
{
    BOARD board;

    PCB_SHAPE* rect = addRect( board, { 10 * MM, 10 * MM }, { 50 * MM, 40 * MM } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 7 },
                                           { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ rect };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    BOOST_CHECK_EQUAL( rect->GetStart(), VECTOR2I( 10 * MM, 10 * MM ) );
    BOOST_CHECK_EQUAL( rect->GetEnd(), VECTOR2I( 50 * MM, 40 * MM ) );
}


// Shrinking rounded rectangle reclamps stored corner radius
// SetStart and SetEnd alone leave serialized radius out of range so Apply must push it back through clamping setter
BOOST_AUTO_TEST_CASE( RectShrinkReclampsCornerRadius )
{
    BOARD board;

    PCB_SHAPE* rect = addRect( board, { 10 * MM, 10 * MM }, { 50 * MM, 40 * MM } );
    rect->SetCornerRadius( 10 * MM );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         8.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ rect };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    int width = std::abs( rect->GetEnd().x - rect->GetStart().x );

    BOOST_CHECK_LE( std::abs( width - 8 * MM ), 5000 );

    // The radius clamped to half the new shorter side, so the stored value stays coherent.
    BOOST_CHECK_LE( rect->GetCornerRadius(), width / 2 );
    BOOST_CHECK_LE( std::abs( rect->GetCornerRadius() - 4 * MM ), 5000 );
}


// A solve that would squash a rect side below the sub-micron floor is a collapse, not intent, so
// Apply must leave the geometry untouched.
BOOST_AUTO_TEST_CASE( RectCollapseGuardHoldsGeometry )
{
    BOARD board;

    PCB_SHAPE* rect = addRect( board, { 10 * MM, 10 * MM }, { 50 * MM, 40 * MM } );

    // A driving width below the collapse floor (0.5 um) deterministically forces the collapse.
    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         0.0005 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ rect };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    adapter.Solve( true );

    std::vector<PCB_SHAPE*> changed = adapter.Apply();

    BOOST_CHECK( changed.empty() );
    BOOST_CHECK_EQUAL( rect->GetStart(), VECTOR2I( 10 * MM, 10 * MM ) );
    BOOST_CHECK_EQUAL( rect->GetEnd(), VECTOR2I( 50 * MM, 40 * MM ) );
}


// A driving fixed length between two polygon vertices moves only the bound vertices; every other
// vertex keeps its minimal-movement soft pin and stays put.
BOOST_AUTO_TEST_CASE( PolyVertexFixedLengthMovesBoundVerticesOnly )
{
    BOARD board;

    const std::vector<VECTOR2I> points{ { 30 * MM, 20 * MM },
                                        { 39 * MM, 27 * MM },
                                        { 36 * MM, 38 * MM },
                                        { 24 * MM, 38 * MM },
                                        { 21 * MM, 27 * MM } };

    PCB_SHAPE* poly = addPoly( board, points );

    // Vertices 1 and 3 start ~18.6mm apart; drive them to 24mm.
    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 },
                                           { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } },
                                         24.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );
    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );
    BOOST_REQUIRE_EQUAL( outline.PointCount(), 5 );

    double driven = ( outline.CPoint( 3 ) - outline.CPoint( 1 ) ).EuclideanNorm();
    BOOST_CHECK_LE( std::abs( driven - 24.0 * MM ), 5000 );

    // The unbound vertices held their soft pins exactly.
    for( int i : { 0, 2, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - points[i] ).EuclideanNorm(), 1000 );
}


// A stale vertex index (the poly has only five vertices) cannot map; Build still succeeds, the
// constraint lands in UnmappedConstraints() and the geometry is untouched.
BOOST_AUTO_TEST_CASE( PolyStaleVertexIndexUnmapped )
{
    BOARD board;

    const std::vector<VECTOR2I> points{ { 30 * MM, 20 * MM },
                                        { 39 * MM, 27 * MM },
                                        { 36 * MM, 38 * MM },
                                        { 24 * MM, 38 * MM },
                                        { 21 * MM, 27 * MM } };

    PCB_SHAPE* poly = addPoly( board, points );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 9 },
                                           { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    for( int i = 0; i < 5; ++i )
        BOOST_CHECK_EQUAL( outline.CPoint( i ), points[i] );
}


// A poly with a hole is not ingested (write-back rebuilds a single outline, which would destroy the
// hole), so a constraint on it stays unmapped and the geometry is untouched.
BOOST_AUTO_TEST_CASE( PolyWithHoleStaysUnmapped )
{
    BOARD board;

    const std::vector<VECTOR2I> points{ { 10 * MM, 10 * MM },
                                        { 50 * MM, 10 * MM },
                                        { 50 * MM, 40 * MM },
                                        { 10 * MM, 40 * MM } };

    PCB_SHAPE* poly = addPoly( board, points );

    SHAPE_POLY_SET& polySet = poly->GetPolyShape();
    polySet.NewHole( 0 );
    polySet.Append( 20 * MM, 20 * MM, 0, 0 );
    polySet.Append( 30 * MM, 20 * MM, 0, 0 );
    polySet.Append( 30 * MM, 30 * MM, 0, 0 );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    BOOST_CHECK_EQUAL( poly->GetPolyShape().HoleCount( 0 ), 1 );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    for( int i = 0; i < 4; ++i )
        BOOST_CHECK_EQUAL( outline.CPoint( i ), points[i] );
}


// A poly whose outline carries an arc is not ingested (write-back would polygonize the arc and drop
// its metadata), so a constraint on it stays unmapped and the arc survives Solve + Apply.
BOOST_AUTO_TEST_CASE( PolyArcOutlineStaysUnmapped )
{
    BOARD board;

    PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 10 * MM, 10 * MM ) );
    chain.Append( VECTOR2I( 50 * MM, 10 * MM ) );
    chain.Append( SHAPE_ARC( { 50 * MM, 10 * MM }, { 55 * MM, 25 * MM }, { 50 * MM, 40 * MM }, 0 ) );
    chain.Append( VECTOR2I( 10 * MM, 40 * MM ) );
    chain.SetClosed( true );

    poly->GetPolyShape().AddOutline( chain );
    board.Add( poly );

    BOOST_REQUIRE_GT( poly->GetPolyShape().COutline( 0 ).ArcCount(), 0 );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    BOOST_CHECK_GT( outline.ArcCount(), 0 );
    BOOST_CHECK_EQUAL( outline.CPoint( 0 ), VECTOR2I( 10 * MM, 10 * MM ) );
    BOOST_CHECK_EQUAL( outline.CPoint( 1 ), VECTOR2I( 50 * MM, 10 * MM ) );
}


// A poly with a second outline is not ingested (write-back rebuilds a single outline, which would
// destroy the other one), so a constraint on it stays unmapped and both outlines survive.
BOOST_AUTO_TEST_CASE( PolyTwoOutlinesStaysUnmapped )
{
    BOARD board;

    const std::vector<VECTOR2I> points{ { 10 * MM, 10 * MM },
                                        { 50 * MM, 10 * MM },
                                        { 50 * MM, 40 * MM },
                                        { 10 * MM, 40 * MM } };

    PCB_SHAPE* poly = addPoly( board, points );

    SHAPE_POLY_SET& polySet = poly->GetPolyShape();
    polySet.NewOutline();
    polySet.Append( 60 * MM, 10 * MM, 1 );
    polySet.Append( 70 * MM, 10 * MM, 1 );
    polySet.Append( 70 * MM, 20 * MM, 1 );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    BOOST_CHECK_EQUAL( poly->GetPolyShape().OutlineCount(), 2 );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    for( int i = 0; i < 4; ++i )
        BOOST_CHECK_EQUAL( outline.CPoint( i ), points[i] );
}


// A poly whose outline has no vertices contributes no params; a constraint naming it stays unmapped
// and the rest of the cluster still solves.
BOOST_AUTO_TEST_CASE( PolyEmptyOutlineStaysUnmapped )
{
    BOARD board;

    PCB_SHAPE* poly = addPoly( board, {} );
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 3 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 },
                                           { seg->m_Uuid, CONSTRAINT_ANCHOR::START } },
                                         30.0 * MM );
    len->SetDriving( true );

    std::vector<PCB_SHAPE*>      shapes{ poly, seg };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    BOOST_REQUIRE_EQUAL( adapter.UnmappedConstraints().size(), 1 );
    BOOST_CHECK( adapter.UnmappedConstraints().front() == len->m_Uuid );

    BOOST_REQUIRE( adapter.Solve( true ) );
    adapter.Apply();

    BOOST_CHECK_EQUAL( seg->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 3 * MM, 0 ) );
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


// A bezier endpoint coincident with a fixed point maps and snaps onto that point, and each control
// handle rides along with its adjacent endpoint so the curve is translated rather than sheared.
BOOST_AUTO_TEST_CASE( BezierEndpointCoincidentSnaps )
{
    BOARD board;

    PCB_SHAPE* anchor = addSegment( board, { 20 * MM, 5 * MM }, { 25 * MM, 5 * MM } );
    PCB_SHAPE* bezier = addBezier( board, { 0, 0 }, { 3 * MM, 8 * MM }, { 7 * MM, 8 * MM }, { 10 * MM, 0 } );

    const VECTOR2I startBefore = bezier->GetStart();
    const VECTOR2I c1Before = bezier->GetBezierC1();
    const VECTOR2I c2Before = bezier->GetBezierC2();
    const VECTOR2I endBefore = bezier->GetEnd();

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { anchor->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { bezier->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { anchor->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    std::vector<PCB_SHAPE*> shapes{ anchor, bezier };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    // The coincident on the bezier endpoint is a real solver constraint, not silently dropped.
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );

    BOOST_REQUIRE( adapter.Solve() );
    adapter.Apply();

    // The dragged-free endpoint landed on the fixed anchor.
    BOOST_CHECK_LE( ( bezier->GetEnd() - VECTOR2I( 20 * MM, 5 * MM ) ).EuclideanNorm(), 1000 );

    // The far endpoint held; its control handle did not move.
    BOOST_CHECK_EQUAL( bezier->GetStart(), startBefore );
    BOOST_CHECK_EQUAL( bezier->GetBezierC1(), c1Before );

    // The moved endpoint carried its own control handle by the same delta, preserving the curve.
    VECTOR2I endDelta = bezier->GetEnd() - endBefore;
    BOOST_CHECK_EQUAL( bezier->GetBezierC2() - c2Before, endDelta );
}


// Horizontal accepts two point anchors too not just a whole segment so picked points end level
BOOST_AUTO_TEST_CASE( HorizontalAlignsTwoPoints )
{
    BOARD board;

    // Two separate segments whose end anchors start at different heights.
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 5 * MM, 2 * MM } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 15 * MM, 8 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { b->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    std::vector<PCB_SHAPE*>      shapes{ a, b };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );

    // The two-point horizontal is a real solver constraint, not silently dropped.
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );

    BOOST_REQUIRE( adapter.Solve() );
    adapter.Apply();

    BOOST_CHECK_LE( std::abs( a->GetEnd().y - b->GetEnd().y ), 1000 );   // within 1 um
}


// Vertical accepts two point anchors too so picked points end at same x
BOOST_AUTO_TEST_CASE( VerticalAlignsTwoPoints )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 2 * MM, 5 * MM } );
    PCB_SHAPE* b = addSegment( board, { 0, 10 * MM }, { 8 * MM, 15 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::VERTICAL,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { b->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    std::vector<PCB_SHAPE*>      shapes{ a, b };
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_CHECK( adapter.UnmappedConstraints().empty() );

    BOOST_REQUIRE( adapter.Solve() );
    adapter.Apply();

    BOOST_CHECK_LE( std::abs( a->GetEnd().x - b->GetEnd().x ), 1000 );
}


// Dragging a bezier endpoint that is coincident with a segment corner pulls the segment along.
BOOST_AUTO_TEST_CASE( DragBezierEndpointMovesCoincidentNeighbor )
{
    BOARD board;

    PCB_SHAPE* bezier = addBezier( board, { 0, 0 }, { 3 * MM, 8 * MM }, { 7 * MM, 8 * MM }, { 10 * MM, 0 } );
    PCB_SHAPE* seg = addSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { bezier->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { seg->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    CONSTRAINT_MEMBER dragged{ bezier->m_Uuid, CONSTRAINT_ANCHOR::END };
    std::vector<PCB_SHAPE*> modified;

    CONSTRAINT_DIAGNOSIS diag = SolveCluster( &board, dragged, { 13 * MM, 4 * MM }, &modified );

    BOOST_REQUIRE( diag.solved );

    // The bezier end and the segment start moved together and stay coincident.
    BOOST_CHECK_LE( ( bezier->GetEnd() - seg->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( bezier->GetEnd() - VECTOR2I( 13 * MM, 4 * MM ) ).EuclideanNorm(), 1000 );

    // The neighbor segment is reported modified; the dragged bezier is not.
    BOOST_CHECK( std::find( modified.begin(), modified.end(), seg ) != modified.end() );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), bezier ) == modified.end() );
}


// Minimal movement issue 2 resolve moves fewest shapes
// Horizontal on X drops corner so tied start of Y follows but free end of Y stays put
BOOST_AUTO_TEST_CASE( MinimalMovementSettleHoldsSlackFreeEnd )
{
    BOARD board;

    PCB_SHAPE* x = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );        // slightly sloped
    PCB_SHAPE* y = addSegment( board, { 10 * MM, 2 * MM }, { 10 * MM, 12 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { x->m_Uuid, CONSTRAINT_ANCHOR::END }, { y->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    PCB_CONSTRAINT* horizontal = addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                                                { { x->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    const VECTOR2I xEnd0 = x->GetEnd();
    const VECTOR2I yEnd0 = y->GetEnd();

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, horizontal, &modified, []( BOARD_ITEM* ) {} );

    // X turned horizontal, so its end dropped to y = 0.
    BOOST_CHECK_LE( slopeDiffY( x ), 100 );

    // Must move start of Y stayed coincident with moved corner
    BOOST_CHECK_LE( ( y->GetStart() - x->GetEnd() ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( x->GetEnd() != xEnd0 );

    // Slack held free end of Y did not drift so Y grew instead of translating
    BOOST_CHECK_LE( ( y->GetEnd() - yEnd0 ).EuclideanNorm(), 5000.0 );
}


// A driving fixed length grows the edited segment; the solver must extend it along its current
// direction (its free end reaching the minimal spot) rather than rotating it, and the coincident
// neighbour's own free end must not drift.  A plain re-solve rotates the free-direction segment to an
// arbitrary point on the length circle, dragging the neighbour with it.
BOOST_AUTO_TEST_CASE( MinimalMovementSettleExtendsWithoutRotating )
{
    BOARD board;

    PCB_SHAPE* x = addSegment( board, { 0, 0 }, { 4 * MM, 0 } );          // along +x, fixed start
    PCB_SHAPE* y = addSegment( board, { 4 * MM, 0 }, { 4 * MM, 10 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { x->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { x->m_Uuid, CONSTRAINT_ANCHOR::END }, { y->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { x->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 10.0 * MM );

    const VECTOR2I yEnd0 = y->GetEnd();

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, len, &modified, []( BOARD_ITEM* ) {} );

    // Must move X extends straight to length 10 landing near x 10mm y 0 unrotated
    BOOST_CHECK_LE( ( x->GetEnd() - VECTOR2I( 10 * MM, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( y->GetStart() - x->GetEnd() ).EuclideanNorm(), 5000.0 );

    // Slack held free far end of neighbour stayed put
    BOOST_CHECK_LE( ( y->GetEnd() - yEnd0 ).EuclideanNorm(), 5000.0 );
}


// The must-move guarantee on the interactive drag path: dragging a coincident corner pulls the
// hard-tied neighbour end along while its opposite, unconstrained end stays put.
BOOST_AUTO_TEST_CASE( MinimalMovementDragHoldsNeighborFreeEnd )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                     { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    const VECTOR2I bEnd0 = b->GetEnd();

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS diag = SolveCluster( &board, { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                                              { 12 * MM, 3 * MM }, &modified );

    BOOST_REQUIRE( diag.solved );

    // Must move coincident end followed dragged corner to cursor
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( a->GetEnd() - VECTOR2I( 12 * MM, 3 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), b ) != modified.end() );

    // Slack held free far end of neighbour stayed put
    BOOST_CHECK_LE( ( b->GetEnd() - bEnd0 ).EuclideanNorm(), 5000.0 );
}


// Minimal movement issue 2 with multiple shapes edited together A and B translate as a pair only B touches slack neighbour N
// Fewest objects solve must leave N put so threading the full edited set stops B stay pin fighting N pin
BOOST_AUTO_TEST_CASE( MinimalMovementMultiEditedHoldsSlackNeighbor )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 20 * MM, 0 } );
    PCB_SHAPE* n = addSegment( board, { 20 * MM, 0 }, { 30 * MM, 0 } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END }, { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { b->m_Uuid, CONSTRAINT_ANCHOR::END }, { n->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    const VECTOR2I nStart0 = n->GetStart();
    const VECTOR2I nEnd0 = n->GetEnd();

    a->Move( { 0, 5 * MM } );
    b->Move( { 0, 5 * MM } );

    std::vector<PCB_SHAPE*> modified;
    ReSolveShapeClusters( &board, { a, b }, &modified );

    // The slack neighbour is untouched -- both endpoints hold and it is not staged as modified.
    BOOST_CHECK_LE( ( n->GetStart() - nStart0 ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( n->GetEnd() - nEnd0 ).EuclideanNorm(), 100 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), n ) == modified.end() );

    // Both edited shapes keep the geometry the edit gave them at the pinned/seed corner.
    BOOST_CHECK_LE( ( a->GetStart() - VECTOR2I( 0, 5 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( a->GetEnd() - VECTOR2I( 10 * MM, 5 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( b->GetStart() - VECTOR2I( 10 * MM, 5 * MM ) ).EuclideanNorm(), 5000.0 );
}


// The draw-time binding heuristic prefers a single object whose anchors reach both dimension
// endpoints, binding START and END to that one object.
BOOST_AUTO_TEST_CASE( DimensionBindingPrefersSameObject )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    KIID fakeDim;   // a dimension uuid that is not on the board, as during interactive draw
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 10 * MM, 0 ), 1000.0 );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );

    // Both ends bind to the same segment, one to START and one to END.
    BOOST_CHECK( bindings[0].dimAnchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    BOOST_CHECK( bindings[1].dimAnchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ) );
}


// When no single object reaches both endpoints, each end binds to its own nearest anchor, on
// different objects.
BOOST_AUTO_TEST_CASE( DimensionBindingPerEndpointDifferentObjects )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 5 * MM, 5 * MM } );
    PCB_SHAPE* b = addSegment( board, { 20 * MM, 0 }, { 25 * MM, 5 * MM } );

    KIID fakeDim;
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 20 * MM, 0 ), 1000.0 );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( b->m_Uuid, CONSTRAINT_ANCHOR::START ) );
}


// Only one endpoint near an object still binds (partial is fine); the far endpoint is left free.
BOOST_AUTO_TEST_CASE( DimensionBindingPartial )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 5 * MM, 5 * MM } );

    KIID fakeDim;
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 50 * MM, 50 * MM ), 1000.0 );

    BOOST_REQUIRE_EQUAL( bindings.size(), 1 );
    BOOST_CHECK( bindings[0].dimAnchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    (void) a;
}


// Single object with START nearest both dimension ends must still bind both via distinct anchor pair not split END onto closer neighbour
// End near A START at 2mm beats A END at 8mm yet A spans both through START and END so A wins over B
BOOST_AUTO_TEST_CASE( DimensionBindingDistinctPairOverSplit )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 2 * MM, 1 * MM }, { 2 * MM, 20 * MM } );

    KIID fakeDim;
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 2 * MM, 0 ),
                                             12.0 * MM );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );

    // Both ends bind to segment A on distinct anchors; segment B never enters the binding.
    BOOST_CHECK( bindings[0].dimAnchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    BOOST_CHECK( bindings[1].dimAnchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::END ) );
    (void) b;
}


BOOST_AUTO_TEST_SUITE_END()
