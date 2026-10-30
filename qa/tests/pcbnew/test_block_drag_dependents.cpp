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

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <tools/block_drag_dependents.h>


namespace
{

const int TOL = pcbIUScale.mmToIU( 0.001 );

PCB_SHAPE* makeSegment( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd, PCB_LAYER_ID aLayer = Edge_Cuts )
{
    PCB_SHAPE* segment = new PCB_SHAPE( &aBoard, SHAPE_T::SEGMENT );
    segment->SetStart( aStart );
    segment->SetEnd( aEnd );
    segment->SetLayer( aLayer );
    aBoard.Add( segment );
    return segment;
}

BLOCK_DRAG_ENDPOINT movedStart( PCB_SHAPE* aShape )
{
    return { aShape, true, aShape->GetStart() };
}

PCB_SHAPE* makeArc( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd,
                    PCB_LAYER_ID aLayer = Edge_Cuts )
{
    PCB_SHAPE* arc = new PCB_SHAPE( &aBoard, SHAPE_T::ARC );
    arc->SetArcGeometry( aStart, aMid, aEnd );
    arc->SetLayer( aLayer );
    aBoard.Add( arc );
    return arc;
}

const BLOCK_DRAG_ENDPOINT* find( const std::vector<BLOCK_DRAG_ENDPOINT>& aDependents, const PCB_SHAPE* aShape )
{
    for( const BLOCK_DRAG_ENDPOINT& dep : aDependents )
    {
        if( dep.shape == aShape )
            return &dep;
    }

    return nullptr;
}

} // namespace


BOOST_AUTO_TEST_SUITE( BlockDragDependentsTests )


BOOST_AUTO_TEST_CASE( ExactAndToleranceMatches )
{
    BOARD board;
    board.SetOutlinesChainingEpsilon( TOL );

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* exact = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ) );
    PCB_SHAPE* nearSeg = makeSegment( board, VECTOR2I( TOL, 0 ), VECTOR2I( 50000, 100000 ) );
    PCB_SHAPE* farSeg = makeSegment( board, VECTOR2I( TOL + 1, TOL + 1 ), VECTOR2I( 90000, 90000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { exact, nearSeg, farSeg }, { movedStart( moved ) }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 2 );

    int exactCount = 0;
    int nearCount = 0;

    for( const BLOCK_DRAG_ENDPOINT& dep : dependents )
    {
        exactCount += dep.shape == exact ? 1 : 0;
        nearCount += dep.shape == nearSeg ? 1 : 0;

        BOOST_CHECK( dep.shape != farSeg );
        BOOST_CHECK( dep.isStart );
    }

    BOOST_CHECK_EQUAL( exactCount, 1 );
    BOOST_CHECK_EQUAL( nearCount, 1 );
}


