/************/
/* colors.h */
/************/

#ifndef _COLORS_H
#define _COLORS_H

#include <wx/wx.h>

/** The color enumeration. Also contains a flag and the alpha value in
 * the upper bits
 */
enum EDA_COLOR_T
{
    UNSPECIFIED_COLOR = -1,
    BLACK = 0,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE,
    DARKDARKGRAY,
    DARKBLUE,
    DARKGREEN,
    DARKCYAN,
    DARKRED,
    DARKMAGENTA,
    DARKBROWN,
    LIGHTYELLOW,
    LASTCOLOR,
    HIGHLIGHT_FLAG =  ( 1<<19 ),
    NBCOLOR        =    24,      ///< Number of colors
    MASKCOLOR      =    31       ///< mask for color index into ColorRefs[]
};

/// Checked cast. Use only when necessary (ex. I/O)
inline EDA_COLOR_T ColorFromInt( int aColor )
{
    wxASSERT( aColor >= UNSPECIFIED_COLOR && aColor < LASTCOLOR );
    return static_cast<EDA_COLOR_T>( aColor );
}

/// Return only the plain color part
inline EDA_COLOR_T ColorGetBase( EDA_COLOR_T aColor)
{
    return static_cast<EDA_COLOR_T>( aColor & MASKCOLOR );
}

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

// list of existing Colors:
extern StructColors ColorRefs[NBCOLOR];

/**
 * Function MakeColour
 * returns a wxWidgets wxColor from a KiCad color index with alpha value.
 * Note that alpha support is not available on every wxWidgets platform.  On
 * such platform the behavior is the same as for wxALPHA_OPAQUE and that
 * means the alpha value has no effect and will be ignored.  wxGtk 2.8.4 is
 * not supporting alpha.
 * @return wxColour - given a KiCad color index with alpha value
 */
inline wxColour MakeColour( EDA_COLOR_T aColor )
{
#if wxCHECK_VERSION(2,8,5)
    int alpha = GetAlpha( aColor );
    alpha = alpha ? alpha : wxALPHA_OPAQUE;
#endif
    int ndx = aColor & MASKCOLOR;

    return wxColour( ColorRefs[ndx].m_Red,
                     ColorRefs[ndx].m_Green,
                     ColorRefs[ndx].m_Blue
#if wxCHECK_VERSION(2,8,5)
                     ,(unsigned char) alpha
#endif
        );
}

#endif /* ifndef _COLORS_H */
