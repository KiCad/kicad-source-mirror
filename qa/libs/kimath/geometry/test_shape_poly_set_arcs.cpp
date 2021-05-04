/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tuple>
#include <unit_test_utils/unit_test_utils.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include "fixtures_geometry.h"


BOOST_AUTO_TEST_SUITE( CurvedPolys )

/**
 * Simplify the polygon a large number of times and check that the area
 * does not change.
 */
BOOST_AUTO_TEST_CASE( TestSimplify )
{
    KI_TEST::CommonTestData testData;
    SHAPE_POLY_SET testPoly = testData.holeyCurvedPolySingle;

    double originalArea = testData.holeyCurvedPolySingle.Area();

    for( int i = 1; i < 20; i++ )
    {
        BOOST_TEST_CONTEXT( "Simplify Iteration " << i )
        {
            testPoly.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

            BOOST_CHECK_EQUAL( testPoly.Area(), originalArea );
    }
    }
}

BOOST_AUTO_TEST_SUITE_END()
