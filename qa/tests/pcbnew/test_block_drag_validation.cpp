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
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <geometry/shape_poly_set.h>
#include <pcb_shape.h>
#include <zone.h>
#include <tools/block_drag_validation.h>


namespace
{

struct BLOCK_DRAG_VALIDATION_FIXTURE
{
    BOARD                board;
    TOOL_MANAGER         mgr;
    KI_TEST::DUMMY_TOOL* tool;

    BLOCK_DRAG_VALIDATION_FIXTURE() :
            tool( new KI_TEST::DUMMY_TOOL() )
    {
        mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
        mgr.RegisterTool( tool );
    }

    ZONE* makeSquareZone()
    {
        ZONE* zone = new ZONE( &board );
        zone->Outline()->NewOutline();
        zone->Outline()->Append( 0, 0 );
        zone->Outline()->Append( 100000, 0 );
        zone->Outline()->Append( 100000, 100000 );
        zone->Outline()->Append( 0, 100000 );
        board.Add( zone );
        return zone;
    }

    /// A square zone with a square hole well inside it.
    ZONE* makeHoledZone()
    {
        ZONE* zone = makeSquareZone();

        zone->Outline()->NewHole( 0 );
        zone->Outline()->Append( 30000, 30000, 0, 0 );
        zone->Outline()->Append( 70000, 30000, 0, 0 );
        zone->Outline()->Append( 70000, 70000, 0, 0 );
        zone->Outline()->Append( 30000, 70000, 0, 0 );
        return zone;
    }

    PCB_SHAPE* makeEdgeSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* segment = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
        segment->SetStart( aStart );
        segment->SetEnd( aEnd );
        segment->SetLayer( Edge_Cuts );
        board.Add( segment );
        return segment;
    }

    PCB_SHAPE* makeClosedOutline()
    {
        PCB_SHAPE* first = makeEdgeSegment( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ) );
        makeEdgeSegment( VECTOR2I( 1000000, 0 ), VECTOR2I( 1000000, 1000000 ) );
        makeEdgeSegment( VECTOR2I( 1000000, 1000000 ), VECTOR2I( 0, 1000000 ) );
        makeEdgeSegment( VECTOR2I( 0, 1000000 ), VECTOR2I( 0, 0 ) );
        return first;
    }
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( BlockDragValidationTests, BLOCK_DRAG_VALIDATION_FIXTURE )


BOOST_AUTO_TEST_CASE( InvalidZoneRestoresStagedGeometry )
{
    ZONE* zone = makeSquareZone();

    std::vector<VECTOR2I> original;

    for( int ii = 0; ii < zone->Outline()->TotalVertices(); ++ii )
        original.push_back( zone->Outline()->CVertex( ii ) );

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    BOARD_COMMIT commit( tool );
    commit.Modify( zone );

    // Swap two corners into a bow tie, which crosses itself
    zone->Outline()->SetVertex( 2, VECTOR2I( 0, 100000 ) );
    zone->Outline()->SetVertex( 3, VECTOR2I( 100000, 100000 ) );

    BOOST_REQUIRE( zone->Outline()->IsSelfIntersecting() );
    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::INVALID_ZONE );

    commit.Revert();

    BOOST_REQUIRE_EQUAL( zone->Outline()->TotalVertices(), (int) original.size() );

    for( int ii = 0; ii < zone->Outline()->TotalVertices(); ++ii )
        BOOST_CHECK( zone->Outline()->CVertex( ii ) == original[ii] );

    BOOST_CHECK( !zone->Outline()->IsSelfIntersecting() );
    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


