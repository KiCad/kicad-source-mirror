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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include <base_units.h>
#include <eda_text.h>
#include <font/font.h>
#include <font/glyph_types.h>
#include <io/target3001/target3001_fonts.h>
#include <io/target3001/target3001_parser.h>


/// Centre of the last glyph of @a aText at @a aSize, measured from the stem of H
static double lastCentreFromStem( const wxString& aText, const VECTOR2I& aSize )
{
    auto strokes = [&]( const wxString& aStr )
    {
        std::vector<std::unique_ptr<KIFONT::GLYPH>> glyphs;

        KIFONT::FONT::GetFont()->GetTextAsGlyphs( nullptr, &glyphs, aStr, aSize, VECTOR2I( 0, 0 ), ANGLE_0, false,
                                                  VECTOR2I( 0, 0 ), 0 );

        return glyphs;
    };

    auto span = [&]( const KIFONT::GLYPH& aGlyph )
    {
        std::pair<double, double> result( std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest() );

        for( const std::vector<VECTOR2D>& stroke : static_cast<const KIFONT::STROKE_GLYPH&>( aGlyph ) )
        {
            for( const VECTOR2D& pt : stroke )
            {
                result.first = std::min( result.first, pt.x );
                result.second = std::max( result.second, pt.x );
            }
        }

        return result;
    };

    std::pair<double, double> last = span( *strokes( aText ).back() );

    return ( last.first + last.second ) / 2.0 - span( *strokes( wxS( "H" ) ).front() ).first;
}


BOOST_AUTO_TEST_SUITE( Target3001Fonts )


BOOST_AUTO_TEST_CASE( DinGlyphsFollowVendorAdvance )
{
    // Note in vorverst_vdc_v4.6.1_FET,Differenzverstaerker; the vendor PDF centres its last glyph 57.64 mm from the pen
    TARGET3001::ELEMENT elem;
    elem.text.font = "DIN-ISO-ANSI";
    elem.text.height = 3000000;
    elem.text.width = 1500000;
    elem.text.weight = 13;

    EDA_TEXT          text( pcbIUScale, wxS( "X5: 200: 1,51V; 201: +1,62V; 202: 1,65V; " ) );
    TARGET3001::FONTS fonts;

    fonts.Apply( &text, elem, 1 );

    double centre = lastCentreFromStem( text.GetText(), text.GetTextSize() );

    BOOST_CHECK_LT( std::abs( centre - pcbIUScale.mmToIU( 57.64 ) ), pcbIUScale.mmToIU( 1.0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
