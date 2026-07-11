/*
 * Phase-0 smoke test for the vendored planegcs solver (KiCad issue #2329).
 *
 * Proves the vendored + shimmed planegcs compiles against system/vendored Eigen
 * and actually solves a small 2D constraint system. Standalone (no KiCad libs)
 * so it doubles as the compile-cost measurement target.
 *
 * Geometry: segment A is a fixed horizontal reference (0,0)-(10,0). Segment B's
 * endpoints are the unknowns, initialized off-constraint at (1,5)-(9,7). We add
 *   Parallel(B, A), P2PDistance(B) = 8, CoordinateX(B.p1)=2, CoordinateY(B.p1)=5
 * which fully determines B as (2,5)-(10,5). We then assert the solver reported
 * Success and that every constraint is actually satisfied in the output coords.
 */

#include <cmath>
#include <cstdio>
#include <vector>

#include <GCS.h>

static int g_failures = 0;

static void check( bool aCond, const char* aWhat )
{
    std::printf( "  [%s] %s\n", aCond ? "PASS" : "FAIL", aWhat );

    if( !aCond )
        g_failures++;
}

int main()
{
    GCS::System sys;

    // Fixed horizontal reference segment A (endpoints not declared as unknowns).
    double ax0 = 0.0, ay0 = 0.0, ax1 = 10.0, ay1 = 0.0;
    GCS::Point a0{ &ax0, &ay0 };
    GCS::Point a1{ &ax1, &ay1 };
    GCS::Line  lineA;
    lineA.p1 = a0;
    lineA.p2 = a1;

    // Segment B: the unknowns, deliberately initialized off every constraint.
    double bx0 = 1.0, by0 = 5.0, bx1 = 9.0, by1 = 7.0;
    GCS::Point b0{ &bx0, &by0 };
    GCS::Point b1{ &bx1, &by1 };
    GCS::Line  lineB;
    lineB.p1 = b0;
    lineB.p2 = b1;

    // Driving parameter values (constants, NOT unknowns).
    double length = 8.0;
    double pinX = 2.0;
    double pinY = 5.0;

    int tag = 1;
    sys.addConstraintParallel( lineB, lineA, tag++ );
    sys.addConstraintP2PDistance( b0, b1, &length, tag++ );
    sys.addConstraintCoordinateX( b0, &pinX, tag++ );
    sys.addConstraintCoordinateY( b0, &pinY, tag++ );

    GCS::VEC_pD unknowns{ &bx0, &by0, &bx1, &by1 };
    sys.declareUnknowns( unknowns );
    sys.initSolution();

    int ret = sys.solve();
    sys.applySolution();

    std::printf( "solve() returned %d (0=Success), dof=%d\n", ret, sys.dofsNumber() );
    std::printf( "B solved: (%.4f, %.4f) -> (%.4f, %.4f)\n", bx0, by0, bx1, by1 );

    check( ret == GCS::Success, "solver reported Success" );

    // Parallel to a horizontal reference => B is horizontal.
    check( std::fabs( by1 - by0 ) < 1e-6, "Parallel: B endpoints share Y" );

    // Distance constraint satisfied.
    double dx = bx1 - bx0, dy = by1 - by0;
    double dist = std::sqrt( dx * dx + dy * dy );
    check( std::fabs( dist - length ) < 1e-6, "P2PDistance: |B| == 8" );

    // Coordinate pins satisfied.
    check( std::fabs( bx0 - pinX ) < 1e-6 && std::fabs( by0 - pinY ) < 1e-6,
           "CoordinateX/Y: B.p1 pinned at (2,5)" );

    // Fully determined => no remaining degrees of freedom.
    check( sys.dofsNumber() == 0, "system has 0 DOF (well-constrained)" );

    std::printf( g_failures == 0 ? "\nSMOKE TEST PASSED\n" : "\nSMOKE TEST FAILED (%d)\n",
                 g_failures );
    return g_failures == 0 ? 0 : 1;
}
