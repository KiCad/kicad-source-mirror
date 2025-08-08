/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_commit.h>
#include <pcb_shape.h>
#include <pcb_group.h>

BOOST_AUTO_TEST_SUITE( BoardCommit )

BOOST_AUTO_TEST_CASE( RecursesThroughGroups )
{
    BOARD board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL *dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );
    BOARD_COMMIT commit( dummyTool );

    PCB_SHAPE s1( nullptr, SHAPE_T::SEGMENT );
    PCB_SHAPE s2( nullptr, SHAPE_T::SEGMENT );
    PCB_GROUP group( nullptr );
    group.AddItem( &s1 );
    group.AddItem( &s2 );

    commit.Stage( &group, CHT_MODIFY, nullptr, RECURSE_MODE::RECURSE );

    BOOST_CHECK_EQUAL( commit.GetStatus( &s1 ), CHT_MODIFY );
    BOOST_CHECK_EQUAL( commit.GetStatus( &s2 ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( MakeImageCreatesTransientCopy )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    EDA_ITEM* copy = BOARD_COMMIT::MakeImage( &shape );

    BOOST_REQUIRE( copy );
    BOOST_CHECK( copy != &shape );
    BOOST_CHECK( copy->HasFlag( UR_TRANSIENT ) );

    delete copy;
}

BOOST_AUTO_TEST_CASE( ReturnsBoardFromManager )
{
    BOARD board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    BOARD_COMMIT commit( dummyTool );

    BOOST_CHECK_EQUAL( commit.GetBoard(), &board );
}

BOOST_AUTO_TEST_SUITE_END()

