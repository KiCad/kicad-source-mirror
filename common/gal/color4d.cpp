/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <json_common.h>
#include <gal/color4d.h>
#include <i18n_utility.h>
#include <wx/crt.h>
#include <math/util.h>
#include <core/kicad_algo.h>

using namespace KIGFX;

#define TS( string ) wxString( _HKI( string ) ).ToStdString()

// We can't have this as a plain static variable, because it is referenced during the initialization
// of other static variables, so we must initialize it explicitly on first use.
const StructColors* colorRefs()
{
    static StructColors s_ColorRefs[NBCOLORS] =
    {
        { 0,    0,   0,   BLACK,         TS( "Black" ),     DARKDARKGRAY      },
        { 72,   72,  72,  DARKDARKGRAY,  TS( "Gray 1" ),    DARKGRAY          },
        { 132,  132, 132, DARKGRAY,      TS( "Gray 2" ),    LIGHTGRAY         },
        { 194,  194, 194, LIGHTGRAY,     TS( "Gray 3" ),    WHITE             },
        { 255,  255, 255, WHITE,         TS( "White" ),     WHITE             },
        { 194,  255, 255, LIGHTYELLOW,   TS( "L.Yellow" ),  WHITE             },
        { 191,  229, 255, LIGHTERORANGE, TS( "L.Orange" ),  WHITE             },
        { 72,   0,   0,   DARKBLUE,      TS( "Blue 1" ),    BLUE              },
        { 0,    72,  0,   DARKGREEN,     TS( "Green 1" ),   GREEN             },
        { 72,   72,  0,   DARKCYAN,      TS( "Cyan 1" ),    CYAN              },
        { 0,    0,   72,  DARKRED,       TS( "Red 1" ),     RED               },
        { 72,   0,   72,  DARKMAGENTA,   TS( "Magenta 1" ), MAGENTA           },
        { 0,    72,  72,  DARKBROWN,     TS( "Brown 1" ),   BROWN             },
        { 0,    77,  128, DARKORANGE,    TS( "Orange 1" ),  ORANGE            },
        { 132,  0,   0,   BLUE,          TS( "Blue 2" ),    LIGHTBLUE         },
        { 0,    132, 0,   GREEN,         TS( "Green 2" ),   LIGHTGREEN        },
        { 132,  132, 0,   CYAN,          TS( "Cyan 2" ),    LIGHTCYAN         },
        { 0,    0,   132, RED,           TS( "Red 2" ),     LIGHTRED          },
        { 132,  0,   132, MAGENTA,       TS( "Magenta 2" ), LIGHTMAGENTA      },
        { 0,    132, 132, BROWN,         TS( "Brown 2" ),   YELLOW            },
        { 0,    102, 204, ORANGE,        TS( "Orange 2" ),  LIGHTORANGE       },
        { 194,  0,   0,   LIGHTBLUE,     TS( "Blue 3" ),    PUREBLUE,         },
        { 0,    194, 0,   LIGHTGREEN,    TS( "Green 3" ),   PUREGREEN         },
        { 194,  194, 0,   LIGHTCYAN,     TS( "Cyan 3" ),    PURECYAN          },
        { 0,    0,   194, LIGHTRED,      TS( "Red 3" ),     PURERED           },
        { 194,  0,   194, LIGHTMAGENTA,  TS( "Magenta 3" ), PUREMAGENTA       },
        { 0,    194, 194, YELLOW,        TS( "Yellow 3" ),  PUREYELLOW        },
        { 0,    133, 221, LIGHTORANGE,   TS( "Orange 3" ),  PUREORANGE        },
        { 255,  0,   0,   PUREBLUE,      TS( "Blue 4" ),    WHITE             },
        { 0,    255, 0,   PUREGREEN,     TS( "Green 4" ),   WHITE             },
        { 255,  255, 0,   PURECYAN,      TS( "Cyan 4" ),    WHITE             },
        { 0,    0,   255, PURERED,       TS( "Red 4" ),     WHITE             },
        { 255,  0,   255, PUREMAGENTA,   TS( "Magenta 4" ), WHITE             },
        { 0,    255, 255, PUREYELLOW,    TS( "Yellow 4" ),  WHITE             },
        { 0,    153, 255, PUREORANGE,    TS( "Orange 4" ),  WHITE             },
    };
    return s_ColorRefs;
}


