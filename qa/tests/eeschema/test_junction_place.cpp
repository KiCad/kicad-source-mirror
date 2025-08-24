/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_line.h>
#include <sch_junction.h>
#include <sch_screen.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <sch_commit.h>

BOOST_AUTO_TEST_SUITE( JunctionPlacement )

BOOST_AUTO_TEST_CASE( CrossingSegmentsKeepJunction )
{
    SCH_SCREEN screen;

    SCH_LINE* h1 = new SCH_LINE( VECTOR2I( -100, 0 ), LAYER_WIRE );
    h1->SetEndPoint( VECTOR2I( 100, 0 ) );
    screen.Append( h1 );

    SCH_LINE* v1 = new SCH_LINE( VECTOR2I( 0, -100 ), LAYER_WIRE );
    v1->SetEndPoint( VECTOR2I( 0, 100 ) );
    screen.Append( v1 );

    SCH_LINE* v2 = new SCH_LINE( VECTOR2I( 20, -100 ), LAYER_WIRE );
    v2->SetEndPoint( VECTOR2I( 20, 100 ) );
    screen.Append( v2 );

    TOOL_MANAGER mgr;
    SCH_COMMIT commit( &mgr );

    SCH_JUNCTION* j1 = new SCH_JUNCTION( VECTOR2I( 0, 0 ) );
    screen.Append( j1 );
    commit.Added( j1, &screen );

    SCHEMATIC schematic( nullptr);
    schematic.CleanUp( &commit, &screen );

    BOOST_CHECK( screen.GetItem( VECTOR2I( 0, 0 ), 0, SCH_JUNCTION_T ) != nullptr );

    SCH_COMMIT commit2( &mgr );

    SCH_LINE* h2 = h1->BreakAt( &commit2, VECTOR2I( 20, 0 ) );
    screen.Append( h2 );

    SCH_LINE* v3 = v2->BreakAt( &commit2, VECTOR2I( 20, 0 ) );
    screen.Append( v3 );

    SCH_JUNCTION* j2 = new SCH_JUNCTION( VECTOR2I( 20, 0 ) );
    screen.Append( j2 );
    commit2.Added( j2, &screen );

    schematic.CleanUp( &commit2, &screen );

    BOOST_CHECK( screen.IsExplicitJunction( VECTOR2I( 20, 0 ) ) );
    BOOST_CHECK_EQUAL( screen.GetBusesAndWires( VECTOR2I( 20, 0 ), false ).size(), 4 );
}

BOOST_AUTO_TEST_SUITE_END()

