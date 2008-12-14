/********************/
/* Fichier colors.h */
/********************/

#ifndef _COLORS_H
#define _COLORS_H

#ifndef COMMON_GLOBL
#define COMMON_GLOBL extern
#endif

/* Definitions des Numeros des Couleurs ( palette de 32) */
#define NBCOLOR             32

#define MASKCOLOR           31           ///< mask for color index into ColorRefs[]

/// bit indicateur d'affichage (vu / non vu) des items : (defini dans les valeurs des couleurs
#define ITEM_NOT_SHOW       (1<<18)         // 0x40000

/// Definition du bit de surbrillance
#define HIGHT_LIGHT_FLAG    (1<<19)         // 0x80000


/**
 * Function SetAlpha
 * ORs in the alpha blend parameter in to a color index.
 */
static inline void SetAlpha( int* aColor, int aBlend )
{
    const int MASKALPHA = 0xFF;

    *aColor = (*aColor & ~(MASKALPHA << 24)) | ((aBlend & MASKALPHA) << 24);
}


/**
 * Function GetAlpha
 * returns the alpha blend parameter from a color index.
 */
static inline int GetAlpha( int aColor )
{
    const int MASKALPHA = 0xFF;
    return (aColor >> 24) & MASKALPHA;
}


enum EDA_Colors
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
    LASTCOLOR
};


struct StructColors
{
    unsigned char   m_Blue;
    unsigned char   m_Green;
    unsigned char   m_Red;
    unsigned char   m_Numcolor;

    const wxChar*   m_Name;
    int             m_LightColor;
};


extern StructColors ColorRefs[NBCOLOR];
#ifdef MAIN
StructColors ColorRefs[NBCOLOR] =
{
     { 0,  0,   0,  BLACK, wxT("BLACK"), DARKDARKGRAY},
     { 192,  0,  0, BLUE, wxT("BLUE"), LIGHTBLUE},
     { 0, 160,  0,  GREEN, wxT("GREEN"), LIGHTGREEN },
     { 160, 160, 0,  CYAN, wxT("CYAN"), LIGHTCYAN },
     { 0,  0, 160,  RED, wxT("RED"), LIGHTRED },
     { 160,  0, 160,  MAGENTA, wxT("MAGENTA"), LIGHTMAGENTA },
     { 0, 128, 128,  BROWN, wxT("BROWN"), YELLOW },
     { 192, 192, 192,  LIGHTGRAY, wxT("GRAY"), WHITE },
     { 128,  128,  128,  DARKGRAY, wxT("DARKGRAY"), LIGHTGRAY },
     { 255,   0, 0,  LIGHTBLUE, wxT("LIGHTBLUE"),  LIGHTBLUE },
     { 0, 255, 0, LIGHTGREEN, wxT("LIGHTGREEN"), LIGHTGREEN },
     { 255, 255, 0, LIGHTCYAN, wxT("LIGHTCYAN"), LIGHTCYAN },
     { 0,  0, 255, LIGHTRED, wxT("LIGHTRED"), LIGHTRED },
     { 255,  0, 255, LIGHTMAGENTA, wxT("LIGHTMAGENTA"), LIGHTMAGENTA },
     { 0, 255, 255, YELLOW, wxT("YELLOW"), YELLOW },
     { 255, 255, 255, WHITE, wxT("WHITE"), WHITE },
     {  64,  64, 64,  DARKDARKGRAY, wxT("DARKDARKGRAY"),  DARKGRAY },
     {  64,   0,  0,  DARKBLUE, wxT("DARKBLUE"), BLUE },
     {    0,  64,  0,  DARKGREEN, wxT("DARKGREEN"),  GREEN },
     {  64,  64,  0,  DARKCYAN, wxT("DARKCYAN"),  CYAN },
     {    0,   0, 80,  DARKRED, wxT("DARKRED"),  RED },
     {  64,   0, 64,  DARKMAGENTA, wxT("DARKMAGENTA"), MAGENTA },
     {    0,  64, 64,  DARKBROWN, wxT("DARKBROWN"),  BROWN },
     {  128, 255, 255,   LIGHTYELLOW, wxT("LIGHTYELLOW"),   LIGHTYELLOW }
   };
#endif /* ifdef MAIN */


/**
 * Function MakeColour
 * returns a wxWidgets wxColor from a KICAD color index with alpha value.
 * Note that alpha support is not available on every wxWidgets platform.  On
 * such platform the behavior is the same as for wxALPHA_OPAQUE and that
 * means the alpha value has no effect and will be ignored.  wxGtk 2.8.4 is
 * not supporting alpha.
 * @return wxColour - given a KICAD color index with alpha value
 */
static inline wxColour MakeColour( int aColor )
{
#if wxCHECK_VERSION(2,8,5)
    int alpha = GetAlpha( aColor );
    alpha = alpha ? alpha : wxALPHA_OPAQUE;
#endif
    int ndx = aColor & MASKCOLOR;

    return wxColour(
                ColorRefs[ndx].m_Red,
                ColorRefs[ndx].m_Green,
                ColorRefs[ndx].m_Blue
#if wxCHECK_VERSION(2,8,5)
                ,(unsigned char) alpha
#endif
                );
}

#endif /* ifndef _COLORS_H */