COLOR4D::COLOR4D( EDA_COLOR_T aColor )
{
    if( aColor <= UNSPECIFIED_COLOR || aColor >= NBCOLORS )
    {
        *this = COLOR4D::UNSPECIFIED;
        return;
    }

    int candidate = 0;

    for( ; candidate < NBCOLORS; ++candidate )
    {
        if( colorRefs()[candidate].m_Numcolor == aColor )
            break;
    }

    if( candidate >= NBCOLORS )
    {
        *this = COLOR4D::UNSPECIFIED;
        return;
    }

    r = colorRefs()[candidate].m_Red / 255.0;
    g = colorRefs()[candidate].m_Green / 255.0;
    b = colorRefs()[candidate].m_Blue / 255.0;
    a = 1.0;
    m_text = std::nullopt;
}


COLOR4D::COLOR4D( const wxString& aColorStr )
{
    if( !SetFromHexString( aColorStr ) && !SetFromWxString( aColorStr ) )
        m_text = aColorStr;
}


COLOR4D::COLOR4D( const wxColour& aColor )
{
    r = aColor.Red() / 255.0;
    g = aColor.Green() / 255.0;
    b = aColor.Blue() / 255.0;
    a = aColor.Alpha() / 255.0;
    m_text = std::nullopt;
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
        m_text = std::nullopt;

        return true;
    }

    return false;
}


wxString COLOR4D::ToCSSString() const
{
    wxColour c = ToColour();
    wxString str;

    const int  red = c.Red();
    const int  green = c.Green();
    const int  blue = c.Blue();
    const int  alpha = c.Alpha();

    if ( alpha == wxALPHA_OPAQUE )
    {
        str.Printf( wxT( "rgb(%d, %d, %d)" ), red, green, blue );
    }
    else // use rgba() form
    {
        wxString alpha_str = wxString::FromCDouble( alpha / 255.0, 3 );

        // The wxC2S_CSS_SYNTAX is particularly sensitive to ','s (as it uses them for value
        // delimiters), and wxWidgets is known to be buggy in this respect when dealing with
        // Serbian and Russian locales (at least), so we enforce an extra level of safety.
        alpha_str.Replace( wxT( "," ), wxT( "." ) );

        str.Printf( wxT( "rgba(%d, %d, %d, %s)" ), red, green, blue, alpha_str );
    }

    return str;
}


bool COLOR4D::SetFromHexString( const wxString& aColorString )
{
    wxString str = aColorString;
    str.Trim( true );
    str.Trim( false );

    if( str.length() < 7 || !str.StartsWith( '#' ) )
        return false;

    unsigned long tmp;

    if( wxSscanf( str.wx_str() + 1, wxT( "%lx" ), &tmp ) != 1 )
        return false;

    if( str.length() >= 9 )
    {
        r = ( (tmp >> 24) & 0xFF ) / 255.0;
        g = ( (tmp >> 16) & 0xFF ) / 255.0;
        b = ( (tmp >>  8) & 0xFF ) / 255.0;
        a = (  tmp        & 0xFF ) / 255.0;
    }
    else
    {
        r = ( (tmp >> 16) & 0xFF ) / 255.0;
        g = ( (tmp >>  8) & 0xFF ) / 255.0;
        b = (  tmp        & 0xFF ) / 255.0;
        a = 1.0;
    }

    m_text = std::nullopt;

    return true;
}