// A zone with a hole has to count as valid at capture time, or the baseline comparison
// accepts every later state including a broken one
BOOST_AUTO_TEST_CASE( HoledZoneMovePasses )
{
    ZONE* zone = makeHoledZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    zone->Outline()->Move( VECTOR2I( 5000, 5000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


BOOST_AUTO_TEST_CASE( OutlineDraggedAcrossItsHoleRejects )
{
    ZONE* zone = makeHoledZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    // Pull the far corner of the outline into the middle of the hole
    zone->Outline()->Outline( 0 ).SetPoint( 2, VECTOR2I( 40000, 40000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::INVALID_ZONE );
}


BOOST_AUTO_TEST_CASE( HoleDraggedOutOfItsOutlineRejects )
{
    ZONE* zone = makeHoledZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    // Clear of the outline altogether, so nothing crosses and only containment catches it
    zone->Outline()->Hole( 0, 0 ).Move( VECTOR2I( 200000, 0 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::INVALID_ZONE );
}


BOOST_AUTO_TEST_CASE( ValidZoneMovePasses )
{
    ZONE* zone = makeSquareZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    for( int ii = 0; ii < zone->Outline()->TotalVertices(); ++ii )
        zone->Outline()->SetVertex( ii, zone->Outline()->CVertex( ii ) + VECTOR2I( 5000, 5000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


// RemoveNullSegments can leave a zone with two corners. It no longer encloses
// anything, but it does not cross itself either, so it needs its own check
BOOST_AUTO_TEST_CASE( CollapsedOutlineRejects )
{
    ZONE* zone = makeSquareZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    zone->Outline()->Outline( 0 ).Remove( 2, 3 );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::INVALID_ZONE );
}


BOOST_AUTO_TEST_CASE( BrokenOutlineRejectsAndRestores )
{
    PCB_SHAPE* moved = makeClosedOutline();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { moved } );

    BOARD_COMMIT commit( tool );
    commit.Modify( moved );

    moved->SetStart( VECTOR2I( 500000, 500000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::INVALID_OUTLINE );

    commit.Revert();

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


// A drag that starts from an outline that is already open is not refused, or a broken
// outline could never be dragged back into shape. The move has to leave it open as well,
// otherwise the result is OK whether or not the baseline is consulted
BOOST_AUTO_TEST_CASE( PreBrokenOutlineDoesNotReject )
{
    // Three segments only, so the outline is already open before the drag
    makeEdgeSegment( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ) );
    makeEdgeSegment( VECTOR2I( 1000000, 0 ), VECTOR2I( 1000000, 1000000 ) );
    PCB_SHAPE* moved = makeEdgeSegment( VECTOR2I( 1000000, 1000000 ), VECTOR2I( 0, 1000000 ) );

    SHAPE_POLY_SET outlines;
    BOOST_REQUIRE( !board.GetBoardPolygonOutlines( outlines, false ) );

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { moved } );

    moved->SetEnd( VECTOR2I( 400000, 700000 ) );

    BOOST_REQUIRE( !board.GetBoardPolygonOutlines( outlines, false ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


BOOST_AUTO_TEST_CASE( PreInvalidZoneDoesNotReject )
{
    ZONE* zone = new ZONE( &board );
    zone->Outline()->NewOutline();
    zone->Outline()->Append( 0, 0 );
    zone->Outline()->Append( 100000, 0 );
    zone->Outline()->Append( 0, 100000 );
    zone->Outline()->Append( 100000, 100000 );
    board.Add( zone );

    BOOST_REQUIRE( zone->Outline()->IsSelfIntersecting() );

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    for( int ii = 0; ii < zone->Outline()->TotalVertices(); ++ii )
        zone->Outline()->SetVertex( ii, zone->Outline()->CVertex( ii ) + VECTOR2I( 5000, 5000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


// The carried items decide whether the outline is checked at all, so a break in
// geometry the drag never touched does not stop it
BOOST_AUTO_TEST_CASE( UncarriedOutlineDamageDoesNotReject )
{
    PCB_SHAPE* untouched = makeClosedOutline();
    ZONE*      zone = makeSquareZone();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { zone } );

    untouched->SetStart( VECTOR2I( 500000, 500000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


BOOST_AUTO_TEST_CASE( FootprintHolderOutlineIsNotProtected )
{
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    PCB_SHAPE* moved = makeClosedOutline();

    BLOCK_DRAG_VALIDATION validation( board );
    validation.CaptureBaseline( { moved } );

    moved->SetStart( VECTOR2I( 500000, 500000 ) );

    BOOST_CHECK( validation.Validate() == BLOCK_DRAG_VALIDATION_RESULT::OK );
}


BOOST_AUTO_TEST_SUITE_END()
