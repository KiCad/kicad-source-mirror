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
 * @file test_gal_xor_mode.cpp
 * Tests for GAL XOR/difference mode (issue #18983).
 *
 * These tests verify that CAIRO_OPERATOR_DIFFERENCE correctly computes
 * |src - dst| for XOR-style layer comparison in gerbview.
 */

#include <boost/test/unit_test.hpp>

#include <gal/color4d.h>
#include <cairo.h>
#include <cmath>

using namespace KIGFX;


BOOST_AUTO_TEST_SUITE( GalXorMode )


/**
 * Helper to get a pixel color from a Cairo ARGB32 surface
 */
static COLOR4D getPixelColor( cairo_surface_t* surface, int x, int y )
{
    cairo_surface_flush( surface );

    int stride = cairo_image_surface_get_stride( surface );
    unsigned char* data = cairo_image_surface_get_data( surface );

    unsigned char* pixel = data + y * stride + x * 4;

    // Cairo ARGB32 format: B, G, R, A (little endian)
    double b = pixel[0] / 255.0;
    double g = pixel[1] / 255.0;
    double r = pixel[2] / 255.0;
    double a = pixel[3] / 255.0;

    return COLOR4D( r, g, b, a );
}


/**
 * Helper to check if two colors are approximately equal
 */
static bool colorsApproxEqual( const COLOR4D& a, const COLOR4D& b, double tolerance = 0.02 )
{
    return std::abs( a.r - b.r ) < tolerance &&
           std::abs( a.g - b.g ) < tolerance &&
           std::abs( a.b - b.b ) < tolerance;
}


/**
 * Test that CAIRO_OPERATOR_DIFFERENCE correctly computes |src - dst|
 * for identical overlapping colors (should result in black).
 */
BOOST_AUTO_TEST_CASE( CairoDifferenceIdenticalColors )
{
    const int width = 100;
    const int height = 100;

    // Create destination surface (red background)
    cairo_surface_t* destSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* destCtx = cairo_create( destSurface );

    // Fill with solid red
    cairo_set_source_rgba( destCtx, 1.0, 0.0, 0.0, 1.0 );
    cairo_paint( destCtx );

    // Create source surface (also red)
    cairo_surface_t* srcSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* srcCtx = cairo_create( srcSurface );

    // Fill with solid red (same as destination)
    cairo_set_source_rgba( srcCtx, 1.0, 0.0, 0.0, 1.0 );
    cairo_paint( srcCtx );

    // Apply CAIRO_OPERATOR_DIFFERENCE: |src - dst| = |red - red| = black
    cairo_set_operator( destCtx, CAIRO_OPERATOR_DIFFERENCE );
    cairo_set_source_surface( destCtx, srcSurface, 0, 0 );
    cairo_paint( destCtx );

    // Check that the center pixel is black (identical colors cancel out)
    COLOR4D result = getPixelColor( destSurface, 50, 50 );
    COLOR4D expected( 0.0, 0.0, 0.0, 1.0 );

    BOOST_CHECK_MESSAGE( colorsApproxEqual( result, expected ),
                         "Identical colors should cancel to black. Got: ("
                         << result.r << ", " << result.g << ", " << result.b << ")" );

    // Cleanup
    cairo_destroy( srcCtx );
    cairo_surface_destroy( srcSurface );
    cairo_destroy( destCtx );
    cairo_surface_destroy( destSurface );
}


/**
 * Test that CAIRO_OPERATOR_DIFFERENCE shows the difference for different colors.
 */
BOOST_AUTO_TEST_CASE( CairoDifferenceDifferentColors )
{
    const int width = 100;
    const int height = 100;

    // Create destination surface (red)
    cairo_surface_t* destSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* destCtx = cairo_create( destSurface );

    cairo_set_source_rgba( destCtx, 1.0, 0.0, 0.0, 1.0 );  // Red
    cairo_paint( destCtx );

    // Create source surface (green)
    cairo_surface_t* srcSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* srcCtx = cairo_create( srcSurface );

    cairo_set_source_rgba( srcCtx, 0.0, 1.0, 0.0, 1.0 );  // Green
    cairo_paint( srcCtx );

    // Apply CAIRO_OPERATOR_DIFFERENCE: |green - red| = (|0-1|, |1-0|, 0) = (1, 1, 0) = yellow
    cairo_set_operator( destCtx, CAIRO_OPERATOR_DIFFERENCE );
    cairo_set_source_surface( destCtx, srcSurface, 0, 0 );
    cairo_paint( destCtx );

    // Check that the result is yellow (difference of red and green)
    COLOR4D result = getPixelColor( destSurface, 50, 50 );
    COLOR4D expected( 1.0, 1.0, 0.0, 1.0 );  // Yellow

    BOOST_CHECK_MESSAGE( colorsApproxEqual( result, expected ),
                         "Red - Green difference should be yellow. Got: ("
                         << result.r << ", " << result.g << ", " << result.b << ")" );

    // Cleanup
    cairo_destroy( srcCtx );
    cairo_surface_destroy( srcSurface );
    cairo_destroy( destCtx );
    cairo_surface_destroy( destSurface );
}


/**
 * Test that non-overlapping content (source on empty destination) shows the source.
 */
BOOST_AUTO_TEST_CASE( CairoDifferenceNonOverlapping )
{
    const int width = 100;
    const int height = 100;

    // Create destination surface (transparent/black)
    cairo_surface_t* destSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* destCtx = cairo_create( destSurface );

    // Clear to black (no content)
    cairo_set_source_rgba( destCtx, 0.0, 0.0, 0.0, 1.0 );
    cairo_paint( destCtx );

    // Create source surface (blue)
    cairo_surface_t* srcSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* srcCtx = cairo_create( srcSurface );

    cairo_set_source_rgba( srcCtx, 0.0, 0.0, 1.0, 1.0 );  // Blue
    cairo_paint( srcCtx );

    // Apply CAIRO_OPERATOR_DIFFERENCE: |blue - black| = blue
    cairo_set_operator( destCtx, CAIRO_OPERATOR_DIFFERENCE );
    cairo_set_source_surface( destCtx, srcSurface, 0, 0 );
    cairo_paint( destCtx );

    // Check that the result is blue (source on empty shows source)
    COLOR4D result = getPixelColor( destSurface, 50, 50 );
    COLOR4D expected( 0.0, 0.0, 1.0, 1.0 );  // Blue

    BOOST_CHECK_MESSAGE( colorsApproxEqual( result, expected ),
                         "Blue on black should show blue. Got: ("
                         << result.r << ", " << result.g << ", " << result.b << ")" );

    // Cleanup
    cairo_destroy( srcCtx );
    cairo_surface_destroy( srcSurface );
    cairo_destroy( destCtx );
    cairo_surface_destroy( destSurface );
}


/**
 * Test partial overlap scenario - simulating gerbview XOR mode with two layers.
 */
BOOST_AUTO_TEST_CASE( CairoDifferencePartialOverlap )
{
    const int width = 100;
    const int height = 100;

    // Create destination surface with a red rectangle on the left half
    cairo_surface_t* destSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* destCtx = cairo_create( destSurface );

    // Clear to black
    cairo_set_source_rgba( destCtx, 0.0, 0.0, 0.0, 1.0 );
    cairo_paint( destCtx );

    // Draw red rectangle on left half
    cairo_set_source_rgba( destCtx, 1.0, 0.0, 0.0, 1.0 );
    cairo_rectangle( destCtx, 0, 0, 50, 100 );
    cairo_fill( destCtx );

    // Create source surface with a red rectangle on the right half (partially overlapping)
    cairo_surface_t* srcSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, width, height );
    cairo_t* srcCtx = cairo_create( srcSurface );

    // Clear to black
    cairo_set_source_rgba( srcCtx, 0.0, 0.0, 0.0, 1.0 );
    cairo_paint( srcCtx );

    // Draw red rectangle on right half (overlaps with left at x=25-50)
    cairo_set_source_rgba( srcCtx, 1.0, 0.0, 0.0, 1.0 );
    cairo_rectangle( srcCtx, 25, 0, 50, 100 );
    cairo_fill( srcCtx );

    // Apply CAIRO_OPERATOR_DIFFERENCE
    cairo_set_operator( destCtx, CAIRO_OPERATOR_DIFFERENCE );
    cairo_set_source_surface( destCtx, srcSurface, 0, 0 );
    cairo_paint( destCtx );

    // Check three regions:
    // 1. Left-only (x=10): dest=red, src=black → |red-black| = red
    // 2. Overlap (x=37): dest=red, src=red → |red-red| = black
    // 3. Right-only (x=65): dest=black, src=red → |red-black| = red

    COLOR4D leftOnly = getPixelColor( destSurface, 10, 50 );
    COLOR4D overlap = getPixelColor( destSurface, 37, 50 );
    COLOR4D rightOnly = getPixelColor( destSurface, 65, 50 );

    COLOR4D expectedRed( 1.0, 0.0, 0.0, 1.0 );
    COLOR4D expectedBlack( 0.0, 0.0, 0.0, 1.0 );

    BOOST_CHECK_MESSAGE( colorsApproxEqual( leftOnly, expectedRed ),
                         "Left-only region should be red. Got: ("
                         << leftOnly.r << ", " << leftOnly.g << ", " << leftOnly.b << ")" );

    BOOST_CHECK_MESSAGE( colorsApproxEqual( overlap, expectedBlack ),
                         "Overlap region should be black (canceled). Got: ("
                         << overlap.r << ", " << overlap.g << ", " << overlap.b << ")" );

    BOOST_CHECK_MESSAGE( colorsApproxEqual( rightOnly, expectedRed ),
                         "Right-only region should be red. Got: ("
                         << rightOnly.r << ", " << rightOnly.g << ", " << rightOnly.b << ")" );

    // Cleanup
    cairo_destroy( srcCtx );
    cairo_surface_destroy( srcSurface );
    cairo_destroy( destCtx );
    cairo_surface_destroy( destSurface );
}


BOOST_AUTO_TEST_SUITE_END()