wxString COLOR4D::ToHexString() const
{
    return wxString::Format( wxT("#%02X%02X%02X%02X" ),
                             KiROUND( r * 255.0 ),
                             KiROUND( g * 255.0 ),
                             KiROUND( b * 255.0 ),
                             KiROUND( a * 255.0 ) );
}


wxColour COLOR4D::ToColour() const
{
    using CHAN_T = wxColourBase::ChannelType;

    const wxColour colour(
            static_cast<CHAN_T>( r * 255 + 0.5 ), static_cast<CHAN_T>( g * 255 + 0.5 ),
            static_cast<CHAN_T>( b * 255 + 0.5 ), static_cast<CHAN_T>( a * 255 + 0.5 ) );
    return colour;
}


COLOR4D COLOR4D::LegacyMix( const COLOR4D& aColor ) const
{
    COLOR4D candidate;

    // Blend the two colors (i.e. OR the RGB values)
    candidate.r = ( (unsigned) ( 255.0 * r ) | (unsigned) ( 255.0 * aColor.r ) ) / 255.0,
    candidate.g = ( (unsigned) ( 255.0 * g ) | (unsigned) ( 255.0 * aColor.g ) ) / 255.0,
    candidate.b = ( (unsigned) ( 255.0 * b ) | (unsigned) ( 255.0 * aColor.b ) ) / 255.0,

    // the alpha channel can be reinitialized but what is the best value?
    candidate.a = ( aColor.a + a ) / 2;

    return candidate;
}


namespace KIGFX {

bool operator==( const COLOR4D& lhs, const COLOR4D& rhs )
{
    if( lhs.m_text.has_value() || rhs.m_text.has_value() )
        return lhs.m_text.value_or( wxEmptyString ) == rhs.m_text.value_or( wxEmptyString );

    return lhs.a == rhs.a && lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}


bool operator!=( const COLOR4D& lhs, const COLOR4D& rhs )
{
    return !( lhs == rhs );
}


bool operator<( const COLOR4D& lhs, const COLOR4D& rhs )
{
    if( lhs.m_text.has_value() || rhs.m_text.has_value() )
        return lhs.m_text.value_or( wxEmptyString ) < rhs.m_text.value_or( wxEmptyString );

    if( lhs.r < rhs.r )
        return true;
    else if( lhs.g < rhs.g )
        return true;
    else if( lhs.b < rhs.b )
        return true;
    else if( lhs.a < rhs.a )
        return true;

    return false;
}


std::ostream &operator<<( std::ostream &aStream, COLOR4D const &aColor )
{
    if( aColor.m_text.has_value() )
        return aStream << aColor.m_text.value();

    return aStream << aColor.ToCSSString();
}


void to_json( nlohmann::json& aJson, const COLOR4D& aColor )
{
    if( aColor.m_text.has_value() )
        aJson = nlohmann::json( aColor.m_text.value().ToStdString() );
    else
        aJson = nlohmann::json( aColor.ToCSSString().ToStdString() );
}


void from_json( const nlohmann::json& aJson, COLOR4D& aColor )
{
    aColor.SetFromWxString( aJson.get<std::string>() );
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

    m_text = std::nullopt;
}


void COLOR4D::ToHSV( double& aOutHue, double& aOutSaturation, double& aOutValue,
                     bool aAlwaysDefineHue ) const
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

