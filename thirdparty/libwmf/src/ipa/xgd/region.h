/* libwmf ("ipa/xgd/region.h"): library for wmf conversion
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


static void wmf_gd_region_frame (wmfAPI* API,wmfPolyRectangle_t* poly_rect)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	wmfPen* pen = 0;

	wmfRGB* rgb = 0;

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint TL;
	gdPoint BR;

	unsigned int i;

	int width;
	int height;

	int color;

	WMF_DEBUG (API,"wmf_[gd_]region_frame");

	if (poly_rect->count == 0) return;

	pen = WMF_DC_PEN (poly_rect->dc);

	rgb = WMF_PEN_COLOR (pen);
	color = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	width  = gd_width  (API,poly_rect->width );
	height = gd_height (API,poly_rect->height);

	if (width  < 1) width  = 1;
	if (height < 1) height = 1;

	for (i = 0; i < poly_rect->count; i++)
	{	TL = gd_translate (API,poly_rect->TL[i]);
		BR = gd_translate (API,poly_rect->BR[i]);

		gdImageFilledRectangle (gd->image,TL.x-width,TL.y-height,TL.x,BR.y,color);
		gdImageFilledRectangle (gd->image,TL.x-width,BR.y,BR.x,BR.y+height,color);
		gdImageFilledRectangle (gd->image,TL.x,TL.y-height,BR.x+width,TL.y,color);
		gdImageFilledRectangle (gd->image,BR.x,TL.y,BR.x+width,BR.y+height,color);
	}
}

static void wmf_gd_region_paint (wmfAPI* API,wmfPolyRectangle_t* poly_rect)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint TL;
	gdPoint BR;

	unsigned int i;

	int brushstyle;

	WMF_DEBUG (API,"wmf_[gd_]region_paint");

	if (poly_rect->count == 0) return;

	if (TO_FILL (poly_rect))
	{	brushstyle = setbrushstyle (API,poly_rect->dc);

		for (i = 0; i < poly_rect->count; i++)
		{	TL = gd_translate (API,poly_rect->TL[i]);
			BR = gd_translate (API,poly_rect->BR[i]);

			gdImageFilledRectangle (gd->image,TL.x,TL.y,BR.x,BR.y,brushstyle);
		}
	}
}

static void wmf_gd_region_clip (wmfAPI* API,wmfPolyRectangle_t* poly_rect)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint TL;
	gdPoint BR;

#ifdef HAVE_SYS_GD
	int x_min = 0;
	int x_max = 0;
	int y_min = 0;
	int y_max = 0;
#else
	gdClipRectangle rect;
#endif

	unsigned int i;

	WMF_DEBUG (API,"wmf_[gd_]region_clip");

	wmf_gd_clip_reset (gd->image);

	if (poly_rect->count == 0) return;

	for (i = 0; i < poly_rect->count; i++)
	{	TL = gd_translate (API,poly_rect->TL[i]);
		BR = gd_translate (API,poly_rect->BR[i]);

#ifdef HAVE_SYS_GD
		if (i == 0)
		{	x_min = MIN (TL.x,BR.x);
			x_max = MAX (TL.x,BR.x) - 1;
			y_min = MIN (TL.y,BR.y);
			y_max = MAX (TL.y,BR.y) - 1;
			continue;
		}

		x_min = MIN (x_min,MIN (TL.x,BR.x));
		x_max = MAX (x_max,MAX (TL.x,BR.x) - 1);
		y_min = MIN (y_min,MIN (TL.y,BR.y));
		y_max = MAX (y_max,MAX (TL.y,BR.y) - 1);
#else
		rect.x_min = MIN (TL.x,BR.x);
		rect.x_max = MAX (TL.x,BR.x) - 1;
		rect.y_min = MIN (TL.y,BR.y);
		rect.y_max = MAX (TL.y,BR.y) - 1;

		gdClipSetAdd (gd->image,&rect);
#endif
	}

#ifdef HAVE_SYS_GD
	/* System libgd accepts a single clip rectangle, so use the
	 * bounding box of the WMF clip region here.
	 */
	gdImageSetClip (gd->image,x_min,y_min,x_max,y_max);
#endif
}
