/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <colors.h>


/**
 * The predefined colors used in KiCad.
 * Please: if you change a value, remember these values are carefully chosen
 * to have good results in Pcbnew, that uses the ORed value of basic colors
 * when displaying superimposed objects
 * This list must have exactly NBCOLORS items
 */
const StructColors g_ColorRefs[NBCOLORS] =
{
    { 0,    0,   0,   BLACK,         wxT( "Black" ),     DARKDARKGRAY      },
    { 72,   72,  72,  DARKDARKGRAY,  wxT( "Gray 1" ),    DARKGRAY          },
    { 132,  132, 132, DARKGRAY,      wxT( "Gray 2" ),    LIGHTGRAY         },
    { 194,  194, 194, LIGHTGRAY,     wxT( "Gray 3" ),    WHITE             },
    { 255,  255, 255, WHITE,         wxT( "White" ),     WHITE             },
    { 194,  255, 255, LIGHTYELLOW,   wxT( "L.Yellow" ),  WHITE             },
    { 72,   0,   0,   DARKBLUE,      wxT( "Blue 1" ),    BLUE              },
    { 0,    72,  0,   DARKGREEN,     wxT( "Green 1" ),   GREEN             },
    { 72,   72,  0,   DARKCYAN,      wxT( "Cyan 1" ),    CYAN              },
    { 0,    0,   72,  DARKRED,       wxT( "Red 1" ),     RED               },
    { 72,   0,   72,  DARKMAGENTA,   wxT( "Magenta 1" ), MAGENTA           },
    { 0,    72,  72,  DARKBROWN,     wxT( "Brown 1" ),   BROWN             },
    { 132,  0,   0,   BLUE,          wxT( "Blue 2" ),    LIGHTBLUE         },
    { 0,    132, 0,   GREEN,         wxT( "Green 2" ),   LIGHTGREEN        },
    { 132,  132, 0,   CYAN,          wxT( "Cyan 2" ),    LIGHTCYAN         },
    { 0,    0,   132, RED,           wxT( "Red 2" ),     LIGHTRED          },
    { 132,  0,   132, MAGENTA,       wxT( "Magenta 2" ), LIGHTMAGENTA      },
    { 0,    132, 132, BROWN,         wxT( "Brown 2" ),   YELLOW            },
    { 194,  0,   0,   LIGHTBLUE,     wxT( "Blue 3" ),    PUREBLUE,         },
    { 0,    194, 0,   LIGHTGREEN,    wxT( "Green 3" ),   PUREGREEN         },
    { 194,  194, 0,   LIGHTCYAN,     wxT( "Cyan 3" ),    PURECYAN          },
    { 0,    0,   194, LIGHTRED,      wxT( "Red 3" ),     PURERED           },
    { 194,  0,   194, LIGHTMAGENTA,  wxT( "Magenta 3" ), PUREMAGENTA       },
    { 0,    194, 194, YELLOW,        wxT( "Yellow 3" ),  PUREYELLOW        },
    { 255,  0,   0,   PUREBLUE,      wxT( "Blue 4" ),    WHITE             },
    { 0,    255, 0,   PUREGREEN,     wxT( "Green 4" ),   WHITE             },
    { 255,  255, 0,   PURECYAN,      wxT( "Cyan 4" ),    WHITE             },
    { 0,    0,   255, PURERED,       wxT( "Red 4" ),     WHITE             },
    { 255,  0,   255, PUREMAGENTA,   wxT( "Magenta 4" ), WHITE             },
    { 0,    255, 255, PUREYELLOW,    wxT( "Yellow 4" ),  WHITE             },
};


EDA_COLOR_T ColorByName( const wxString& aName )
{
    // look for a match in the palette itself
    for( EDA_COLOR_T trying = BLACK; trying < NBCOLORS; trying = NextColor(trying) )
    {
        if( 0 == aName.CmpNoCase( g_ColorRefs[trying].m_Name ) )
            return trying;
    }

    // Not found, no idea...
    return UNSPECIFIED_COLOR;
}


bool ColorIsLight( EDA_COLOR_T aColor )
{
    const StructColors &c = g_ColorRefs[ColorGetBase( aColor )];
    int r = c.m_Red;
    int g = c.m_Green;
    int b = c.m_Blue;
    return ((r * r) + (g * g) + (b * b)) > (128 * 128 * 3);
}


EDA_COLOR_T ColorFindNearest( const wxColour &aColor )
{
    return ColorFindNearest( aColor.Red(), aColor.Green(), aColor.Blue() );
}


EDA_COLOR_T ColorFindNearest( int aR, int aG, int aB )
{
    EDA_COLOR_T candidate = BLACK;

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

    for( EDA_COLOR_T trying = BLACK; trying < NBCOLORS; trying = NextColor(trying) )
    {
        const StructColors &c = g_ColorRefs[trying];
        int distance = (aR - c.m_Red) * (aR - c.m_Red) +
            (aG - c.m_Green) * (aG - c.m_Green) +
            (aB - c.m_Blue) * (aB - c.m_Blue);

        if( distance < nearest_distance && c.m_Red >= aR &&
            c.m_Green >= aG && c.m_Blue >= aB )
        {
            nearest_distance = distance;
            candidate = trying;
        }
    }

    return candidate;
}


EDA_COLOR_T ColorMix( EDA_COLOR_T aColor1, EDA_COLOR_T aColor2 )
{
    /* Memoization storage. This could be potentially called for each
     * color merge so a cache is useful (there are few colours anyway) */
    static EDA_COLOR_T mix_cache[NBCOLORS][NBCOLORS];

    // TODO how is alpha used? it's a mac only thing, I have no idea
    aColor1 = ColorGetBase( aColor1 );
    aColor2 = ColorGetBase( aColor2 );

    // First easy thing: a black gives always the other colour
    if( aColor1 == BLACK )
        return aColor2;

    if( aColor2 == BLACK)
        return aColor1;

    /* Now we are sure that black can't occur, so the rule is:
     * BLACK means not computed yet. If we're lucky we already have
     * an answer */
    EDA_COLOR_T candidate = mix_cache[aColor1][aColor2];

    if( candidate != BLACK )
        return candidate;

    // Blend the two colors (i.e. OR the RGB values)
    const StructColors &c1 = g_ColorRefs[aColor1];
    const StructColors &c2 = g_ColorRefs[aColor2];

    // Ask the palette for the nearest color to the mix
    wxColour mixed( c1.m_Red | c2.m_Red,
                    c1.m_Green | c2.m_Green,
                    c1.m_Blue | c2.m_Blue );
    candidate = ColorFindNearest( mixed );

    /* Here, BLACK is *not* a good answer, since it would recompute the next time.
     * Even theorically its not possible (with the current rules), but
     * maybe the metric will change in the future */
    if( candidate == BLACK )
        candidate = DARKDARKGRAY;

    // Store the result in the cache. The operation is commutative, too
    mix_cache[aColor1][aColor2] = candidate;
    mix_cache[aColor2][aColor1] = candidate;
    return candidate;
}

