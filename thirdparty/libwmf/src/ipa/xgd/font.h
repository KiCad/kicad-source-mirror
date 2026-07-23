/* libwmf ("ipa/xgd/font.h"): library for wmf conversion
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


static void wmf_gd_draw_text (wmfAPI* API,wmfDrawText_t* draw_text)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint pt;
	gdPoint pts[4];

	wmfFont* font = 0;

	wmfRGB* fg = 0;
	wmfRGB* bg = 0;

	wmfD_Coord TL;
	wmfD_Coord BR;
	wmfD_Coord BL;
	wmfD_Coord TR;

	int color;

	float font_height;
	float font_ratio;

	double theta;
	double phi;

	FT_Face face;

	FT_F26Dot6 width;
	FT_F26Dot6 height;

	FT_Matrix matrix;

	FT_Vector pen;

	WMF_DEBUG (API,"wmf_[gd_]draw_text");

	if (WMF_DC_OPAQUE (draw_text->dc))
	{	if ((draw_text->BR.x > draw_text->TL.x) && (draw_text->BR.y > draw_text->TL.y))
		{	TL = draw_text->TL;
			BR = draw_text->BR;

			TR.x = draw_text->BR.x;
			TR.y = draw_text->TL.y;
			BL.x = draw_text->TL.x;
			BL.y = draw_text->BR.y;
		}
		else
		{	TL = draw_text->bbox.TL;
			BR = draw_text->bbox.BR;
			TR = draw_text->bbox.TR;
			BL = draw_text->bbox.BL;
		}

		bg = WMF_DC_BACKGROUND (draw_text->dc);

		color = gdImageColorResolve (gd->image,bg->r,bg->g,bg->b);

		pts[0] = gd_translate (API,TL);
		pts[1] = gd_translate (API,TR);
		pts[2] = gd_translate (API,BR);
		pts[3] = gd_translate (API,BL);

		gdImageFilledPolygon (gd->image,pts,4,color);
	}
	else
	{	bg = &wmf_white; /* improper for gd? TODO */
	}

	if (strlen (draw_text->str) > 1)
	{	wmf_ipa_draw_text (API,draw_text,wmf_gd_draw_text);
		return;
	}

	font = WMF_DC_FONT (draw_text->dc);

	face = WMF_FONT_FTFACE (font);

	font_height = gd_height (API,(float)  draw_text->font_height);
	font_ratio  = gd_width  (API,(float) (draw_text->font_height * draw_text->font_ratio));
	font_ratio /= font_height;

	width  = (FT_F26Dot6) (64 * font_height * font_ratio);
	height = (FT_F26Dot6) (64 * font_height);

	FT_Set_Char_Size (face,width,height,0,0);

	theta = - WMF_TEXT_ANGLE (font);

	phi = atan2 (sin (theta),font_ratio * cos (theta));

	pt = gd_translate_ft64 (API,draw_text->pt,&pen);

	matrix.xx = (FT_Fixed) (  cos (phi) * 0x10000L);
	matrix.xy = (FT_Fixed) (  sin (phi) * 0x10000L);
	matrix.yx = (FT_Fixed) (- sin (phi) * 0x10000L);
	matrix.yy = (FT_Fixed) (  cos (phi) * 0x10000L);

	FT_Set_Transform (face,&matrix,&pen);

	FT_Load_Char (face,draw_text->str[0],FT_LOAD_RENDER);

	pt.x += face->glyph->bitmap_left;
	pt.y -= face->glyph->bitmap_top;

	fg = WMF_DC_TEXTCOLOR (draw_text->dc);

	gd_draw_ftbitmap (API,&(face->glyph->bitmap),pt,fg,bg);
}

static void gd_draw_ftbitmap (wmfAPI* API,FT_Bitmap* bitmap,gdPoint pt,wmfRGB* fg,wmfRGB* bg)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	wmfRGB rgb;

	float red;
	float green;
	float blue;
	float grey; /* 1 = black(fg); 0 = white(bg) */

	unsigned char* buffer;

	unsigned int row;
	unsigned int rows;
	unsigned int col;
	unsigned int cols;

	int x;
	int y;

	int fg_pixel;
	int color;

	rows = bitmap->rows;
	cols = bitmap->width;

	fg_pixel = gdImageColorResolve (gd->image,fg->r,fg->g,fg->b);

	for (row = 0; row < rows; row++)
	{	buffer = bitmap->buffer + row * bitmap->pitch;

		y = pt.y + row;

		for (col = 0; col < cols; col++)
		{	if (bitmap->pixel_mode == ft_pixel_mode_grays)
			{	grey = (float) buffer[col] / (float) (bitmap->num_grays - 1);
			}
			else if (bitmap->pixel_mode == ft_pixel_mode_mono)
			{	grey = ((buffer[col>>3] << (col & 7)) & 128) ? 1 : 0;
			}
			else /* Unsupported ft_pixel_mode */
			{	grey = 0;
			}

			if (grey > 0) /* if not background */
			{	x = pt.x + col;

				if ((grey >= 1) && (bitmap->pixel_mode != ft_pixel_mode_mono))
				{	color = fg_pixel;
				}
				else /* find anti-aliased color */
				{	red   = grey * (float) fg->r + (1 - grey) * (float) bg->r;
					green = grey * (float) fg->g + (1 - grey) * (float) bg->g;
					blue  = grey * (float) fg->b + (1 - grey) * (float) bg->b;

					rgb = wmf_rgb_color (API,red/255,green/255,blue/255);

					color = gdImageColorResolve (gd->image,rgb.r,rgb.g,rgb.b);
				}

				gdImageSetPixel (gd->image,x,y,color);
			}
		}
	}
}
