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

#include <gal/color4d.h>

using namespace KiGfx;

COLOR4D::COLOR4D( EDA_COLOR_T aColor )
{
    r = g_ColorRefs[aColor].m_Red;
    g = g_ColorRefs[aColor].m_Green;
    b = g_ColorRefs[aColor].m_Blue;
    a = 1.0;
}


#ifdef WX_COMPATIBILITY
    COLOR4D::COLOR4D( const wxColour& aColor )
    {
        r = aColor.Red();
        g = aColor.Green();
        b = aColor.Blue();
        a = aColor.Alpha();
    }
#endif


const bool COLOR4D::operator==( const COLOR4D& aColor )
{
    return a == aColor.a && r == aColor.r && g == aColor.g && b == aColor.b;
}


const bool COLOR4D::operator!=( const COLOR4D& aColor )
{
    return a != aColor.a || r != aColor.r || g != aColor.g || b != aColor.b;
}

void COLOR4D::ToHSV(double& out_h, double& out_s, double& out_v) const
{
    double      min, max, delta;

    min = r < g ? r : g;
    min = min  < b ? min  : b;

    max = r > g ? r : g;
    max = max  > b ? max  : b;

    out_v = max;                                // v
    delta = max - min;
    
    if( max > 0.0 ) {
        out_s = (delta / max);                  // s
    } else {
        // r = g = b = 0                        // s = 0, v is undefined
        out_s = 0.0;
        out_h = NAN;                            // its now undefined
        return;
    }
    if( r >= max )                           // > is bogus, just keeps compilor happy
        out_h = ( g - b ) / delta;        // between yellow & magenta
    else
    if( g >= max )
        out_h = 2.0 + ( b - r ) / delta;  // between cyan & yellow
    else
        out_h = 4.0 + ( r - g ) / delta;  // between magenta & cyan

    out_h *= 60.0;                              // degrees

    if( out_h < 0.0 )
        out_h += 360.0;
}

void COLOR4D::FromHSV(double in_h, double in_s, double in_v)
{
    double      hh, p, q, t, ff;
    long        i;

    if(in_s <= 0.0) {       // < is bogus, just shuts up warnings
        r = in_v;
        g = in_v;
        b = in_v;
        return;
    }
    hh = in_h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in_v * (1.0 - in_s);
    q = in_v * (1.0 - (in_s * ff));
    t = in_v * (1.0 - (in_s * (1.0 - ff)));

    switch(i) {
    case 0:
        r = in_v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = in_v;
        b = p;
        break;
    case 2:
        r = p;
        g = in_v;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = in_v;
        break;
    case 4:
        r = t;
        g = p;
        b = in_v;
        break;
    case 5:
    default:
        r = in_v;
        g = p;
        b = q;
        break;
    }
    
}

COLOR4D& COLOR4D::Saturate( double aFactor )
{
    double h, s, v;
    ToHSV(h, s, v);
    FromHSV(h, aFactor, 1.0);
    
    return *this;
}
        