// A candidate close enough on each axis on its own but too far away as the crow
// flies has to be rejected, because the tolerance is a real distance
BOOST_AUTO_TEST_CASE( ToleranceIsEuclidean )
{
    BOARD board;
    board.SetOutlinesChainingEpsilon( TOL );

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* diagonal = makeSegment( board, VECTOR2I( TOL, TOL ), VECTOR2I( 90000, 90000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { diagonal }, { movedStart( moved ) }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( ArcCandidateFollowsWhole )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* arc = makeArc( board, VECTOR2I( 0, 0 ), VECTOR2I( 50000, 50000 ), VECTOR2I( 100000, 0 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result = BlockDragFindDependents( { arc }, { movedStart( moved ) }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 1 );
    BOOST_CHECK_EQUAL( dependents[0].shape, arc );
    BOOST_CHECK( dependents[0].follow == BLOCK_DRAG_FOLLOW::WHOLE );
}


BOOST_AUTO_TEST_CASE( FilletedCornerCarriesArcAndStretchesNextSegment )
{
    // A filleted corner: a straight edge, a corner arc, then the next edge. Dragging
    // the first edge's joint carries the arc whole and moves the end of the far
    // segment that touches it, while that segment's other end holds.
    BOARD board;

    // Quarter arc centred on (0, 100000), from (0,0) round to (100000,100000)
    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( -100000, 0 ), VECTOR2I( 0, 0 ) );
    PCB_SHAPE* arc = makeArc( board, VECTOR2I( 0, 0 ), VECTOR2I( 70711, 29289 ), VECTOR2I( 100000, 100000 ) );
    PCB_SHAPE* next = makeSegment( board, VECTOR2I( 100000, 100000 ), VECTOR2I( 100000, 400000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { arc, next }, { { moved, false, moved->GetEnd() } }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 2 );

    const BLOCK_DRAG_ENDPOINT* arcDep = find( dependents, arc );
    const BLOCK_DRAG_ENDPOINT* nextDep = find( dependents, next );

    BOOST_REQUIRE( arcDep && nextDep );
    BOOST_CHECK( arcDep->follow == BLOCK_DRAG_FOLLOW::WHOLE );
    BOOST_CHECK( arcDep->isStart );

    // The walk stops at the segment, which stretches, so its far end holds
    BOOST_CHECK( nextDep->follow == BLOCK_DRAG_FOLLOW::STRETCH );
    BOOST_CHECK( nextDep->isStart );
}


BOOST_AUTO_TEST_CASE( ClosedChainDoesNotCascadeIntoItself )
{
    // Four arcs joined end to end with no segment anywhere. The walk visits each one
    // once and stops, so it cannot loop back to the arc it started from.
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( -100000, 0 ), VECTOR2I( 0, 0 ) );

    // A full circle centred on (0, 150000), cut into four quarters, closing on (0,0)
    PCB_SHAPE* a1 = makeArc( board, VECTOR2I( 0, 0 ), VECTOR2I( 106066, 43934 ), VECTOR2I( 150000, 150000 ) );
    PCB_SHAPE* a2 = makeArc( board, VECTOR2I( 150000, 150000 ), VECTOR2I( 106066, 256066 ), VECTOR2I( 0, 300000 ) );
    PCB_SHAPE* a3 = makeArc( board, VECTOR2I( 0, 300000 ), VECTOR2I( -106066, 256066 ), VECTOR2I( -150000, 150000 ) );
    PCB_SHAPE* a4 = makeArc( board, VECTOR2I( -150000, 150000 ), VECTOR2I( -106066, 43934 ), VECTOR2I( 0, 0 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { a1, a2, a3, a4 }, { { moved, false, moved->GetEnd() } }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );

    // Every arc follows, and each one exactly once. Checking for "at most once" would
    // also pass if the walk found nothing at all
    BOOST_REQUIRE_EQUAL( dependents.size(), 4 );

    for( const PCB_SHAPE* arc : { a1, a2, a3, a4 } )
    {
        int seen = 0;

        for( const BLOCK_DRAG_ENDPOINT& dep : dependents )
        {
            if( dep.shape == arc )
            {
                seen++;
                BOOST_CHECK( dep.follow == BLOCK_DRAG_FOLLOW::WHOLE );
            }
        }

        BOOST_CHECK_EQUAL( seen, 1 );
    }
}


BOOST_AUTO_TEST_CASE( LockedArcInTheChainRefuses )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( -100000, 0 ), VECTOR2I( 0, 0 ) );
    PCB_SHAPE* arc = makeArc( board, VECTOR2I( 0, 0 ), VECTOR2I( 70711, 29289 ), VECTOR2I( 100000, 100000 ) );

    arc->SetLocked( true );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { arc }, { { moved, false, moved->GetEnd() } }, false, dependents );

    BOOST_CHECK( result == BLOCK_DRAG_DEPENDENT_RESULT::LOCKED );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( CircleCandidateIsStillSkipped )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );

    PCB_SHAPE* circle = new PCB_SHAPE( &board, SHAPE_T::CIRCLE );
    circle->SetStart( VECTOR2I( 0, 0 ) );
    circle->SetEnd( VECTOR2I( 50000, 0 ) );
    circle->SetLayer( moved->GetLayer() );
    board.Add( circle );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BLOCK_DRAG_DEPENDENT_RESULT result =
            BlockDragFindDependents( { circle }, { movedStart( moved ) }, false, dependents );

    BOOST_REQUIRE( result == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( DifferentLayerDoesNotMatch )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ), Edge_Cuts );
    makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ), F_SilkS );

    std::vector<PCB_SHAPE*> candidates;

    for( BOARD_ITEM* drawing : board.Drawings() )
    {
        if( drawing != moved )
            candidates.push_back( static_cast<PCB_SHAPE*>( drawing ) );
    }

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE( BlockDragFindDependents( candidates, { movedStart( moved ) }, false, dependents )
                   == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( AmbiguousCandidatesAreNotGuessed )
{
    BOARD board;
    board.SetOutlinesChainingEpsilon( TOL );

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );

    // Both close enough to the moved endpoint, but not to each other
    PCB_SHAPE* left = makeSegment( board, VECTOR2I( -TOL, 0 ), VECTOR2I( -50000, 50000 ) );
    PCB_SHAPE* right = makeSegment( board, VECTOR2I( TOL, 0 ), VECTOR2I( 50000, 50000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE( BlockDragFindDependents( { left, right }, { movedStart( moved ) }, false, dependents )
                   == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( MutuallyCoincidentClusterFollowsTogether )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* first = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( -50000, 50000 ) );
    PCB_SHAPE* second = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 50000, 50000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE( BlockDragFindDependents( { first, second }, { movedStart( moved ) }, false, dependents )
                   == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 2 );

    int firstCount = 0;
    int secondCount = 0;

    for( const BLOCK_DRAG_ENDPOINT& dep : dependents )
    {
        firstCount += dep.shape == first ? 1 : 0;
        secondCount += dep.shape == second ? 1 : 0;
    }

    BOOST_CHECK_EQUAL( firstCount, 1 );
    BOOST_CHECK_EQUAL( secondCount, 1 );
}


BOOST_AUTO_TEST_CASE( MovedEndpointsAreNotDependents )
{
    BOARD board;

    // Both segments' starts are picked and sit on each other, so neither can follow
    PCB_SHAPE* movedA = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* movedB = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE( BlockDragFindDependents( { movedA, movedB }, { movedStart( movedA ), movedStart( movedB ) }, false,
                                            dependents )
                   == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_CHECK( dependents.empty() );
}


BOOST_AUTO_TEST_CASE( DependentAppearsOnce )
{
    BOARD board;

    // Two picked endpoints sit on the same neighbour endpoint
    PCB_SHAPE* movedA = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* movedB = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ) );
    PCB_SHAPE* neighbor = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( -50000, -50000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE(
            BlockDragFindDependents( { neighbor }, { movedStart( movedA ), movedStart( movedB ) }, false, dependents )
            == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 1 );
    BOOST_CHECK( dependents.front().shape == neighbor );
}


BOOST_AUTO_TEST_CASE( LockedDependentRejectsUnlessOverridden )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* locked = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ) );
    locked->SetLocked( true );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_CHECK( BlockDragFindDependents( { locked }, { movedStart( moved ) }, false, dependents )
                 == BLOCK_DRAG_DEPENDENT_RESULT::LOCKED );
    BOOST_CHECK( dependents.empty() );

    BOOST_CHECK( BlockDragFindDependents( { locked }, { movedStart( moved ) }, true, dependents )
                 == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 1 );
}


