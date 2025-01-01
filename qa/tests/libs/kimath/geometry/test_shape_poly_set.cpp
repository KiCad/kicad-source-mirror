/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <geometry/shape_poly_set.h>
#include <trigo.h>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"


BOOST_AUTO_TEST_SUITE( ShapePolySet )

BOOST_AUTO_TEST_CASE( RemoveNullSegments )
{
    SHAPE_POLY_SET base_set;

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );

    int removed = base_set.RemoveNullSegments();

    BOOST_CHECK_EQUAL( removed, 3 );
    BOOST_CHECK_EQUAL( base_set.VertexCount(), 4 );

    base_set.DeletePolygon( 0 );

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );
    base_set.Append( 0, 0, -1, -1, true );

    removed = base_set.RemoveNullSegments();

    BOOST_CHECK_EQUAL( removed, 4 );
    BOOST_CHECK_EQUAL( base_set.VertexCount(), 4 );
}

BOOST_AUTO_TEST_CASE( GetNeighbourIndexes )
{
    SHAPE_POLY_SET base_set;

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );

    // Check we're testing what we think
    BOOST_REQUIRE( base_set.OutlineCount() == 1 );
    BOOST_REQUIRE( base_set.FullPointCount() == 4 );

    int  prev = 0;
    int  next = 0;
    bool ok = false;

    ok = base_set.GetNeighbourIndexes( 0, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 3 );
    BOOST_TEST( next == 1 );

    ok = base_set.GetNeighbourIndexes( 1, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 0 );
    BOOST_TEST( next == 2 );

    ok = base_set.GetNeighbourIndexes( 2, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 1 );
    BOOST_TEST( next == 3 );

    ok = base_set.GetNeighbourIndexes( 3, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 2 );
    BOOST_TEST( next == 0 );

    ok = base_set.GetNeighbourIndexes( 4, &prev, &next );
    BOOST_TEST( !ok );

    ok = base_set.GetNeighbourIndexes( -1, &prev, &next );
    BOOST_TEST( !ok );
}

BOOST_AUTO_TEST_CASE( GetNeighbourIndexes_MultiOutline )
{
    SHAPE_POLY_SET base_set;

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );

    base_set.NewOutline();
    base_set.Append( 20, 0, -1, -1, true );
    base_set.Append( 20, 10, -1, -1, true );
    base_set.Append( 30, 10, -1, -1, true );
    base_set.Append( 30, 0, -1, -1, true );

    // Check we're testing what we think
    BOOST_TEST_REQUIRE( base_set.OutlineCount() == 2 );
    BOOST_TEST_REQUIRE( base_set.FullPointCount() == 8 );

    int  next = 0;
    int  prev = 0;
    bool ok = false;

    // Can we still get outline 0?
    ok = base_set.GetNeighbourIndexes( 0, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 3 );
    BOOST_TEST( next == 1 );

    // End out outline 0
    ok = base_set.GetNeighbourIndexes( 3, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 2 );
    BOOST_TEST( next == 0 );

    // Check outline 1
    ok = base_set.GetNeighbourIndexes( 4, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 7 );
    BOOST_TEST( next == 5 );

    // End out outline 1
    ok = base_set.GetNeighbourIndexes( 7, &prev, &next );
    BOOST_TEST( ok );
    BOOST_TEST( prev == 6 );
    BOOST_TEST( next == 4 );

    // Bad indexes
    ok = base_set.GetNeighbourIndexes( 8, &prev, &next );
    BOOST_TEST( !ok );
    ok = base_set.GetNeighbourIndexes( -1, &prev, &next );
    BOOST_TEST( !ok );
}

BOOST_AUTO_TEST_SUITE_END()
