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


#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_poly_set.h>
#include <tool/edit_constraints.h>
#include <tool/edit_points.h>
#include <tool/grid_helper.h>
#include <tool/point_editor_behavior.h>


BOOST_AUTO_TEST_SUITE( EditConstraints )


BOOST_AUTO_TEST_CASE( TriangleEdgeDragPassesThroughTheCollapsePoint )
{
    EDIT_POINTS points( nullptr );
    points.AddPoint( VECTOR2I( 0, 0 ) );
    points.AddPoint( VECTOR2I( 40, 0 ) );
    points.AddPoint( VECTOR2I( 20, 30 ) );
    points.AddLine( points.Point( 0 ), points.Point( 1 ) );
    points.AddLine( points.Point( 1 ), points.Point( 2 ) );
    points.AddLine( points.Point( 2 ), points.Point( 0 ) );

    EDIT_LINE& line = points.Line( 0 );
    line.SetConstraint( new EC_CONVERGING( line, points ) );
    line.SetPosition( VECTOR2I( 20, 60 ) );

    GRID_HELPER grid;
    line.ApplyConstraint( grid );

    BOOST_CHECK_EQUAL( line.GetOrigin().GetPosition(), VECTOR2I( 40, 60 ) );
    BOOST_CHECK_EQUAL( line.GetEnd().GetPosition(), VECTOR2I( 0, 60 ) );
}


BOOST_AUTO_TEST_CASE( TriangularHoleEdgeDragKeepsItsCorners )
{
    SHAPE_POLY_SET poly;

    poly.NewOutline();
    poly.Append( VECTOR2I( 0, 0 ) );
    poly.Append( VECTOR2I( 60, 0 ) );
    poly.Append( VECTOR2I( 60, 60 ) );
    poly.Append( VECTOR2I( 0, 60 ) );
    poly.NewHole();
    poly.Append( VECTOR2I( 20, 20 ), 0, 0 );
    poly.Append( VECTOR2I( 40, 20 ), 0, 0 );
    poly.Append( VECTOR2I( 30, 40 ), 0, 0 );

    EDIT_POINTS points( nullptr );
    POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( points, poly );

    EDIT_LINE& line = points.Line( 4 );
    BOOST_REQUIRE_EQUAL( line.GetOrigin().GetPosition(), VECTOR2I( 20, 20 ) );
    BOOST_REQUIRE_EQUAL( line.GetEnd().GetPosition(), VECTOR2I( 40, 20 ) );

    line.SetConstraint( new EC_CONVERGING( line, points ) );
    line.SetPosition( VECTOR2I( 30, 30 ) );

    GRID_HELPER grid;
    line.ApplyConstraint( grid );

    BOOST_CHECK_EQUAL( line.GetOrigin().GetPosition(), VECTOR2I( 25, 30 ) );
    BOOST_CHECK_EQUAL( line.GetEnd().GetPosition(), VECTOR2I( 35, 30 ) );
}


BOOST_AUTO_TEST_SUITE_END()
