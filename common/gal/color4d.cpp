/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright 2017-2019 Kicad Developers, see AUTHORS.txt for contributors.
 *
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

#include <map>

#include <gal/color4d.h>

using namespace KIGFX;

COLOR4D::COLOR4D( EDA_COLOR_T aColor )
{
    if( aColor <= UNSPECIFIED_COLOR || aColor >= NBCOLORS )
    {
        *this = COLOR4D::UNSPECIFIED;
        return;
    }

    r = g_ColorRefs[aColor].m_Red / 255.0;
    g = g_ColorRefs[aColor].m_Green / 255.0;
    b = g_ColorRefs[aColor].m_Blue / 255.0;
    a = 1.0;
}


#ifdef WX_COMPATIBILITY
    COLOR4D::COLOR4D( const wxColour& aColor )
    {
        r = aColor.Red() / 255.0;
        g = aColor.Green() / 255.0;
        b = aColor.Blue() / 255.0;
        a = aColor.Alpha() / 255.0;
    }


    bool COLOR4D::SetFromWxString( const wxString& aColorString )
    {
        wxColour c;

        if( c.Set( aColorString ) )
        {
            r = c.Red() / 255.0;
            g = c.Green() / 255.0;
            b = c.Blue() / 255.0;
            a = c.Alpha() / 255.0;

            return true;
        }

        return false;
    }


    wxString COLOR4D::ToWxString( long flags ) const
    {
        wxColour c = ToColour();
        return c.GetAsString( flags );
    }


    wxColour COLOR4D::ToColour() const
    {
        using CHAN_T = wxColourBase::ChannelType;

        const wxColour colour(
            static_cast<CHAN_T>( r * 255 + 0.5 ),
            static_cast<CHAN_T>( g * 255 + 0.5 ),
            static_cast<CHAN_T>( b * 255 + 0.5 ),
            static_cast<CHAN_T>( a * 255 + 0.5 )
        );
        return colour;
    }


    COLOR4D COLOR4D::LegacyMix( COLOR4D aColor ) const
    {
        COLOR4D candidate;

        // Blend the two colors (i.e. OR the RGB values)
        candidate.r = ( (unsigned)( 255.0 * r ) | (unsigned)( 255.0 * aColor.r ) ) / 255.0,
        candidate.g = ( (unsigned)( 255.0 * g ) | (unsigned)( 255.0 * aColor.g ) ) / 255.0,
        candidate.b = ( (unsigned)( 255.0 * b ) | (unsigned)( 255.0 * aColor.b ) ) / 255.0,

        // the alpha channel can be reinitialized
        // but what is the best value?
        candidate.a = ( aColor.a + a ) / 2;

        return candidate;
    }


    unsigned int COLOR4D::ToU32() const
    {
        return ToColour().GetRGB();
    }


    void COLOR4D::FromU32( unsigned int aPackedColor )
    {
        wxColour c;
        c.SetRGB( aPackedColor );
        r = c.Red() / 255.0;
        g = c.Green() / 255.0;
        b = c.Blue() / 255.0;
        a = c.Alpha() / 255.0;
    }

#endif

namespace KIGFX {

const bool operator==( const COLOR4D& lhs, const COLOR4D& rhs )
{
    return lhs.a == rhs.a && lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}


const bool operator!=( const COLOR4D& lhs, const COLOR4D& rhs )
{
    return !( lhs == rhs );
}

std::ostream &operator<<( std::ostream &aStream, COLOR4D const &aColor )
{
    return aStream << aColor.ToWxString( wxC2S_CSS_SYNTAX );
}

}


void COLOR4D::ToHSL( double& aOutHue, double& aOutSaturation, double& aOutLightness ) const
{
    auto min = std::min( r, std::min( g, b ) );
    auto max = std::max( r, std::max( g, b ) );
    auto diff = max - min;

    aOutLightness = ( max + min ) / 2.0;

    if( aOutLightness >= 1.0 )
        aOutSaturation = 0.0;
    else
        aOutSaturation = diff / ( 1.0 - std::abs( 2.0 * aOutLightness - 1.0 ) );

    double hue;

    if( diff <= 0.0 )
        hue = 0.0;
    else if( max == r )
        hue = ( g - b ) / diff;
    else if( max == g )
        hue = ( b - r ) / diff + 2.0;
    else
        hue = ( r - g ) / diff + 4.0;

    aOutHue = hue > 0.0 ? hue * 60.0 : hue * 60.0 + 360.0;

    while( aOutHue < 0.0 )
        aOutHue += 360.0;
}


