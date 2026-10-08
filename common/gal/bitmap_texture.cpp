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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gal/bitmap_texture.h>

#include <math/util.h>

#include <algorithm>

#include <wx/image.h>


namespace KIGFX
{

wxImage FitImageToTexture( const wxImage& aImage, int aMaxTextureSize )
{
    const int width = aImage.GetWidth();
    const int height = aImage.GetHeight();

    if( aMaxTextureSize <= 0 || ( width <= aMaxTextureSize && height <= aMaxTextureSize ) )
        return aImage;

    const double scale = std::min( static_cast<double>( aMaxTextureSize ) / width,
                                   static_cast<double>( aMaxTextureSize ) / height );

    const int newWidth = std::clamp( KiROUND( width * scale ), 1, aMaxTextureSize );
    const int newHeight = std::clamp( KiROUND( height * scale ), 1, aMaxTextureSize );

    return aImage.Scale( newWidth, newHeight, wxIMAGE_QUALITY_HIGH );
}


std::vector<uint8_t> RgbToRgba( const uint8_t* aRgb, const uint8_t* aAlpha, size_t aPixelCount )
{
    std::vector<uint8_t> rgba( aPixelCount * 4 );

    for( size_t ii = 0; ii < aPixelCount; ++ii )
    {
        rgba[ii * 4 + 0] = aRgb[ii * 3 + 0];
        rgba[ii * 4 + 1] = aRgb[ii * 3 + 1];
        rgba[ii * 4 + 2] = aRgb[ii * 3 + 2];
        rgba[ii * 4 + 3] = aAlpha ? aAlpha[ii] : wxIMAGE_ALPHA_OPAQUE;
    }

    return rgba;
}


std::vector<uint8_t> ImageToRgba( const wxImage& aImage )
{
    const size_t pixelCount = static_cast<size_t>( aImage.GetWidth() ) * aImage.GetHeight();

    std::vector<uint8_t> rgba =
            RgbToRgba( aImage.GetData(), aImage.HasAlpha() ? aImage.GetAlpha() : nullptr, pixelCount );

    if( !aImage.HasAlpha() && aImage.HasMask() )
    {
        const uint8_t maskRed = aImage.GetMaskRed();
        const uint8_t maskGreen = aImage.GetMaskGreen();
        const uint8_t maskBlue = aImage.GetMaskBlue();

        for( size_t ii = 0; ii < pixelCount; ++ii )
        {
            uint8_t* px = &rgba[ii * 4];

            if( px[0] == maskRed && px[1] == maskGreen && px[2] == maskBlue )
                px[3] = wxIMAGE_ALPHA_TRANSPARENT;
        }
    }

    return rgba;
}

} // namespace KIGFX
