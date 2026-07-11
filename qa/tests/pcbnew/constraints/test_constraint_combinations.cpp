/*
 * A matrix of constraint combinations: each satisfiable case must diagnose clean, each contradiction
 * must be flagged. Guards the over-constrained detection against regressions across many shapes.
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <core/mirror.h>
#include <geometry/eda_angle.h>
#include <geometry/seg.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;

namespace
{
void author( BOARD& aBoard, PCB_CONSTRAINT_TYPE aType, const std::vector<CONSTRAINT_MEMBER>& aMembers,
             std::optional<double> aValue = std::nullopt )
{
    PCB_CONSTRAINT*         c = addConstraint( aBoard, aType, aMembers, aValue );
    std::vector<PCB_SHAPE*> mod;
    ApplyConstraintImmediately( &aBoard, c, &mod,
                                []( PCB_SHAPE* )
                                {
                                } );
}

CONSTRAINT_MEMBER whole( PCB_SHAPE* s )
{
    return { s->m_Uuid, CONSTRAINT_ANCHOR::WHOLE };
}

// Each scenario runs in this helper's frame, so only one BOARD is on the stack at a time.
void run( const char* aName, bool aExpectConflict, const std::function<void( BOARD& )>& aBuild )
{
    BOARD board;
    aBuild( board );

    BOARD_CONSTRAINT_DIAGNOSTICS d = DiagnoseBoardConstraints( &board );

    BOOST_CHECK_MESSAGE( ( !d.conflicting.empty() ) == aExpectConflict,
                         aName << ": expected " << ( aExpectConflict ? "conflict" : "clean" )
                               << " but conflicting=" << d.conflicting.size() );
}
} // namespace

BOOST_AUTO_TEST_SUITE( ConstraintCombinations )

BOOST_AUTO_TEST_CASE( Sweep )
{
    using T = PCB_CONSTRAINT_TYPE;

    // ---- Satisfiable (expect CLEAN) ----
    run( "parallel(2 seg)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 5 * MM }, { 8 * MM, 3 * MM } );
             author( b, T::PARALLEL, { whole( s1 ), whole( s2 ) } );
         } );
    run( "perpendicular(2 seg)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 5 * MM }, { 8 * MM, 3 * MM } );
             author( b, T::PERPENDICULAR, { whole( s1 ), whole( s2 ) } );
         } );
    run( "horizontal(1 seg)", false,
         []( BOARD& b )
         {
             auto* s = addSegment( b, { 0, 0 }, { 10 * MM, 2 * MM } );
             author( b, T::HORIZONTAL, { whole( s ) } );
         } );
    run( "vertical(1 seg)", false,
         []( BOARD& b )
         {
             auto* s = addSegment( b, { 0, 0 }, { 2 * MM, 10 * MM } );
             author( b, T::VERTICAL, { whole( s ) } );
         } );
    run( "collinear(2 seg)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 12 * MM, 1 * MM }, { 20 * MM, 2 * MM } );
             author( b, T::COLLINEAR, { whole( s1 ), whole( s2 ) } );
         } );
    run( "equal_length(2 seg)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 5 * MM }, { 4 * MM, 5 * MM } );
             author( b, T::EQUAL_LENGTH, { whole( s1 ), whole( s2 ) } );
         } );
    run( "fixed_length(1 seg)", false,
         []( BOARD& b )
         {
             auto* s = addSegment( b, { 0, 0 }, { 4 * MM, 0 } );
             author( b, T::FIXED_LENGTH, { whole( s ) }, 10.0 * MM );
         } );
    run( "angular(2 seg)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 0 }, { 0, 10 * MM } );
             author( b, T::ANGULAR_DIMENSION, { whole( s1 ), whole( s2 ) }, 90.0 );
         } );
    run( "concentric(2 circ)", false,
         []( BOARD& b )
         {
             auto* c1 = addCircle( b, { 0, 0 }, 5 * MM );
             auto* c2 = addCircle( b, { 2 * MM, 1 * MM }, 3 * MM );
             author( b, T::CONCENTRIC, { whole( c1 ), whole( c2 ) } );
         } );
    run( "equal_radius(2 circ)", false,
         []( BOARD& b )
         {
             auto* c1 = addCircle( b, { 0, 0 }, 5 * MM );
             auto* c2 = addCircle( b, { 20 * MM, 0 }, 3 * MM );
             author( b, T::EQUAL_RADIUS, { whole( c1 ), whole( c2 ) } );
         } );
    run( "fixed_radius(1 circ)", false,
         []( BOARD& b )
         {
             auto* c = addCircle( b, { 0, 0 }, 5 * MM );
             author( b, T::FIXED_RADIUS, { whole( c ) }, 8.0 * MM );
         } );
    run( "tangent(line,circ)", false,
         []( BOARD& b )
         {
             auto* s = addSegment( b, { 0, 0 }, { 20 * MM, 0 } );
             auto* c = addCircle( b, { 10 * MM, 8 * MM }, 3 * MM );
             author( b, T::TANGENT, { whole( s ), whole( c ) } );
         } );
    run( "tangent(2 circ)", false,
         []( BOARD& b )
         {
             auto* c1 = addCircle( b, { 0, 0 }, 5 * MM );
             auto* c2 = addCircle( b, { 15 * MM, 0 }, 4 * MM );
             author( b, T::TANGENT, { whole( c1 ), whole( c2 ) } );
         } );

    // ---- Contradictions (expect CONFLICT) ----
    run( "parallel+perpendicular", true,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 5 * MM }, { 8 * MM, 3 * MM } );
             author( b, T::PARALLEL, { whole( s1 ), whole( s2 ) } );
             author( b, T::PERPENDICULAR, { whole( s1 ), whole( s2 ) } );
         } );
    run( "horizontal+vertical", true,
         []( BOARD& b )
         {
             auto* s = addSegment( b, { 0, 0 }, { 10 * MM, 2 * MM } );
             author( b, T::HORIZONTAL, { whole( s ) } );
             author( b, T::VERTICAL, { whole( s ) } );
         } );
    run( "collinear+perpendicular", true,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 12 * MM, 1 * MM }, { 20 * MM, 2 * MM } );
             author( b, T::COLLINEAR, { whole( s1 ), whole( s2 ) } );
             author( b, T::PERPENDICULAR, { whole( s1 ), whole( s2 ) } );
         } );
    run( "equal_radius+two fixed_radius", true,
         []( BOARD& b )
         {
             auto* c1 = addCircle( b, { 0, 0 }, 5 * MM );
             auto* c2 = addCircle( b, { 20 * MM, 0 }, 3 * MM );
             author( b, T::EQUAL_RADIUS, { whole( c1 ), whole( c2 ) } );
             author( b, T::FIXED_RADIUS, { whole( c1 ) }, 5.0 * MM );
             author( b, T::FIXED_RADIUS, { whole( c2 ) }, 9.0 * MM );
         } );

    // ---- Redundant (expect CLEAN, redundant>0) ----
    run( "horiz+horiz+parallel(redundant)", false,
         []( BOARD& b )
         {
             auto* s1 = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* s2 = addSegment( b, { 0, 5 * MM }, { 10 * MM, 5 * MM } );
             author( b, T::HORIZONTAL, { whole( s1 ) } );
             author( b, T::HORIZONTAL, { whole( s2 ) } );
             author( b, T::PARALLEL, { whole( s1 ), whole( s2 ) } );
         } );

    // ---- Locked variants ----
    run( "perpendicular(locked+free)", false,
         []( BOARD& b )
         {
             auto* lk = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* fr = addSegment( b, { 0, 5 * MM }, { 8 * MM, 3 * MM } );
             lk->SetLocked( true );
             author( b, T::PERPENDICULAR, { whole( lk ), whole( fr ) } );
         } );
    run( "parallel+perp(locked+free)", true,
         []( BOARD& b )
         {
             auto* lk = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
             auto* fr = addSegment( b, { 0, 5 * MM }, { 8 * MM, 3 * MM } );
             lk->SetLocked( true );
             author( b, T::PARALLEL, { whole( lk ), whole( fr ) } );
             author( b, T::PERPENDICULAR, { whole( lk ), whole( fr ) } );
         } );
}

// Flipping a shape breaks its constraint, and the transform re-solve must restore it. Here a circle
// tangent to a line is flipped away, then re-solved back to tangent.
BOOST_AUTO_TEST_CASE( FlipRestoresTangent )
{
    BOARD b;
    auto* line = addSegment( b, { 0, 0 }, { 20 * MM, 0 } );
    auto* circ = addCircle( b, { 10 * MM, 5 * MM }, 5 * MM );
    author( b, PCB_CONSTRAINT_TYPE::TANGENT, { whole( line ), whole( circ ) } );

    auto tangentErr = [&]()
    {
        SEG seg( line->GetStart(), line->GetEnd() );
        return std::abs( (double) seg.LineDistance( circ->GetCenter() ) - circ->GetRadius() );
    };

    BOOST_CHECK_LE( tangentErr(), 5000.0 ); // authored: tangent

    circ->Flip( { 10 * MM, 30 * MM }, FLIP_DIRECTION::TOP_BOTTOM );
    BOOST_CHECK_GT( tangentErr(), 1 * MM ); // flip broke tangency

    std::vector<PCB_SHAPE*> shapes{ circ }, mod;
    ReSolveShapeClusters( &b, shapes, &mod,
                          []( PCB_SHAPE* )
                          {
                          } );

    BOOST_CHECK_LE( tangentErr(), 5000.0 ); // re-solve restored tangency
    BOOST_CHECK( DiagnoseBoardConstraints( &b ).conflicting.empty() );
}

// Rotating a shape breaks its constraint, and the transform re-solve must restore it. Here one of
// two parallel segments is rotated away, then re-solved back to parallel.
BOOST_AUTO_TEST_CASE( RotateRestoresParallel )
{
    BOARD b;
    auto* a = addSegment( b, { 0, 0 }, { 10 * MM, 0 } );
    auto* c = addSegment( b, { 0, 5 * MM }, { 10 * MM, 5 * MM } );
    author( b, PCB_CONSTRAINT_TYPE::PARALLEL, { whole( a ), whole( c ) } );

    // |sin(angle between the two segment directions)|: zero when parallel.
    auto parallelErr = [&]()
    {
        VECTOR2I da = a->GetEnd() - a->GetStart();
        VECTOR2I dc = c->GetEnd() - c->GetStart();
        double   cross = (double) da.x * dc.y - (double) da.y * dc.x;
        return std::abs( cross / ( da.EuclideanNorm() * dc.EuclideanNorm() ) );
    };

    BOOST_CHECK_LE( parallelErr(), 1e-3 ); // authored: parallel

    c->Rotate( c->GetStart(), EDA_ANGLE( 30.0, DEGREES_T ) );
    BOOST_CHECK_GT( parallelErr(), 0.1 ); // rotate broke parallelism

    std::vector<PCB_SHAPE*> shapes{ c }, mod;
    ReSolveShapeClusters( &b, shapes, &mod,
                          []( PCB_SHAPE* )
                          {
                          } );

    // The re-solve pulls the pair back near parallel (approximate, not solver-exact) and the
    // constraint stays satisfiable.
    BOOST_CHECK_LE( parallelErr(), 0.05 );
    BOOST_CHECK( DiagnoseBoardConstraints( &b ).conflicting.empty() );
}

BOOST_AUTO_TEST_SUITE_END()
