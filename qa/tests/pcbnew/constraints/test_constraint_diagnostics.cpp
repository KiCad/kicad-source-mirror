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

#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <core/kicad_algo.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


BOOST_AUTO_TEST_SUITE( ConstraintSolverDiagnostics )


// Each cluster on the board maps to the correct per-shape verdict, and unconstrained shapes
// get no entry.
BOOST_AUTO_TEST_CASE( PerClusterVerdicts )
{
    BOARD board;

    // Cluster 1: a segment with both endpoints pinned -> well constrained (zero free DOF).
    PCB_SHAPE* wellSeg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { wellSeg->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { wellSeg->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    // Cluster 2: a segment with only a direction constraint -> under constrained.
    PCB_SHAPE* underSeg = addSegment( board, { 0, 20 * MM }, { 10 * MM, 22 * MM } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                   { { underSeg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // An unconstrained segment.
    PCB_SHAPE* loneSeg = addSegment( board, { 0, 40 * MM }, { 5 * MM, 40 * MM } );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_CHECK( diag.shapeStates[wellSeg->m_Uuid] == CONSTRAINT_STATE::WELL_CONSTRAINED );
    BOOST_CHECK( diag.shapeStates[underSeg->m_Uuid] == CONSTRAINT_STATE::UNDER_CONSTRAINED );
    BOOST_CHECK( diag.shapeStates.find( loneSeg->m_Uuid ) == diag.shapeStates.end() );

    // The under-constrained cluster contributes free DOF to the board total.
    BOOST_CHECK_GT( diag.totalFreeDof, 0 );
}


// Two contradictory fixed-length constraints on one segment are flagged over-constrained.
BOOST_AUTO_TEST_CASE( ConflictingClusterIsOverConstrained )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 10.0 * MM );
    addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 8.0 * MM );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    // The contradiction is surfaced: the shape is not reported as cleanly well-constrained.
    BOOST_CHECK( diag.shapeStates[seg->m_Uuid] == CONSTRAINT_STATE::OVER_CONSTRAINED );
    BOOST_CHECK( !diag.conflicting.empty() );
}


// A constraint referencing a deleted item is reported as errored (Zulip 2026-06-18 error state).
BOOST_AUTO_TEST_CASE( DanglingMemberIsErrored )
{
    BOARD board;

    PCB_SHAPE* a = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    c->AddMember( KIID(), CONSTRAINT_ANCHOR::START );   // references a non-existent item
    board.Add( c );

    KIID constraintId = c->m_Uuid;

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_REQUIRE_EQUAL( diag.errored.size(), 1 );
    BOOST_CHECK( diag.errored[0] == constraintId );
}


// A constraint whose member kind is incompatible with its type (a parallel referencing a circle,
// e.g. after a shape was changed to a circle) must not disable the whole cluster: it is reported
// errored while a valid constraint in the same cluster still takes effect.
BOOST_AUTO_TEST_CASE( IncompatibleKindConstraintIsErroredAndClusterStillSolves )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );
    PCB_SHAPE* circle = addCircle( board, { 0, 20 * MM }, 5 * MM );

    // Malformed: a parallel cannot map a circle onto a line, so it is unmappable.
    PCB_CONSTRAINT* bad = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::PARALLEL );
    bad->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    bad->AddMember( circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( bad );

    // A valid constraint in the same cluster (shares seg) must still be honoured.
    addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_CHECK( alg::contains( diag.errored, bad->m_Uuid ) );

    // The cluster was not disabled by the bad constraint: its shapes still get a verdict.
    BOOST_CHECK( diag.shapeStates.find( seg->m_Uuid ) != diag.shapeStates.end() );
}


