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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <base_units.h>
#include <line_ending.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <richio.h>

#include <string>
#include <vector>


BOOST_AUTO_TEST_SUITE( LineEnding )


static std::string formatLineEnding( LINE_ENDING_STYLE aStyle, int aLength = 0,
                                     int aWidth = 0 )
{
    LINE_ENDING ending( aStyle, aLength, aWidth );

    STRING_FORMATTER formatter;
    ending.Format( &formatter, pcbIUScale, "start_shape" );

    return formatter.GetString();
}


static void checkDoesNotWriteFill( const std::string& aFormatted )
{
    BOOST_CHECK_EQUAL( aFormatted.find( "(fill" ), std::string::npos );
}


static void checkVectorEqual( const VECTOR2I& aActual, const VECTOR2I& aExpected )
{
    BOOST_CHECK_EQUAL( aActual.x, aExpected.x );
    BOOST_CHECK_EQUAL( aActual.y, aExpected.y );
}


static void checkEffectiveSize( int aLength, int aWidth, const VECTOR2I& aExpected )
{
    const int lineWidth = 100;

    LINE_ENDING ending( LINE_ENDING_STYLE::ARROW, aLength, aWidth );

    checkVectorEqual( ending.GetEffectiveSize( lineWidth ), aExpected );
}


static void checkShortenDepth( LINE_ENDING_STYLE aStyle, int aLength, int aWidth,
                               int aStrokeWidth, int aExpected )
{
    const int lineWidth = 100;

    LINE_ENDING ending( aStyle, aLength, aWidth, aStrokeWidth );

    BOOST_CHECK_EQUAL( ending.GetShortenDepth( lineWidth ), aExpected );
}


static void checkCurveOrientationDepth( LINE_ENDING_STYLE aStyle, int aLength, int aWidth,
                                        int aStrokeWidth, int aExpected )
{
    const int lineWidth = 100;

    LINE_ENDING ending( aStyle, aLength, aWidth, aStrokeWidth );

    BOOST_CHECK_EQUAL( ending.GetCurveOrientationDepth( lineWidth ), aExpected );
}


BOOST_AUTO_TEST_CASE( FormatDoesNotWriteFill )
{
    const std::string closedArrow = formatLineEnding( LINE_ENDING_STYLE::ARROW );
    const std::string openArrow = formatLineEnding( LINE_ENDING_STYLE::ARROW_OPEN );
    const std::string circle = formatLineEnding( LINE_ENDING_STYLE::CIRCLE );
    const std::string square = formatLineEnding( LINE_ENDING_STYLE::SQUARE );

    BOOST_CHECK_EQUAL( closedArrow, "(start_shape arrow)" );
    BOOST_CHECK_EQUAL( openArrow, "(start_shape arrow_open)" );
    BOOST_CHECK_EQUAL( circle, "(start_shape circle)" );
    BOOST_CHECK_EQUAL( square, "(start_shape square)" );

    checkDoesNotWriteFill( closedArrow );
    checkDoesNotWriteFill( openArrow );
    checkDoesNotWriteFill( circle );
    checkDoesNotWriteFill( square );
}


BOOST_AUTO_TEST_CASE( FormatPreservesExplicitEqualLengthWidth )
{
    const std::string explicitSquare = formatLineEnding( LINE_ENDING_STYLE::SQUARE,
                                                         pcbIUScale.mmToIU( 2.0 ),
                                                         pcbIUScale.mmToIU( 2.0 ) );

    BOOST_CHECK_NE( explicitSquare.find( "(length " ), std::string::npos );
    BOOST_CHECK_NE( explicitSquare.find( "(width " ), std::string::npos );

    const std::string autoWidthArrow = formatLineEnding( LINE_ENDING_STYLE::ARROW,
                                                         pcbIUScale.mmToIU( 2.0 ), 0 );

    BOOST_CHECK_NE( autoWidthArrow.find( "(length " ), std::string::npos );
    BOOST_CHECK_EQUAL( autoWidthArrow.find( "(width " ), std::string::npos );
}


BOOST_AUTO_TEST_CASE( GetEffectiveSizeUsesConfiguredAndAutoAxes )
{
    checkEffectiveSize( 700, 300, { 700, 300 } );
    checkEffectiveSize( 700, 0, { 700, 700 } );
    checkEffectiveSize( 0, 300, { 300, 300 } );
    checkEffectiveSize( 0, 0, { 500, 500 } );
}


