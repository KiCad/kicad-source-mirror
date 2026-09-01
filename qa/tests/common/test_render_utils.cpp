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

/**
 * @file test_render_utils.cpp
 * Test suite for the generic render utilities in common (render_utils.h).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <render_utils.h>

#include <vector>


namespace
{

/// A 1x1 opaque image of the given colour.
wxImage makeOpaquePixel( unsigned char aR, unsigned char aG, unsigned char aB )
{
    wxImage img( 1, 1 );
    img.SetRGB( 0, 0, aR, aG, aB );
    return img;
}

/**
 * Compose an alpha-having pixel over a flat background, as an opaque renderer does.
 */
unsigned char compositeOver( int aColour, int aAlpha, int aBackground )
{
    return ( aColour * aAlpha + aBackground * ( 255 - aAlpha ) ) / 255;
}


/**
 * Check a pixel's RGBA values against expectations, reporting each channel separately.
 */
void checkPixelRGBA( const wxImage& aImage, int aX, int aY, int aR, int aG, int aB, int aA )
{
    BOOST_REQUIRE( aImage.HasAlpha() );

    BOOST_TEST_INFO( "pixel (" << aX << ", " << aY << "): got rgba("
                               << static_cast<int>( aImage.GetRed( aX, aY ) ) << ", "
                               << static_cast<int>( aImage.GetGreen( aX, aY ) ) << ", "
                               << static_cast<int>( aImage.GetBlue( aX, aY ) ) << ", "
                               << static_cast<int>( aImage.GetAlpha( aX, aY ) )
                               << "), expected rgba(" << aR << ", " << aG << ", " << aB << ", " << aA
                               << ")" );

    BOOST_CHECK_EQUAL( static_cast<int>( aImage.GetRed( aX, aY ) ), aR );
    BOOST_CHECK_EQUAL( static_cast<int>( aImage.GetGreen( aX, aY ) ), aG );
    BOOST_CHECK_EQUAL( static_cast<int>( aImage.GetBlue( aX, aY ) ), aB );
    BOOST_CHECK_EQUAL( static_cast<int>( aImage.GetAlpha( aX, aY ) ), aA );
}

} // namespace


BOOST_AUTO_TEST_SUITE( RenderUtils )


BOOST_AUTO_TEST_CASE( OpaquePixelIsPreserved )
{
    // An opaque pixel renders identically over white and black.
    const wxImage onWhite = makeOpaquePixel( 255, 0, 0 );
    const wxImage onBlack = onWhite;

    const tl::expected<wxImage, std::string> result = CreateAlphaImageFromTwoRenders( onWhite, onBlack );

    BOOST_REQUIRE( result.has_value() );

    const wxImage& image = *result;

    checkPixelRGBA( image, 0, 0, 255, 0, 0, 255 );
}


BOOST_AUTO_TEST_CASE( TransparentPixelHasNoColour )
{
    // Fully transparent content shows the background: white over white, black over black.
    const wxImage onWhite = makeOpaquePixel( 255, 255, 255 );
    const wxImage onBlack = makeOpaquePixel( 0, 0, 0 );

    const tl::expected<wxImage, std::string> result = CreateAlphaImageFromTwoRenders( onWhite, onBlack );

    BOOST_REQUIRE( result.has_value() );

    const wxImage& image = *result;

    checkPixelRGBA( image, 0, 0, 0, 0, 0, 0 );
}


BOOST_AUTO_TEST_CASE( SemiTransparentPixelRecoversAlphaAndColour )
{
    // A 50% red pixel composites over white to (255, 128, 128) and over black to
    // (128, 0, 0): recovered alpha is 128 and the straight colour is pure red.
    const wxImage onWhite = makeOpaquePixel( 255, 128, 128 );
    const wxImage onBlack = makeOpaquePixel( 128, 0, 0 );

    const tl::expected<wxImage, std::string> result =
            CreateAlphaImageFromTwoRenders( onWhite, onBlack );

    BOOST_REQUIRE( result.has_value() );

    const wxImage& image = *result;

    checkPixelRGBA( image, 0, 0, 255, 0, 0, 128 );
}


