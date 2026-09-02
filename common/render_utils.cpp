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

#include <render_utils.h>

#include <algorithm>
#include <cmath>
#include <math/util.h>


tl::expected<wxImage, std::string> CreateAlphaImageFromTwoRenders( const wxImage& aOnWhite, const wxImage& aOnBlack )
{
    if( !aOnWhite.IsOk() || !aOnBlack.IsOk() )
        return tl::make_unexpected( "Invalid input images" );

    if( aOnWhite.GetSize() != aOnBlack.GetSize() )
        return tl::make_unexpected( "Input images have different sizes" );

    const int width = aOnWhite.GetWidth();
    const int height = aOnWhite.GetHeight();

    wxImage result( width, height );
    result.InitAlpha();

    const unsigned char* rgbWhite = aOnWhite.GetData();
    const unsigned char* rgbBlack = aOnBlack.GetData();
    unsigned char*       rgbResult = result.GetData();
    unsigned char*       alphaResult = result.GetAlpha();

    for( int i = 0; i < width * height; ++i )
    {
        const int idx = i * 3;

        const int rW = rgbWhite[idx];
        const int gW = rgbWhite[idx + 1];
        const int bW = rgbWhite[idx + 2];
        const int rB = rgbBlack[idx];
        const int gB = rgbBlack[idx + 1];
        const int bB = rgbBlack[idx + 2];

        // The difference between the white- and black-background renders reveals the
        // per-pixel coverage; the black render carries the premultiplied colour.
        const int diffR = rW - rB;
        const int diffG = gW - gB;
        const int diffB = bW - bB;
        const int avgDiff = ( diffR + diffG + diffB ) / 3;

        const int alpha = std::clamp( 255 - avgDiff, 0, 255 );
        alphaResult[i] = static_cast<unsigned char>( alpha );

        if( alpha > 0 )
        {
            // Un-premultiply the colour, recovering the straight colour.
            rgbResult[idx]     = static_cast<unsigned char>( std::min( 255, rB * 255 / alpha ) );
            rgbResult[idx + 1] = static_cast<unsigned char>( std::min( 255, gB * 255 / alpha ) );
            rgbResult[idx + 2] = static_cast<unsigned char>( std::min( 255, bB * 255 / alpha ) );
        }
        else
        {
            // Fully transparent pixels carry no meaningful colour.
            rgbResult[idx]     = 0;
            rgbResult[idx + 1] = 0;
            rgbResult[idx + 2] = 0;
        }
    }

    return result;
}


/*
 * This is the "Color to Alpha" algorithm from GIMP (the GEGL color-to-alpha
 * operation).
 *
 * The implementation of this is taken with love from GEGL's
 * operations/common-gpl3+/color-to-alpha.c, which is licensed under the GNU
 * General Public License version 3 or later (GPLv3+). The original code is
 * Copyright (C) 1995-2017 by the GIMP Development Team and is licensed under the
 * GPLv3+.
 *
 * A pixel that:
 *   - matches the background colour within the transparency threshold becomes
 *     fully transparent
 *   - has channels that are further than the opacity threshold stays fully opaque
 *   - everything in between gets a proportional opacity.
 */
void ConvertColourToAlphaInPlace( wxImage& aImage, const wxColour& aColour )
{
    /*
     * Thresholds of 0 and 1 are the GIMP defaults, which means a linear
     * scale from full match = transparent to full mismatch = opaque.
     * These can be made into parameters if needed, but for now they are hard-coded.
     */
    constexpr float TRANSPARENCY_THRESHOLD = 0.0f;
    constexpr float OPACITY_THRESHOLD = 1.0f;

    const int w = aImage.GetWidth();
    const int h = aImage.GetHeight();

    if( w == 0 || h == 0 )
        return;

    aImage.UnShare();

    if( !aImage.HasAlpha() )
        aImage.InitAlpha();

    unsigned char* rgb = aImage.GetData();
    unsigned char* alpha = aImage.GetAlpha();

    // Background colour to make transparent, as float components in [0, 1].
    const float bgRgb[3] = {
        static_cast<float>( aColour.Red() ) / 255.0f,
        static_cast<float>( aColour.Green() ) / 255.0f,
        static_cast<float>( aColour.Blue() ) / 255.0f,
    };

    constexpr float EPSILON = 0.00001f;
    const float     transparencyThreshold = TRANSPARENCY_THRESHOLD + EPSILON;
    const float     opacityThreshold = OPACITY_THRESHOLD - EPSILON;

    for( int y = 0; y < h; ++y )
    {
        for( int x = 0; x < w; ++x )
        {
            // Index into the RGB array for the pixel
            const int pos = ( y * w + x ) * 3;
            const int alphaPos = y * w + x;

            const float srcRgb[3] = {
                static_cast<float>( rgb[pos] ) / 255.0f,
                static_cast<float>( rgb[pos + 1] ) / 255.0f,
                static_cast<float>( rgb[pos + 2] ) / 255.0f,
            };
            const float srcAlpha = static_cast<float>( alpha[alphaPos] ) / 255.0f;

            // The largest fraction of the pixel that each channel can keep
            // without leaving its colour range, and the channel distance that
            // produced it.
            float outAlpha = 0.0f;
            float dist = 0.0f;

            for( int i = 0; i < 3; ++i )
            {
                const float channelDist = std::fabs( srcRgb[i] - bgRgb[i] );

                float a = 0.0f;

                if( channelDist < transparencyThreshold )
                {
                    a = 0.0f;
                }
                else if( channelDist > opacityThreshold )
                {
                    a = 1.0f;
                }
                else if( srcRgb[i] < bgRgb[i] )
                {
                    const float factor = std::min( opacityThreshold, bgRgb[i] ) - transparencyThreshold;
                    a = ( channelDist - transparencyThreshold ) / factor;
                }
                else
                {
                    const float factor = std::min( opacityThreshold, 1.0f - bgRgb[i] ) - transparencyThreshold;
                    a = ( channelDist - transparencyThreshold ) / factor;
                }

                // Choose the largest (most opaque) alpha value from the three channels
                if( a > outAlpha )
                {
                    outAlpha = a;
                    dist = channelDist;
                }
            }

            // If the new alpha is nonzero, remove the background contribution from the colour and
            // un-premultiply the colour by the new alpha. Otherwise leave the colour as-is.
            if( outAlpha > EPSILON )
            {
                const float ratio = transparencyThreshold / dist;
                const float alphaInv = 1.0f / outAlpha;

                for( int i = 0; i < 3; ++i )
                {
                    const float c = bgRgb[i] + ( srcRgb[i] - bgRgb[i] ) * ratio;
                    const int   value = KiROUND( ( c + ( srcRgb[i] - c ) * alphaInv ) * 255.0f );
                    rgb[pos + i] = std::clamp( value, 0, 255 );
                }
            }

            alpha[alphaPos] = std::clamp( KiROUND( srcAlpha * outAlpha * 255.0f ), 0, 255 );
        }
    }
}
