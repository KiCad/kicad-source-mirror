/* libwmf (<libwmf/defs.h>): library for wmf conversion
   Copyright (C) 2000 - various; see CREDITS, ChangeLog, and sources

   The libwmf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The libwmf Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the libwmf Library; see the file COPYING.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#ifndef LIBWMF_DEFS_H
#define LIBWMF_DEFS_H

/* PolyFill() Modes */
#undef  ALTERNATE
#define ALTERNATE           1
#undef  WINDING
#define WINDING             2
#undef  POLYFILL_LAST
#define POLYFILL_LAST       2

/* Brush Styles */
#undef  BS_SOLID
#define BS_SOLID            0
#undef  BS_NULL
#define BS_NULL             1
#undef  BS_HOLLOW
#define BS_HOLLOW     BS_NULL
#undef  BS_HATCHED
#define BS_HATCHED          2
#undef  BS_PATTERN
#define BS_PATTERN          3
#undef  BS_INDEXED
#define BS_INDEXED          4
#undef  BS_DIBPATTERN
#define BS_DIBPATTERN       5
#undef  BS_DIBPATTERNPT
#define BS_DIBPATTERNPT     6
#undef  BS_PATTERN8X8
#define BS_PATTERN8X8       7
#undef  BS_DIBPATTERN8X8
#define BS_DIBPATTERN8X8    8

#define TO_FILL(Z) ((Z)->dc->brush->lbStyle != BS_NULL)

/* Hatch Styles */
#undef  HS_HORIZONTAL
#define HS_HORIZONTAL       0       /* ----- */
#undef  HS_VERTICAL
#define HS_VERTICAL         1       /* ||||| */
#undef  HS_FDIAGONAL
#define HS_FDIAGONAL        2       /* \\\\\ */
#undef  HS_BDIAGONAL
#define HS_BDIAGONAL        3       /* ///// */
#undef  HS_CROSS
#define HS_CROSS            4       /* +++++ */
#undef  HS_DIAGCROSS
#define HS_DIAGCROSS        5       /* xxxxx */

/* Pen Styles */
#undef  PS_SOLID
#define PS_SOLID            0
#undef  PS_DASH
#define PS_DASH             1       /* -------  */
#undef  PS_DOT
#define PS_DOT              2       /* .......  */
#undef  PS_DASHDOT
#define PS_DASHDOT          3       /* _._._._  */
#undef  PS_DASHDOTDOT
#define PS_DASHDOTDOT       4       /* _.._.._  */
#undef  PS_NULL
#define PS_NULL             5
#undef  PS_INSIDEFRAME
#define PS_INSIDEFRAME      6
#undef  PS_USERSTYLE
#define PS_USERSTYLE        7
#undef  PS_ALTERNATE
#define PS_ALTERNATE        8
#undef  PS_STYLE_MASK
#define PS_STYLE_MASK       0x0000000F

#define TO_DRAW(Z) (((Z)->dc->pen->lopnStyle & PS_STYLE_MASK) != PS_NULL)

#undef  PS_ENDCAP_ROUND
#define PS_ENDCAP_ROUND     0x00000000
#undef  PS_ENDCAP_SQUARE
#define PS_ENDCAP_SQUARE    0x00000100
#undef  PS_ENDCAP_FLAT
#define PS_ENDCAP_FLAT      0x00000200
#undef  PS_ENDCAP_MASK
#define PS_ENDCAP_MASK      0x00000F00

#undef  PS_JOIN_ROUND
#define PS_JOIN_ROUND       0x00000000
#undef  PS_JOIN_BEVEL
#define PS_JOIN_BEVEL       0x00001000
#undef  PS_JOIN_MITER
#define PS_JOIN_MITER       0x00002000
#undef  PS_JOIN_MASK
#define PS_JOIN_MASK        0x0000F000

#undef  PS_COSMETIC
#define PS_COSMETIC         0x00000000
#undef  PS_GEOMETRIC
#define PS_GEOMETRIC        0x00010000
#undef  PS_TYPE_MASK
#define PS_TYPE_MASK        0x000F0000

/* Object Definitions for EnumObjects() */
#undef  OBJ_PEN
#define OBJ_PEN             1
#undef  OBJ_BRUSH
#define OBJ_BRUSH           2
#undef  OBJ_DC
#define OBJ_DC              3
#undef  OBJ_METADC
#define OBJ_METADC          4
#undef  OBJ_PAL
#define OBJ_PAL             5
#undef  OBJ_FONT
#define OBJ_FONT            6
#undef  OBJ_BITMAP
#define OBJ_BITMAP          7
#undef  OBJ_REGION
#define OBJ_REGION          8
#undef  OBJ_METAFILE
#define OBJ_METAFILE        9
#undef  OBJ_MEMDC
#define OBJ_MEMDC          10
#undef  OBJ_EXTPEN
#define OBJ_EXTPEN         11
#undef  OBJ_ENHMETADC
#define OBJ_ENHMETADC      12
#undef  OBJ_ENHMETAFILE
#define OBJ_ENHMETAFILE    13

/* Text Alignment Options */
#undef  TA_NOUPDATECP
#define TA_NOUPDATECP       0
#undef  TA_UPDATECP
#define TA_UPDATECP         1

#undef  TA_LEFT
#define TA_LEFT             0
#undef  TA_RIGHT
#define TA_RIGHT            2
#undef  TA_CENTER
#define TA_CENTER           6

