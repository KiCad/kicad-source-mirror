/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <boost/test/unit_test.hpp>

#include <pcb_shape.h>
#include <fix_board_shape.h>

// Coordinates are in nanometres.  The weld cap is 0.01 mm (10000 nm).
// The chaining value is large so the ends always get paired and only the gap size decides.
static const int CHAINING = 3000000; // 3 mm, the Heal Shapes default

BOOST_AUTO_TEST_SUITE( ConnectBoardShapesTests )


// A hairline gap between two near-straight facets should weld the closest ends together.
BOOST_AUTO_TEST_CASE( HairlineCollinearGapWelds )
{
    PCB_SHAPE a( nullptr, SHAPE_T::SEGMENT );
    a.SetStart( VECTOR2I( 0, 0 ) );
    a.SetEnd( VECTOR2I( 1000000, 10000 ) );

    PCB_SHAPE b( nullptr, SHAPE_T::SEGMENT );
    b.SetStart( VECTOR2I( 1000100, 10100 ) ); // 141 nm diagonal gap from a.end
    b.SetEnd( VECTOR2I( 2000100, 20100 ) );   // same direction as a -> angle ~0

    std::vector<PCB_SHAPE*> shapes{ &a, &b };
    ConnectBoardShapes( shapes, CHAINING );

    BOOST_CHECK( a.GetEnd() == b.GetStart() );
    BOOST_CHECK_EQUAL( a.GetEnd().x, 1000050 );
    BOOST_CHECK_EQUAL( a.GetEnd().y, 10050 );
}


// A big gap is a real feature (a slot or neck), not rounding noise, so leave it alone.
BOOST_AUTO_TEST_CASE( LargeCollinearGapSurvives )
{
    PCB_SHAPE a( nullptr, SHAPE_T::SEGMENT );
    a.SetStart( VECTOR2I( 0, 0 ) );
    a.SetEnd( VECTOR2I( 1000000, 10000 ) );

    PCB_SHAPE b( nullptr, SHAPE_T::SEGMENT );
    b.SetStart( VECTOR2I( 2000000, 20000 ) ); // ~1 mm gap, well over the 0.01 mm cap
    b.SetEnd( VECTOR2I( 3000000, 30000 ) );

    std::vector<PCB_SHAPE*> shapes{ &a, &b };
    ConnectBoardShapes( shapes, CHAINING );

    BOOST_CHECK_EQUAL( a.GetEnd().x, 1000000 );
    BOOST_CHECK_EQUAL( a.GetEnd().y, 10000 );
    BOOST_CHECK_EQUAL( b.GetStart().x, 2000000 );
    BOOST_CHECK_EQUAL( b.GetStart().y, 20000 );
}


// A real corner should still extend the segments to their sharp crossing, not the midpoint.
BOOST_AUTO_TEST_CASE( RealCornerExtendsToIntersection )
{
    PCB_SHAPE a( nullptr, SHAPE_T::SEGMENT );
    a.SetStart( VECTOR2I( 0, 0 ) );
    a.SetEnd( VECTOR2I( 999000, 0 ) ); // horizontal, short of the corner

    PCB_SHAPE b( nullptr, SHAPE_T::SEGMENT );
    b.SetStart( VECTOR2I( 1000000, 1000 ) ); // vertical, short of the corner
    b.SetEnd( VECTOR2I( 1000000, 1000000 ) );

    std::vector<PCB_SHAPE*> shapes{ &a, &b };
    ConnectBoardShapes( shapes, CHAINING );

    // The two lines cross at (1000000, 0), not at the midpoint.
    BOOST_CHECK( a.GetEnd() == b.GetStart() );
    BOOST_CHECK_EQUAL( a.GetEnd().x, 1000000 );
    BOOST_CHECK_EQUAL( a.GetEnd().y, 0 );
}


// Right at the cap: just under 0.01 mm welds, just over is left alone.
BOOST_AUTO_TEST_CASE( WeldCapBoundary )
{
    {
        PCB_SHAPE a( nullptr, SHAPE_T::SEGMENT );
        a.SetStart( VECTOR2I( 0, 0 ) );
        a.SetEnd( VECTOR2I( 1000000, 0 ) );

        PCB_SHAPE b( nullptr, SHAPE_T::SEGMENT );
        b.SetStart( VECTOR2I( 1009000, 0 ) ); // 9 um gap, under the 10 um cap
        b.SetEnd( VECTOR2I( 2009000, 0 ) );

        std::vector<PCB_SHAPE*> shapes{ &a, &b };
        ConnectBoardShapes( shapes, CHAINING );

        BOOST_CHECK( a.GetEnd() == b.GetStart() );
    }

    {
        PCB_SHAPE a( nullptr, SHAPE_T::SEGMENT );
        a.SetStart( VECTOR2I( 0, 0 ) );
        a.SetEnd( VECTOR2I( 1000000, 0 ) );

        PCB_SHAPE b( nullptr, SHAPE_T::SEGMENT );
        b.SetStart( VECTOR2I( 1011000, 0 ) ); // 11 um gap, over the 10 um cap
        b.SetEnd( VECTOR2I( 2011000, 0 ) );

        std::vector<PCB_SHAPE*> shapes{ &a, &b };
        ConnectBoardShapes( shapes, CHAINING );

        BOOST_CHECK_EQUAL( a.GetEnd().x, 1000000 );
        BOOST_CHECK_EQUAL( b.GetStart().x, 1011000 );
    }
}


BOOST_AUTO_TEST_SUITE_END()
