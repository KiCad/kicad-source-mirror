/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_shape.h>
#include <geometry/shape_poly_set.h>
#include <zone.h>
#include <tool/edit_points.h>
#include <tool/grid_helper.h>
#include <tool/point_editor_behavior.h>
#include <tools/block_drag_displacement.h>


namespace
{

std::shared_ptr<EDIT_POINTS> makeTriangle( EDA_ITEM* aParent, int aOffset = 0 )
{
    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aParent );

    points->AddPoint( VECTOR2I( aOffset, 0 ) );
    points->AddPoint( VECTOR2I( aOffset + 1000, 0 ) );
    points->AddPoint( VECTOR2I( aOffset + 1000, 1000 ) );
    points->AddLine( points->Point( 0 ), points->Point( 1 ) );
    points->AddLine( points->Point( 1 ), points->Point( 2 ) );

    return points;
}

} // namespace


BOOST_AUTO_TEST_SUITE( BlockDragDisplacementTests )


// Two items, one drag. Every picked handle moves by the same offset, and no point
// is snapped again against its own item's grid
BOOST_AUTO_TEST_CASE( OneDisplacementReachesEveryPickedHandle )
{
    // These handles have no drag rule, so the grid is here only to be passed along
    GRID_HELPER testGrid;
    BOARD board;

    ZONE*      zone = new ZONE( &board );
    PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );

    board.Add( zone );
    board.Add( poly );

    std::shared_ptr<EDIT_POINTS> zonePoints = makeTriangle( zone );
    std::shared_ptr<EDIT_POINTS> polyPoints = makeTriangle( poly, 5000 );

    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *PCB_HANDLE_KEY::Make( *zonePoints, &zonePoints->Point( 0 ) ) );
    picked.insert( *PCB_HANDLE_KEY::Make( *zonePoints, &zonePoints->Point( 2 ) ) );
    picked.insert( *PCB_HANDLE_KEY::Make( *polyPoints, &polyPoints->Point( 1 ) ) );

    std::vector<EDIT_POINTS*> collections{ zonePoints.get(), polyPoints.get() };

    std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );

    BOOST_REQUIRE_EQUAL( origins.size(), 3 );

    // An offset that lines up with no grid, as if it had been snapped against the
    // grabbed item alone. Every point lands at its start plus exactly this offset.
    const VECTOR2I delta( 333, -77 );

    BOOST_REQUIRE( BlockDragApplyDisplacement( collections, origins, delta, testGrid ) );

    BOOST_CHECK( zonePoints->Point( 0 ).GetPosition() == VECTOR2I( 333, -77 ) );
    BOOST_CHECK( zonePoints->Point( 2 ).GetPosition() == VECTOR2I( 1333, 923 ) );
    BOOST_CHECK( polyPoints->Point( 1 ).GetPosition() == VECTOR2I( 6333, -77 ) );

    BOOST_CHECK( zonePoints->Point( 1 ).GetPosition() == VECTOR2I( 1000, 0 ) );
    BOOST_CHECK( polyPoints->Point( 0 ).GetPosition() == VECTOR2I( 5000, 0 ) );

    // Positions come from the starting points, so a second call with a new offset
    // does not add to the first
    const VECTOR2I delta2( 10, 10 );

    BOOST_REQUIRE( BlockDragApplyDisplacement( collections, origins, delta2, testGrid ) );
    BOOST_CHECK( zonePoints->Point( 0 ).GetPosition() == VECTOR2I( 10, 10 ) );
}


