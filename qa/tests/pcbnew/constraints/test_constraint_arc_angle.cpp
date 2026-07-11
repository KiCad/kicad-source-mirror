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
 * Tests the arc swept-angle constraint (issue #2329): authoring measures GetArcAngle(), and the
 * solver drives the arc's swept angle, holding the free radius so it cannot collapse.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <vector>

#include <board.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;

namespace
{
PCB_SHAPE* addQuarterArc( BOARD& aBoard )
{
    // A quarter arc (0 -> 90 deg) about the origin, radius 10 mm.
    return addArc( aBoard, { 10 * MM, 0 }, { 7071068, 7071068 }, { 0, 10 * MM } );
}
}


BOOST_AUTO_TEST_SUITE( ConstraintArcAngle )


BOOST_AUTO_TEST_CASE( AuthorMeasuresSweptAngle )
{
    BOARD      board;
    PCB_SHAPE* arc = addQuarterArc( board );

    std::unique_ptr<PCB_CONSTRAINT> c =
            BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ARC_ANGLE, { arc } );

    BOOST_REQUIRE( c );
    BOOST_REQUIRE( c->HasValue() );
    BOOST_CHECK_CLOSE( *c->GetValue(), 90.0, 0.1 );
    BOOST_CHECK( c->IsDriving() );
}


BOOST_AUTO_TEST_CASE( AuthorRejectsNonArc )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );

    BOOST_CHECK( !BuildConstraintFromItems( &board, PCB_CONSTRAINT_TYPE::ARC_ANGLE, { circle } ) );
}


BOOST_AUTO_TEST_CASE( DriveSweptAngleReachesTarget )
{
    BOARD      board;
    PCB_SHAPE* arc = addQuarterArc( board );

    addConstraint( board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                   { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 60.0 );

    solveAndApply( board, { arc } );

    BOOST_CHECK_CLOSE( arc->GetArcAngle().AsDegrees(), 60.0, 0.5 );
}


// The apply-on-create path holds the free arc radius, so shrinking the swept angle rotates an
// endpoint about a stable radius instead of collapsing the arc to a point.  (Winding is preserved
// only once the arc is otherwise constrained; a fully free arc can still flip -- that is arc-edit
// robustness, item 4.)
BOOST_AUTO_TEST_CASE( ApplyHoldsArcRadius )
{
    BOARD      board;
    PCB_SHAPE* arc = addQuarterArc( board );
    int        radius0 = arc->GetRadius();

    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                                       { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 60.0 );

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, c, &modified );

    BOOST_CHECK_CLOSE( arc->GetArcAngle().AsDegrees(), 60.0, 0.5 );
    BOOST_CHECK_LE( std::abs( arc->GetRadius() - radius0 ), 5000 );
}


// Driving a 90 degree arc up to 270 degrees reaches the reflex value rather than no-oping on the
// nearer 90 degree reading -- the target is the mod-2*pi representative that moves the endpoint.
BOOST_AUTO_TEST_CASE( DriveToReflexAngle )
{
    BOARD      board;
    PCB_SHAPE* arc = addQuarterArc( board );

    addConstraint( board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                   { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 270.0 );

    solveAndApply( board, { arc } );

    BOOST_CHECK_CLOSE( arc->GetArcAngle().AsDegrees(), 270.0, 0.5 );
}


BOOST_AUTO_TEST_CASE( ArcAngleOnCircleIsUnmapped )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );

    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                                       { { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 90.0 );

    std::vector<PCB_CONSTRAINT*> constraints = { c };
    BOARD_CONSTRAINT_ADAPTER     adapter;
    BOOST_REQUIRE( adapter.Build( { circle }, constraints ) );

    BOOST_CHECK( std::find( adapter.UnmappedConstraints().begin(), adapter.UnmappedConstraints().end(),
                            c->m_Uuid )
                 != adapter.UnmappedConstraints().end() );
}


// A reference arc-angle stays live: reshape the arc, re-solve, and the stored value follows.
BOOST_AUTO_TEST_CASE( ReferenceArcAngleTracksGeometry )
{
    BOARD      board;
    PCB_SHAPE* arc = addQuarterArc( board );   // 90 deg

    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::ARC_ANGLE,
                                       { { arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 90.0 );
    c->SetDriving( false );

    // Widen the arc to a half turn (0 -> 180 deg).
    arc->SetArcGeometry( { 10 * MM, 0 }, { 0, 10 * MM }, { -10 * MM, 0 } );

    std::vector<BOARD_ITEM*> staged;
    ReSolveShapeClusters( &board, { arc }, nullptr, [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    BOOST_REQUIRE( c->GetValue().has_value() );
    BOOST_CHECK_CLOSE( *c->GetValue(), 180.0, 0.5 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), c ) != staged.end() );
}


BOOST_AUTO_TEST_SUITE_END()
