/* libwmf (<libwmf/macro.h>): library for wmf conversion
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


#ifndef LIBWMF_MACRO_H
#define LIBWMF_MACRO_H

#include <math.h>

#include <libwmf/defs.h>
#include <libwmf/types.h>
#include <libwmf/ipa.h>

/**
 * WMF_PEN_STYLE(wmfPen* P)  -> (U16) pen style
 */
#define WMF_PEN_STYLE(P)  ((P)->lopnStyle & PS_STYLE_MASK)

/**
 * WMF_PEN_ENDCAP(wmfPen* P) -> (U16) endcap style
 */
#define WMF_PEN_ENDCAP(P) ((P)->lopnStyle & PS_ENDCAP_MASK)

/**
 * WMF_PEN_JOIN(wmfPen* P)   -> (U16) join style
 */
#define WMF_PEN_JOIN(P)   ((P)->lopnStyle & PS_JOIN_MASK)

/**
 * WMF_PEN_TYPE(wmfPen* P)   -> (U16) `type' of pen (??)
 */
#define WMF_PEN_TYPE(P)   ((P)->lopnStyle & PS_TYPE_MASK)

/**
 * WMF_PEN_WIDTH(wmfPen* P)  -> (double) pen `width'  (thickness w.r.t. x-axis)
 */
#define WMF_PEN_WIDTH(P)  ((P)->width)

/**
 * WMF_PEN_HEIGHT(wmfPen* P) -> (double) pen `height' (thickness w.r.t. y-axis)
 */
#define WMF_PEN_HEIGHT(P) ((P)->height)

/**
 * WMF_PEN_COLOR(wmfPen* P) -> (wmfRGB*) pen color
 */
#define WMF_PEN_COLOR(P)  (&((P)->lopnColor))

/**
 * WMF_PEN_SET_STYLE(wmfPen* P,(U16) pen style)
 */
#define WMF_PEN_SET_STYLE(P,Z)  (P)->lopnStyle = ((P)->lopnStyle & ~PS_STYLE_MASK ) | ((Z) & PS_STYLE_MASK )

/**
 * WMF_PEN_SET_ENDCAP(wmfPen* P,(U16) endcap style)
 */
#define WMF_PEN_SET_ENDCAP(P,Z) (P)->lopnStyle = ((P)->lopnStyle & ~PS_ENDCAP_MASK) | ((Z) & PS_ENDCAP_MASK)

/**
 * WMF_PEN_SET_JOIN(wmfPen* P,(U16) join style)
 */
#define WMF_PEN_SET_JOIN(P,Z)   (P)->lopnStyle = ((P)->lopnStyle & ~PS_JOIN_MASK  ) | ((Z) & PS_JOIN_MASK  )

/**
 * WMF_PEN_SET_TYPE(wmfPen* P,(U16) `type' of pen)
 */
#define WMF_PEN_SET_TYPE(P,Z)   (P)->lopnStyle = ((P)->lopnStyle & ~PS_TYPE_MASK  ) | ((Z) & PS_TYPE_MASK  )

/**
 * WMF_PEN_SET_WIDTH(wmfPen* P,(double) pen `width')
 */
#define WMF_PEN_SET_WIDTH(P,Z)  (P)->width  = (Z)

/**
 * WMF_PEN_SET_HEIGHT(wmfPen* P,(double) pen `height')
 */
#define WMF_PEN_SET_HEIGHT(P,Z) (P)->height = (Z)

/**
 * WMF_PEN_SET_COLOR(wmfPen* P,(wmfRGB*) pen color)
 */
#define WMF_PEN_SET_COLOR(P,Z)  (P)->lopnColor = (*(Z))

/**
 * WMF_BRUSH_STYLE(wmfBrush* B) -> (U16) brush style
 */
#define WMF_BRUSH_STYLE(B)  ((B)->lbStyle)

/**
 * WMF_BRUSH_HATCH(wmfBrush* B) -> (U16) brush hatch style
 */
#define WMF_BRUSH_HATCH(B)  ((B)->lbHatch)

