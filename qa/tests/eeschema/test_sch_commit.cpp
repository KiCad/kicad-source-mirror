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
#include <sch_commit.h>
#include <sch_group.h>
#include <sch_text.h>

BOOST_AUTO_TEST_SUITE( SchCommit )

BOOST_AUTO_TEST_CASE( RecursesThroughGroups )
{
    TOOL_MANAGER mgr;
    SCH_COMMIT commit( &mgr );

    SCH_TEXT t1;
    SCH_TEXT t2;
    SCH_GROUP group;
    group.AddItem( &t1 );
    group.AddItem( &t2 );

    commit.Stage( &group, CHT_MODIFY, nullptr, RECURSE_MODE::RECURSE );

    BOOST_CHECK_EQUAL( commit.GetStatus( &t1 ), CHT_MODIFY );
    BOOST_CHECK_EQUAL( commit.GetStatus( &t2 ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( ClearsSelectedByDragFlag )
{
    TOOL_MANAGER mgr;
    SCH_COMMIT commit( &mgr );

    SCH_TEXT text;
    text.SetFlags( SELECTED_BY_DRAG );
    text.SetSelected();

    commit.Stage( &text, CHT_MODIFY );

    BOOST_CHECK( text.IsSelected() );
    BOOST_CHECK_EQUAL( commit.GetStatus( &text ), CHT_MODIFY );
}

BOOST_AUTO_TEST_SUITE_END()

