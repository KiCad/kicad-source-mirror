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
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <pcb_shape.h>
#include <settings/settings_manager.h>
#include <trigo.h>

BOOST_AUTO_TEST_SUITE( Issue25650 )


BOOST_AUTO_TEST_CASE( CollinearArcLoadsAsSegment )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "issue25650/issue25650", board );

    BOOST_REQUIRE_EQUAL( board->Drawings().size(), 1u );

    PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( *board->Drawings().begin() );

    BOOST_CHECK( shape->GetShape() == SHAPE_T::SEGMENT );
    BOOST_CHECK_LT( shape->GetBoundingBox().GetHeight(), 1'000'000 );
}


BOOST_AUTO_TEST_CASE( ShallowArcIsNotStraight )
{
    // Radius of about 1 m puts the center in range, so the bow must survive
    BOOST_CHECK( !IsStraightArc( VECTOR2I( 0, 0 ), VECTOR2I( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 5 ) ),
                                 VECTOR2I( pcbIUScale.mmToIU( 200 ), 0 ) ) );

    // Radius of about 5 m puts the center out of range
    BOOST_CHECK( IsStraightArc( VECTOR2I( 0, 0 ), VECTOR2I( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 1 ) ),
                                VECTOR2I( pcbIUScale.mmToIU( 200 ), 0 ) ) );
    BOOST_CHECK( IsStraightArc( VECTOR2I( 0, 0 ), VECTOR2I( 50, 50 ), VECTOR2I( 100, 100 ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
