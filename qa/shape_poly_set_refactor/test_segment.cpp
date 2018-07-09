/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <geometry/seg.h>

#include <qa/data/fixtures_geometry.h>

/**
 * Declares the IteratorFixture as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( SegmentReference, CommonTestData )

/**
 * Checks whether the construction of a segment referencing external points works.
 */
BOOST_AUTO_TEST_CASE( SegmentReference )
{
    VECTOR2I pointA( 10, 20 );
    VECTOR2I pointB( 100, 200 );

    // Build a segment referencing the previous points
    SEG segment( pointA, pointB );

    BOOST_CHECK_EQUAL( pointA, VECTOR2I( 10, 20) );
    BOOST_CHECK_EQUAL( pointB, VECTOR2I( 100, 200) );

    // Modify the ends of the segments
    segment.A += VECTOR2I( 10, 10 );
    segment.B += VECTOR2I( 100, 100 );

    // Check that the original points are not modified
    BOOST_CHECK_EQUAL( pointA, VECTOR2I( 10, 20) );
    BOOST_CHECK_EQUAL( pointB, VECTOR2I( 100, 200) );

    // Check that the ends in segment are modified
    BOOST_CHECK_EQUAL( segment.A, VECTOR2I( 20, 30) );
    BOOST_CHECK_EQUAL( segment.B, VECTOR2I( 200, 300) );
}

BOOST_AUTO_TEST_SUITE_END()
