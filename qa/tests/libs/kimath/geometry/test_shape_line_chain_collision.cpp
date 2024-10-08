/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>

#include "fixtures_geometry.h"


BOOST_AUTO_TEST_SUITE( SHAPE_LINE_CHAIN_COLLIDE_TEST )

BOOST_AUTO_TEST_CASE( Collide_LineToLine )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 5 ) );
    lineB.Append( VECTOR2I( 5, -5 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_LineToArc )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN arcB;
    arcB.Append( SHAPE_ARC( VECTOR2I( 5, 5 ), VECTOR2I( 6, 4 ), VECTOR2I( 7, 0 ), 0 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &arcB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 7, 0 ), "Expected: " << VECTOR2I( 7, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_ArcToArc )
{
    SHAPE_LINE_CHAIN arcA;
    arcA.Append( SHAPE_ARC( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ), VECTOR2I( 5, 5 ), 0 ) );

    SHAPE_LINE_CHAIN arcB;
    arcB.Append( SHAPE_ARC( VECTOR2I( 5, 5 ), VECTOR2I( 5, -5 ), VECTOR2I( 10, 0 ), 0 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &arcA )->Collide( &arcB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 5, 5 ), "Expected: " << VECTOR2I( 5, 5 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_WithClearance )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 6 ) );
    lineB.Append( VECTOR2I( -5, 6 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 7, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_CHECK_MESSAGE( actual == 6, "Expected: " << 6 << " Actual: " << actual );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 0, 0 ), "Expected: " << VECTOR2I( 0, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_NoClearance )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 6 ) );
    lineB.Append( VECTOR2I( -5, 6 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 0, &actual, &location );

    BOOST_CHECK( !collided );
    BOOST_CHECK_MESSAGE( actual == 0, "Expected: " << 0 << " Actual: " << actual );
}

BOOST_AUTO_TEST_SUITE_END()