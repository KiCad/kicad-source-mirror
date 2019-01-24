/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <unit_test_utils/unit_test_utils.h>

#include <geometry/seg.h>


BOOST_AUTO_TEST_SUITE( Segment )

/**
 * Checks whether the construction of a segment referencing external points works
 * and that the endpoints can be modified as normal points.
 */
BOOST_AUTO_TEST_CASE( EndpointCtorMod )
{
    const VECTOR2I pointA{ 10, 20 };
    const VECTOR2I pointB{ 100, 200 };

    // Build a segment referencing the previous points
    SEG segment( pointA, pointB );

    BOOST_CHECK_EQUAL( pointA, VECTOR2I( 10, 20 ) );
    BOOST_CHECK_EQUAL( pointB, VECTOR2I( 100, 200 ) );

    // Modify the ends of the segments
    segment.A += VECTOR2I( 10, 10 );
    segment.B += VECTOR2I( 100, 100 );

    // Check that the ends in segment are modified
    BOOST_CHECK_EQUAL( segment.A, VECTOR2I( 20, 30 ) );
    BOOST_CHECK_EQUAL( segment.B, VECTOR2I( 200, 300 ) );
}

BOOST_AUTO_TEST_SUITE_END()