// An odd offset with even endpoints. The midpoint of two integers rounds, so setting the
// edge's own position would shift both of its ends a nanometre off target. The handles
// here are built the way the editor builds them, so every line carries the same drag rule
// a real picked edge has.
BOOST_AUTO_TEST_CASE( PickedEdgeWithItsPolicyMovesPerpendicular )
{
    GRID_HELPER testGrid;
    BOARD       board;
    ZONE*       zone = new ZONE( &board );
    board.Add( zone );

    SHAPE_POLY_SET& outline = *zone->Outline();
    outline.NewOutline();
    outline.Append( 0, 0 );
    outline.Append( 100000, 0 );
    outline.Append( 100000, 100000 );
    outline.Append( 0, 100000 );

    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( zone );
    POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( *points, outline );

    BOOST_REQUIRE( points->LinesSize() >= 1 );
    BOOST_REQUIRE_MESSAGE( points->Line( 0 ).GetDragPolicy() != nullptr, "every edge should come with a drag rule" );

    // The first edge runs between corners 0 and 1, the horizontal bottom edge
    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *PCB_HANDLE_KEY::Make( *points, &points->Line( 0 ) ) );

    std::vector<EDIT_POINTS*>          collections{ points.get() };
    std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );

    // one edge key plus its two corners
    BOOST_REQUIRE_EQUAL( origins.size(), 3 );

    const VECTOR2I delta( 7000, 5000 );

    BOOST_REQUIRE( BlockDragApplyDisplacement( collections, origins, delta, testGrid ) );

    // The drag rule only lets the edge move at right angles to itself, so the part of the
    // offset along the edge is dropped and the two corners keep their x
    BOOST_CHECK_EQUAL( points->Point( 0 ).GetPosition().x, 0 );
    BOOST_CHECK_EQUAL( points->Point( 1 ).GetPosition().x, 100000 );
    BOOST_CHECK_EQUAL( points->Point( 0 ).GetPosition().y, 5000 );
    BOOST_CHECK_EQUAL( points->Point( 1 ).GetPosition().y, 5000 );

    // The corners this edge does not touch stay where they were
    BOOST_CHECK( points->Point( 2 ).GetPosition() == VECTOR2I( 100000, 100000 ) );
    BOOST_CHECK( points->Point( 3 ).GetPosition() == VECTOR2I( 0, 100000 ) );
}


// Only where the drag ends should matter. A picked edge's drag rule is rebuilt from the
// current geometry every frame, so a cursor that wanders off and comes back is the case that
// would show it drifting. The outline is compared, not the handle positions, because the
// handles are written straight from origin plus delta and so cannot drift by construction.
BOOST_AUTO_TEST_CASE( PickedEdgeIsIndependentOfThePathTaken )
{
    GRID_HELPER testGrid;

    auto run = [&]( const std::vector<VECTOR2I>& aPath )
    {
        BOARD board;
        ZONE* zone = new ZONE( &board );
        board.Add( zone );

        SHAPE_POLY_SET& outline = *zone->Outline();
        outline.NewOutline();
        outline.Append( 0, 0 );
        outline.Append( 100000, 0 );
        outline.Append( 100000, 100000 );
        outline.Append( 0, 100000 );

        std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( zone );
        POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( *points, outline );

        std::set<PCB_HANDLE_KEY> picked;
        picked.insert( *PCB_HANDLE_KEY::Make( *points, &points->Line( 0 ) ) );

        std::vector<EDIT_POINTS*>          collections{ points.get() };
        std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );

        // One frame is a displacement followed by the outline write, and the write is what
        // reseeds each unpicked edge's drag rule from the geometry as it now stands. Leaving
        // the write out would test a rule that is never rebuilt, which cannot drift.
        for( const VECTOR2I& step : aPath )
        {
            BlockDragApplyDisplacement( collections, origins, step, testGrid );
            POLYGON_POINT_EDIT_BEHAVIOR::UpdateOutlineFromPoints( outline, points->Line( 0 ), *points );
        }

        std::vector<VECTOR2I> result;

        for( int ii = 0; ii < outline.TotalVertices(); ++ii )
            result.push_back( outline.CVertex( ii ) );

        return result;
    };

    const VECTOR2I target( 7000, 5000 );

    std::vector<VECTOR2I> direct = run( { target } );
    std::vector<VECTOR2I> wandered = run( { VECTOR2I( 30000, -20000 ), VECTOR2I( -5000, 40000 ), target } );

    BOOST_REQUIRE_EQUAL( direct.size(), wandered.size() );

    for( size_t ii = 0; ii < direct.size(); ++ii )
    {
        BOOST_CHECK_MESSAGE( direct[ii] == wandered[ii],
                             "corner " << ii << " landed at " << wandered[ii].x << "," << wandered[ii].y
                                       << " by one path and " << direct[ii].x << "," << direct[ii].y << " by another" );
    }
}


