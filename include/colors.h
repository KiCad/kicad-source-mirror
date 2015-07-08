/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef COLORS_H_
#define COLORS_H_

#include <wx/wx.h>

/** The color enumeration. Also contains a flag and the alpha value in
 * the upper bits
 */
enum EDA_COLOR_T
{
    UNSPECIFIED_COLOR = -1,
    BLACK = 0,
    DARKDARKGRAY,
    DARKGRAY,
    LIGHTGRAY,
    WHITE,
    LIGHTYELLOW,
    DARKBLUE,
    DARKGREEN,
    DARKCYAN,
    DARKRED,
    DARKMAGENTA,
    DARKBROWN,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    PUREBLUE,
    PUREGREEN,
    PURECYAN,
    PURERED,
    PUREMAGENTA,
    PUREYELLOW,
    NBCOLORS,                    ///< Number of colors
    HIGHLIGHT_FLAG =  ( 1<<19 ),
    MASKCOLOR      =    31       ///< mask for color index into g_ColorRefs[]
};

/// Checked cast. Use only when necessary (usually I/O)
inline EDA_COLOR_T ColorFromInt( int aColor )
{
    wxASSERT( aColor >= UNSPECIFIED_COLOR && aColor < NBCOLORS );
    return static_cast<EDA_COLOR_T>( aColor );
}

inline EDA_COLOR_T NextColor( EDA_COLOR_T& aColor )
{
    // We have to accept NBCOLORS for loop termination conditions
    wxASSERT( aColor >= UNSPECIFIED_COLOR && aColor <= NBCOLORS );
    aColor = static_cast<EDA_COLOR_T>( int( aColor ) + 1 );
    return aColor;
}

/// Return only the plain color part
inline EDA_COLOR_T ColorGetBase( EDA_COLOR_T aColor)
{
    EDA_COLOR_T base = static_cast<EDA_COLOR_T>( aColor & MASKCOLOR );
    return base;
}

/// Mix two colors in some way (hopefully like a logical OR)
EDA_COLOR_T ColorMix( EDA_COLOR_T aColor1, EDA_COLOR_T aColor2 );

/// Force the color part of a color to darkdarkgray
inline void ColorTurnToDarkDarkGray( EDA_COLOR_T *aColor )
{
    *aColor = static_cast<EDA_COLOR_T>( (int(*aColor) & ~MASKCOLOR) | DARKDARKGRAY );
}

inline void ColorChangeHighlightFlag( EDA_COLOR_T *aColor, bool flag )
{
    if( flag )
        *aColor = static_cast<EDA_COLOR_T>( (int(*aColor) | HIGHLIGHT_FLAG ) );
    else
        *aColor = static_cast<EDA_COLOR_T>( (int(*aColor) & ~HIGHLIGHT_FLAG ) );
}

/**
 * Function SetAlpha
 * ORs in the alpha blend parameter in to a color index.
 */
inline void SetAlpha( EDA_COLOR_T* aColor, unsigned char aBlend )
{
    const unsigned char MASKALPHA = 0xFF;

    *aColor = static_cast<EDA_COLOR_T>((*aColor & ~(MASKALPHA << 24))
                                     | ((aBlend & MASKALPHA) << 24));
}

/**
 * Function GetAlpha
 * returns the alpha blend parameter from a color index.
 */
inline unsigned char GetAlpha( EDA_COLOR_T aColor )
{
    const unsigned char MASKALPHA = 0xFF;
    return (aColor >> 24) & MASKALPHA;
}


struct StructColors
{
    unsigned char   m_Blue;
    unsigned char   m_Green;
    unsigned char   m_Red;
    EDA_COLOR_T     m_Numcolor;

    const wxChar*   m_Name;
    EDA_COLOR_T     m_LightColor;
};

/// list of existing Colors
extern const StructColors g_ColorRefs[NBCOLORS];

/// Step a color to the highlighted version if the highlight flag is set
inline void ColorApplyHighlightFlag( EDA_COLOR_T *aColor )
{
    EDA_COLOR_T base = ColorGetBase( *aColor );
    wxASSERT( base > UNSPECIFIED_COLOR && base < NBCOLORS );
    if( *aColor & HIGHLIGHT_FLAG )
        *aColor = g_ColorRefs[base].m_LightColor;
}

/// Find a color by name
EDA_COLOR_T ColorByName( const wxString& aName );

/// Find the nearest color match
EDA_COLOR_T ColorFindNearest( const wxColour &aColor );

/**
 * Find the nearest color match
 * @param aR is the red component of the color to be matched (in range 0-255)
 * @param aG is the green component of the color to be matched (in range 0-255)
 * @param aB is the blue component of the color to be matched (in range 0-255)
 */
EDA_COLOR_T ColorFindNearest( int aR, int aG, int aB );

/**
 * Check if a color is light i.e. if black would be more readable than
 * white on it
 */
bool ColorIsLight( EDA_COLOR_T aColor );

inline const wxChar *ColorGetName( EDA_COLOR_T aColor )
{
    EDA_COLOR_T base = ColorGetBase( aColor );
    wxASSERT( base > UNSPECIFIED_COLOR && base < NBCOLORS );
    return g_ColorRefs[base].m_Name;
}

inline void ColorSetBrush( wxBrush *aBrush, EDA_COLOR_T aColor )
{
    EDA_COLOR_T base = ColorGetBase( aColor );
    wxASSERT( base > UNSPECIFIED_COLOR && base < NBCOLORS );
    const StructColors &col = g_ColorRefs[base];
    aBrush->SetColour( col.m_Red, col.m_Green, col.m_Blue );
}

/**
 * Function MakeColour
 * returns a wxWidgets wxColor from a KiCad color index with alpha value.
 * Note that alpha support is not available on every wxWidgets platform.  On
 * such platform the behavior is the same as for wxALPHA_OPAQUE and that
 * means the alpha value has no effect and will be ignored.
 * @return wxColour - given a KiCad color index with alpha value
 */
inline wxColour MakeColour( EDA_COLOR_T aColor )
{
    int alpha = GetAlpha( aColor );
    alpha = alpha ? alpha : wxALPHA_OPAQUE;
    EDA_COLOR_T ndx = ColorGetBase( aColor );
    wxASSERT( ndx > UNSPECIFIED_COLOR && ndx < NBCOLORS );

    return wxColour( g_ColorRefs[ndx].m_Red,
                     g_ColorRefs[ndx].m_Green,
                     g_ColorRefs[ndx].m_Blue
                     ,(unsigned char) alpha
        );
}

#endif // COLORS_H_
