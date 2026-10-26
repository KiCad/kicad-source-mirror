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
#include <tool/point_editor_behavior.h>
#include <commit.h>
#include <eda_shape.h>
#include <geometry/shape_poly_set.h>


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
    line.SetDragPolicy( std::make_unique<POLYGON_EDGE_DRAG_POLICY>( line, points ) );
    line.SetPosition( VECTOR2I( 20, 60 ) );

    GRID_HELPER grid;
    line.ApplyRelation( grid );

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

    line.SetDragPolicy( std::make_unique<POLYGON_EDGE_DRAG_POLICY>( line, points ) );
    line.SetPosition( VECTOR2I( 30, 30 ) );

    GRID_HELPER grid;
    line.ApplyRelation( grid );

    BOOST_CHECK_EQUAL( line.GetOrigin().GetPosition(), VECTOR2I( 25, 30 ) );
    BOOST_CHECK_EQUAL( line.GetEnd().GetPosition(), VECTOR2I( 35, 30 ) );
}


class POLY_SHAPE_MOCK : public EDA_SHAPE
{
public:
    POLY_SHAPE_MOCK() :
            EDA_SHAPE( SHAPE_T::POLY, 0, FILL_T::NO_FILL ) {};
};


class POLY_EDIT_COMMIT : public COMMIT
{
public:
    void      Push( const wxString&, int ) override {}
    void      Revert() override {}
    void      RevertToCheckpoint( int aCheckpoint ) override {}
    EDA_ITEM* ResolveItem( KIID& aID ) override { return nullptr; }

protected:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override { return aItem; }
    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override { return aItem->Clone(); }
};


BOOST_AUTO_TEST_CASE( CollapsedPolygonKeepsItsCorners )
{
    SHAPE_POLY_SET poly;

    poly.NewOutline();
    poly.Append( VECTOR2I( 20, 30 ) );
    poly.Append( VECTOR2I( 50, 30 ) );
    poly.Append( VECTOR2I( 35, 55 ) );

    for( int ii = 0; ii < poly.TotalVertices(); ++ii )
        poly.SetVertex( ii, VECTOR2I( 35, 55 ) );

    POLY_SHAPE_MOCK shape;
    shape.SetPolyShape( poly );

    EDIT_POINTS                     points( nullptr );
    EDA_POLYGON_POINT_EDIT_BEHAVIOR behavior( shape );
    POLY_EDIT_COMMIT                commit;

    behavior.FinalizeItem( points, commit );

    BOOST_CHECK_MESSAGE( shape.GetPolyShape().TotalVertices() == 3,
                         "finalizing a collapsed polygon stripped every corner" );
}


BOOST_AUTO_TEST_SUITE_END()
