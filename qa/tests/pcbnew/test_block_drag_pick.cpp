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
#include <pcb_generator.h>
#include <pcb_shape.h>
#include <zone.h>
#include <constraints/pcb_constraint.h>
#include <geometry/shape_line_chain.h>
#include <tool/edit_points.h>
#include <tool/point_editor_behavior.h>
#include <tools/block_drag_capability.h>
#include <tools/block_drag_pick.h>


namespace
{

// A polygon generator with nothing behind it, enough for the capability and pick rules
class TEST_POLY_GENERATOR : public PCB_GENERATOR_POLY
{
public:
    TEST_POLY_GENERATOR( BOARD_ITEM* aParent, const wxString& aGeneratorType ) :
            PCB_GENERATOR_POLY( aParent, F_Cu ),
            m_testGeneratorType( aGeneratorType )
    {
    }

    wxString GetGeneratorType() const override { return m_testGeneratorType; }

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override { return true; }
    void EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    void EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}
    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override {}

    wxString GetPluralName() const override { return wxT( "Test Generators" ); }
    wxString GetCommitMessage() const override { return wxT( "Test Generator" ); }

private:
    wxString m_testGeneratorType;
};


std::shared_ptr<EDIT_POINTS> makeCollection( EDA_ITEM* aParent )
{
    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aParent );

    points->AddPoint( VECTOR2I( 0, 0 ) );
    points->AddPoint( VECTOR2I( 100, 0 ) );
    points->AddPoint( VECTOR2I( 100, 100 ) );
    points->AddLine( points->Point( 0 ), points->Point( 1 ) );

    return points;
}

} // namespace


BOOST_AUTO_TEST_SUITE( BlockDragPickTests )


BOOST_AUTO_TEST_CASE( PickableFollowsCapabilityAndHandleKind )
{
    BOARD board;

    ZONE*      zone = new ZONE( &board );
    PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );
    PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ARC );

    board.Add( zone );
    board.Add( poly );
    board.Add( arc );

    std::shared_ptr<EDIT_POINTS> zonePoints = makeCollection( zone );
    std::shared_ptr<EDIT_POINTS> polyPoints = makeCollection( poly );
    std::shared_ptr<EDIT_POINTS> arcPoints = makeCollection( arc );

    // Corners of reshapeable items can be picked
    BOOST_CHECK( BlockDragHandlePickable( *zonePoints, zonePoints->Point( 0 ) ) );
    BOOST_CHECK( BlockDragHandlePickable( *polyPoints, polyPoints->Point( 2 ) ) );

    // Edge handles can be picked on polygons and zones
    BOOST_CHECK( BlockDragHandlePickable( *zonePoints, zonePoints->Line( 0 ) ) );
    BOOST_CHECK( BlockDragHandlePickable( *polyPoints, polyPoints->Line( 0 ) ) );

    // An item that only travels whole offers nothing to pick
    BOOST_CHECK( !BlockDragHandlePickable( *arcPoints, arcPoints->Point( 0 ) ) );
}


