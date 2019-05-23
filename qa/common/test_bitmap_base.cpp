/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Test suite for BITMAP_BASE
 */

#include <unit_test_utils/unit_test_utils.h>

// Code under test
#include <bitmap_base.h>

#include "wximage_test_utils.h"

#include <wx/mstream.h>


// Dummy PNG image 8x8, 4 tiles:
//
//   green, black,
//   red,   blue
static const std::vector<unsigned char> png_data_4tile = { //
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x02, 0x00, 0x00, 0x00, 0x4B, 0x6D, 0x29,
    0xDC, 0x00, 0x00, 0x00, 0x03, 0x73, 0x42, 0x49, 0x54, 0x08, 0x08, 0x08, 0xDB, 0xE1, 0x4F, 0xE0,
    0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC4, 0x00, 0x00, 0x0E, 0xC4,
    0x01, 0x95, 0x2B, 0x0E, 0x1B, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66,
    0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x77, 0x77, 0x77, 0x2E, 0x69, 0x6E, 0x6B, 0x73, 0x63, 0x61,
    0x70, 0x65, 0x2E, 0x6F, 0x72, 0x67, 0x9B, 0xEE, 0x3C, 0x1A, 0x00, 0x00, 0x00, 0x18, 0x49, 0x44,
    0x41, 0x54, 0x08, 0x5B, 0x63, 0x60, 0xF8, 0xCF, 0x80, 0x40, 0x28, 0x80, 0x7A, 0x12, 0xA8, 0xE2,
    0x48, 0x3C, 0xEA, 0x49, 0x00, 0x00, 0xF1, 0xA9, 0x2F, 0xD1, 0xB5, 0xC6, 0x95, 0xD0, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

///< 4tile is an 8x8 image
static const wxSize size_4tile{ 8, 8 };

static const KIGFX::COLOR4D col_red{ 1.0, 0.0, 0.0, 1.0 };
static const KIGFX::COLOR4D col_green{ 0.0, 1.0, 0.0, 1.0 };
static const KIGFX::COLOR4D col_blue{ 0.0, 0.0, 1.0, 1.0 };
static const KIGFX::COLOR4D col_black{ 0.0, 0.0, 0.0, 1.0 };


class TEST_BITMAP_BASE_FIXTURE
{
public:
    TEST_BITMAP_BASE_FIXTURE()
    {
        wxMemoryInputStream mis( png_data_4tile.data(), png_data_4tile.size() );

        bool ok = m_4tile.ReadImageFile( mis );

        // this is needed for most tests and can fail if the image handlers
        // are not initialised (e.g. with wxInitAllImageHandlers)
        BOOST_REQUIRE( ok );

        // use an easier scale for test purposes
        m_4tile.SetPixelScaleFactor( 2.0 );
        m_4tile.SetScale( 5.0 );
    }

    /*
     * Simple image of 4 coloured tiles
     */
    BITMAP_BASE m_4tile;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( BitmapBase, TEST_BITMAP_BASE_FIXTURE )


/**
 * Check basic properties of a populated image
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    BITMAP_BASE empty;
    // Const to test we can get this data from a const object
    const BITMAP_BASE& cempty = empty;

    BOOST_CHECK_EQUAL( cempty.GetImageData(), nullptr );
    BOOST_CHECK_EQUAL( cempty.GetPPI(), 300 );
    BOOST_CHECK_EQUAL( cempty.GetScale(), 1.0 );
    BOOST_CHECK_EQUAL( cempty.GetPixelScaleFactor(), 1000.0 / 300.0 );

    // can do this on an empty image
    empty.Rotate( true );
    empty.Mirror( true );
}


#ifdef HAVE_EXPECTED_FAILURES

BOOST_AUTO_TEST_CASE( EmptyCopy, *boost::unit_test::expected_failures( 1 ) )
{
    BITMAP_BASE empty;
    BITMAP_BASE copied = empty;

    // should still have nothing in it
    BOOST_CHECK_EQUAL( copied.GetImageData(), nullptr );
}

#endif


struct TEST_PIXEL_CASE
{
    int            m_x;
    int            m_y;
    KIGFX::COLOR4D m_color;
};


/**
 * Check basic properties of the populated images
 */
BOOST_AUTO_TEST_CASE( BasicProps )
{
    // make sure we can do all this to a const img
    const BITMAP_BASE& img = m_4tile;

    // have "some" image data
    BOOST_REQUIRE_NE( img.GetImageData(), nullptr );

    // still default props here
    BOOST_CHECK_EQUAL( img.GetPPI(), 300 );
    BOOST_CHECK_EQUAL( img.GetScale(), 5.0 );

    // we changed this, make sure it's right
    BOOST_CHECK_EQUAL( img.GetPixelScaleFactor(), 2.0 );

    BOOST_CHECK( img.GetSizePixels() == size_4tile );
    BOOST_CHECK( img.GetSize() == size_4tile * 10 );

    const EDA_RECT bb = img.GetBoundingBox();
    BOOST_CHECK( bb.GetPosition() == wxPoint( -40, -40 ) );
    BOOST_CHECK( bb.GetEnd() == wxPoint( 40, 40 ) );
}


/**
 * Check the image is right
 */
BOOST_AUTO_TEST_CASE( BasicImage )
{
    const wxImage* img_data = m_4tile.GetImageData();
    BOOST_REQUIRE_NE( img_data, nullptr );

    //   green, black,
    //   red,   blue
    const std::vector<TEST_PIXEL_CASE> exp_pixels = {
        { 1, 1, col_green },
        { 7, 1, col_black },
        { 1, 7, col_red },
        { 7, 7, col_blue },
    };

    for( const auto& c : exp_pixels )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsImagePixelOfColor, ( *img_data )( c.m_x )( c.m_y )( c.m_color ) );
    }
}

/**
 * Check the image is right after rotating
 */
BOOST_AUTO_TEST_CASE( RotateImage )
{
    // Note the parameter name is wrong here, true is clockwise
    m_4tile.Rotate( false );

    const wxImage* img_data = m_4tile.GetImageData();
    BOOST_REQUIRE_NE( img_data, nullptr );

    //   black, blue,
    //   green, red
    const std::vector<TEST_PIXEL_CASE> exp_pixels = {
        { 1, 1, col_black },
        { 7, 1, col_blue },
        { 1, 7, col_green },
        { 7, 7, col_red },
    };

    for( const auto& c : exp_pixels )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsImagePixelOfColor, ( *img_data )( c.m_x )( c.m_y )( c.m_color ) );
    }
}

/**
 * Check the image is right after mirroring
 */
BOOST_AUTO_TEST_CASE( MirrorImage )
{
    m_4tile.Mirror( true );

    const wxImage* img_data = m_4tile.GetImageData();
    BOOST_REQUIRE_NE( img_data, nullptr );

    //   red,   blue
    //   green, black
    const std::vector<TEST_PIXEL_CASE> exp_pixels = {
        { 1, 1, col_red },
        { 7, 1, col_blue },
        { 1, 7, col_green },
        { 7, 7, col_black },
    };

    for( const auto& c : exp_pixels )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsImagePixelOfColor, ( *img_data )( c.m_x )( c.m_y )( c.m_color ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()