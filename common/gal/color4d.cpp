/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * Color class
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


    wxString COLOR4D::ToWxString( long flags )
    {
        wxColour c = ToColour();
        return c.GetAsString( flags );
    }


    COLOR4D COLOR4D::LegacyMix( COLOR4D aColor ) const
    {
        COLOR4D candidate;

        // Blend the two colors (i.e. OR the RGB values)
        candidate.r = ( (unsigned)( 255.0 * r ) | (unsigned)( 255.0 * aColor.r ) ) / 255.0,
        candidate.g = ( (unsigned)( 255.0 * g ) | (unsigned)( 255.0 * aColor.g ) ) / 255.0,
        candidate.b = ( (unsigned)( 255.0 * b ) | (unsigned)( 255.0 * aColor.b ) ) / 255.0,
        candidate.a = 1.0;

        return candidate;
    }


    COLOR4D& COLOR4D::SetToLegacyHighlightColor()
    {
        EDA_COLOR_T legacyColor = GetNearestLegacyColor( *this );
        EDA_COLOR_T highlightColor = g_ColorRefs[legacyColor].m_LightColor;

        r = g_ColorRefs[highlightColor].m_Red / 255.0;
        g = g_ColorRefs[highlightColor].m_Green / 255.0;
        b = g_ColorRefs[highlightColor].m_Blue / 255.0;
        a = 1.0;

        return *this;
    }


    COLOR4D& COLOR4D::SetToNearestLegacyColor()
    {
        EDA_COLOR_T legacyColor = GetNearestLegacyColor( *this );

        r = g_ColorRefs[legacyColor].m_Red / 255.0;
        g = g_ColorRefs[legacyColor].m_Green / 255.0;
        b = g_ColorRefs[legacyColor].m_Blue / 255.0;
        a = 1.0;

        return *this;
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


    EDA_COLOR_T COLOR4D::GetNearestLegacyColor( COLOR4D &aColor )
    {
        // Cache layer implemented here, because all callers are using wxColour
        static std::map< unsigned int, unsigned int > nearestCache;
        static double hues[NBCOLORS];
        static double values[NBCOLORS];

        unsigned int colorInt = aColor.ToU32();

        auto search = nearestCache.find( colorInt );

        if( search != nearestCache.end() )
            return static_cast<EDA_COLOR_T>( search->second );

        // First use ColorFindNearest to check for exact matches
        EDA_COLOR_T nearest = ColorFindNearest( aColor.r * 255.0, aColor.g * 255.0, aColor.b * 255.0 );

        if( COLOR4D( nearest ) == aColor )
        {
            nearestCache.insert( std::pair< unsigned int, unsigned int >(
                                 colorInt, static_cast<unsigned int>( nearest ) ) );
            return nearest;
        }

        // If not, use hue and value to match.
        // Hue will be NAN for grayscale colors.
        // The legacy color palette is a grid across hue and value.
        // We can exploit that to find a good match -- hue is most apparent to the user.
        // So, first we determine the closest hue match, and then the closest value from that
        // "grid row" in the legacy palette.

        double h, s, v;
        aColor.ToHSV( h, s, v );

        double minDist = 360.0;
        double legacyHue = 0.0;

        if( std::isnan( h ) )
        {
            legacyHue = NAN;
        }
        else
        {
            for( EDA_COLOR_T candidate = ::BLACK;
                    candidate < NBCOLORS; candidate = NextColor( candidate ) )
            {
                double ch, cs, cv;

                if( hues[candidate] == 0.0 && values[candidate] == 0.0 )
                {
                    COLOR4D candidate4d( candidate );

                    candidate4d.ToHSV( ch, cs, cv );

                    values[candidate] = cv;
                    // Set the hue to non-zero for black so that we won't do this more than once
                    hues[candidate] = ( cv == 0.0 ) ? 1.0 : ch;
                }
                else
                {
                    ch = hues[candidate];
                    cv = values[candidate];
                    cv = 0.0;
                }

                if( fabs( ch - h ) < minDist )
                {
                    minDist = fabs( ch - h );
                    legacyHue = ch;
                }
            }
        }

        // Now we have the desired hue; let's find the nearest value
        minDist = 1.0;
        for( EDA_COLOR_T candidate = ::BLACK;
                candidate < NBCOLORS; candidate = NextColor( candidate ) )
        {
            // If the target hue is NAN, we didn't extract the value for any colors above
            if( std::isnan( legacyHue ) )
            {
                double ch, cs, cv;
                COLOR4D candidate4d( candidate );
                candidate4d.ToHSV( ch, cs, cv );
                values[candidate] = cv;
                hues[candidate] = ( cv == 0.0 ) ? 1.0 : ch;
            }

            if( ( std::isnan( legacyHue ) != std::isnan( hues[candidate] ) ) || hues[candidate] != legacyHue )
                continue;

            if( fabs( values[candidate] - v ) < minDist )
            {
                minDist = fabs( values[candidate] - v );
                nearest = candidate;
            }
        }

        nearestCache.insert( std::pair< unsigned int, unsigned int >(
                             colorInt, static_cast<unsigned int>( nearest ) ) );

        return nearest;
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

}


void COLOR4D::ToHSV( double& aOutH, double& aOutS, double& aOutV ) const
{
    double min, max, delta;

    min = r < g ? r : g;
    min = min < b ? min : b;

    max = r > g ? r : g;
    max = max > b ? max : b;

    aOutV = max;                                // v
    delta = max - min;

    if( max > 0.0 )
    {
        aOutS = ( delta / max );                  // s
    }
    else
    {
        // r = g = b = 0                        // s = 0, v is undefined
        aOutS = 0.0;
        aOutH = NAN;                            // its now undefined
        return;
    }

    if( r >= max )                          // > is bogus, just keeps compiler happy
        aOutH = ( g - b ) / delta;          // between yellow & magenta
    else if( g >= max )
        aOutH = 2.0 + ( b - r ) / delta;    // between cyan & yellow
    else
        aOutH = 4.0 + ( r - g ) / delta;    // between magenta & cyan

    aOutH *= 60.0;                          // degrees

    if( aOutH < 0.0 )
        aOutH += 360.0;
}


void COLOR4D::FromHSV( double aInH, double aInS, double aInV )
{
    double hh, p, q, t, ff;
    long i;

    if( aInS <= 0.0 )    // < is bogus, just shuts up warnings
    {
        r = aInV;
        g = aInV;
        b = aInV;
        return;
    }

    hh = aInH;

    if( hh >= 360.0 )
        hh = 0.0;

    hh /= 60.0;

    i = (long) hh;
    ff = hh - i;

    p = aInV * ( 1.0 - aInS );
    q = aInV * ( 1.0 - ( aInS * ff ) );
    t = aInV * ( 1.0 - ( aInS * ( 1.0 - ff ) ) );

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
    double h, s, v;

    ToHSV( h, s, v );
    FromHSV( h, aFactor, 1.0 );

    return *this;
}

const COLOR4D COLOR4D::UNSPECIFIED( 0, 0, 0, 0 );
const COLOR4D COLOR4D::WHITE( 1, 1, 1, 1 );
const COLOR4D COLOR4D::BLACK( 0, 0, 0, 1 );
