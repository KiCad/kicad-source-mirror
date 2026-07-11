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

#include <algorithm>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <collectors.h>
#include <constraints/pcb_constraint.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverExclusions )


static bool listContains( const std::vector<KICAD_T>& aList, KICAD_T aType )
{
    return std::find( aList.begin(), aList.end(), aType ) != aList.end();
}


BOOST_AUTO_TEST_CASE( NotGroupable )
{
    PCB_CONSTRAINT c( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );

    // A constraint references items but is not itself a groupable element.
    BOOST_CHECK( !c.IsGroupableType() );
}


BOOST_AUTO_TEST_CASE( NotInCollectorScanLists )
{
    // Selection and editing flow through these scan lists.  A constraint must appear in
    // none of them, otherwise it would become selectable/editable.
    BOOST_CHECK( !listContains( GENERAL_COLLECTOR::AllBoardItems, PCB_CONSTRAINT_T ) );
    BOOST_CHECK( !listContains( GENERAL_COLLECTOR::BoardLevelItems, PCB_CONSTRAINT_T ) );
    BOOST_CHECK( !listContains( GENERAL_COLLECTOR::FootprintItems, PCB_CONSTRAINT_T ) );
}


BOOST_AUTO_TEST_CASE( VisitWithCollectorListsSkipsConstraint )
{
    BOARD board;

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::PARALLEL );
    board.Add( c );

    auto countConstraintHits =
            [&]( const std::vector<KICAD_T>& aTypes )
            {
                int hits = 0;

                board.Visit(
                        [&]( EDA_ITEM* aItem, void* ) -> INSPECT_RESULT
                        {
                            if( aItem == c )
                                hits++;

                            return INSPECT_RESULT::CONTINUE;
                        },
                        nullptr, aTypes );

                return hits;
            };

    // The production collector scan list never visits the constraint...
    BOOST_CHECK_EQUAL( countConstraintHits( GENERAL_COLLECTOR::AllBoardItems ), 0 );

    // ...but it is genuinely owned and reachable when explicitly scanned for.
    BOOST_CHECK_EQUAL( countConstraintHits( { PCB_CONSTRAINT_T } ), 1 );
}


BOOST_AUTO_TEST_SUITE_END()