// A via stitch is an outline like a zone, so its edges can be picked like a zone's. A
// tuning pattern is carried whole and offers nothing to pick
BOOST_AUTO_TEST_CASE( PolygonGeneratorEdgeIsPickableLikeAZone )
{
    BOARD board;

    TEST_POLY_GENERATOR* stitch = new TEST_POLY_GENERATOR( &board, wxS( "via_stitch" ) );
    TEST_POLY_GENERATOR* tuning = new TEST_POLY_GENERATOR( &board, wxS( "tuning_pattern" ) );
    board.Add( stitch );
    board.Add( tuning );

    std::shared_ptr<EDIT_POINTS> stitchPoints = makeCollection( stitch );
    std::shared_ptr<EDIT_POINTS> tuningPoints = makeCollection( tuning );

    BOOST_REQUIRE( BlockDragCapability( *stitch ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_CHECK( BlockDragHandlePickable( *stitchPoints, stitchPoints->Point( 0 ) ) );
    BOOST_CHECK( BlockDragHandlePickable( *stitchPoints, stitchPoints->Line( 0 ) ) );

    BOOST_REQUIRE( BlockDragCapability( *tuning ) == BLOCK_DRAG_CAPABILITY::RIGID );
    BOOST_CHECK( !BlockDragHandlePickable( *tuningPoints, tuningPoints->Point( 0 ) ) );
}


// An edge handle only means something on an outline that can take a new corner, so it is
// pickable on a zone or a polygon and refused anywhere else. A rectangle's own behaviour
// never offers one, and this is the rule that would refuse it if it did
BOOST_AUTO_TEST_CASE( EdgeHandleIsOnlyPickableOnAnOutline )
{
    BOARD board;

    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    PCB_SHAPE* segment = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    board.Add( rect );
    board.Add( segment );

    std::shared_ptr<EDIT_POINTS> rectPoints = makeCollection( rect );
    std::shared_ptr<EDIT_POINTS> segmentPoints = makeCollection( segment );

    BOOST_REQUIRE( BlockDragCapability( *rect ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_REQUIRE( BlockDragCapability( *segment ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );

    BOOST_CHECK( BlockDragHandlePickable( *rectPoints, rectPoints->Point( 0 ) ) );
    BOOST_CHECK( !BlockDragHandlePickable( *rectPoints, rectPoints->Line( 0 ) ) );
    BOOST_CHECK( !BlockDragHandlePickable( *segmentPoints, segmentPoints->Line( 0 ) ) );
}


// A handle that rebuilds the whole shape reads the shape as the other picked handles have
// already left it, so two rebuilds in one frame compound. This covers the rule alone, with the
// flag set by hand, because the rectangle behaviour that sets it lives inside the point editor
// and cannot be built here. EllipseCentreIsMarkedAndRefused below covers a real behaviour
// setting it.
BOOST_AUTO_TEST_CASE( HandleThatDrivesTheWholeShapeIsNotPickable )
{
    BOARD board;

    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    board.Add( rect );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( rect );

    BOOST_REQUIRE( BlockDragCapability( *rect ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );
    BOOST_REQUIRE( BlockDragHandlePickable( *points, points->Point( 1 ) ) );

    points->Point( 1 ).SetDrivesWholeShape();

    BOOST_CHECK( !BlockDragHandlePickable( *points, points->Point( 1 ) ) );

    // The handles beside it are untouched
    BOOST_CHECK( BlockDragHandlePickable( *points, points->Point( 0 ) ) );
    BOOST_CHECK( BlockDragHandlePickable( *points, points->Point( 2 ) ) );
}


// The ellipse behaviour marks its centre, so a block drag refuses it while the radius handles
// stay pickable. This is the real handle set, not a flag set by hand.
BOOST_AUTO_TEST_CASE( EllipseCentreIsMarkedAndRefused )
{
    BOARD board;

    PCB_SHAPE* ellipse = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( 20000000 );
    ellipse->SetEllipseMinorRadius( 12000000 );
    board.Add( ellipse );

    EDIT_POINTS                     points( ellipse );
    EDA_ELLIPSE_POINT_EDIT_BEHAVIOR behavior( *ellipse );

    behavior.MakePoints( points );

    BOOST_REQUIRE_GE( points.PointsSize(), 3u );
    BOOST_REQUIRE( BlockDragCapability( *ellipse ) == BLOCK_DRAG_CAPABILITY::DEFORMABLE );

    // MakePoints() adds the centre, then the major end, then the minor end
    const EDIT_POINT& centre = points.Point( 0 );
    const EDIT_POINT& majorEnd = points.Point( 1 );
    const EDIT_POINT& minorEnd = points.Point( 2 );

    BOOST_CHECK( centre.DrivesWholeShape() );
    BOOST_CHECK( !majorEnd.DrivesWholeShape() );
    BOOST_CHECK( !minorEnd.DrivesWholeShape() );

    BOOST_CHECK( !BlockDragHandlePickable( points, centre ) );
    BOOST_CHECK( BlockDragHandlePickable( points, majorEnd ) );
    BOOST_CHECK( BlockDragHandlePickable( points, minorEnd ) );
}


BOOST_AUTO_TEST_CASE( ConstrainedParentIsPickable )
{
    BOARD board;

    PCB_SHAPE* segment = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    board.Add( segment );

    PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    constraint->AddMember( segment->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( constraint );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( segment );

    BOOST_CHECK( BlockDragHandlePickable( *points, points->Point( 0 ) ) );
}


BOOST_AUTO_TEST_CASE( PlainPickReplacesSet )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    std::optional<PCB_HANDLE_KEY> key1 = PCB_HANDLE_KEY::Make( *points, &points->Point( 1 ) );
    std::optional<PCB_HANDLE_KEY> key2 = PCB_HANDLE_KEY::Make( *points, &points->Point( 2 ) );

    BOOST_REQUIRE( key0 && key1 && key2 );

    std::set<PCB_HANDLE_KEY> picked{ *key0, *key1 };

    BlockDragApplyPick( picked, *key2, false );

    BOOST_REQUIRE_EQUAL( picked.size(), 1 );
    BOOST_CHECK( picked.count( *key2 ) == 1 );

    // A plain click on an already picked handle leaves just that handle
    picked = { *key0, *key1 };
    BlockDragApplyPick( picked, *key1, false );

    BOOST_REQUIRE_EQUAL( picked.size(), 1 );
    BOOST_CHECK( picked.count( *key1 ) == 1 );
}


BOOST_AUTO_TEST_CASE( AccumulatingPickToggles )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    std::optional<PCB_HANDLE_KEY> key1 = PCB_HANDLE_KEY::Make( *points, &points->Point( 1 ) );

    BOOST_REQUIRE( key0 && key1 );

    std::set<PCB_HANDLE_KEY> picked;

    BlockDragApplyPick( picked, *key0, true );
    BlockDragApplyPick( picked, *key1, true );

    BOOST_REQUIRE_EQUAL( picked.size(), 2 );

    // Toggling one off leaves the rest alone
    BlockDragApplyPick( picked, *key0, true );

    BOOST_REQUIRE_EQUAL( picked.size(), 1 );
    BOOST_CHECK( picked.count( *key1 ) == 1 );
}


BOOST_AUTO_TEST_CASE( LassoPickReplacesWithEnclosed )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    std::optional<PCB_HANDLE_KEY> key1 = PCB_HANDLE_KEY::Make( *points, &points->Point( 1 ) );
    std::optional<PCB_HANDLE_KEY> key2 = PCB_HANDLE_KEY::Make( *points, &points->Point( 2 ) );

    BOOST_REQUIRE( key0 && key1 && key2 );

    std::set<PCB_HANDLE_KEY> picked{ *key2 };

    // All three points sit inside this triangle's bounding box, so only a real polygon
    // test can leave point 2 at (100,100) out. The hypotenuse runs along x + y = 180,
    // and point 2 sums to 200 while points 0 and 1 sum to 0 and 100
    SHAPE_LINE_CHAIN triangle( { VECTOR2I( -20, -20 ), VECTOR2I( 200, -20 ), VECTOR2I( -20, 200 ) }, true );

    BlockDragLassoPick( { points.get() }, triangle, false, picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 2 );
    BOOST_CHECK( picked.count( *key0 ) == 1 );
    BOOST_CHECK( picked.count( *key1 ) == 1 );
    BOOST_CHECK( picked.count( *key2 ) == 0 );
}


BOOST_AUTO_TEST_CASE( LassoPickAccumulateAdds )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    std::optional<PCB_HANDLE_KEY> key2 = PCB_HANDLE_KEY::Make( *points, &points->Point( 2 ) );

    BOOST_REQUIRE( key0 && key2 );

    std::set<PCB_HANDLE_KEY> picked{ *key2 };

    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( -10, -10 ), VECTOR2I( 10, 10 ) ), true, picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 2 );
    BOOST_CHECK( picked.count( *key0 ) == 1 );
    BOOST_CHECK( picked.count( *key2 ) == 1 );

    // Lassoing an already picked handle again leaves the set as it was
    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( -10, -10 ), VECTOR2I( 10, 10 ) ), true, picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 2 );
}