void COLOR4D::FromHSL( double aInHue, double aInSaturation, double aInLightness )
{
    const auto P = ( 1.0 - std::abs( 2.0 * aInLightness - 1.0 ) ) * aInSaturation;
    const auto scaled_hue = aInHue / 60.0;
    const auto Q = P * ( 1.0 - std::abs( std::fmod( scaled_hue, 2.0 ) - 1.0 ) );

    r = g = b = aInLightness - P / 2.0;

    if (scaled_hue < 1.0)
    {
        r += P;
        g += Q;
    }
    else if (scaled_hue < 2.0)
    {
        r += Q;
        g += P;
    }
    else if (scaled_hue < 3.0)
    {
        g += P;
        b += Q;
    }
    else if (scaled_hue < 4.0)
    {
        g += Q;
        b += P;
    }
    else if (scaled_hue < 5.0)
    {
        r += Q;
        b += P;
    }
    else
    {
        r += P;
        b += Q;
    }
}


void COLOR4D::ToHSV( double& aOutHue, double& aOutSaturation, double& aOutValue, bool aAlwaysDefineHue ) const
{
    double min, max, delta;

    min = r < g ? r : g;
    min = min < b ? min : b;

    max = r > g ? r : g;
    max = max > b ? max : b;

    aOutValue = max;                    // value
    delta = max - min;

    if( max > 0.0 )
    {
        aOutSaturation = ( delta / max );
    }
    else    // for black color (r = g = b = 0 )  saturation is set to 0.
    {
        aOutSaturation = 0.0;
        aOutHue = aAlwaysDefineHue ? 0.0 : NAN;
        return;
    }

    /* Hue in degrees (0...360) is coded according to this table
     * 0 or 360 : red
     * 60 : yellow
     * 120 : green
     * 180 : cyan
     * 240 : blue
     * 300 : magenta
    */
    if( delta != 0.0 )
    {
        if( r >= max )
            aOutHue = ( g - b ) / delta;          // between yellow & magenta
        else if( g >= max )
            aOutHue = 2.0 + ( b - r ) / delta;    // between cyan & yellow
        else
            aOutHue = 4.0 + ( r - g ) / delta;    // between magenta & cyan

        aOutHue *= 60.0;                          // degrees

        if( aOutHue < 0.0 )
            aOutHue += 360.0;
    }
    else   // delta = 0 means r = g = b. hue is set to 0.0
    {
        aOutHue = aAlwaysDefineHue ? 0.0 : NAN;
    }
}


void COLOR4D::FromHSV( double aInH, double aInS, double aInV )
{
    if( aInS <= 0.0 )
    {
        r = aInV;
        g = aInV;
        b = aInV;
        return;
    }

    double hh = aInH;

    while( hh >= 360.0 )
        hh -= 360.0;

    /* Hue in degrees (0...360) is coded according to this table
     * 0 or 360 : red
     * 60 : yellow
     * 120 : green
     * 180 : cyan
     * 240 : blue
     * 300 : magenta
    */
    hh /= 60.0;

    int i = (int) hh;
    double ff = hh - i;

    double p = aInV * ( 1.0 - aInS );
    double q = aInV * ( 1.0 - ( aInS * ff ) );
    double t = aInV * ( 1.0 - ( aInS * ( 1.0 - ff ) ) );

    switch( i )
    {
    case 0:
        r = aInV;
        g = t;
        b = p;
        break;

    case 1:
        r = q;
        g = aInV;
        b = p;
        break;

    case 2:
        r = p;
        g = aInV;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = aInV;
        break;

    case 4:
        r = t;
        g = p;
        b = aInV;
        break;

    case 5:
    default:
        r = aInV;
        g = p;
        b = q;
        break;
    }
}


COLOR4D& COLOR4D::Saturate( double aFactor )
{
    // One can saturate a color only when r, v, b are not equal
    if( r == g && r == b )
        return *this;

    double h, s, v;

    ToHSV( h, s, v, true );
    FromHSV( h, aFactor, 1.0 );

    return *this;
}

constexpr COLOR4D COLOR4D::UNSPECIFIED( 0, 0, 0, 0 );
constexpr COLOR4D COLOR4D::WHITE( 1, 1, 1, 1 );
constexpr COLOR4D COLOR4D::BLACK( 0, 0, 0, 1 );
