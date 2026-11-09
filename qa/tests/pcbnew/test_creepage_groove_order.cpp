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
#include <array>
#include <board.h>
#include <pcb_shape.h>
#include <drc/drc_creepage_utils.h>

BOOST_AUTO_TEST_CASE( CreepageCrossingOrderUsesCorrectAxis )
{
    BOARD board;
    std::vector<std::unique_ptr<PCB_SHAPE>> owned;
    std::vector<BOARD_ITEM*> edges;

    for( int x : { 100, 500, 200, 600 } )
    {
        std::unique_ptr<PCB_SHAPE> edge = std::make_unique<PCB_SHAPE>( &board, SHAPE_T::SEGMENT );
        edge->SetStart( { x, 900 } );
        edge->SetEnd( { x, 1100 } );
        edges.push_back( edge.get() );
        owned.push_back( std::move( edge ) );
    }

    BOOST_CHECK( SegmentIntersectsBoard( { 0, 1000 }, { 1000, 1000 }, edges, {}, 200 ) );
    std::reverse( edges.begin(), edges.end() );
    BOOST_CHECK( SegmentIntersectsBoard( { 0, 1000 }, { 1000, 1000 }, edges, {}, 200 ) );
    BOOST_CHECK( !SegmentIntersectsBoard( { 0, 1000 }, { 1000, 1000 }, edges, {}, 99 ) );
}
