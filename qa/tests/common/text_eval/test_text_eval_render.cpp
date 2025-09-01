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

/**
 * @file
 * Test rendering helper functions with expression evaluation.
 */

#include <boost/test/unit_test.hpp>
#include <gr_text.h>
#include <font/font.h>
#include <math/util.h>

BOOST_AUTO_TEST_SUITE( TextEvalRender )

BOOST_AUTO_TEST_CASE( GrTextWidthEval )
{
    KIFONT::FONT* font = KIFONT::FONT::GetFont();
    VECTOR2I size( 100, 100 );
    int thickness = 1;
    const KIFONT::METRICS& metrics = KIFONT::METRICS::Default();

    int widthExpr = GRTextWidth( wxS( "@{1+1}" ), font, size, thickness, false, false, metrics );
    int widthExpected = KiROUND( font->StringBoundaryLimits( wxS( "2" ), size, thickness, false,
                                                             false, metrics ).x );
    int widthRaw = KiROUND( font->StringBoundaryLimits( wxS( "@{1+1}" ), size, thickness, false,
                                                        false, metrics ).x );

    BOOST_CHECK_EQUAL( widthExpr, widthExpected );
    BOOST_CHECK( widthExpr != widthRaw );
}

BOOST_AUTO_TEST_SUITE_END()

