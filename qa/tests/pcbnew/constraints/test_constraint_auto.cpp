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
 * Tests SelectShapeAutoConstraints, the decision logic behind draw time auto constraints.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <vector>

#include <board.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;

namespace
{
// Offsets sized against the production tolerances 0.01mm bind and 0.25mm corridor
constexpr int NEAR_MISS = MM / 10; // inside the corridor but outside the bind tolerance
constexpr int FAR_MISS = MM / 2;   // outside the corridor


// The drawn shape stays off the board as during interactive draw
std::unique_ptr<PCB_SHAPE> drawnSegment( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    auto s = std::make_unique<PCB_SHAPE>( &aBoard, SHAPE_T::SEGMENT );
    s->SetStart( aStart );
    s->SetEnd( aEnd );
    return s;
}


std::unique_ptr<PCB_SHAPE> drawnArc( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    auto s = std::make_unique<PCB_SHAPE>( &aBoard, SHAPE_T::ARC );
    s->SetArcGeometry( aStart, aMid, aEnd );
    return s;
}


std::unique_ptr<PCB_SHAPE> drawnCircle( BOARD& aBoard, const VECTOR2I& aCenter, int aRadius )
{
    auto s = std::make_unique<PCB_SHAPE>( &aBoard, SHAPE_T::CIRCLE );
    s->SetCenter( aCenter );
    s->SetRadius( aRadius );
    return s;
}
} // namespace


BOOST_AUTO_TEST_SUITE( ConstraintAuto )


