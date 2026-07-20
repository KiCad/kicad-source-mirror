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

/**
 * @file
 * Tests reference-value propagation (issue #2329): a non-driving (reference) constraint only
 * measures its quantity, so after a solve its stored value must track the geometry, while a
 * driving constraint's value is an input the solve never overwrites.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <board.h>
#include <pcb_shape.h>

#include <geometry/seg.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;

namespace
{
double cornerAngleOf( const PCB_SHAPE* aA, const PCB_SHAPE* aB )
{
    return MeasureCornerAngle( SEG( aA->GetStart(), aA->GetEnd() ),
                               SEG( aB->GetStart(), aB->GetEnd() ) ).AsDegrees();
}
}


BOOST_AUTO_TEST_SUITE( ConstraintReference )


// A reference (non-driving) length does not force the segment; after a drag shortens it, its stored
// value follows the new measured length.
BOOST_AUTO_TEST_CASE( ReferenceLengthTracksGeometry )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 10.0 * MM );
    len->SetDriving( false );

    std::vector<BOARD_ITEM*> staged;

    // Drag END straight up to (0, 6 mm); the pinned start end keeps the length at 6 mm.
    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { 0, 6 * MM }, nullptr,
                  [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( len->GetValue().has_value() );
    BOOST_CHECK_LE( std::abs( *len->GetValue() - 6.0 * MM ), 5000.0 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), len ) != staged.end() );
}


// The two point form has no owning segment so its value is re measured from the two member
// anchors after a drag it must follow the new distance
BOOST_AUTO_TEST_CASE( ReferenceTwoPointLengthTracksGeometry )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                           { seg->m_Uuid, CONSTRAINT_ANCHOR::END } },
                                         10.0 * MM );
    len->SetDriving( false );

    std::vector<BOARD_ITEM*> staged;

    // Drag END straight up to (0, 6 mm); the pinned start keeps the anchor distance at 6 mm.
    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { 0, 6 * MM }, nullptr,
                  [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( len->GetValue().has_value() );
    BOOST_CHECK_LE( std::abs( *len->GetValue() - 6.0 * MM ), 5000.0 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), len ) != staged.end() );
}


// A driving length forces the geometry, so the same drag leaves the segment 10 mm long and never
// rewrites the stored value.
BOOST_AUTO_TEST_CASE( DrivingLengthValueNotOverwritten )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 10.0 * MM );
    BOOST_REQUIRE( len->IsDriving() );

    std::vector<BOARD_ITEM*> staged;

    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { 0, 6 * MM }, nullptr,
                  [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( len->GetValue().has_value() );
    BOOST_CHECK_EQUAL( *len->GetValue(), 10.0 * MM );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), len ) == staged.end() );
    BOOST_CHECK_LE( std::abs( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() - 10.0 * MM ),
                    5000.0 );
}


// A reference radius follows a resize; the resized circle is held, so the measured radius updates
// the stored value.
BOOST_AUTO_TEST_CASE( ReferenceRadiusTracksGeometry )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );

    PCB_CONSTRAINT* rad = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS,
                                         { { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 5.0 * MM );
    rad->SetDriving( false );

    circle->SetRadius( 8 * MM );   // stand in for a radius-handle drag

    std::vector<BOARD_ITEM*> staged;

    ReSolveAfterShapeResize( &board, circle, nullptr,
                             [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( rad->GetValue().has_value() );
    BOOST_CHECK_LE( std::abs( *rad->GetValue() - 8.0 * MM ), 5000.0 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), rad ) != staged.end() );
}


// A solve that leaves the measured value where it was must not stage the constraint or churn undo.
BOOST_AUTO_TEST_CASE( ReferenceValueUnchangedIsNotStaged )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 10.0 * MM );
    len->SetDriving( false );

    std::vector<BOARD_ITEM*> staged;

    // Re-pin END where it already sits; the length is unchanged, so nothing is written back.
    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { 10 * MM, 0 }, nullptr,
                  [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_CHECK( std::find( staged.begin(), staged.end(), len ) == staged.end() );
    BOOST_CHECK_EQUAL( *len->GetValue(), 10.0 * MM );
}


// Driving a corner to 120 degrees from an 100 degree start must keep the obtuse corner, not collapse
// to the 60 degree mirror -- the directed-target mapping picks the candidate nearest the current
// configuration.
BOOST_AUTO_TEST_CASE( DrivingObtuseCornerSurvivesSolve )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );          // ray +x
    PCB_SHAPE* b = addSegment( board, { 0, 0 }, { -1736482, 9848078 } );   // ray at 100 deg

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::START }, { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } },
                   120.0 );

    solveAndApply( board, { a, b } );

    BOOST_CHECK_CLOSE( cornerAngleOf( a, b ), 120.0, 0.5 );
}


// A chained-polyline corner (end-to-start coincidence, odd endpoint parity) driven across the 90
// degree midline must land on the requested corner, not its supplement -- the directed target is
// chosen from the parity-correct candidate pair.
BOOST_AUTO_TEST_CASE( DrivingChainedCornerCrossesNinety )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );   // 90 deg chained corner

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::END }, { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION,
                   { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } },
                   60.0 );

    solveAndApply( board, { a, b } );

    BOOST_CHECK_CLOSE( cornerAngleOf( a, b ), 60.0, 0.5 );
}


// A reference angular dimension tracks the corner it measures: reshape one segment, re-solve, and
// the stored value follows to the new corner.
BOOST_AUTO_TEST_CASE( ReferenceAngleTracksGeometry )
{
    BOARD      board;
    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );   // ray +x
    PCB_SHAPE* b = addSegment( board, { 0, 0 }, { 0, 10 * MM } );   // ray +y, a 90 deg corner

    PCB_CONSTRAINT* angle = addConstraint(
            board, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION,
            { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE }, { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 90.0 );
    angle->SetDriving( false );

    b->SetEnd( { -5 * MM, 8660254 } );   // open the corner to 120 deg

    std::vector<BOARD_ITEM*> staged;
    ReSolveShapeClusters( &board, { b }, nullptr, [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( angle->GetValue().has_value() );
    BOOST_CHECK_CLOSE( *angle->GetValue(), 120.0, 0.5 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), angle ) != staged.end() );
}


BOOST_AUTO_TEST_SUITE_END()
