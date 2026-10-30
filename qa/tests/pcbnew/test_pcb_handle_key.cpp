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

#include <set>

#include <board.h>
#include <pcb_shape.h>
#include <tool/edit_points.h>
#include <tools/pcb_handle_key.h>


namespace
{

// Built the same way the point editor behaviors build their points in MakePoints()
std::shared_ptr<EDIT_POINTS> makeQuadCollection( EDA_ITEM* aParent, int aOffset = 0 )
{
    std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aParent );

    points->AddPoint( VECTOR2I( aOffset, 0 ) );
    points->AddPoint( VECTOR2I( aOffset + 100, 0 ) );
    points->AddPoint( VECTOR2I( aOffset + 100, 100 ) );
    points->AddPoint( VECTOR2I( aOffset, 100 ) );
    points->AddLine( points->Point( 0 ), points->Point( 1 ) );
    points->AddLine( points->Point( 1 ), points->Point( 2 ) );

    return points;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PcbHandleKeyTests )


BOOST_AUTO_TEST_CASE( PointSelectionFlagsAreIndependent )
{
    EDIT_POINT point( VECTOR2I( 0, 0 ) );

    BOOST_CHECK( !point.IsSelected() );
    BOOST_CHECK( !point.IsDependent() );

    point.SetSelected();
    point.SetActive();
    point.SetHover();

    BOOST_CHECK( point.IsSelected() );
    BOOST_CHECK( !point.IsDependent() );

    point.SetActive( false );
    point.SetHover( false );

    BOOST_CHECK( point.IsSelected() );
    BOOST_CHECK( !point.IsActive() );
    BOOST_CHECK( !point.IsHover() );

    point.SetDependent();
    point.SetSelected( false );

    BOOST_CHECK( point.IsDependent() );
    BOOST_CHECK( !point.IsSelected() );
}


BOOST_AUTO_TEST_CASE( KeyCreationForPointsAndLines )
{
    BOARD      board;
    PCB_SHAPE* shape = new PCB_SHAPE( &board, SHAPE_T::POLY );
    board.Add( shape );

    std::shared_ptr<EDIT_POINTS> points = makeQuadCollection( shape );

    std::optional<PCB_HANDLE_KEY> pointKey = PCB_HANDLE_KEY::Make( *points, &points->Point( 2 ) );

    BOOST_REQUIRE( pointKey.has_value() );
    BOOST_CHECK( pointKey->Parent() == shape->m_Uuid );
    BOOST_CHECK( !pointKey->IsLine() );
    BOOST_CHECK_EQUAL( pointKey->Index(), 2 );

    std::optional<PCB_HANDLE_KEY> lineKey = PCB_HANDLE_KEY::Make( *points, &points->Line( 1 ) );

    BOOST_REQUIRE( lineKey.has_value() );
    BOOST_CHECK( lineKey->IsLine() );
    BOOST_CHECK_EQUAL( lineKey->Index(), 1 );

    EDIT_POINT foreign( VECTOR2I( 5, 5 ) );

    BOOST_CHECK( !PCB_HANDLE_KEY::Make( *points, &foreign ).has_value() );
    BOOST_CHECK( !PCB_HANDLE_KEY::Make( *points, nullptr ).has_value() );
}


BOOST_AUTO_TEST_CASE( KeyResolvesAfterRegeneration )
{
    BOARD      board;
    PCB_SHAPE* shape = new PCB_SHAPE( &board, SHAPE_T::POLY );
    board.Add( shape );

    std::shared_ptr<EDIT_POINTS> before = makeQuadCollection( shape );

    std::optional<PCB_HANDLE_KEY> pointKey = PCB_HANDLE_KEY::Make( *before, &before->Point( 2 ) );
    std::optional<PCB_HANDLE_KEY> lineKey = PCB_HANDLE_KEY::Make( *before, &before->Line( 0 ) );

    BOOST_REQUIRE( pointKey.has_value() );
    BOOST_REQUIRE( lineKey.has_value() );

    // Rebuild the collection with the same parent and the same handle counts, but moved
    std::shared_ptr<EDIT_POINTS> after = makeQuadCollection( shape, 50 );

    EDIT_POINT* resolvedPoint = pointKey->Resolve( *after );
    EDIT_POINT* resolvedLine = lineKey->Resolve( *after );

    BOOST_REQUIRE( resolvedPoint != nullptr );
    BOOST_CHECK( resolvedPoint == &after->Point( 2 ) );
    BOOST_CHECK_EQUAL( resolvedPoint->GetPosition().x, 150 );

    BOOST_REQUIRE( resolvedLine != nullptr );
    BOOST_CHECK( resolvedLine == &after->Line( 0 ) );
}


