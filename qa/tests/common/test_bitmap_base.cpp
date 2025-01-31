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
 * @file
 * Test suite for BITMAP_BASE
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <bitmap_base.h>

#include "wximage_test_utils.h"

#include <wx/mstream.h>


// Dummy PNG image 8x8, 4 tiles:
//
//   green, black,
//   red,   blue
//   (the black tile is a circle, otherwise older wx's seem to crash)
static const std::vector<unsigned char> png_data_4tile = { //
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x06, 0x00, 0x00, 0x00, 0xC4, 0x0F, 0xBE,
    0x8B, 0x00, 0x00, 0x00, 0x04, 0x73, 0x42, 0x49, 0x54, 0x08, 0x08, 0x08, 0x08, 0x7C, 0x08, 0x64,
    0x88, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC4, 0x00, 0x00, 0x0E,
    0xC4, 0x01, 0x95, 0x2B, 0x0E, 0x1B, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F,
    0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x77, 0x77, 0x77, 0x2E, 0x69, 0x6E, 0x6B, 0x73, 0x63,
    0x61, 0x70, 0x65, 0x2E, 0x6F, 0x72, 0x67, 0x9B, 0xEE, 0x3C, 0x1A, 0x00, 0x00, 0x00, 0x40, 0x49,
    0x44, 0x41, 0x54, 0x18, 0x95, 0xA5, 0xCD, 0x4B, 0x0A, 0xC0, 0x20, 0x14, 0x43, 0xD1, 0x63, 0x77,
    0xA7, 0xDD, 0xFF, 0x06, 0xD4, 0x7D, 0x3C, 0x07, 0x95, 0x42, 0x3F, 0xE0, 0xC0, 0x84, 0x0C, 0x02,
    0xE1, 0x86, 0x78, 0x99, 0x13, 0x1D, 0x0D, 0xC5, 0xCF, 0xA0, 0x21, 0x66, 0xEA, 0x61, 0xA9, 0x2F,
    0xA1, 0x4C, 0x4A, 0x45, 0x4E, 0x71, 0xA1, 0x6E, 0xA5, 0x67, 0xB5, 0xBC, 0xD8, 0x1F, 0x0C, 0xA7,
    0xFD, 0x23, 0x67, 0x70, 0xDA, 0x89, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
    0x42, 0x60, 0x82
};

///< 4tile is an 8x8 image
static const VECTOR2I size_4tile{ 8, 8 };

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
        m_4tile.SetPixelSizeIu( 2.0 );
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
    const int expected_ppi = 300;

    BOOST_CHECK_EQUAL( cempty.GetImageData(), nullptr );
    BOOST_CHECK_EQUAL( cempty.GetPPI(), expected_ppi );
    BOOST_CHECK_EQUAL( cempty.GetScale(), 1.0 );
    BOOST_CHECK_EQUAL( cempty.GetPixelSizeIu(), 254000.0 / expected_ppi );

    // can do this on an empty image
    empty.Rotate( true );
    empty.Mirror( FLIP_DIRECTION::TOP_BOTTOM );
}


/**
 * Check we can validly copy an empty bitmap
 */
BOOST_AUTO_TEST_CASE( EmptyCopy )
{
    BITMAP_BASE empty;
    BITMAP_BASE copied = empty;

    // should still have nothing in it
    BOOST_CHECK_EQUAL( copied.GetImageData(), nullptr );
}


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
    const int expected_ppi = 94;
    BOOST_CHECK_EQUAL( img.GetPPI(), expected_ppi );
    BOOST_CHECK_EQUAL( img.GetScale(), 5.0 );

    // we changed this, make sure it's right
    BOOST_CHECK_EQUAL( img.GetPixelSizeIu(), 2.0 );

    BOOST_CHECK( img.GetSizePixels() == size_4tile );
    BOOST_CHECK( img.GetSize() == size_4tile * 10 );

    const BOX2I bb = img.GetBoundingBox();
    BOOST_CHECK( bb.GetPosition() == VECTOR2I( -40, -40 ) );
    BOOST_CHECK( bb.GetEnd() == VECTOR2I( 40, 40 ) );
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
        { 6, 1, col_black },
        { 1, 6, col_red },
        { 6, 6, col_blue },
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
        { 6, 1, col_blue },
        { 1, 6, col_green },
        { 6, 6, col_red },
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
    m_4tile.Mirror( FLIP_DIRECTION::TOP_BOTTOM );

    const wxImage* img_data = m_4tile.GetImageData();
    BOOST_REQUIRE_NE( img_data, nullptr );

    //   red,   blue
    //   green, black
    const std::vector<TEST_PIXEL_CASE> exp_pixels = {
        { 1, 1, col_red },
        { 6, 1, col_blue },
        { 1, 6, col_green },
        { 6, 6, col_black },
    };

    for( const auto& c : exp_pixels )
    {
        BOOST_CHECK_PREDICATE(
                KI_TEST::IsImagePixelOfColor, ( *img_data )( c.m_x )( c.m_y )( c.m_color ) );
    }
}

/**
 * Check setting image data by SetImage produces saveable data
 * via SaveImageData.
 *
 * Regression test for: https://gitlab.com/kicad/code/kicad/-/issues/19772
 */
BOOST_AUTO_TEST_CASE( SetImageOutputsData )
{
    BITMAP_BASE bitmap;
    wxImage     img( 2, 2 );
    img.SetRGB( 0, 0, 0, 255, 0 );
    img.SetRGB( 1, 0, 0, 0, 255 );
    img.SetRGB( 0, 1, 255, 0, 0 );
    img.SetRGB( 1, 1, 0, 0, 255 );

    // Set the wxImage directly, not via a stream/file
    // (this happens, e.g. when the clipboard gives you a wxImage)
    bitmap.SetImage( img );

    wxMemoryOutputStream mos;
    BOOST_CHECK( bitmap.SaveImageData( mos ) );

    BOOST_REQUIRE( mos.GetSize() > 0 );

    // check the output is the same as the input
    wxMemoryInputStream mis( mos );
    wxImage             img2;

    BOOST_CHECK( img2.LoadFile( mis, wxBITMAP_TYPE_PNG ) );

    BOOST_CHECK_PREDICATE( KI_TEST::ImagesHaveSamePixels, (img) ( img2 ) );
}

BOOST_AUTO_TEST_SUITE_END()