#undef  TA_TOP
#define TA_TOP              0
#undef  TA_BOTTOM
#define TA_BOTTOM           8
#undef  TA_BASELINE
#define TA_BASELINE        24
#if (WINVER >= 0x0400)
#undef  TA_RTLREADING
#define TA_RTLREADING     256
#undef  TA_MASK
#define TA_MASK              (TA_BASELINE+TA_CENTER+TA_UPDATECP+TA_RTLREADING)
#else
#undef  TA_MASK
#define TA_MASK              (TA_BASELINE+TA_CENTER+TA_UPDATECP)
#endif

/* Binary raster ops */
#undef  R2_BLACK
#define R2_BLACK            1   /*  0       */
#undef  R2_NOTMERGEPEN
#define R2_NOTMERGEPEN      2   /* DPon     */
#undef  R2_MASKNOTPEN
#define R2_MASKNOTPEN       3   /* DPna     */
#undef  R2_NOTCOPYPEN
#define R2_NOTCOPYPEN       4   /* PN       */
#undef  R2_MASKPENNOT
#define R2_MASKPENNOT       5   /* PDna     */
#undef  R2_NOT
#define R2_NOT              6   /* Dn       */
#undef  R2_XORPEN
#define R2_XORPEN           7   /* DPx      */
#undef  R2_NOTMASKPEN
#define R2_NOTMASKPEN       8   /* DPan     */
#undef  R2_MASKPEN
#define R2_MASKPEN          9   /* DPa      */
#undef  R2_NOTXORPEN
#define R2_NOTXORPEN       10   /* DPxn     */
#undef  R2_NOP
#define R2_NOP             11   /* D        */
#undef  R2_MERGENOTPEN
#define R2_MERGENOTPEN     12   /* DPno     */
#undef  R2_COPYPEN
#define R2_COPYPEN         13   /* P        */
#undef  R2_MERGEPENNOT
#define R2_MERGEPENNOT     14   /* PDno     */
#undef  R2_MERGEPEN
#define R2_MERGEPEN        15   /* DPo      */
#undef  R2_WHITE
#define R2_WHITE           16   /*  1       */
#undef  R2_LAST
#define R2_LAST            16

/* Ternary raster operations */
#undef  SRCCOPY
#define SRCCOPY        (U32)0x00CC0020 /* dest = source                   */
#undef  SRCPAINT
#define SRCPAINT       (U32)0x00EE0086 /* dest = source OR dest           */
#undef  SRCAND
#define SRCAND         (U32)0x008800C6 /* dest = source AND dest          */
#undef  SRCINVERT
#define SRCINVERT      (U32)0x00660046 /* dest = source XOR dest          */
#undef  SRCERASE
#define SRCERASE       (U32)0x00440328 /* dest = source AND (NOT dest )   */
#undef  NOTSRCCOPY
#define NOTSRCCOPY     (U32)0x00330008 /* dest = (NOT source)             */
#undef  NOTSRCERASE
#define NOTSRCERASE    (U32)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
#undef  MERGECOPY
#define MERGECOPY      (U32)0x00C000CA /* dest = (source AND pattern)     */
#undef  MERGEPAINT
#define MERGEPAINT     (U32)0x00BB0226 /* dest = (NOT source) OR dest     */
#undef  PATCOPY
#define PATCOPY        (U32)0x00F00021 /* dest = pattern                  */
#undef  PATPAINT
#define PATPAINT       (U32)0x00FB0A09 /* dest = DPSnoo                   */
#undef  PATINVERT
#define PATINVERT      (U32)0x005A0049 /* dest = pattern XOR dest         */
#undef  DSTINVERT
#define DSTINVERT      (U32)0x00550009 /* dest = (NOT dest)               */
#undef  BLACKNESS
#define BLACKNESS      (U32)0x00000042 /* dest = BLACK                    */
#undef  WHITENESS
#define WHITENESS      (U32)0x00FF0062 /* dest = WHITE                    */

/* StretchBlt() Modes */
#undef  BLACKONWHITE
#define BLACKONWHITE        1
#undef  WHITEONBLACK
#define WHITEONBLACK        2
#undef  COLORONCOLOR
#define COLORONCOLOR        3
#undef  HALFTONE
#define HALFTONE            4
#undef  MAXSTRETCHBLTMODE
#define MAXSTRETCHBLTMODE   4

#if(WINVER >= 0x0400)
/* New StretchBlt() Modes */
#undef  STRETCH_ANDSCANS
#define STRETCH_ANDSCANS    BLACKONWHITE
#undef  STRETCH_ORSCANS
#define STRETCH_ORSCANS     WHITEONBLACK
#undef  STRETCH_DELETESCANS
#define STRETCH_DELETESCANS COLORONCOLOR
#undef  STRETCH_HALFTONE
#define STRETCH_HALFTONE    HALFTONE
#endif /* WINVER >= 0x0400 */

/* Background Modes */
#undef  TRANSPARENT
#define TRANSPARENT         1
#undef  OPAQUE
#define OPAQUE              2
#undef  BKMODE_LAST
#define BKMODE_LAST         2

#undef  ETO_OPAQUE
#define ETO_OPAQUE          0x0002
#undef  ETO_CLIPPED
#define ETO_CLIPPED         0x0004
#if(WINVER >= 0x0400)
#undef  ETO_GLYPH_INDEX
#define ETO_GLYPH_INDEX     0x0010
#undef  ETO_RTLREADING
#define ETO_RTLREADING      0x0080
#endif /* WINVER >= 0x0400 */

/* ExtFloodFill style flags */
#undef   FLOODFILLBORDER
#define  FLOODFILLBORDER    0
#undef   FLOODFILLSURFACE
#define  FLOODFILLSURFACE   1

#endif /* ! LIBWMF_DEFS_H */
