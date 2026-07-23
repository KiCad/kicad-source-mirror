/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <tool/edit_points.h>
#include <tool/edit_relations.h>
#include <tool/grid_helper.h>


BOOST_AUTO_TEST_SUITE( EditRelations )


BOOST_AUTO_TEST_CASE( PointRelationsExposeTypedSemantics )
{
    EDIT_POINT constrained( VECTOR2I( 30, 40 ) );
    EDIT_POINT reference( VECTOR2I( 10, 20 ) );

    constrained.SetRelation( EDIT_RELATION::SameX( reference ) );

    BOOST_REQUIRE( constrained.GetRelation() );
    BOOST_CHECK( constrained.GetRelation()->Kind() == EDIT_RELATION_KIND::SAME_X );

    GRID_HELPER grid;
    constrained.ApplyRelation( grid );
    BOOST_CHECK_EQUAL( constrained.GetPosition(), VECTOR2I( 10, 40 ) );
}


BOOST_AUTO_TEST_CASE( PerpendicularLinePreservesDirection )
{
    EDIT_POINT origin( VECTOR2I( 0, 0 ) );
    EDIT_POINT end( VECTOR2I( 20, 0 ) );
    EDIT_LINE  line( origin, end );

    line.SetRelation( EDIT_RELATION::PerpendicularTranslation( line ) );
    line.SetPosition( VECTOR2I( 17, 9 ) );

    GRID_HELPER grid;
    line.ApplyRelation( grid );

    BOOST_CHECK_EQUAL( end.GetPosition() - origin.GetPosition(), VECTOR2I( 20, 0 ) );
    BOOST_CHECK_EQUAL( origin.GetY(), end.GetY() );
    BOOST_CHECK_LE( std::abs( line.GetX() - 10 ), 1 );
}


BOOST_AUTO_TEST_CASE( PolygonFixedLengthPreservesEndpoints )
{
    EDIT_POINTS points( nullptr );
    points.AddPoint( VECTOR2I( 0, 10 ) );
    points.AddPoint( VECTOR2I( 0, 0 ) );
    points.AddPoint( VECTOR2I( 20, 0 ) );
    points.AddPoint( VECTOR2I( 20, 10 ) );
    points.AddLine( points.Point( 0 ), points.Point( 1 ) );
    points.AddLine( points.Point( 1 ), points.Point( 2 ) );
    points.AddLine( points.Point( 2 ), points.Point( 3 ) );
    points.AddLine( points.Point( 3 ), points.Point( 0 ) );

    EDIT_LINE& line = points.Line( 1 );
    line.SetDragPolicy( std::make_unique<POLYGON_EDGE_DRAG_POLICY>( line, points, POLYGON_LINE_MODE::FIXED_LENGTH ) );
    line.SetPosition( VECTOR2I( 10, 5 ) );

    GRID_HELPER grid;
    line.ApplyRelation( grid );

    BOOST_REQUIRE( line.GetDragPolicy() );
    BOOST_CHECK( line.GetDragPolicy()->GetMode() == POLYGON_LINE_MODE::FIXED_LENGTH );
    BOOST_CHECK_EQUAL( line.GetEnd().GetPosition() - line.GetOrigin().GetPosition(), VECTOR2I( 20, 0 ) );
    BOOST_CHECK_EQUAL( line.GetOrigin().GetY(), line.GetEnd().GetY() );
}


BOOST_AUTO_TEST_CASE( PolygonConvergenceClampsAtAdjacentIntersection )
{
    EDIT_POINTS points( nullptr );
    points.AddPoint( VECTOR2I( 10, 10 ) );
    points.AddPoint( VECTOR2I( 0, 0 ) );
    points.AddPoint( VECTOR2I( 20, 0 ) );
    points.AddPoint( VECTOR2I( 10, 10 ) );
    points.AddLine( points.Point( 0 ), points.Point( 1 ) );
    points.AddLine( points.Point( 1 ), points.Point( 2 ) );
    points.AddLine( points.Point( 2 ), points.Point( 3 ) );
    points.AddLine( points.Point( 3 ), points.Point( 0 ) );

    EDIT_LINE& line = points.Line( 1 );
    line.SetDragPolicy( std::make_unique<POLYGON_EDGE_DRAG_POLICY>( line, points ) );
    line.SetPosition( VECTOR2I( 10, 20 ) );

    GRID_HELPER grid;
    line.ApplyRelation( grid );

    BOOST_CHECK_EQUAL( line.GetOrigin().GetPosition(), VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( line.GetEnd().GetPosition(), VECTOR2I( 10, 10 ) );
}


BOOST_AUTO_TEST_SUITE_END()
