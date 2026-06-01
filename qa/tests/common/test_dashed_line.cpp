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

/**
 * @file test_dashed_line.cpp
 * Tests that KIGFX::DrawDashedLine stays bounded on huge segments (issue #23824).
 */

#include <boost/test/unit_test.hpp>

#include <gal/graphics_abstraction_layer.h>
#include <gal/gal_display_options.h>
#include <geometry/seg.h>
#include <preview_items/item_drawing_utils.h>

using namespace KIGFX;

namespace
{

/// Minimal GAL that counts the line segments it is asked to draw.
class COUNTING_GAL : public GAL
{
public:
    COUNTING_GAL( GAL_DISPLAY_OPTIONS& aOptions ) : GAL( aOptions ) {}

    void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override
    {
        m_lineCount++;
    }

    unsigned m_lineCount = 0;
};

} // namespace


BOOST_AUTO_TEST_SUITE( DashedLine )


// A normally sized dashed line still produces a bounded run of dashes.
BOOST_AUTO_TEST_CASE( NormalLength )
{
    GAL_DISPLAY_OPTIONS opts;
    COUNTING_GAL        gal( opts );

    const SEG    seg( VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000000 ) );
    const double dashSize = 1000000;

    DrawDashedLine( gal, seg, dashSize );

    BOOST_CHECK_GT( gal.m_lineCount, 1u );
    BOOST_CHECK_LT( gal.m_lineCount, 200u );
}


// The #23824 regression. The old code looped length/dashCycle (~1.4e9) times here and hung.
BOOST_AUTO_TEST_CASE( PathologicallyLongLine )
{
    GAL_DISPLAY_OPTIONS opts;
    COUNTING_GAL        gal( opts );

    // ~2147 mm at 1 nm resolution, near the int32 coordinate limit.
    const SEG    seg( VECTOR2I( 0, 0 ), VECTOR2I( 0, 2100000000 ) );
    const double dashSize = 1;

    DrawDashedLine( gal, seg, dashSize );

    BOOST_CHECK_LE( gal.m_lineCount, 2u );
}


// A zero dash size must not divide by zero or loop forever.
BOOST_AUTO_TEST_CASE( ZeroDashSize )
{
    GAL_DISPLAY_OPTIONS opts;
    COUNTING_GAL        gal( opts );

    const SEG seg( VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000000 ) );

    DrawDashedLine( gal, seg, 0.0 );

    BOOST_CHECK_EQUAL( gal.m_lineCount, 1u );
}


BOOST_AUTO_TEST_SUITE_END()
