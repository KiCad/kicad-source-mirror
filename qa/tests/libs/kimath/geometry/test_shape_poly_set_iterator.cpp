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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include "fixtures_geometry.h"

/**
* Fixture for the Iterator test suite. It contains an instance of the common data, three polysets with null segments and a vector containing their points.
*/
struct IteratorFixture
{
    // Structure to store the common data.
    struct KI_TEST::CommonTestData common;

    // Polygons to test whether the RemoveNullSegments method works
    SHAPE_POLY_SET lastNullSegmentPolySet;
    SHAPE_POLY_SET firstNullSegmentPolySet;
    SHAPE_POLY_SET insideNullSegmentPolySet;

    // Null segments points
    std::vector<VECTOR2I> nullPoints;

    IteratorFixture()
    {
        nullPoints.emplace_back( 100, 100 );
        nullPoints.emplace_back( 0, 100 );
        nullPoints.emplace_back( 0, 0 );

        // Create a polygon with its last segment null
        SHAPE_LINE_CHAIN polyLine;
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[2] );
        polyLine.Append( nullPoints[2], true );
        polyLine.SetClosed( true );

        lastNullSegmentPolySet.AddOutline( polyLine );

        // Create a polygon with its first segment null
        polyLine.Clear();
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[0], true );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[2] );
        polyLine.SetClosed( true );

        firstNullSegmentPolySet.AddOutline( polyLine );

        // Create a polygon with an inside segment null
        polyLine.Clear();
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[1], true );
        polyLine.Append( nullPoints[2] );
        polyLine.SetClosed( true );

        insideNullSegmentPolySet.AddOutline( polyLine );
    }

    ~IteratorFixture()
    {
    }
};

/**
 * Declares the IteratorFixture as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( PolygonIterator, IteratorFixture )

/**
 * Checks whether the iteration on the vertices of a common polygon is correct.
 */
BOOST_AUTO_TEST_CASE( VertexIterator )
{
    SHAPE_POLY_SET::ITERATOR iterator;
    int                      vertexIndex = 0;

    for( iterator = common.holeyPolySet.IterateWithHoles(); iterator; iterator++ )
    {
        BOOST_CHECK_EQUAL( common.holeyPoints[vertexIndex], *iterator );
        vertexIndex++;
    }
}

/**
 * Checks whether the iteration on the segments of a common polygon is correct.
 */
BOOST_AUTO_TEST_CASE( SegmentIterator )
{
    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;
    int                              segmentIndex = 0;

    for( iterator = common.holeyPolySet.IterateSegmentsWithHoles(); iterator; iterator++ )
    {
        SEG segment = *iterator;

        BOOST_CHECK_EQUAL( common.holeySegments[segmentIndex].A, segment.A );
        BOOST_CHECK_EQUAL( common.holeySegments[segmentIndex].B, segment.B );

        segmentIndex++;
    }
}

/**
 * Checks whether the iteration on the segments of an empty polygon is correct.
 */
BOOST_AUTO_TEST_CASE( EmptyPolygon )
{
    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;

    for( iterator = common.emptyPolySet.IterateSegmentsWithHoles(); iterator; iterator++ )
    {
        BOOST_FAIL( "Empty set is being iterated!" );
    }
}

/**
 * Checks whether the iteration on the segments of a polygon with one vertex is correct.
 */
BOOST_AUTO_TEST_CASE( UniqueVertex )
{
    SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;
    iterator = common.uniqueVertexPolySet.IterateSegmentsWithHoles();

    SEG segment = *iterator;
    BOOST_CHECK_EQUAL( segment.A, common.uniquePoints[0] );
    BOOST_CHECK_EQUAL( segment.B, common.uniquePoints[0] );

    iterator++;

    BOOST_CHECK( !iterator );
}

/**
 * Checks whether the counting of the total number of vertices is correct.
 */
BOOST_AUTO_TEST_CASE( TotalVertices )
{
    BOOST_CHECK_EQUAL( common.emptyPolySet.TotalVertices(), 0 );
    BOOST_CHECK_EQUAL( common.uniqueVertexPolySet.TotalVertices(), 1 );
    BOOST_CHECK_EQUAL( common.solidPolySet.TotalVertices(), 0 );
    BOOST_CHECK_EQUAL( common.holeyPolySet.TotalVertices(), 12 );
}

/**
 * Checks whether the removal of null segments, wherever they are placed, is correct.
 */
BOOST_AUTO_TEST_CASE( RemoveNullSegments )
{
    SHAPE_POLY_SET polygonSets[3] = { lastNullSegmentPolySet, firstNullSegmentPolySet,
        insideNullSegmentPolySet };

    for( SHAPE_POLY_SET polygonSet : polygonSets )
    {
        BOOST_CHECK_EQUAL( polygonSet.TotalVertices(), 4 );
        BOOST_CHECK_EQUAL( polygonSet.RemoveNullSegments(), 1 );
        BOOST_CHECK_EQUAL( polygonSet.TotalVertices(), 3 );

        BOOST_CHECK_EQUAL( polygonSet.CVertex( 0 ), nullPoints[0] );
        BOOST_CHECK_EQUAL( polygonSet.CVertex( 1 ), nullPoints[1] );
        BOOST_CHECK_EQUAL( polygonSet.CVertex( 2 ), nullPoints[2] );
    }
}


BOOST_AUTO_TEST_SUITE_END()
