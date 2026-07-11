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

/*
 * Build-integration smoke test for the vendored planegcs solver (issue #2329).
 * Proves planegcs links into the pcbnew QA target and solves a small 2D system.
 * The geometry mirrors thirdparty/planegcs/kicad/smoke_solve.cpp.
 */

#include <cmath>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <GCS.h>


BOOST_AUTO_TEST_SUITE( PlanegcsSmokeSolve )


BOOST_AUTO_TEST_CASE( ParallelDistancePinnedSegment )
{
    GCS::System sys;

    // Fixed horizontal reference segment A (endpoints not declared as unknowns).
    double    ax0 = 0.0, ay0 = 0.0, ax1 = 10.0, ay1 = 0.0;
    GCS::Point a0{ &ax0, &ay0 };
    GCS::Point a1{ &ax1, &ay1 };
    GCS::Line  lineA;
    lineA.p1 = a0;
    lineA.p2 = a1;

    // Segment B: the unknowns, deliberately initialized off every constraint.
    double     bx0 = 1.0, by0 = 5.0, bx1 = 9.0, by1 = 7.0;
    GCS::Point b0{ &bx0, &by0 };
    GCS::Point b1{ &bx1, &by1 };
    GCS::Line  lineB;
    lineB.p1 = b0;
    lineB.p2 = b1;

    // Driving parameter values (constants, not unknowns).
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

    BOOST_TEST_INFO( "solve() returned " << ret << " dof=" << sys.dofsNumber() );
    BOOST_CHECK_EQUAL( ret, GCS::Success );

    // Parallel to a horizontal reference means B is horizontal.
    BOOST_CHECK_SMALL( by1 - by0, 1e-6 );

    // Distance constraint satisfied.
    double dx = bx1 - bx0, dy = by1 - by0;
    BOOST_CHECK_SMALL( std::sqrt( dx * dx + dy * dy ) - length, 1e-6 );

    // Coordinate pins satisfied.
    BOOST_CHECK_SMALL( bx0 - pinX, 1e-6 );
    BOOST_CHECK_SMALL( by0 - pinY, 1e-6 );

    // Fully determined, so no remaining degrees of freedom.
    BOOST_CHECK_EQUAL( sys.dofsNumber(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