BOOST_AUTO_TEST_CASE( GetShapesProducesStyleGeometry )
{
    const VECTOR2I  endpoint( 1000, 2000 );
    const EDA_ANGLE tangent( 0.0, DEGREES_T );
    std::vector<VECTOR2I> polygon;

    LINE_ENDING none( LINE_ENDING_STYLE::NONE, 300, 200 );
    none.GetShapes( endpoint, tangent, 100, polygon );
    BOOST_CHECK( polygon.empty() );

    LINE_ENDING arrow( LINE_ENDING_STYLE::ARROW, 300, 200 );
    arrow.GetShapes( endpoint, tangent, 100, polygon );
    BOOST_REQUIRE_EQUAL( polygon.size(), 4 );
    checkVectorEqual( polygon[0], { 1000, 2000 } );
    checkVectorEqual( polygon[1], { 700, 2100 } );
    checkVectorEqual( polygon[2], { 700, 1900 } );
    checkVectorEqual( polygon[3], { 1000, 2000 } );

    LINE_ENDING openArrow( LINE_ENDING_STYLE::ARROW_OPEN, 300, 200 );
    openArrow.GetShapes( endpoint, tangent, 100, polygon );
    BOOST_REQUIRE_EQUAL( polygon.size(), 3 );
    checkVectorEqual( polygon[0], { 700, 1900 } );
    checkVectorEqual( polygon[1], { 1000, 2000 } );
    checkVectorEqual( polygon[2], { 700, 2100 } );

    LINE_ENDING square( LINE_ENDING_STYLE::SQUARE, 400, 200 );
    square.GetShapes( endpoint, tangent, 100, polygon );
    BOOST_REQUIRE_EQUAL( polygon.size(), 5 );
    checkVectorEqual( polygon[0], { 1200, 2100 } );
    checkVectorEqual( polygon[1], { 1200, 1900 } );
    checkVectorEqual( polygon[2], { 800, 1900 } );
    checkVectorEqual( polygon[3], { 800, 2100 } );
    checkVectorEqual( polygon[4], { 1200, 2100 } );

    LINE_ENDING circle( LINE_ENDING_STYLE::CIRCLE, 400, 200 );
    circle.GetShapes( endpoint, tangent, 100, polygon );
    BOOST_REQUIRE_EQUAL( polygon.size(), 33 );
    checkVectorEqual( polygon.front(), { 1200, 2000 } );
    checkVectorEqual( polygon.back(), { 1200, 2000 } );
}


BOOST_AUTO_TEST_CASE( OpenArrowOrientationUsesVisualLength )
{
    checkCurveOrientationDepth( LINE_ENDING_STYLE::NONE, 800, 300, 0, 0 );

    checkCurveOrientationDepth( LINE_ENDING_STYLE::ARROW, 800, 300, 0, 800 );
    checkCurveOrientationDepth( LINE_ENDING_STYLE::CIRCLE, 800, 300, 0, 400 );
    checkCurveOrientationDepth( LINE_ENDING_STYLE::SQUARE, 800, 300, 0, 400 );

    checkShortenDepth( LINE_ENDING_STYLE::ARROW_OPEN, 800, 300, 0, 0 );
    checkCurveOrientationDepth( LINE_ENDING_STYLE::ARROW_OPEN, 800, 300, 0, 800 );
    checkCurveOrientationDepth( LINE_ENDING_STYLE::ARROW_OPEN, 0, 0, 0, 500 );
}


BOOST_AUTO_TEST_CASE( GetShortenDepthUsesEndpointPolicy )
{
    checkShortenDepth( LINE_ENDING_STYLE::NONE, 800, 300, 0, 0 );

    // Configured sizes: arrow tip consumes the full length, centered shapes consume half.
    checkShortenDepth( LINE_ENDING_STYLE::ARROW, 800, 300, 0, 800 );
    checkShortenDepth( LINE_ENDING_STYLE::ARROW_OPEN, 800, 300, 0, 0 );
    checkShortenDepth( LINE_ENDING_STYLE::CIRCLE, 800, 300, 0, 400 );
    checkShortenDepth( LINE_ENDING_STYLE::SQUARE, 800, 300, 0, 400 );

    // Auto sizes use DEFAULT_RATIO_LENGTH against the parent line width.
    checkShortenDepth( LINE_ENDING_STYLE::ARROW, 0, 0, 0, 500 );
    checkShortenDepth( LINE_ENDING_STYLE::CIRCLE, 0, 0, 0, 250 );
    checkShortenDepth( LINE_ENDING_STYLE::SQUARE, 0, 0, 0, 250 );

    // Stroked outlines reduce the line-body shortening by half the outline width.
    checkShortenDepth( LINE_ENDING_STYLE::ARROW, 800, 300, 100, 750 );
    checkShortenDepth( LINE_ENDING_STYLE::CIRCLE, 800, 300, 100, 350 );
    checkShortenDepth( LINE_ENDING_STYLE::SQUARE, 100, 300, 300, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