BOOST_AUTO_TEST_CASE( LassoPickSkipsNonPickable )
{
    BOARD board;

    ZONE*      zone = new ZONE( &board );
    PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ARC );

    board.Add( zone );
    board.Add( arc );

    std::shared_ptr<EDIT_POINTS> zonePoints = makeCollection( zone );
    std::shared_ptr<EDIT_POINTS> arcPoints = makeCollection( arc );

    std::set<PCB_HANDLE_KEY> picked;

    BlockDragLassoPick( { zonePoints.get(), arcPoints.get() },
                        BlockDragPickRect( VECTOR2I( -10, -10 ), VECTOR2I( 200, 200 ) ), false, picked );

    // All three zone corners, and nothing from the arc, which only travels whole
    BOOST_REQUIRE_EQUAL( picked.size(), 3 );

    for( const PCB_HANDLE_KEY& key : picked )
        BOOST_CHECK( key.Parent() == zone->m_Uuid );
}


BOOST_AUTO_TEST_CASE( BoundaryHandleCountsAsEnclosed )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    std::optional<PCB_HANDLE_KEY> key1 = PCB_HANDLE_KEY::Make( *points, &points->Point( 1 ) );

    BOOST_REQUIRE( key0 && key1 );

    std::set<PCB_HANDLE_KEY> picked;

    // The lasso's right edge runs right through point 1 at ( 100, 0 )
    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( -10, -10 ), VECTOR2I( 100, 50 ) ), false,
                        picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 2 );
    BOOST_CHECK( picked.count( *key0 ) == 1 );
    BOOST_CHECK( picked.count( *key1 ) == 1 );
}


