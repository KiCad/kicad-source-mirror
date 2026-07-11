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
#include <constraints/pcb_constraint.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverCommit )


struct COMMIT_FIXTURE
{
    BOARD                board;
    TOOL_MANAGER         mgr;
    KI_TEST::DUMMY_TOOL* tool;

    COMMIT_FIXTURE() : tool( new KI_TEST::DUMMY_TOOL() )
    {
        mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
        mgr.RegisterTool( tool );
    }
};


BOOST_FIXTURE_TEST_CASE( AddPushThenRemovePush, COMMIT_FIXTURE )
{
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::PARALLEL );
    KIID            id = c->m_Uuid;

    BOARD_COMMIT addCommit( tool );
    addCommit.Add( c );
    addCommit.Push( wxT( "add constraint" ) );

    BOOST_CHECK_EQUAL( board.Constraints().size(), 1 );
    BOOST_CHECK( board.ResolveItem( id, true ) == c );

    // Removing must not trip the CHT_REMOVE wxASSERT(false) default for an unhandled type.
    BOARD_COMMIT removeCommit( tool );
    removeCommit.Remove( c );
    removeCommit.Push( wxT( "remove constraint" ) );

    BOOST_CHECK( board.Constraints().empty() );
    BOOST_CHECK( board.ResolveItem( id, true ) == nullptr );
}


BOOST_FIXTURE_TEST_CASE( AddRevertLeavesNothing, COMMIT_FIXTURE )
{
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );

    BOARD_COMMIT commit( tool );
    commit.Add( c );

    // Revert before Push undoes the staged add and frees the constraint.
    commit.Revert();

    BOOST_CHECK( board.Constraints().empty() );
}


BOOST_FIXTURE_TEST_CASE( ModifyRevertRestoresValue, COMMIT_FIXTURE )
{
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    c->SetValue( 1.0 );

    BOARD_COMMIT addCommit( tool );
    addCommit.Add( c );
    addCommit.Push( wxT( "add" ) );

    BOARD_COMMIT modifyCommit( tool );
    modifyCommit.Modify( c );
    c->SetValue( 2.0 );
    modifyCommit.Revert();

    BOOST_REQUIRE( c->HasValue() );
    BOOST_CHECK_CLOSE( *c->GetValue(), 1.0, 1e-9 );
}


BOOST_AUTO_TEST_SUITE_END()
