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

#include "wximage_test_utils.h"

#include "color4d_test_utils.h"


namespace KI_TEST
{

bool IsImagePixelOfColor( const wxImage& aImage, int aX, int aY, const KIGFX::COLOR4D& aColor )
{
    const wxSize imageSize = aImage.GetSize();

    if( imageSize.x < aX || imageSize.y < aY )
    {
        BOOST_TEST_INFO( "Pixel (" << aX << ", " << aY << "is not in image of size (" << imageSize.x
                                   << ", " << imageSize.y << ")" );
        return false;
    }

    const int r = aImage.GetRed( aX, aY );
    const int g = aImage.GetGreen( aX, aY );
    const int b = aImage.GetBlue( aX, aY );

    const int a = aImage.HasAlpha() ? aImage.GetAlpha( aX, aY ) : 255;

    if( !KI_TEST::IsColorNearHex( aColor, r, g, b, a ) )
    {
        BOOST_TEST_INFO( "Colour doesn't match: got rgba(" << r << ", " << g << ", " << b << ", "
                                                           << a << "), expected " << aColor );
        return false;
    }

    return true;
}


bool ImagesHaveSamePixels( const wxImage& aImgA, const wxImage& aImgB )
{
    if( aImgA.GetSize() != aImgB.GetSize() )
    {
        BOOST_TEST_INFO( "Image sizes differ: " << aImgA.GetSize().GetWidth() << "x"
                                                << aImgA.GetSize().GetHeight() << " vs "
                                                << aImgB.GetSize().GetWidth() << "x"
                                                << aImgB.GetSize().GetHeight() );
        return false;
    }

    for( int y = 0; y < aImgA.GetHeight(); ++y )
    {
        for( int x = 0; x < aImgA.GetWidth(); ++x )
        {
            const int rA = aImgA.GetRed( x, y );
            const int gA = aImgA.GetGreen( x, y );
            const int bA = aImgA.GetBlue( x, y );

            const int rB = aImgB.GetRed( x, y );
            const int gB = aImgB.GetGreen( x, y );
            const int bB = aImgB.GetBlue( x, y );

            if( rA != rB || gA != gB || bA != bB )
            {
                BOOST_TEST_INFO( "Pixel (" << x << ", " << y << ") differs: "
                                           << "A(" << rA << ", " << gA << ", " << bA << ") "
                                           << "B(" << rB << ", " << gB << ", " << bB << ")" );
                return false;
            }
        }
    }

    return true;
}

} // namespace KI_TEST


std::ostream& boost_test_print_type( std::ostream& os, wxImage const& aImage )
{
    const wxSize size = aImage.GetSize();
    os << "wxImage[" << size.x << "x" << size.y << "]";
    return os;
}