    m_text = std::nullopt;
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


COLOR4D& COLOR4D::Desaturate()
{
    // One can desaturate a color only when r, v, b are not equal
    if( r == g && r == b )
        return *this;

    double h, s, l;

    ToHSL( h, s, l );
    FromHSL( h, 0.0, l );

    return *this;
}


const COLOR4D COLOR4D::UNSPECIFIED( 0, 0, 0, 0 );
const COLOR4D COLOR4D::WHITE( 1, 1, 1, 1 );
const COLOR4D COLOR4D::BLACK( 0, 0, 0, 1 );
const COLOR4D COLOR4D::CLEAR( 1, 0, 1, 0 );


double COLOR4D::Distance( const COLOR4D& other ) const
{
    return ( r - other.r ) * ( r - other.r )
            + ( g - other.g ) * ( g - other.g )
            + ( b - other.b ) * ( b - other.b );
}


EDA_COLOR_T COLOR4D::FindNearestLegacyColor( int aR, int aG, int aB )
{
    EDA_COLOR_T candidate = EDA_COLOR_T::BLACK;

    /* Find the 'nearest' color in the palette. This is fun. There is
       a gazilion of metrics for the color space and no one of the
       useful one is in the RGB color space. Who cares, this is a CAD,
       not a photosomething...

       I hereby declare that the distance is the sum of the square of the
       component difference. Think about the RGB color cube. Now get the
       euclidean distance, but without the square root... for ordering
       purposes it's the same, obviously. Also each component can't be
       less of the target one, since I found this currently work better...
       */
    int nearest_distance = 255 * 255 * 3 + 1; // Can't beat this

    for( EDA_COLOR_T trying = EDA_COLOR_T::BLACK; trying < EDA_COLOR_T::NBCOLORS;
            trying = static_cast<EDA_COLOR_T>( int( trying ) + 1 ) )
    {
        const StructColors &c = colorRefs()[trying];
        int distance = ( aR - c.m_Red ) * ( aR - c.m_Red ) +
                       ( aG - c.m_Green ) * ( aG - c.m_Green ) +
                       ( aB - c.m_Blue ) * ( aB - c.m_Blue );

        if( distance < nearest_distance && c.m_Red >= aR &&
            c.m_Green >= aG && c.m_Blue >= aB )
        {
            nearest_distance = distance;
            candidate = trying;
        }
    }

    return candidate;
}


COLOR4D& COLOR4D::FromCSSRGBA( int aRed, int aGreen, int aBlue, double aAlpha )
{
    r = std::clamp( aRed, 0, 255 ) / 255.0;
    g = std::clamp( aGreen, 0, 255 ) / 255.0;
    b = std::clamp( aBlue, 0, 255 ) / 255.0;
    a = std::clamp( aAlpha, 0.0, 1.0 );
    m_text = std::nullopt;

    return *this;
}


int COLOR4D::Compare( const COLOR4D& aRhs ) const
{
    if( m_text.has_value() || aRhs.m_text.has_value() )
        return ( m_text.value_or( wxEmptyString ) < aRhs.m_text.value_or( wxEmptyString ) ) ? -1 : 1;

    if( r != aRhs.r )
        return ( r < aRhs.r ) ? -1 : 1;

    if( g != aRhs.g )
        return ( g < aRhs.g ) ? -1 : 1;

    if( b != aRhs.b )
        return ( b < aRhs.b ) ? -1 : 1;

    if( a != aRhs.a )
        return ( a < aRhs.a ) ? -1 : 1;

    return 0;
}


double COLOR4D::RelativeLuminance() const
{
    // Formula from https://www.w3.org/TR/WCAG21/#dfn-relative-luminance
    double cr = ( r <= 0.04045 ) ? ( r / 12.92 ) : std::pow( ( r + 0.055 ) / 1.055, 2.4 );
    double cg = ( g <= 0.04045 ) ? ( g / 12.92 ) : std::pow( ( g + 0.055 ) / 1.055, 2.4 );
    double cb = ( b <= 0.04045 ) ? ( b / 12.92 ) : std::pow( ( b + 0.055 ) / 1.055, 2.4 );

    return 0.2126 * cr + 0.7152 * cg + 0.0722 * cb;
}


double COLOR4D::ContrastRatio( const COLOR4D& aLeft, const COLOR4D& aRight )
{
    // Formula from https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio
    double aRL = aLeft.RelativeLuminance();
    double bRL = aRight.RelativeLuminance();

    if( aRL > bRL )
        return ( aRL + 0.05 ) / ( bRL + 0.05 );
    else
        return ( bRL + 0.05 ) / ( aRL + 0.05 );
}