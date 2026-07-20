/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <geometry/roundrect.h>
#include <geometry/shape_poly_set.h>

BOOST_AUTO_TEST_SUITE( Roundrect )

// The polygon must fit its rectangle. One maxError of slack covers the arc
// chords. Catches the maxed-radius circle coming out too big (#24623).
static void CheckPolygonFitsRect( int aWidth, int aHeight, int aRadius )
{
    const int      maxError = 100;
    const VECTOR2I pos( 1000, 2000 );

    ROUNDRECT rr( SHAPE_RECT( pos, aWidth, aHeight ), aRadius );

    SHAPE_POLY_SET poly;
    rr.TransformToPolygon( poly, maxError );

    BOOST_REQUIRE_EQUAL( poly.OutlineCount(), 1 );

    const BOX2I bbox = poly.BBox();
    const int   tol = maxError;

    BOOST_CHECK_LE( std::abs( bbox.GetLeft() - pos.x ), tol );
    BOOST_CHECK_LE( std::abs( bbox.GetTop() - pos.y ), tol );
    BOOST_CHECK_LE( std::abs( bbox.GetWidth() - aWidth ), tol );
    BOOST_CHECK_LE( std::abs( bbox.GetHeight() - aHeight ), tol );
}

// Radius is half of both sides, so it becomes a circle (#24623).
BOOST_AUTO_TEST_CASE( CircleFromSquare )
{
    CheckPolygonFitsRect( 10000000, 10000000, 5000000 );
}

// Radius maxed on the height only, giving a horizontal oval.
BOOST_AUTO_TEST_CASE( OvalWide )
{
    CheckPolygonFitsRect( 20000000, 10000000, 5000000 );
}

// Radius maxed on the width only, giving a vertical oval.
BOOST_AUTO_TEST_CASE( OvalTall )
{
    CheckPolygonFitsRect( 10000000, 20000000, 5000000 );
}

// Ordinary roundrect, corners below the max radius.
BOOST_AUTO_TEST_CASE( NormalRoundrect )
{
    CheckPolygonFitsRect( 20000000, 10000000, 2000000 );
}

// Zero radius is a plain rectangle.
BOOST_AUTO_TEST_CASE( PlainRectangle )
{
    CheckPolygonFitsRect( 20000000, 10000000, 0 );
}

// Zero radius corners always come out TL TR BR BL regardless of orientation
// pcbnew VERTEX constraints rely on this order reorder means silent rebind
BOOST_AUTO_TEST_CASE( PlainRectangleCornerOrder )
{
    const VECTOR2I pos( 1000, 2000 );
    const int      w = 20000000;
    const int      h = 10000000;

    // Normalized rect plus negative extent variant hits normalization recursion
    for( const SHAPE_RECT& rect : { SHAPE_RECT( pos, w, h ),
                                    SHAPE_RECT( pos + VECTOR2I( w, h ), -w, -h ) } )
    {
        ROUNDRECT rr( rect, 0 );

        SHAPE_POLY_SET poly;
        rr.TransformToPolygon( poly, 100 );

        BOOST_REQUIRE_EQUAL( poly.OutlineCount(), 1 );

        const SHAPE_LINE_CHAIN& outline = poly.COutline( 0 );

        BOOST_REQUIRE_EQUAL( outline.PointCount(), 4 );
        BOOST_CHECK_EQUAL( outline.CPoint( 0 ), pos );
        BOOST_CHECK_EQUAL( outline.CPoint( 1 ), pos + VECTOR2I( w, 0 ) );
        BOOST_CHECK_EQUAL( outline.CPoint( 2 ), pos + VECTOR2I( w, h ) );
        BOOST_CHECK_EQUAL( outline.CPoint( 3 ), pos + VECTOR2I( 0, h ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
