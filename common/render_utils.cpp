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