// A circle has no endpoints, so a constraint anchored on a circle's START/END is unmappable and is
// reported errored -- it must never silently alias the circle's centre (which shares that slot).
BOOST_AUTO_TEST_CASE( CircleEndpointAnchorIsErrored )
{
    BOARD board;

    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* seg = addSegment( board, { 20 * MM, 0 }, { 30 * MM, 0 } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( circle->m_Uuid, CONSTRAINT_ANCHOR::START );   // circles have no START
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_CHECK( alg::contains( diag.errored, c->m_Uuid ) );
}


// Constraints owned by a footprint (footprint-editor scope) are diagnosed too, not only
// board-level ones.
BOOST_AUTO_TEST_CASE( FootprintScopedConstraintsAreDiagnosed )
{
    BOARD board;

    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );

    PCB_SHAPE* seg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    seg->SetStart( { 0, 0 } );
    seg->SetEnd( { 10 * MM, 2 * MM } );
    fp->Add( seg );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( fp, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    fp->Add( c );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    // The footprint's segment is reported (under-constrained: only a direction is fixed).
    BOOST_REQUIRE( diag.shapeStates.find( seg->m_Uuid ) != diag.shapeStates.end() );
    BOOST_CHECK( diag.shapeStates[seg->m_Uuid] == CONSTRAINT_STATE::UNDER_CONSTRAINED );
}


// A reference (non-driving) value that drifted from the geometry it measures is annotation, not a
// contradiction, so the residual pass must not paint it as conflicting.
BOOST_AUTO_TEST_CASE( DriftedReferenceValueIsNotConflicting )
{
    BOARD board;

    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                         { { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 5.0 * MM );
    len->SetDriving( false );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_CHECK( !alg::contains( diag.conflicting, len->m_Uuid ) );
}


// A contradiction on one segment of a multi-shape cluster still flags the constraints incident on
// that segment, and the diagnosis never moves the geometry.
BOOST_AUTO_TEST_CASE( ContradictionInMultiShapeClusterIsFlagged )
{
    BOARD board;

    PCB_SHAPE* doomed = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );
    PCB_SHAPE* bystander = addSegment( board, { 0, 20 * MM }, { 10 * MM, 20 * MM } );

    PCB_CONSTRAINT* h = addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                                       { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* v = addConstraint( board, PCB_CONSTRAINT_TYPE::VERTICAL,
                                       { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::EQUAL_LENGTH,
                   { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE },
                     { bystander->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );

    BOOST_CHECK( alg::contains( diag.conflicting, h->m_Uuid )
                 || alg::contains( diag.conflicting, v->m_Uuid ) );

    BOOST_CHECK_EQUAL( doomed->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( doomed->GetEnd(), VECTOR2I( 10 * MM, 2 * MM ) );
}


// A plain (non-stabilize) solve satisfies horizontal+vertical by collapsing the segment with zero
// residual, so only the collapse check can flag it.  The flags must land on the constraints
// incident on the collapsed segment; a satisfied constraint on a connected bystander and a
// reference measurement stay clean.
BOOST_AUTO_TEST_CASE( ConvergentCollapseFlagsOnlyIncidentConstraints )
{
    BOARD board;

    PCB_SHAPE* doomed = addSegment( board, { 0, 0 }, { 10 * MM, 2 * MM } );
    PCB_SHAPE* bystander = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* h = addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                                       { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );
    PCB_CONSTRAINT* v = addConstraint( board, PCB_CONSTRAINT_TYPE::VERTICAL,
                                       { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    // Joins the cluster at a shared endpoint without constraining the doomed segment's length.
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { doomed->m_Uuid, CONSTRAINT_ANCHOR::START },
                     { bystander->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    PCB_CONSTRAINT* bystanderOnly = addConstraint( board, PCB_CONSTRAINT_TYPE::HORIZONTAL,
                                                   { { bystander->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    PCB_CONSTRAINT* reference = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
                                               { { doomed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } }, 5.0 * MM );
    reference->SetDriving( false );

    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );
    std::vector<PCB_SHAPE*>      shapes{ doomed, bystander };

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( shapes, constraints ) );
    BOOST_REQUIRE( adapter.Solve() );

    CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();

    BOOST_CHECK( alg::contains( diag.conflicting, h->m_Uuid )
                 || alg::contains( diag.conflicting, v->m_Uuid ) );

    BOOST_CHECK( !alg::contains( diag.conflicting, bystanderOnly->m_Uuid ) );
    BOOST_CHECK( !alg::contains( diag.conflicting, reference->m_Uuid ) );
}


BOOST_AUTO_TEST_SUITE_END()
