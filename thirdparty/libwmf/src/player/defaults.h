/* libwmf ("player/defaults.h"): library for wmf conversion
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


#ifndef WMFPLAYER_DEFAULTS_H
#define WMFPLAYER_DEFAULTS_H

static void SetDefaults (wmfAPI* API,wmfPen* pen,wmfBrush* brush,wmfFont* font)
{	wmfFontData* FD = (wmfFontData*) API->font_data;

	wmfBMP bmp;

	WMF_PEN_SET_COLOR  (pen,&wmf_black);
	WMF_PEN_SET_WIDTH  (pen,1);
	WMF_PEN_SET_HEIGHT (pen,1);
	WMF_PEN_SET_STYLE  (pen,PS_SOLID);
	WMF_PEN_SET_ENDCAP (pen,PS_ENDCAP_ROUND);
	WMF_PEN_SET_JOIN   (pen,PS_JOIN_ROUND);
	WMF_PEN_SET_TYPE   (pen,PS_COSMETIC);

	bmp.width  = 0;
	bmp.height = 0;
	bmp.data   = 0;

	WMF_BRUSH_SET_COLOR  (brush,&wmf_black);
	WMF_BRUSH_SET_STYLE  (brush,BS_NULL);
	WMF_BRUSH_SET_HATCH  (brush,HS_HORIZONTAL);
	WMF_BRUSH_SET_BITMAP (brush,&bmp);

	WMF_FONT_SET_HEIGHT      (font,12);
	WMF_FONT_SET_WIDTH       (font,12);
	WMF_FONT_SET_ESCAPEMENT  (font, 0);
	WMF_FONT_SET_ORIENTATION (font, 0);
	WMF_FONT_SET_WEIGHT      (font, 0);
	WMF_FONT_SET_ITALIC      (font, 0);
	WMF_TEXT_SET_UNDERLINE   (font, 0);
	WMF_TEXT_SET_STRIKEOUT   (font, 0);
	WMF_FONT_SET_CHARSET     (font, 0);
	WMF_FONT_SET_OUT         (font, 0);
	WMF_FONT_SET_CLIP        (font, 0);
	WMF_FONT_SET_QUALITY     (font, 0);
	WMF_FONT_SET_PITCH       (font, 0);
	WMF_FONT_SET_NAME        (font,wmf_strdup (API,"Times"));

	font->user_data = 0;

	FD->map (API,font);
}

#endif /* ! WMFPLAYER_DEFAULTS_H */
