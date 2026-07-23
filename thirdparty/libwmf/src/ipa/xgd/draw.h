/* libwmf ("ipa/xgd/draw.h"): library for wmf conversion
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


static void wmf_gd_flood_interior (wmfAPI* API,wmfFlood_t* flood)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	wmfRGB* rgb;

	gdPoint pt;

	int color;

	WMF_DEBUG (API,"wmf_[gd_]flood_interior");

	pt = gd_translate (API,flood->pt);

	rgb = &(flood->color);
	color = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	gdImageFillToBorder (gd->image,pt.x,pt.y,color,color);
}

static void wmf_gd_flood_exterior (wmfAPI* API,wmfFlood_t* flood)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	wmfRGB* rgb;

	gdPoint pt;

	int color;

	WMF_DEBUG (API,"wmf_[gd_]flood_exterior");

	pt = gd_translate (API,flood->pt);

	rgb = &(flood->color);
	color = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	if (flood->type == FLOODFILLSURFACE)
	{	gdImageFill (gd->image,pt.x,pt.y,color);
	}
	else
	{	gdImageFillToBorder (gd->image,pt.x,pt.y,color,color);
	}
}

static void wmf_gd_draw_pixel (wmfAPI* API,wmfDrawPixel_t* draw_pixel)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint pt;

	wmfRGB* rgb;

	int color;

	WMF_DEBUG (API,"wmf_[gd_]draw_pixel");

	pt = gd_translate (API,draw_pixel->pt);

	rgb = &(draw_pixel->color);

	color = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	gdImageSetPixel (gd->image,pt.x,pt.y,color);
}

static void wmf_gd_draw_pie (wmfAPI* API,wmfDrawArc_t* draw_arc)
{	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]draw_pie");

	gd_draw_arc (API,draw_arc,gd_arc_pie);
}

static void wmf_gd_draw_chord (wmfAPI* API,wmfDrawArc_t* draw_arc)
{	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]draw_chord");

	gd_draw_arc (API,draw_arc,gd_arc_chord);
}

static void wmf_gd_draw_arc (wmfAPI* API,wmfDrawArc_t* draw_arc)
{	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]draw_arc");

	gd_draw_arc (API,draw_arc,gd_arc_open);
}

static void wmf_gd_draw_ellipse (wmfAPI* API,wmfDrawArc_t* draw_arc)
{	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]draw_ellipse");

	gd_draw_arc (API,draw_arc,gd_arc_ellipse);
}

static void gd_draw_arc (wmfAPI* API,wmfDrawArc_t* draw_arc,gd_arc_t finish)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint TL;
	gdPoint BR;

	gdPoint s_pt;
	gdPoint e_pt;

	int Ox;
	int Oy;
	int w;
	int h;
	int start = 0;
	int end = 360;

	int brushstyle;
	int linestyle;

	WMF_DEBUG (API,"~~~~~~~~gd_draw_arc");

	TL = gd_translate (API,draw_arc->TL);
	BR = gd_translate (API,draw_arc->BR);

	Ox = (BR.x + TL.x) / 2; /* origin of ellipse */
	Oy = (BR.y + TL.y) / 2;

	w = (BR.x - TL.x);  /* axes of ellipse */
	h = (BR.y - TL.y);

	if (finish != gd_arc_ellipse)
	{	start = (int) (atan2 (draw_arc->start.y,draw_arc->start.x) * 180 / PI);
		end   = (int) (atan2 (draw_arc->end.y  ,draw_arc->end.x  ) * 180 / PI);

		if (start < 0)
		{	start += 360;
			end   += 360;
		}
		if (end < start) end += 360;

		if (finish != gd_arc_open)
		{	s_pt = gd_translate (API,draw_arc->start);
			s_pt.x += Ox;
			s_pt.y += Oy;
			e_pt = gd_translate (API,draw_arc->end);
			e_pt.x += Ox;
			e_pt.y += Oy;
		}
	}

	if (TO_FILL (draw_arc))
	{	brushstyle = setbrushstyle (API,draw_arc->dc);

		if (finish == gd_arc_ellipse)
		{	gdImageFilledArc (gd->image,Ox,Oy,w,h,start,end,brushstyle,gdChord);
		}
		else if (finish == gd_arc_open)
		{	/* Do nothing */
		}
		else if (finish == gd_arc_pie)
		{	gdImageFilledArc (gd->image,Ox,Oy,w,h,start,end,brushstyle,gdPie);
		}
		else if (finish == gd_arc_chord)
		{	gdImageFilledArc (gd->image,Ox,Oy,w,h,start,end,brushstyle,gdChord);
		}
	}
	if (TO_DRAW (draw_arc))
	{	linestyle = setlinestyle (API,draw_arc->dc);

		gdImageArc (gd->image,Ox,Oy,w,h,start,end,linestyle);

		if (finish == gd_arc_ellipse)
		{	/* Do nothing */
		}
		else if (finish == gd_arc_open)
		{	/* Do nothing */
		}
		else if (finish == gd_arc_pie)
		{	gdImageLine (gd->image,e_pt.x,e_pt.y,Ox,Oy,linestyle);
			gdImageLine (gd->image,Ox,Oy,s_pt.x,s_pt.y,linestyle);
		}
		else if (finish == gd_arc_chord)
		{	gdImageLine (gd->image,e_pt.x,e_pt.y,s_pt.x,s_pt.y,linestyle);
		}
	}
}

