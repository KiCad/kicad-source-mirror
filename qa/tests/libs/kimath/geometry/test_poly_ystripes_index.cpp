/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/poly_ystripes_index.h>

#include <cstdlib>
#include <random>
#include <vector>

namespace
{

SHAPE_POLY_SET makeSquare( int aSize )
{
    SHAPE_POLY_SET   poly;
    SHAPE_LINE_CHAIN outline;

    outline.Append( 0, 0 );
    outline.Append( aSize, 0 );
    outline.Append( aSize, aSize );
    outline.Append( 0, aSize );
    outline.SetClosed( true );
    poly.AddOutline( outline );

    return poly;
}


std::vector<VECTOR2I> generateRandomPoints( const BOX2I& aBBox, int aCount, uint32_t aSeed )
{
    std::mt19937          gen( aSeed );
    std::vector<VECTOR2I> points;
    points.reserve( aCount );

    std::uniform_int_distribution<int> distX( aBBox.GetLeft(), aBBox.GetRight() );
    std::uniform_int_distribution<int> distY( aBBox.GetTop(), aBBox.GetBottom() );

    for( int i = 0; i < aCount; i++ )
        points.emplace_back( distX( gen ), distY( gen ) );

    return points;
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( PolyYStripesIndex )


BOOST_AUTO_TEST_CASE( CorrectnessAllStrategiesAgree )
{
    // An axis-aligned square is the one subject whose containment is decidable without consulting
    // any implementation under test, so the sweep can assert truth rather than consensus.
    constexpr int SIZE = 1000000;
    constexpr int MARGIN = 1000;

    SHAPE_POLY_SET square = makeSquare( SIZE );

    POLY_YSTRIPES_INDEX ystripes;
    ystripes.Build( square );

    BOX2I sampleArea( VECTOR2I( -SIZE / 2, -SIZE / 2 ), VECTOR2I( 2 * SIZE, 2 * SIZE ) );

    int tested = 0;
    int inside = 0;

    auto nearEdge = []( int v, int lo, int hi )
                    {
                        return std::abs( v - lo ) <= MARGIN || std::abs( v - hi ) <= MARGIN;
                    };

    for( const VECTOR2I& pt : generateRandomPoints( sampleArea, 10000, 42 ) )
    {
        // Points hugging an edge are genuinely ambiguous; they are not what this case is about
        if( nearEdge( pt.x, 0, SIZE ) || nearEdge( pt.y, 0, SIZE ) )
            continue;

        const bool expected = pt.x > 0 && pt.x < SIZE && pt.y > 0 && pt.y < SIZE;

        tested++;
        inside += expected ? 1 : 0;

        BOOST_TEST_CONTEXT( "point (" << pt.x << ", " << pt.y << ")" )
        {
            BOOST_CHECK_EQUAL( square.Contains( pt ), expected );
            BOOST_CHECK_EQUAL( ystripes.Contains( pt ), expected );
        }
    }

    // A sweep that sampled only one side of the boundary would prove nothing
    BOOST_CHECK_GT( tested, 5000 );
    BOOST_CHECK_GT( inside, 0 );
    BOOST_CHECK_LT( inside, tested );
}


BOOST_AUTO_TEST_CASE( CorrectnessEdgeCases )
{
    SHAPE_POLY_SET square = makeSquare( 1000000 );

    POLY_YSTRIPES_INDEX ystripes;
    ystripes.Build( square );

    struct TEST_POINT
    {
        VECTOR2I    pt;
        bool        expectedInside;
        std::string desc;
    };

    const std::vector<TEST_POINT> cases = {
        { { 500000, 500000 },   true,  "center" },
        { { 100000, 100000 },   true,  "inside near corner" },
        { { 900000, 900000 },   true,  "inside far corner" },
        { { -100000, 500000 },  false, "outside left" },
        { { 500000, -100000 },  false, "outside above" },
        { { 1500000, 500000 },  false, "outside right" },
        { { 500000, 1500000 },  false, "outside below" },
        { { 500000, -1000000 }, false, "outside Y range above" },
        { { 500000, 3000000 },  false, "outside Y range below" },
    };

    for( const TEST_POINT& tc : cases )
    {
        BOOST_TEST_CONTEXT( tc.desc )
        {
            BOOST_CHECK_EQUAL( square.Contains( tc.pt ), tc.expectedInside );
            BOOST_CHECK_EQUAL( ystripes.Contains( tc.pt ), tc.expectedInside );
        }
    }
}


BOOST_AUTO_TEST_CASE( CorrectnessPolygonWithHoles )
{
    SHAPE_POLY_SET   poly;
    SHAPE_LINE_CHAIN outline;

    outline.Append( 0, 0 );
    outline.Append( 1000, 0 );
    outline.Append( 1000, 1000 );
    outline.Append( 0, 1000 );
    outline.SetClosed( true );
    poly.AddOutline( outline );

    SHAPE_LINE_CHAIN hole;
    hole.Append( 400, 400 );
    hole.Append( 600, 400 );
    hole.Append( 600, 600 );
    hole.Append( 400, 600 );
    hole.SetClosed( true );
    poly.AddHole( hole );

    POLY_YSTRIPES_INDEX ystripes;
    ystripes.Build( poly );

    const std::vector<std::pair<VECTOR2I, bool>> cases = {
        { { 100, 100 },  true },
        { { 500, 500 },  false },
        { { 1500, 500 }, false },
        { { 800, 200 },  true },
    };

    for( const auto& [pt, expected] : cases )
    {
        BOOST_TEST_CONTEXT( "point (" << pt.x << ", " << pt.y << ")" )
        {
            BOOST_CHECK_EQUAL( ystripes.Contains( pt ), expected );
            BOOST_CHECK_EQUAL( poly.Contains( pt ), expected );
        }
    }

    // The proximity fallback must skip hole edges.  Probe a vertical one; Build() drops
    // horizontal edges, so a point near the hole's top would never reach that branch
    BOOST_CHECK( !ystripes.Contains( VECTOR2I( 410, 500 ), 20 ) );

    // Same distance from a real outline edge, to prove the probe above is not passing
    // simply because the fallback found nothing
    BOOST_CHECK( ystripes.Contains( VECTOR2I( -10, 500 ), 20 ) );

    BOX2I bbox = poly.BBox();
    int   mismatches = 0;

    for( const VECTOR2I& pt : generateRandomPoints( bbox, 10000, 42 ) )
    {
        if( ystripes.Contains( pt ) != poly.Contains( pt ) )
            mismatches++;
    }

    BOOST_CHECK_EQUAL( mismatches, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