// A segment drawn from an existing endpoint binds one coincident and needs no solve
BOOST_AUTO_TEST_CASE( EndpointBindsCoincident )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    auto drawn = drawnSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT );
    BOOST_CHECK( !picks[0].needsSolve );

    const std::vector<CONSTRAINT_MEMBER>& members = picks[0].constraint->GetMembers();
    BOOST_REQUIRE_EQUAL( members.size(), 2 );
    BOOST_CHECK( members[0] == CONSTRAINT_MEMBER( drawn->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    BOOST_CHECK( members[1] == CONSTRAINT_MEMBER( target->m_Uuid, CONSTRAINT_ANCHOR::END ) );
}


// An endpoint on a segment midpoint binds midpoint not point on line
BOOST_AUTO_TEST_CASE( MidpointUpgrade )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    auto drawn = drawnSegment( board, { 5 * MM, 0 }, { 5 * MM, 8 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::MIDPOINT );

    const std::vector<CONSTRAINT_MEMBER>& members = picks[0].constraint->GetMembers();
    BOOST_CHECK( members[0] == CONSTRAINT_MEMBER( drawn->m_Uuid, CONSTRAINT_ANCHOR::START ) );
    BOOST_CHECK( members[1] == CONSTRAINT_MEMBER( target->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
}


// An endpoint elsewhere on the outline binds point on line
BOOST_AUTO_TEST_CASE( PointOnLineFallback )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    auto drawn = drawnSegment( board, { 3 * MM, 0 }, { 3 * MM, 8 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    BOOST_CHECK( picks[0].constraint->GetMembers()[1]
                 == CONSTRAINT_MEMBER( target->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
}


// An arc leaving a segment end near tangency adds a tangent that needs the snap solve
BOOST_AUTO_TEST_CASE( TangentWithinThreshold )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    // Centre above the shared point so the arc leaves horizontally exactly tangent
    // The mid sits 45 degrees around the 5mm radius from the centre at (10, 5) mm
    auto drawn = drawnArc( board, { 10 * MM, 0 }, { 10 * MM + 3536000, 5 * MM - 3536000 }, { 15 * MM, 5 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 2 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT );
    BOOST_CHECK( picks[1].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::TANGENT );
    BOOST_CHECK( picks[1].needsSolve );

    // Target first so the snap solve pins the board and rotates the drawn arc
    const std::vector<CONSTRAINT_MEMBER>& members = picks[1].constraint->GetMembers();
    BOOST_CHECK( members[0] == CONSTRAINT_MEMBER( target->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
    BOOST_CHECK( members[1] == CONSTRAINT_MEMBER( drawn->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
}


// An arc leaving well off tangency binds only the coincident
BOOST_AUTO_TEST_CASE( TangentRejectedBeyondThreshold )
{
    BOARD board;
    addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    // Centre at (13, 4) mm makes the leave angle about 37 degrees off the segment
    auto drawn = drawnArc( board, { 10 * MM, 0 }, { 13 * MM, -1 * MM }, { 18 * MM, 4 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT );
}


// Collinear chained segments never read as tangent
BOOST_AUTO_TEST_CASE( SegmentsNeverTangent )
{
    BOARD board;
    addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    auto drawn = drawnSegment( board, { 10 * MM, 0 }, { 20 * MM, 0 } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT );
}


// An anchor a near miss inside the corridor binds point on line and needs the snap solve
BOOST_AUTO_TEST_CASE( CorridorBindsNearMiss )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 5 * MM, 5 * MM }, { 5 * MM, 15 * MM } );

    auto drawn = drawnSegment( board, { 0, 15 * MM + NEAR_MISS }, { 10 * MM, 15 * MM + NEAR_MISS } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    BOOST_CHECK( picks[0].needsSolve );

    // The existing anchor first so the snap solve pins it and pulls the drawn line through
    const std::vector<CONSTRAINT_MEMBER>& members = picks[0].constraint->GetMembers();
    BOOST_CHECK( members[0] == CONSTRAINT_MEMBER( target->m_Uuid, CONSTRAINT_ANCHOR::END ) );
    BOOST_CHECK( members[1] == CONSTRAINT_MEMBER( drawn->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
}


// Beyond the corridor nothing binds
BOOST_AUTO_TEST_CASE( CorridorIgnoresFarMiss )
{
    BOARD board;
    addSegment( board, { 5 * MM, 5 * MM }, { 5 * MM, 15 * MM } );

    auto drawn = drawnSegment( board, { 0, 15 * MM + FAR_MISS }, { 10 * MM, 15 * MM + FAR_MISS } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_CHECK( picks.empty() );
}


// An anchor near the drawn endpoint belongs to the coincident pass not the corridor
BOOST_AUTO_TEST_CASE( CorridorExcludesDrawnEndpoints )
{
    BOARD board;
    addSegment( board, { 5 * MM, 5 * MM }, { 5 * MM, 15 * MM } );

    // Starts 0.1mm from the target end which is too far to coincide and excluded from the corridor
    auto drawn = drawnSegment( board, { 5 * MM + NEAR_MISS, 15 * MM }, { 15 * MM, 15 * MM } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_CHECK( picks.empty() );
}


// Axis aligned segments keep their axis only when the mode asks for it
BOOST_AUTO_TEST_CASE( AxisAlignedBindsInConstrainedMode )
{
    BOARD board;

    auto horizontal = drawnSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    auto picks = SelectShapeAutoConstraints( &board, horizontal.get(), &board, true );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::HORIZONTAL );

    auto vertical = drawnSegment( board, { 0, 0 }, { 0, 10 * MM } );
    picks = SelectShapeAutoConstraints( &board, vertical.get(), &board, true );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::VERTICAL );

    picks = SelectShapeAutoConstraints( &board, horizontal.get(), &board, false );
    BOOST_CHECK( picks.empty() );

    auto diagonal = drawnSegment( board, { 0, 0 }, { 10 * MM, 10 * MM } );
    picks = SelectShapeAutoConstraints( &board, diagonal.get(), &board, true );
    BOOST_CHECK( picks.empty() );
}


// A circle centre binds concentric on a centre coincident on an anchor and point on line on an outline
BOOST_AUTO_TEST_CASE( CentreCascade )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* segment = addSegment( board, { 20 * MM, 0 }, { 30 * MM, 0 } );

    auto onCentre = drawnCircle( board, { 0, 0 }, 8 * MM );
    auto picks = SelectShapeAutoConstraints( &board, onCentre.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::CONCENTRIC );
    BOOST_CHECK( picks[0].constraint->GetMembers()[0]
                 == CONSTRAINT_MEMBER( circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );

    auto onEndpoint = drawnCircle( board, { 30 * MM, 0 }, 2 * MM );
    picks = SelectShapeAutoConstraints( &board, onEndpoint.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT );
    BOOST_CHECK( picks[0].constraint->GetMembers()[0]
                 == CONSTRAINT_MEMBER( onEndpoint->m_Uuid, CONSTRAINT_ANCHOR::CENTER ) );

    auto onOutline = drawnCircle( board, { 23 * MM, 0 }, 2 * MM );
    picks = SelectShapeAutoConstraints( &board, onOutline.get(), &board, false );

    BOOST_REQUIRE_EQUAL( picks.size(), 1 );
    BOOST_CHECK( picks[0].constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    BOOST_CHECK( picks[0].constraint->GetMembers()[1]
                 == CONSTRAINT_MEMBER( segment->m_Uuid, CONSTRAINT_ANCHOR::WHOLE ) );
}


// A chord drawn across a shallow arc binds both ends but authors the tangent only once
BOOST_AUTO_TEST_CASE( ChordAuthorsSingleTangent )
{
    BOARD board;

    // Radius 60mm over a 20mm chord leaves both ends about 9.6 degrees off the chord
    // The mid sags by 60 - sqrt(60^2 - 10^2) mm below the chord
    const int sagitta = 839200;

    addArc( board, { 0, 0 }, { 10 * MM, -sagitta }, { 20 * MM, 0 } );

    auto drawn = drawnSegment( board, { 0, 0 }, { 20 * MM, 0 } );
    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    int coincident = 0;
    int tangent = 0;

    for( const AUTO_CONSTRAINT& pick : picks )
    {
        if( pick.constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::COINCIDENT )
            coincident++;
        else if( pick.constraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::TANGENT )
            tangent++;
    }

    BOOST_CHECK_EQUAL( coincident, 2 );
    BOOST_CHECK_EQUAL( tangent, 1 );
}


// A binding equal to one already on the board is not authored again
BOOST_AUTO_TEST_CASE( DuplicateNotReauthored )
{
    BOARD      board;
    PCB_SHAPE* target = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    auto drawn = drawnSegment( board, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* existing = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    existing->AddMember( drawn->m_Uuid, CONSTRAINT_ANCHOR::START );
    existing->AddMember( target->m_Uuid, CONSTRAINT_ANCHOR::END );
    board.Add( existing );

    auto picks = SelectShapeAutoConstraints( &board, drawn.get(), &board, false );

    BOOST_CHECK( picks.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