BOOST_AUTO_TEST_CASE( KeyEqualityDeduplicates )
{
    BOARD      board;
    PCB_SHAPE* shapeA = new PCB_SHAPE( &board, SHAPE_T::POLY );
    PCB_SHAPE* shapeB = new PCB_SHAPE( &board, SHAPE_T::POLY );
    board.Add( shapeA );
    board.Add( shapeB );

    std::shared_ptr<EDIT_POINTS> pointsA = makeQuadCollection( shapeA );
    std::shared_ptr<EDIT_POINTS> pointsB = makeQuadCollection( shapeB );

    std::optional<PCB_HANDLE_KEY> key1 = PCB_HANDLE_KEY::Make( *pointsA, &pointsA->Point( 1 ) );
    std::optional<PCB_HANDLE_KEY> key2 = PCB_HANDLE_KEY::Make( *pointsA, &pointsA->Point( 1 ) );
    std::optional<PCB_HANDLE_KEY> otherIndex = PCB_HANDLE_KEY::Make( *pointsA, &pointsA->Point( 2 ) );
    std::optional<PCB_HANDLE_KEY> otherKind = PCB_HANDLE_KEY::Make( *pointsA, &pointsA->Line( 1 ) );
    std::optional<PCB_HANDLE_KEY> otherParent = PCB_HANDLE_KEY::Make( *pointsB, &pointsB->Point( 1 ) );

    BOOST_REQUIRE( key1 && key2 && otherIndex && otherKind && otherParent );

    BOOST_CHECK( *key1 == *key2 );
    BOOST_CHECK( !( *key1 == *otherIndex ) );
    BOOST_CHECK( !( *key1 == *otherKind ) );
    BOOST_CHECK( !( *key1 == *otherParent ) );

    std::set<PCB_HANDLE_KEY> picked;
    picked.insert( *key1 );
    picked.insert( *key2 );
    picked.insert( *otherIndex );
    picked.insert( *otherKind );
    picked.insert( *otherParent );

    BOOST_CHECK_EQUAL( picked.size(), 4 );
}


BOOST_AUTO_TEST_CASE( MissingIdentityInvalidates )
{
    BOARD      board;
    PCB_SHAPE* shapeA = new PCB_SHAPE( &board, SHAPE_T::POLY );
    PCB_SHAPE* shapeB = new PCB_SHAPE( &board, SHAPE_T::POLY );
    board.Add( shapeA );
    board.Add( shapeB );

    std::shared_ptr<EDIT_POINTS> before = makeQuadCollection( shapeA );

    std::optional<PCB_HANDLE_KEY> pointKey = PCB_HANDLE_KEY::Make( *before, &before->Point( 2 ) );
    std::optional<PCB_HANDLE_KEY> lineKey = PCB_HANDLE_KEY::Make( *before, &before->Line( 1 ) );

    BOOST_REQUIRE( pointKey.has_value() );
    BOOST_REQUIRE( lineKey.has_value() );

    // A different parent fails even with the same handle counts
    std::shared_ptr<EDIT_POINTS> otherParent = makeQuadCollection( shapeB );

    BOOST_CHECK( pointKey->Resolve( *otherParent ) == nullptr );

    // Changing the handle counts kills the key even though the index still exists
    std::shared_ptr<EDIT_POINTS> fewerPoints = std::make_shared<EDIT_POINTS>( shapeA );
    fewerPoints->AddPoint( VECTOR2I( 0, 0 ) );
    fewerPoints->AddPoint( VECTOR2I( 100, 0 ) );
    fewerPoints->AddPoint( VECTOR2I( 100, 100 ) );
    fewerPoints->AddLine( fewerPoints->Point( 0 ), fewerPoints->Point( 1 ) );
    fewerPoints->AddLine( fewerPoints->Point( 1 ), fewerPoints->Point( 2 ) );

    BOOST_CHECK( pointKey->Resolve( *fewerPoints ) == nullptr );

    // The same number of points but fewer lines counts as a change too
    std::shared_ptr<EDIT_POINTS> fewerLines = std::make_shared<EDIT_POINTS>( shapeA );
    fewerLines->AddPoint( VECTOR2I( 0, 0 ) );
    fewerLines->AddPoint( VECTOR2I( 100, 0 ) );
    fewerLines->AddPoint( VECTOR2I( 100, 100 ) );
    fewerLines->AddPoint( VECTOR2I( 0, 100 ) );
    fewerLines->AddLine( fewerLines->Point( 0 ), fewerLines->Point( 1 ) );

    BOOST_CHECK( lineKey->Resolve( *fewerLines ) == nullptr );
}


BOOST_AUTO_TEST_CASE( AStaleKeyNamesTheSameHandleButStopsResolving )
{
    BOARD      board;
    PCB_SHAPE* shape = new PCB_SHAPE( &board, SHAPE_T::POLY );
    board.Add( shape );

    std::shared_ptr<EDIT_POINTS> points = makeQuadCollection( shape );

    std::optional<PCB_HANDLE_KEY> before = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    BOOST_REQUIRE( before );

    points->AddPoint( VECTOR2I( 50, 50 ) );

    std::optional<PCB_HANDLE_KEY> after = PCB_HANDLE_KEY::Make( *points, &points->Point( 0 ) );
    BOOST_REQUIRE( after );

    // Both name corner 0 of the same shape, so a picked set holds that handle once
    BOOST_CHECK( *before == *after );

    std::set<PCB_HANDLE_KEY> picked{ *before };

    BOOST_CHECK( !picked.insert( *after ).second );

    // The key made before the shape gained a corner no longer resolves, so the caller
    // drops it instead of moving whichever handle now sits at that index
    BOOST_CHECK( after->Resolve( *points ) == &points->Point( 0 ) );
    BOOST_CHECK( before->Resolve( *points ) == nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
