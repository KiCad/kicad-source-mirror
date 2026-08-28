/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may search the http://www.gnu.org web site for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <router/pns_line.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

// The walkaround graph carries vertices only, so the arcs have to be pushed back into the
// point runs the walkaround left alone.  https://gitlab.com/kicad/code/kicad/-/issues/25292

namespace
{

// A track running left to right whose middle third is a semicircular arc.  The obstacle in the
// tests below only ever reaches the straight run before it.
SHAPE_LINE_CHAIN buildLineWithTrailingArc()
{
    SHAPE_LINE_CHAIN chain;

    chain.Append( VECTOR2I( 0, 0 ) );
    chain.Append( VECTOR2I( 40000000, 0 ) );
    chain.Append( SHAPE_ARC( VECTOR2I( 40000000, 0 ), VECTOR2I( 50000000, 10000000 ),
                             VECTOR2I( 60000000, 0 ), 0 ) );
    chain.Append( VECTOR2I( 100000000, 0 ) );

    return chain;
}


PNS::LINE makeLine( const SHAPE_LINE_CHAIN& aChain )
{
    PNS::LINE line;

    line.SetWidth( 250000 );
    line.SetShape( aChain );

    return line;
}


// A closed convex hull straddling the straight run, well clear of the arc
SHAPE_LINE_CHAIN buildObstacleHull()
{
    SHAPE_LINE_CHAIN hull;

    hull.Append( VECTOR2I( 10000000, -5000000 ) );
    hull.Append( VECTOR2I( 20000000, -5000000 ) );
    hull.Append( VECTOR2I( 20000000, 5000000 ) );
    hull.Append( VECTOR2I( 10000000, 5000000 ) );
    hull.SetClosed( true );

    return hull;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PnsWalkaroundArcs )


// The arc sits past the obstacle, so it has to survive the walkaround intact
BOOST_AUTO_TEST_CASE( ArcOutsideTheObstacleSurvives )
{
    SHAPE_LINE_CHAIN original = buildLineWithTrailingArc();

    BOOST_REQUIRE_EQUAL( original.ArcCount(), 1 );

    PNS::LINE        line = makeLine( original );
    SHAPE_LINE_CHAIN path;

    BOOST_REQUIRE( line.Walkaround( buildObstacleHull(), path, true ) );

    BOOST_CHECK_MESSAGE( path.ArcCount() == 1,
                         "The walkaround flattened an arc it never touched" );

    // The detour has to leave the far end of the track where it found it
    BOOST_CHECK( path.CPoint( -1 ) == original.CPoint( -1 ) );
}


// A line with no arcs must come back byte-for-byte the same as before the arc restoration
BOOST_AUTO_TEST_CASE( ArclessLineIsUnaffected )
{
    SHAPE_LINE_CHAIN original;

    original.Append( VECTOR2I( 0, 0 ) );
    original.Append( VECTOR2I( 100000000, 0 ) );

    PNS::LINE        line = makeLine( original );
    SHAPE_LINE_CHAIN path;

    BOOST_REQUIRE( line.Walkaround( buildObstacleHull(), path, true ) );

    BOOST_CHECK_EQUAL( path.ArcCount(), 0 );
    BOOST_CHECK( path.CPoint( 0 ) == original.CPoint( 0 ) );
    BOOST_CHECK( path.CPoint( -1 ) == original.CPoint( -1 ) );
}


BOOST_AUTO_TEST_SUITE_END()