BOOST_AUTO_TEST_CASE( RoundTripReproducesTheRenders )
{
    // Synthesise the white/black renders of a known straight-alpha pixel, then check the
    // recovered image recomposites back to those same renders (within the rounding of the
    // 8-bit integer arithmetic).
    struct PIXEL
    {
        unsigned char r, g, b, a;
    };

    const std::vector<PIXEL> cases = {
        { 255, 0,   0,   255 },
        { 255, 0,   0,   128 },
        { 0,   255, 0,   64  },
        { 0,   0,   255, 200 },
        { 200, 100, 50,  80  },
        { 255, 255, 255, 30  },
        { 0,   0,   0,   0   },
    };

    const auto checkNear =
            []( int a, int b, const char* channel )
            {
                BOOST_TEST_INFO( "channel " << channel << ": got " << a << ", expected " << b );
                BOOST_CHECK( a >= b - 1 && a <= b + 1 );
            };

    for( const PIXEL& pix : cases )
    {
        BOOST_TEST_CONTEXT( "rgba(" << pix.r << ", " << pix.g << ", " << pix.b << ", " << pix.a << ")" )
        {
            const wxImage onWhite = makeOpaquePixel( compositeOver( pix.r, pix.a, 255 ),
                                                     compositeOver( pix.g, pix.a, 255 ),
                                                     compositeOver( pix.b, pix.a, 255 ) );
            const wxImage onBlack = makeOpaquePixel( compositeOver( pix.r, pix.a, 0 ),
                                                     compositeOver( pix.g, pix.a, 0 ),
                                                     compositeOver( pix.b, pix.a, 0 ) );

            const tl::expected<wxImage, std::string> result = CreateAlphaImageFromTwoRenders( onWhite, onBlack );

            BOOST_REQUIRE( result.has_value() );

            const wxImage& image = *result;

            BOOST_REQUIRE( image.HasAlpha() );

            // The recovered coverage is within rounding of the original.
            const int alpha = image.GetAlpha( 0, 0 );
            BOOST_CHECK( alpha >= pix.a - 1 && alpha <= pix.a + 1 );

            // Recompositing over white and black must reproduce the input renders.
            for( const int bg : { 255, 0 } )
            {
                const wxImage& render = bg ? onWhite : onBlack;

                checkNear( compositeOver( image.GetRed( 0, 0 ), alpha, bg ), render.GetRed( 0, 0 ), "red" );
                checkNear( compositeOver( image.GetGreen( 0, 0 ), alpha, bg ), render.GetGreen( 0, 0 ), "green" );
                checkNear( compositeOver( image.GetBlue( 0, 0 ), alpha, bg ), render.GetBlue( 0, 0 ), "blue" );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( InvalidInputsReturnError )
{
    const wxImage empty;
    const wxImage pixel = makeOpaquePixel( 0, 0, 0 );

    wxImage wide( 2, 1 );
    wide.SetRGB( 0, 0, 0, 0, 0 );
    wide.SetRGB( 1, 0, 0, 0, 0 );

    const auto emptyInput = CreateAlphaImageFromTwoRenders( empty, pixel );
    BOOST_CHECK( !emptyInput.has_value() );
    BOOST_CHECK( !emptyInput.error().empty() );

    const auto emptyInput2 = CreateAlphaImageFromTwoRenders( pixel, empty );
    BOOST_CHECK( !emptyInput2.has_value() );
    BOOST_CHECK( !emptyInput2.error().empty() );

    const auto sizeMismatch = CreateAlphaImageFromTwoRenders( pixel, wide );
    BOOST_CHECK( !sizeMismatch.has_value() );
    BOOST_CHECK( !sizeMismatch.error().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