BOOST_AUTO_TEST_CASE( PickedLineLandsExactlyOnAnOddDelta )
{
    // These handles have no drag rule, so the grid is here only to be passed along
    GRID_HELPER testGrid;
    BOARD       board;
    ZONE*       zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeTriangle( zone );

    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *PCB_HANDLE_KEY::Make( *points, &points->Line( 0 ) ) );

    std::vector<EDIT_POINTS*> collections{ points.get() };

    std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );

    const VECTOR2I delta( 501, 500 );

    BOOST_REQUIRE( BlockDragApplyDisplacement( collections, origins, delta, testGrid ) );

    BOOST_CHECK( points->Point( 0 ).GetPosition() == VECTOR2I( 501, 500 ) );
    BOOST_CHECK( points->Point( 1 ).GetPosition() == VECTOR2I( 1501, 500 ) );
}


BOOST_AUTO_TEST_CASE( PickedLineExpandsWithoutDoubleMove )
{
    // These handles have no drag rule, so the grid is here only to be passed along
    GRID_HELPER testGrid;
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeTriangle( zone );

    // Line 0 runs between points 0 and 1, and point 1 is picked on its own too
    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *PCB_HANDLE_KEY::Make( *points, &points->Line( 0 ) ) );
    picked.insert( *PCB_HANDLE_KEY::Make( *points, &points->Point( 1 ) ) );

    std::vector<EDIT_POINTS*> collections{ points.get() };

    std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );

    // Corners 0 and 1, plus the edge itself, which the frame drives through its drag
    // rule once they are placed
    BOOST_REQUIRE_EQUAL( origins.size(), 3 );

    int lineKeys = 0;

    for( const auto& [key, origin] : origins )
        lineKeys += key.IsLine() ? 1 : 0;

    BOOST_CHECK_EQUAL( lineKeys, 1 );

    const VECTOR2I delta( 500, 500 );

    BOOST_REQUIRE( BlockDragApplyDisplacement( collections, origins, delta, testGrid ) );

    BOOST_CHECK( points->Point( 0 ).GetPosition() == VECTOR2I( 500, 500 ) );
    BOOST_CHECK( points->Point( 1 ).GetPosition() == VECTOR2I( 1500, 500 ) );
    BOOST_CHECK( points->Point( 2 ).GetPosition() == VECTOR2I( 1000, 1000 ) );
}


BOOST_AUTO_TEST_CASE( MissingIdentityInvalidatesGesture )
{
    // These handles have no drag rule, so the grid is here only to be passed along
    GRID_HELPER testGrid;
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> before = makeTriangle( zone );

    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *PCB_HANDLE_KEY::Make( *before, &before->Point( 2 ) ) );

    std::vector<EDIT_POINTS*> collections{ before.get() };

    std::map<PCB_HANDLE_KEY, VECTOR2I> origins = BlockDragMoveOrigins( collections, picked );
    BOOST_REQUIRE_EQUAL( origins.size(), 1 );

    // The item loses a corner mid drag, so the collection is rebuilt smaller
    std::shared_ptr<EDIT_POINTS> after = std::make_shared<EDIT_POINTS>( zone );
    after->AddPoint( VECTOR2I( 0, 0 ) );
    after->AddPoint( VECTOR2I( 1000, 0 ) );

    std::vector<EDIT_POINTS*> rebuilt{ after.get() };

    BOOST_CHECK( !BlockDragApplyDisplacement( rebuilt, origins, VECTOR2I( 10, 10 ), testGrid ) );

    // Capture also refuses to start when a picked key has gone stale
    BOOST_CHECK( BlockDragMoveOrigins( rebuilt, picked ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