static void wmf_gd_draw_line (wmfAPI* API,wmfDrawLine_t* draw_line)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint from;
	gdPoint to;

	int linestyle;

	WMF_DEBUG (API,"wmf_[gd_]draw_line");

	if (TO_DRAW (draw_line))
	{	linestyle = setlinestyle (API,draw_line->dc);

		from = gd_translate (API,draw_line->from);
		to   = gd_translate (API,draw_line->to  );

		gdImageLine (gd->image,from.x,from.y,to.x,to.y,linestyle);
	}
}

static void wmf_gd_poly_line (wmfAPI* API,wmfPolyLine_t* poly_line)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint from;
	gdPoint to;

	int linestyle;

	U16 i;

	WMF_DEBUG (API,"wmf_[gd_]poly_line");

	if (TO_DRAW (poly_line) && (poly_line->count > 1))
	{	linestyle = setlinestyle (API,poly_line->dc);

		from = gd_translate (API,poly_line->pt[0]);

		for (i = 1; i < poly_line->count; i++)
		{	to = gd_translate (API,poly_line->pt[i]);

			gdImageLine (gd->image,from.x,from.y,to.x,to.y,linestyle);

			from = to;
		}
	}
}

static void wmf_gd_draw_polygon (wmfAPI* API,wmfPolyLine_t* poly_line)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint* pt;

	int brushstyle;
	int linestyle;

	U16 i;

	WMF_DEBUG (API,"wmf_[gd_]draw_polygon");

	if (poly_line->count < 3) return;

	pt = (gdPoint*) wmf_malloc (API,poly_line->count * sizeof (gdPoint));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	for (i = 0; i < poly_line->count; i++)
	{	pt[i] = gd_translate (API,poly_line->pt[i]);
	}

	if (TO_FILL (poly_line))
	{	brushstyle = setbrushstyle (API,poly_line->dc);

		gdImageFilledPolygon (gd->image,pt,poly_line->count,brushstyle);
	}
	if (TO_DRAW (poly_line))
	{	linestyle = setlinestyle (API,poly_line->dc);

		gdImagePolygon (gd->image,pt,poly_line->count,linestyle);
	}

	wmf_free (API,pt);
}

static void wmf_gd_draw_rectangle (wmfAPI* API,wmfDrawRectangle_t* draw_rectangle)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdPoint TL;
	gdPoint BR;

	int brushstyle;
	int linestyle;

	WMF_DEBUG (API,"wmf_[gd_]draw_rectangle");

	TL = gd_translate (API,draw_rectangle->TL);
	BR = gd_translate (API,draw_rectangle->BR);

	if (TO_FILL (draw_rectangle))
	{	brushstyle = setbrushstyle (API,draw_rectangle->dc);

		gdImageFilledRectangle (gd->image,TL.x,TL.y,BR.x,BR.y,brushstyle);
	}
	if (TO_DRAW (draw_rectangle))
	{	linestyle = setlinestyle (API,draw_rectangle->dc);

		gdImageRectangle (gd->image,TL.x,TL.y,BR.x,BR.y,linestyle);
	}
}