/**
 * WMF_BRUSH_COLOR(wmfBrush* B) -> (wmfRGB*) brush color
 */
#define WMF_BRUSH_COLOR(B)  (&((B)->lbColor))

/**
 * WMF_BRUSH_BITMAP(wmfBrush* B) -> (wmfBMP*) brush bitmap
 */
#define WMF_BRUSH_BITMAP(B) (&((B)->bmp))

/**
 * WMF_BRUSH_SET_STYLE(wmfBrush* B,(U16) brush style)
 */
#define WMF_BRUSH_SET_STYLE(B,Z)  (B)->lbStyle = (Z)

/**
 * WMF_BRUSH_SET_HATCH(wmfBrush* B,(U16) brush hatch style)
 */
#define WMF_BRUSH_SET_HATCH(B,Z)  (B)->lbHatch = (Z)

/**
 * WMF_BRUSH_SET_COLOR(wmfBrush* B,(wmfRGB*) brush color)
 */
#define WMF_BRUSH_SET_COLOR(B,Z)  (B)->lbColor = (*(Z))

/**
 * WMF_BRUSH_SET_BITMAP(wmfBrush* B,(wmfBMP*) brush bitmap)
 */
#define WMF_BRUSH_SET_BITMAP(B,Z) (B)->bmp = (*(Z))

/**
 * WMF_TEXT_ANGLE(wmfFont* F) -> (double) text angle in radians
 */
#define WMF_TEXT_ANGLE(F)     ((((double) (F)->lfEscapement) / 10) * M_PI / 180)

/**
 * WMF_TEXT_UNDERLINE(wmfFont* F) -> (U8) ?? whether to underline (?? how thick)
 */
#define WMF_TEXT_UNDERLINE(F)   ((F)->lfUnderline)

/**
 * WMF_TEXT_STRIKEOUT(wmfFont* F) -> (U8) ?? whether to strikeout (?? how thick)
 */
#define WMF_TEXT_STRIKEOUT(F)   ((F)->lfStrikeOut)

/**
 * WMF_FONT_WIDTH(wmfFont* F)   -> (U16) font `width'
 */
#define WMF_FONT_WIDTH(F)       ((F)->lfWidth)

/**
 * WMF_FONT_HEIGHT(wmfFont* F)  -> (U16) font height
 */
#define WMF_FONT_HEIGHT(F)      ((F)->lfHeight)

/**
 * WMF_FONT_ESCAPEMENT(wmfFont* F)  -> (S16) escapement [use WMF_TEXT_ANGLE instead]
 */
#define WMF_FONT_ESCAPEMENT(F)  ((F)->lfEscapement)

/**
 * WMF_FONT_ORIENTATION(wmfFont* F) -> (S16) orientation [??]
 */
#define WMF_FONT_ORIENTATION(F) ((F)->lfOrientation)

/**
 * WMF_FONT_WEIGHT(wmfFont* F) -> (U16) weight
 */
#define WMF_FONT_WEIGHT(F)      ((F)->lfWeight)

/**
 * WMF_FONT_ITALIC(wmfFont* F) -> (U8) italic
 */
#define WMF_FONT_ITALIC(F)      ((F)->lfItalic)

/**
 * WMF_FONT_OUT(wmfFont* F)     -> (U8) output precision [??]
 */
#define WMF_FONT_OUT(F)         ((F)->lfOutPrecision)

/**
 * WMF_FONT_CLIP(wmfFont* F)    -> (U8) clip precision [??]
 */
#define WMF_FONT_CLIP(F)        ((F)->lfClipPrecision)

/**
 * WMF_FONT_QUALITY(wmfFont* F) -> (U8) quality [??]
 */
#define WMF_FONT_QUALITY(F)     ((F)->lfQuality)

/**
 * WMF_FONT_PITCH(wmfFont* F)   -> (U8) pitch & family [??]
 */
#define WMF_FONT_PITCH(F)       ((F)->lfPitchAndFamily)

/**
 * WMF_FONT_CHARSET(wmfFont* F) -> (U8) character set
 */
#define WMF_FONT_CHARSET(F)     ((F)->lfCharSet)

