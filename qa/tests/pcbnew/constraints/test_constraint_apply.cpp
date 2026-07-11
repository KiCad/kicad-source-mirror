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
 * Tests ApplyConstraintImmediately (issue #2329 UI): creating a constraint solves its cluster on
 * the spot, pinning the first member and moving the rest so the geometry satisfies the relation.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>

#include <board.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


BOOST_AUTO_TEST_SUITE( ConstraintApply )


BOOST_AUTO_TEST_CASE( CoincidentSnapsSecondPointToFirst )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( board, { 11 * MM, 1 * MM }, { 20 * MM, 0 } );

    // Bind a's END to b's START; pinning a's END should pull b's START onto it.
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                       { { a->m_Uuid, CONSTRAINT_ANCHOR::END },
                                         { b->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( a->GetEnd().x, 10 * MM );   // pinned, unmoved
    BOOST_CHECK_EQUAL( a->GetEnd().y, 0 );
    BOOST_CHECK_EQUAL( b->GetStart().x, 10 * MM ); // pulled onto a's END
    BOOST_CHECK_EQUAL( b->GetStart().y, 0 );
}


BOOST_AUTO_TEST_CASE( SingleShapeFixedLengthStagesThePinnedShape )
{
    BOARD board;

    // A 12mm segment driven to 8mm: pinning START keeps it put, END moves in. The pinned shape is
    // the only shape, so it must be reported as modified (else the move would not be committed).
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 12 * MM, 0 } );
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                       { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } },
                                       8.0 * MM );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_TEST( ( std::find( modified.begin(), modified.end(), seg ) != modified.end() ) );
    BOOST_CHECK_EQUAL( seg->GetStart().x, 0 );
    BOOST_CHECK_EQUAL( seg->GetEnd().x, 8 * MM );
}


BOOST_AUTO_TEST_CASE( CircleFirstMemberPinsCenterAndSolves )
{
    BOARD board;

    // Concentric circles authored as WHOLE members: WHOLE must pin the circle's CENTER (its only
    // anchor), not a non-existent START, or the solve silently does nothing.
    PCB_SHAPE* a = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* b = addCircle( board, { 3 * MM, 0 }, 2 * MM );
    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::CONCENTRIC,
                                       { { a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                                         { b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, c, &modified );

    BOOST_TEST( diag.solved );
    BOOST_CHECK_EQUAL( a->GetCenter().x, 0 );       // pinned
    BOOST_CHECK_EQUAL( b->GetCenter().x, 0 );       // moved concentric with a
    BOOST_CHECK_EQUAL( b->GetCenter().y, 0 );
}


BOOST_AUTO_TEST_CASE( FailedOrEmptyClusterLeavesGeometry )
{
    BOARD board;

    // A constraint with no members cannot pin or solve anything.
    PCB_CONSTRAINT empty( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );

    std::vector<PCB_SHAPE*> modified;
    CONSTRAINT_DIAGNOSIS    diag = ApplyConstraintImmediately( &board, &empty, &modified );

    BOOST_TEST( !diag.solved );
    BOOST_TEST( modified.empty() );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( ConstraintStateSummaryTests )


BOOST_AUTO_TEST_CASE( SummaryReflectsState )
{
    bool over = true;

    // No constrained shapes.
    BOOST_CHECK( ConstraintStateSummary( {}, &over ).Contains( "No geometric constraints" ) );
    BOOST_TEST( !over );

    // Fully constrained: shapes present, zero free DOF.
    BOARD_CONSTRAINT_DIAGNOSTICS well;
    well.shapeStates[KIID()] = CONSTRAINT_STATE::WELL_CONSTRAINED;
    well.totalFreeDof = 0;
    BOOST_CHECK( ConstraintStateSummary( well, &over ).Contains( "fully constrained" ) );
    BOOST_TEST( !over );

    // Under-constrained: free DOF reported in the text.
    BOARD_CONSTRAINT_DIAGNOSTICS under;
    under.shapeStates[KIID()] = CONSTRAINT_STATE::UNDER_CONSTRAINED;
    under.totalFreeDof = 3;
    wxString underText = ConstraintStateSummary( under, &over );
    BOOST_CHECK( underText.Contains( "under-constrained" ) );
    BOOST_CHECK( underText.Contains( "3" ) );
    BOOST_TEST( !over );

    // Conflicting and errored both flag the over-constrained warning.
    BOARD_CONSTRAINT_DIAGNOSTICS conflicting;
    conflicting.conflicting.push_back( KIID() );
    BOOST_CHECK( ConstraintStateSummary( conflicting, &over ).Contains( "over-constrained" ) );
    BOOST_TEST( over );

    BOARD_CONSTRAINT_DIAGNOSTICS errored;
    errored.errored.push_back( KIID() );
    ConstraintStateSummary( errored, &over );
    BOOST_TEST( over );
}


BOOST_AUTO_TEST_SUITE_END()