BOOST_AUTO_TEST_CASE( LockedParentOfDependentRejects )
{
    BOARD board;

    PCB_SHAPE* moved = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );

    // The candidate is not locked itself, but it sits inside a locked footprint
    FOOTPRINT* footprint = new FOOTPRINT( &board );
    footprint->SetLocked( true );

    PCB_SHAPE* child = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );
    child->SetStart( VECTOR2I( 0, 0 ) );
    child->SetEnd( VECTOR2I( 0, 100000 ) );
    child->SetLayer( Edge_Cuts );
    footprint->Add( child );
    board.Add( footprint );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_CHECK( BlockDragFindDependents( { child }, { movedStart( moved ) }, false, dependents )
                 == BLOCK_DRAG_DEPENDENT_RESULT::LOCKED );
    BOOST_CHECK( dependents.empty() );

    BOOST_CHECK( BlockDragFindDependents( { child }, { movedStart( moved ) }, true, dependents )
                 == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 1 );
}


BOOST_AUTO_TEST_CASE( BothEndsOfOneNeighborCanFollow )
{
    BOARD board;

    PCB_SHAPE* movedA = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) );
    PCB_SHAPE* movedB = makeSegment( board, VECTOR2I( 0, 50000 ), VECTOR2I( 100000, 50000 ) );
    PCB_SHAPE* bridge = makeSegment( board, VECTOR2I( 0, 0 ), VECTOR2I( 0, 50000 ) );

    std::vector<BLOCK_DRAG_ENDPOINT> dependents;

    BOOST_REQUIRE(
            BlockDragFindDependents( { bridge }, { movedStart( movedA ), movedStart( movedB ) }, false, dependents )
            == BLOCK_DRAG_DEPENDENT_RESULT::OK );
    BOOST_REQUIRE_EQUAL( dependents.size(), 2 );
    BOOST_CHECK( dependents[0].shape == bridge );
    BOOST_CHECK( dependents[1].shape == bridge );
    BOOST_CHECK( dependents[0].isStart != dependents[1].isStart );
}


BOOST_AUTO_TEST_SUITE_END()