/**
 * WMF_FONT_NAME(wmfFont* F)   -> (char*) font name supplied by metafile
 */
#define WMF_FONT_NAME(F)        ((F)->lfFaceName)

/**
 * WMF_TEXT_SET_UNDERLINE(wmfFont* F,(U8) ?? whether to underline (?? how thick))
 */
#define WMF_TEXT_SET_UNDERLINE(F,Z)   (F)->lfUnderline = (Z)

/**
 * WMF_TEXT_SET_STRIKEOUT(wmfFont* F,(U8) ?? whether to strikeout (?? how thick))
 */
#define WMF_TEXT_SET_STRIKEOUT(F,Z)   (F)->lfStrikeOut = (Z)

/**
 * WMF_FONT_SET_WIDTH(wmfFont* F,(U16) font `width')
 */
#define WMF_FONT_SET_WIDTH(F,Z)       (F)->lfWidth = (Z)

/**
 * WMF_FONT_SET_HEIGHT(wmfFont* F,(U16) font height)
 */
#define WMF_FONT_SET_HEIGHT(F,Z)      (F)->lfHeight = (Z)

/**
 * WMF_FONT_SET_ESCAPEMENT(wmfFont* F,(S16) escapement)
 */
#define WMF_FONT_SET_ESCAPEMENT(F,Z)  (F)->lfEscapement = (Z)

/**
 * WMF_FONT_SET_ORIENTATION(wmfFont* F,(S16) orientation [??])
 */
#define WMF_FONT_SET_ORIENTATION(F,Z) (F)->lfOrientation = (Z)

/**
 * WMF_FONT_SET_WEIGHT(wmfFont* F,(U16) weight)
 */
#define WMF_FONT_SET_WEIGHT(F,Z)      (F)->lfWeight = (Z)

/**
 * WMF_FONT_SET_ITALIC(wmfFont* F,(U8) italic)
 */
#define WMF_FONT_SET_ITALIC(F,Z)      (F)->lfItalic = (Z)

/**
 * WMF_FONT_SET_OUT(wmfFont* F,(U8) output precision [??])
 */
#define WMF_FONT_SET_OUT(F,Z)         (F)->lfOutPrecision = (Z)

/**
 * WMF_FONT_SET_CLIP(wmfFont* F,(U8) clip precision [??])
 */
#define WMF_FONT_SET_CLIP(F,Z)        (F)->lfClipPrecision = (Z)

/**
 * WMF_FONT_SET_QUALITY(wmfFont* F,(U8) quality [??])
 */
#define WMF_FONT_SET_QUALITY(F,Z)     (F)->lfQuality = (Z)

/**
 * WMF_FONT_SET_PITCH(wmfFont* F,(U8) pitch & family [??])
 */
#define WMF_FONT_SET_PITCH(F,Z)       (F)->lfPitchAndFamily = (Z)

/**
 * WMF_FONT_SET_CHARSET(wmfFont* F,(U8) chracter set)
 */
#define WMF_FONT_SET_CHARSET(F,Z)     (F)->lfCharSet = (Z)

/**
 * WMF_FONT_SET_NAME(wmfFont* F,(char*) font name supplied by metafile [do not free this string!])
 */
#define WMF_FONT_SET_NAME(F,Z)        (F)->lfFaceName = (Z)

/**
 * WMF_DC_BRUSH(wmfDC* C) -> (wmfBrush*) current brush
 */
#define WMF_DC_BRUSH(C)        ((C)->brush)

/**
 * WMF_DC_PEN(wmfDC* C)   -> (wmfPen*)   current pen
 */
#define WMF_DC_PEN(C)          ((C)->pen)

/**
 * WMF_DC_FONT(wmfDC* C)  -> (wmfFont*)  current font
 */
#define WMF_DC_FONT(C)         ((C)->font)

/**
 * WMF_DC_TEXTCOLOR(wmfDC* C)  -> (wmfRGB*) text color
 */
#define WMF_DC_TEXTCOLOR(C)  (&((C)->textcolor))

/**
 * WMF_DC_BACKGROUND(wmfDC* C) -> (wmfRGB*) background color
 */