// A lasso picks corners only. One that surrounds a zone edge's midpoint handle but
// neither of its corners picks nothing, because an edge follows its corners
BOOST_AUTO_TEST_CASE( EnclosedLineMidpointPicksNothing )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::set<PCB_HANDLE_KEY> picked;

    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( 40, -10 ), VECTOR2I( 60, 10 ) ), false, picked );

    BOOST_CHECK( picked.empty() );
}


BOOST_AUTO_TEST_CASE( EmptyPlainLassoClearsSet )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );

    BOOST_REQUIRE( key0 );

    std::set<PCB_HANDLE_KEY> picked{ *key0 };

    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( 500, 500 ), VECTOR2I( 600, 600 ) ), false,
                        picked );

    BOOST_CHECK( picked.empty() );

    // The same empty lasso with Shift leaves the set alone
    picked = { *key0 };
    BlockDragLassoPick( { points.get() }, BlockDragPickRect( VECTOR2I( 500, 500 ), VECTOR2I( 600, 600 ) ), true,
                        picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 1 );
    BOOST_CHECK( picked.count( *key0 ) == 1 );
}


// A chain with fewer than three points surrounds nothing, and a plain lasso with
// one still clears the picked set
BOOST_AUTO_TEST_CASE( DegenerateLassoPicksNothing )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    board.Add( zone );

    std::shared_ptr<EDIT_POINTS> points = makeCollection( zone );

    std::optional<PCB_HANDLE_KEY> key0 = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );

    BOOST_REQUIRE( key0 );

    std::set<PCB_HANDLE_KEY> picked{ *key0 };

    SHAPE_LINE_CHAIN twoPoints( { VECTOR2I( -10, -10 ), VECTOR2I( 200, 200 ) }, true );

    BlockDragLassoPick( { points.get() }, twoPoints, false, picked );

    BOOST_CHECK( picked.empty() );

    picked = { *key0 };
    BlockDragLassoPick( { points.get() }, twoPoints, true, picked );

    BOOST_REQUIRE_EQUAL( picked.size(), 1 );
}


BOOST_AUTO_TEST_SUITE_END()