#define WMF_DC_BACKGROUND(C) (&((C)->bgcolor))

/**
 * WMF_DC_OPAQUE(wmfDC* C)      -> (U16) whether to fill opaque (non-zero if true)
 */
#define WMF_DC_OPAQUE(C)       ((C)->bgmode == OPAQUE)

/**
 * WMF_DC_TRANSPARENT(wmfDC* C) -> (U16) whether to fill transparent
 */
#define WMF_DC_TRANSPARENT(C)  ((C)->bgmode == TRANSPARENT)

/**
 * WMF_DC_POLYFILL(wmfDC* C) -> (U16) how to fill polygons
 */
#define WMF_DC_POLYFILL(C)     ((C)->polyfillmode)

/**
 * WMF_DC_ROP(wmfDC* C) -> (U16) ROP mode
 */
#define WMF_DC_ROP(C)          ((C)->ROPmode)

/**
 * WMF_DC_TEXTALIGN(wmfDC* C) -> (U16) how to align text
 */
#define WMF_DC_TEXTALIGN(C)    ((C)->textalign)

/**
 * WMF_DC_CHAREXTRA(wmfDC* C)  -> (U16) char [extended character set??]
 */
#define WMF_DC_CHAREXTRA(C)    ((C)->charextra)

/**
 * WMF_DC_BREAKEXTRA(wmfDC* C) -> (U16) break [line break??]
 */
#define WMF_DC_BREAKEXTRA(C)   ((C)->breakextra)

/* Definition subject to change:
 */

/**
 * WMF_DC_SET_PEN(wmfDC* C,(wmfPen*) current pen)
 */
#define WMF_DC_SET_PEN(C,Z)    (C)->pen = (Z)

/**
 * WMF_DC_SET_BRUSH(wmfDC* C,(wmfBrush*) current brush)
 */
#define WMF_DC_SET_BRUSH(C,Z)  (C)->brush = (Z)

/**
 * WMF_DC_SET_FONT(wmfDC* C,(wmfFont*) current font)
 */
#define WMF_DC_SET_FONT(C,Z)   (C)->font = (Z)

/**
 * WMF_DC_SET_TEXTCOLOR(wmfDC* C,(wmfRGB*) text color)
 */
#define WMF_DC_SET_TEXTCOLOR(C,Z)  (C)->textcolor = (*(Z))

/**
 * WMF_DC_SET_BACKGROUND(wmfDC* C,(wmfRGB*) background color)
 */
#define WMF_DC_SET_BACKGROUND(C,Z) (C)->bgcolor = (*(Z))

/**
 * WMF_DC_SET_OPAQUE(wmfDC* C)
 */
#define WMF_DC_SET_OPAQUE(C)       (C)->bgmode = OPAQUE

/**
 * WMF_DC_SET_TRANSPARENT(wmfDC* C)
 */
#define WMF_DC_SET_TRANSPARENT(C)  (C)->bgmode = TRANSPARENT

/**
 * WMF_DC_SET_POLYFILL(wmfDC* C,(U16) how to fill polygons)
 */
#define WMF_DC_SET_POLYFILL(C,Z)   (C)->polyfillmode = (Z)

/**
 * WMF_DC_SET_ROP(wmfDC* C,(U16) ROP mode)
 */
#define WMF_DC_SET_ROP(C,Z)        (C)->ROPmode = (Z)

/**
 * WMF_DC_SET_TEXTALIGN(wmfDC* C,(U16) how to align text)
 */
#define WMF_DC_SET_TEXTALIGN(C,Z)  (C)->textalign = (Z)

/**
 * WMF_DC_SET_CHAREXTRA(wmfDC* C,(U16) char [extended character set??])
 */
#define WMF_DC_SET_CHAREXTRA(C,Z)  (C)->charextra = (Z)

/**
 * WMF_DC_SET_BREAKEXTRA(wmfDC* C,(U16) break [line break??])
 */
#define WMF_DC_SET_BREAKEXTRA(C,Z) (C)->breakextra = (Z)

#endif /* ! LIBWMF_MACRO_H */
