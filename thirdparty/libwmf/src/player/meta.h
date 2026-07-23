/* libwmf ("player/meta.h"): library for wmf conversion
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


#ifndef WMFPLAYER_META_H
#define WMFPLAYER_META_H

static int meta_mapmode (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	U16 par_U16;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	par_U16 = ParU16 (API,Record,0);

	WmfSetMapMode (API,par_U16);

	return (changed);
}

static int meta_orgext (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	S32 par_S32_x;
	S32 par_S32_y;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 1",Record->size);
	}

	par_S32_x = ParS32 (API,Record,1);
	par_S32_y = ParS32 (API,Record,0);

	switch (Record->function)
	{
	case META_SETWINDOWORG:
		P->dc->Window.Ox = par_S32_x;
		P->dc->Window.Oy = par_S32_y;
	break;

	case META_SETVIEWPORTORG:
		P->Viewport_Origin.x = (float) ((double) par_S32_x * P->dc->pixel_width );
		P->Viewport_Origin.y = (float) ((double) par_S32_y * P->dc->pixel_height);
	break;

	case META_SETVIEWPORTEXT:
		P->Viewport_Width  = par_S32_x;
		P->Viewport_Height = par_S32_y;
		PixelWidth (API);
		PixelHeight (API); /* Recalculate pixel size */
	break;

	case META_SETWINDOWEXT:
		P->dc->Window.width  = par_S32_x;
		P->dc->Window.height = par_S32_y;
		PixelWidth (API);
		PixelHeight (API); /* Recalculate pixel size */
	break;

	case META_OFFSETWINDOWORG:
		P->dc->Window.Ox += par_S32_x;
		P->dc->Window.Oy += par_S32_y;
	break;

	case META_OFFSETVIEWPORTORG:
		P->Viewport_Origin.x += (float) ((double) par_S32_x * P->dc->pixel_width );
		P->Viewport_Origin.y += (float) ((double) par_S32_y * P->dc->pixel_height);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	return (changed);
}

static int meta_scale (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	S32 par_S32_x1;
	S32 par_S32_x2;
	S32 par_S32_y1;
	S32 par_S32_y2;

	double x1;
	double x2;
	double y1;
	double y2;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
	}

	par_S32_x2 = ParS32 (API,Record,3);
	par_S32_x1 = ParS32 (API,Record,2);
	par_S32_y2 = ParS32 (API,Record,1);
	par_S32_y1 = ParS32 (API,Record,0);

	if ((par_S32_x1 == 0) || (par_S32_y1 == 0))
	{	WMF_ERROR (API,"meta file attempts division by zero!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	x2 = (double) par_S32_x2;
	x1 = (double) par_S32_x1;
	y2 = (double) par_S32_y2;
	y1 = (double) par_S32_y1;

	switch (Record->function)
	{
	case META_SCALEWINDOWEXT:
		P->dc->Window.width  = (S32) (((double) P->dc->Window.width  * x2) / x1);
		P->dc->Window.height = (S32) (((double) P->dc->Window.height * y2) / y1);
	break;

	case META_SCALEVIEWPORTEXT:
		P->Viewport_Width  = (S32) (((double) P->Viewport_Width  * x2) / x1);
		P->Viewport_Height = (S32) (((double) P->Viewport_Height * y2) / y1);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	PixelWidth (API);
	PixelHeight (API); /* Recalculate pixel size */

	return (changed);
}

static int meta_moveto (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	U16 par_U16_x;
	U16 par_U16_y;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 1",Record->size);
	}

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	P->current = L_Coord (par_U16_x,par_U16_y);

	return (changed);
}

static int meta_flood (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfFlood_t flood;

	U16 par_U16_x;
	U16 par_U16_y;
	U16 par_U16_rg;
	U16 par_U16_b;
	U16 par_U16_t;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 4",Record->size);
	}

	par_U16_x  = ParU16 (API,Record,4);
	par_U16_y  = ParU16 (API,Record,3);
	par_U16_b  = ParU16 (API,Record,2);
	par_U16_rg = ParU16 (API,Record,1);
	par_U16_t  = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	flood.pt = wmf_D_Coord_translate (API,l_pt);

	flood.color = rgb (par_U16_rg,par_U16_b);

	if (SCAN (API))
	{	wmf_ipa_color_add (API,&(flood.color));
		D_Coord_Register (API,flood.pt,0);
		return (changed);
	}

	flood.dc = P->dc;

	flood.type = par_U16_t;

	flood.pixel_width  = ABS (P->dc->pixel_width );
	flood.pixel_height = ABS (P->dc->pixel_height);

	switch (Record->function)
	{
	case META_FLOODFILL:
		if (FR->flood_interior) FR->flood_interior (API,&flood);
	break;

	case META_EXTFLOODFILL:
		if (FR->flood_exterior) FR->flood_exterior (API,&flood);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	return (changed);
}

static int meta_pixel (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfDrawPixel_t drawpixel;

	U16 par_U16_x;
	U16 par_U16_y;
	U16 par_U16_rg;
	U16 par_U16_b;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
	}

	par_U16_x  = ParU16 (API,Record,3);
	par_U16_y  = ParU16 (API,Record,2);
	par_U16_b  = ParU16 (API,Record,1);
	par_U16_rg = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawpixel.pt = wmf_D_Coord_translate (API,l_pt);

	drawpixel.color = rgb (par_U16_rg,par_U16_b);

	drawpixel.pixel_width  = ABS (P->dc->pixel_width );
	drawpixel.pixel_height = ABS (P->dc->pixel_height);

	if (SCAN (API))
	{	wmf_ipa_color_add (API,&(drawpixel.color));
		scope = (float) MAX (drawpixel.pixel_width,drawpixel.pixel_height);
		D_Coord_Register (API,drawpixel.pt,scope);
		return (changed);
	}

	drawpixel.dc = P->dc;

	if (FR->draw_pixel) FR->draw_pixel (API,&drawpixel);

	return (changed);
}

static int meta_arc (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;
	wmfD_Coord d_pt;
	wmfD_Coord c_pt;

	wmfDrawArc_t drawarc;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;
	U16 end_x;
	U16 end_y;

	char Qs;
	char Qe;

	float scope = 0;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 7",Record->size);
	}

	par_U16_x = ParU16 (API,Record,7);
	par_U16_y = ParU16 (API,Record,6);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.TL = wmf_D_Coord_translate (API,l_pt);

	par_U16_x = ParU16 (API,Record,5);
	par_U16_y = ParU16 (API,Record,4);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.BR = wmf_D_Coord_translate (API,l_pt);

	par_U16_x = ParU16 (API,Record,3);
	par_U16_y = ParU16 (API,Record,2);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.end = wmf_D_Coord_translate (API,l_pt);

	end_x = par_U16_x;
	end_y = par_U16_y;

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	if ((end_x == par_U16_x) && (end_y == par_U16_y))
	{	/* start == end: This is probably an ellipse... TODO */
	}

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.start = wmf_D_Coord_translate (API,l_pt);

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		D_Coord_Register (API,drawarc.end,scope);
		D_Coord_Register (API,drawarc.start,scope);
	}

	c_pt.x = (drawarc.TL.x + drawarc.BR.x) / 2; /* ellipse origin */
	c_pt.y = (drawarc.TL.y + drawarc.BR.y) / 2;

	drawarc.start.x -= c_pt.x;
	drawarc.start.y -= c_pt.y;

	drawarc.end.x -= c_pt.x;
	drawarc.end.y -= c_pt.y;

	if (SCAN (API))
	{	if ((drawarc.start.x > 0) && (drawarc.start.y >= 0)) Qs = '1';
		else if ((drawarc.start.x <= 0) && (drawarc.start.y > 0)) Qs = '2';
		else if ((drawarc.start.x < 0) && (drawarc.start.y <= 0)) Qs = '3';
		else Qs = '4';

		if ((drawarc.end.x > 0) && (drawarc.end.y >= 0)) Qe = '1';
		else if ((drawarc.end.x <= 0) && (drawarc.end.y > 0)) Qe = '2';
		else if ((drawarc.end.x < 0) && (drawarc.end.y <= 0)) Qe = '3';
		else Qe = '4';

		switch (Qs)
		{
		case '1':
			switch (Qe)
			{
			case '1':
				if ( (drawarc.end.x < drawarc.start.x)
				  || (drawarc.end.y < drawarc.start.y)) break;
				d_pt.x = drawarc.BR.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '4':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.BR.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '3':
				d_pt.x = drawarc.TL.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			default:
			case '2':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.TL.y;
				D_Coord_Register (API,d_pt,scope);
			break;
			}
		break;

		case '2':
			switch (Qe)
			{
			case '2':
				if ( (drawarc.end.x < drawarc.start.x)
				  || (drawarc.end.y > drawarc.start.y)) break;
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.TL.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '1':
				d_pt.x = drawarc.BR.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '4':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.BR.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			default:
			case '3':
				d_pt.x = drawarc.TL.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
			break;
			}
		break;

		case '3':
			switch (Qe)
			{
			case '3':
				if ( (drawarc.end.x > drawarc.start.x)
				  || (drawarc.end.y > drawarc.start.y)) break;
				d_pt.x = drawarc.TL.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '2':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.TL.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '1':
				d_pt.x = drawarc.BR.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			default:
			case '4':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.BR.y;
				D_Coord_Register (API,d_pt,scope);
			break;
			}
		break;

		case '4':
		default:
			switch (Qe)
			{
			case '4':
				if ( (drawarc.end.x > drawarc.start.x)
				  || (drawarc.end.y < drawarc.start.y)) break;
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.BR.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '3':
				d_pt.x = drawarc.TL.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			case '2':
				d_pt.x = c_pt.x;
				d_pt.y = drawarc.TL.y;
				D_Coord_Register (API,d_pt,scope);
				/* fallthrough */
			default:
			case '1':
				d_pt.x = drawarc.BR.x;
				d_pt.y = c_pt.y;
				D_Coord_Register (API,d_pt,scope);
			break;
			}
		break;
		}

		return (changed);
	}

	d_pt.x = (drawarc.BR.x - drawarc.TL.x) / 2; /* elliptic axes */
	d_pt.y = (drawarc.BR.y - drawarc.TL.y) / 2;

	if ((drawarc.start.x == 0) && (drawarc.start.y == 0)) drawarc.start.x = d_pt.x;

	if (drawarc.start.x >   d_pt.x ) drawarc.start.x =   d_pt.x;
	if (drawarc.start.x < (-d_pt.x)) drawarc.start.x = - d_pt.x;
	if (drawarc.start.y >   d_pt.y ) drawarc.start.y =   d_pt.y;
	if (drawarc.start.y < (-d_pt.y)) drawarc.start.y = - d_pt.y;

	if ((drawarc.end.x == 0) && (drawarc.end.y == 0)) drawarc.end.x = d_pt.x;

	if (drawarc.end.x >   d_pt.x ) drawarc.end.x =   d_pt.x;
	if (drawarc.end.x < (-d_pt.x)) drawarc.end.x = - d_pt.x;
	if (drawarc.end.y >   d_pt.y ) drawarc.end.y =   d_pt.y;
	if (drawarc.end.y < (-d_pt.y)) drawarc.end.y = - d_pt.y;

	drawarc.dc = P->dc;

	switch (Record->function)
	{
	case META_PIE:
		if (FR->draw_pie) FR->draw_pie (API,&drawarc);
	break;

	case META_CHORD:
		if (FR->draw_chord) FR->draw_chord (API,&drawarc);
	break;

	case META_ARC:
		if (FR->draw_arc) FR->draw_arc (API,&drawarc);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	return (changed);
}

static int meta_ellipse (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfDrawArc_t drawarc;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
	}

	par_U16_x = ParU16 (API,Record,3);
	par_U16_y = ParU16 (API,Record,2);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.TL = wmf_D_Coord_translate (API,l_pt);

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawarc.BR = wmf_D_Coord_translate (API,l_pt);

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		D_Coord_Register (API,drawarc.TL,scope);
		D_Coord_Register (API,drawarc.BR,scope);
		return (changed);
	}

	drawarc.dc = P->dc;

	if (FR->draw_ellipse) FR->draw_ellipse (API,&drawarc);

	return (changed);
}

static int meta_line (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfDrawLine_t drawline;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 1",Record->size);
	}

	drawline.from = wmf_D_Coord_translate (API,P->current);

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawline.to = wmf_D_Coord_translate (API,l_pt);

	P->current = l_pt;

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		D_Coord_Register (API,drawline.from,scope);
		D_Coord_Register (API,drawline.to,scope);
		return (changed);
	}

	drawline.dc = P->dc;

	if (FR->draw_line) FR->draw_line (API,&drawline);

	return (changed);
}

static int meta_lines (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;
	wmfD_Coord d_pt;

	wmfPolyLine_t polyline;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	U16 i;

	unsigned long index;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	polyline.count = ParU16 (API,Record,0);

	if (Record->size < (unsigned long) 1 + 2 * polyline.count)
	{	WMF_ERROR (API,"Bad record - too few parameters for polyline!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,",%lu",(unsigned long) (2 * polyline.count));
	}

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		index = 1;
		for (i = 0; i < polyline.count; i++)
		{	par_U16_x = ParU16 (API,Record,index);
			index++;
			par_U16_y = ParU16 (API,Record,index);
			index++;
			l_pt = L_Coord (par_U16_x,par_U16_y);
			d_pt = wmf_D_Coord_translate (API,l_pt);
			D_Coord_Register (API,d_pt,scope);
		}
		return (changed);
	}

	polyline.pt = (wmfD_Coord*) wmf_malloc (API,polyline.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	index = 1;
	for (i = 0; i < polyline.count; i++)
	{	par_U16_x = ParU16 (API,Record,index);
		index++;
		par_U16_y = ParU16 (API,Record,index);
		index++;
		l_pt = L_Coord (par_U16_x,par_U16_y);
		polyline.pt[i] = wmf_D_Coord_translate (API,l_pt);
	}

	polyline.dc = P->dc;

	if (FR->poly_line) FR->poly_line (API,&polyline);

	wmf_free (API,polyline.pt);

	return (changed);
}

static int meta_polygon (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;
	wmfD_Coord d_pt;

	wmfPolyLine_t polyline;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	U16 i;

	unsigned long index;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	polyline.count = ParU16 (API,Record,0);

	if (Record->size < (unsigned long) 1 + 2 * polyline.count)
	{	WMF_ERROR (API,"Bad record - too few parameters for polygon!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,",%lu",(unsigned long) (2 * polyline.count));
	}

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		index = 1;
		for (i = 0; i < polyline.count; i++)
		{	par_U16_x = ParU16 (API,Record,index);
			index++;
			par_U16_y = ParU16 (API,Record,index);
			index++;
			l_pt = L_Coord (par_U16_x,par_U16_y);
			d_pt = wmf_D_Coord_translate (API,l_pt);
			D_Coord_Register (API,d_pt,scope);
		}
		return (changed);
	}

	polyline.pt = (wmfD_Coord*) wmf_malloc (API,polyline.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	index = 1;
	for (i = 0; i < polyline.count; i++)
	{	par_U16_x = ParU16 (API,Record,index);
		index++;
		par_U16_y = ParU16 (API,Record,index);
		index++;
		l_pt = L_Coord (par_U16_x,par_U16_y);
		polyline.pt[i] = wmf_D_Coord_translate (API,l_pt);
	}

	polyline.dc = P->dc;

	if (FR->draw_polygon) FR->draw_polygon (API,&polyline);

	wmf_free (API,polyline.pt);

	return (changed);
}

static int meta_polygons (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;
	wmfD_Coord d_pt;

	wmfPolyLine_t polyline;
	wmfPolyPoly_t polypoly;

	wmfRecord Polygon;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	unsigned long num_pars;
	unsigned long count;
	U16 style;

	U16 i;
	U16 j;

	unsigned long index;

	float scope;

	int skip_record;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	polypoly.npoly = ParU16 (API,Record,0);

	if (polypoly.npoly == 0) return (changed);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,",%lu",(unsigned long) polypoly.npoly);
	}

	polypoly.pt = (wmfD_Coord**) wmf_malloc (API, polypoly.npoly * sizeof (wmfD_Coord*));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polypoly.count = (U16*) wmf_malloc (API, polypoly.npoly * sizeof (U16));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	count = 0;
	num_pars = 0;
	skip_record = 0;
	for (i = 0; i < polypoly.npoly; i++)
	{	polypoly.count[i] = ParU16 (API,Record,(unsigned long) (1 + i));
		count += polypoly.count[i] + 2; /* for polypoly->polyline fill constructor */
		num_pars += polypoly.count[i];
		if ((polypoly.count[i] < 3) && (skip_record == 0))
		{	WMF_DEBUG (API,"strange polygon in polypolygon list; skipping record...");
			skip_record = 1;
		}
		if (skip_record)
		{	polypoly.pt[i] = 0;
		}
		else
		{	polypoly.pt[i] = (wmfD_Coord*) wmf_malloc (API, polypoly.count[i] * sizeof (wmfD_Coord));
			if (ERR (API)) break;
		}
	}
	int too_short = Record->size < 1 + polypoly.npoly + 2 * (uint64_t) num_pars;
	if (skip_record || too_short)
	{	if (too_short)
		{	WMF_ERROR (API,"Bad record - too few parameters for polypolygon!");
			API->err = wmf_E_BadFormat;
		}
		for (i = 0; i < polypoly.npoly; i++)
		{	if (polypoly.pt[i]) wmf_free (API, polypoly.pt[i]);
		}
		wmf_free (API, polypoly.pt);
		wmf_free (API, polypoly.count);
		return (changed);
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,",%lu",(unsigned long) (polypoly.npoly + 2 * num_pars));
	}

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		unsigned long ipt;
		index = 1 + polypoly.npoly;
		for (ipt = 0; ipt < num_pars; ipt++)
		{	par_U16_x = ParU16 (API,Record,index);
			if (ERR (API)) break;
			index++;
			par_U16_y = ParU16 (API,Record,index);
			if (ERR (API)) break;
			index++;
			l_pt = L_Coord (par_U16_x,par_U16_y);
			d_pt = wmf_D_Coord_translate (API,l_pt);
			D_Coord_Register (API,d_pt,scope);
		}
		return (changed);
	}

	polypoly.dc = P->dc;

	Polygon = OffsetRecord (API,Record,(unsigned long) (1 + polypoly.npoly));

	for (i = 0; i < polypoly.npoly; i++)
	{	polyline.count = polypoly.count[i];
		index = 0;
		for (j = 0; j < polyline.count; j++)
		{	par_U16_x = ParU16 (API,&Polygon,index);
			index++;
			par_U16_y = ParU16 (API,&Polygon,index);
			index++;
			l_pt = L_Coord (par_U16_x,par_U16_y);
			polypoly.pt[i][j] = wmf_D_Coord_translate (API,l_pt);
		}
		Polygon = OffsetRecord (API,&Polygon,index);
	}

	if (FR->draw_polypolygon)
	{	FR->draw_polypolygon (API,&polypoly);
	}
	else if (FR->draw_polygon)
	{	if (TO_FILL (&polypoly))
		{	style = polypoly.dc->pen->lopnStyle; /* [TODO: use macros ??] */
			polypoly.dc->pen->lopnStyle = PS_NULL;

			polyline.dc = polypoly.dc;
			if (count > SIZE_MAX / sizeof (wmfD_Coord))
			{	WMF_ERROR (API,"polypolygon point count too large!");
				API->err = wmf_E_InsMem;
				return (changed);
			}
			polyline.pt = (wmfD_Coord*) wmf_malloc (API, count * sizeof (wmfD_Coord));
			polyline.count = 0;

			if (ERR (API))
			{	WMF_DEBUG (API,"bailing...");
				return (changed);
			}

			polypoly_construct (API, &polypoly, &polyline, 0);

			if (polyline.count > 2) FR->draw_polygon (API,&polyline);

			wmf_free (API, polyline.pt);

			polypoly.dc->pen->lopnStyle = style;
		}
		if (TO_DRAW (&polypoly))
		{	style = polypoly.dc->brush->lbStyle; /* [TODO: use macros ??] */
			polypoly.dc->brush->lbStyle = BS_NULL;
			for (i = 0; i < polypoly.npoly; i++)
			{	polyline.dc = polypoly.dc;
				polyline.pt = polypoly.pt[i];
				polyline.count = polypoly.count[i];
				if ((polyline.count > 2) && polyline.pt)
				{	FR->draw_polygon (API,&polyline);
				}
			}
			polypoly.dc->brush->lbStyle = style;
		}
	}

	for (i = 0; i < polypoly.npoly; i++)
	{	if (polypoly.pt[i]) wmf_free (API, polypoly.pt[i]);
	}
	wmf_free (API, polypoly.pt);
	wmf_free (API, polypoly.count);

	return (changed);
}

static void polypoly_construct (wmfAPI* API,wmfPolyPoly_t* polypoly,wmfPolyLine_t* polyline,U16 ipoly)
{	U16 count = polypoly->count[ipoly];
	U16 i;
	U16 imin;
	U16 last;

	double x2;
	double y2;
	double r2;
	double r2_min = 0;

	if ((polyline->pt == 0) || (polypoly->pt == 0)) return; /* erk!! */

	if ((polypoly->pt[ipoly] == 0) || (polypoly->count[ipoly] < 3)) return;

	while ((polypoly->pt[ipoly][0].x == polypoly->pt[ipoly][count-1].x)
	    && (polypoly->pt[ipoly][0].y == polypoly->pt[ipoly][count-1].y))
	{
		count--;
		if (count < 3) break;
	}
	if (count < 3) return;

	last = 0;
	if (ipoly < (polypoly->npoly - 1))
	{	if ((polypoly->pt[ipoly+1] == 0) || (polypoly->count[ipoly+1] < 3))
		{	last = 1; /* erk!! */
		}
	}
	else
	{	last = 1; /* last poly, yay! */
	}
	if (last)
	{	for (i = 0; i < count; i++)
		{	polyline->pt[polyline->count].x = polypoly->pt[ipoly][i].x;
			polyline->pt[polyline->count].y = polypoly->pt[ipoly][i].y;
			polyline->count++;
		}
		polyline->pt[polyline->count].x = polypoly->pt[ipoly][0].x;
		polyline->pt[polyline->count].y = polypoly->pt[ipoly][0].y;
		polyline->count++;

		return;
	}

	/* find polygon point closest to point 0 in next polygon [TODO: improve this??]
	 */
	imin = 0;
	for (i = 0; i < count; i++)
	{	x2 = (double) polypoly->pt[ipoly][i].x - (double) polypoly->pt[ipoly+1][0].x;
		x2 *= x2;
		y2 = (double) polypoly->pt[ipoly][i].y - (double) polypoly->pt[ipoly+1][0].y;
		y2 *= y2;
		r2 = x2 + y2;
		if (i == 0)
		{	r2_min = r2;
		}
		else if (r2 < r2_min)
		{	r2_min = r2;
			imin = i;
		}
	}

	for (i = 0; i <= imin; i++)
	{	polyline->pt[polyline->count].x = polypoly->pt[ipoly][i].x;
		polyline->pt[polyline->count].y = polypoly->pt[ipoly][i].y;
		polyline->count++;
	}

	polypoly_construct (API, polypoly, polyline, (U16)(ipoly + 1));

	for (i = imin; i < count; i++)
	{	polyline->pt[polyline->count].x = polypoly->pt[ipoly][i].x;
		polyline->pt[polyline->count].y = polypoly->pt[ipoly][i].y;
		polyline->count++;
	}
	polyline->pt[polyline->count].x = polypoly->pt[ipoly][0].x;
	polyline->pt[polyline->count].y = polypoly->pt[ipoly][0].y;
	polyline->count++;
}

static int meta_round (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfDrawRectangle_t drawrect;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 5",Record->size);
	}

	par_U16_x = ParU16 (API,Record,5);
	par_U16_y = ParU16 (API,Record,4);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawrect.TL = wmf_D_Coord_translate (API,l_pt);

	par_U16_x = ParU16 (API,Record,3);
	par_U16_y = ParU16 (API,Record,2);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawrect.BR = wmf_D_Coord_translate (API,l_pt);

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		D_Coord_Register (API,drawrect.TL,scope);
		D_Coord_Register (API,drawrect.BR,scope);
		return (changed);
	}

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	drawrect.width  = (float) ((double) par_U16_x * ABS (P->dc->pixel_width ));
	drawrect.height = (float) ((double) par_U16_y * ABS (P->dc->pixel_height));

	drawrect.dc = P->dc;

	if (FR->draw_rectangle) FR->draw_rectangle (API,&drawrect);

	return (changed);
}

static int meta_rect (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfL_Coord l_pt;

	wmfDrawRectangle_t drawrect;

	wmfPen* pen = 0;

	U16 par_U16_x;
	U16 par_U16_y;

	float scope;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
	}

	par_U16_x = ParU16 (API,Record,3);
	par_U16_y = ParU16 (API,Record,2);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawrect.TL = wmf_D_Coord_translate (API,l_pt);

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	drawrect.BR = wmf_D_Coord_translate (API,l_pt);

	if (SCAN (API))
	{	pen = WMF_DC_PEN (P->dc);

		scope = (float) (MAX (WMF_PEN_WIDTH (pen),WMF_PEN_HEIGHT (pen))) / 2;

		D_Coord_Register (API,drawrect.TL,scope);
		D_Coord_Register (API,drawrect.BR,scope);
		return (changed);
	}

	drawrect.width  = 0;
	drawrect.height = 0;

	drawrect.dc = P->dc;

	if (FR->draw_rectangle) FR->draw_rectangle (API,&drawrect);

	return (changed);
}

static int meta_rgn_brush (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfObject* objects;
	wmfObject* obj_region;
	wmfObject* obj_brush;

	wmfRegion* region;
	wmfRegion* clip;

	wmfBrush* brush;
	wmfBrush* temp_brush;

	wmfD_Coord d_pt;

	wmfPolyRectangle_t polyrect;

	U16 oid_region;
	U16 oid_brush;

	U16 par_U16_x;
	U16 par_U16_y;

	unsigned int i;

	unsigned long max_index;

	float width;
	float height;

	objects = P->objects;

	if (Record->function == META_FRAMEREGION)
	{	max_index = 3;
	}
	else
	{	max_index = 1;
	}

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = %lu",Record->size,max_index);
	}

	oid_region = ParU16 (API,Record,0);
	oid_brush  = ParU16 (API,Record,1);

	if ((oid_region >= NUM_OBJECTS (API)) || (oid_brush >= NUM_OBJECTS (API)))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	obj_region = objects + oid_region;
	obj_brush  = objects + oid_brush;

	if (SCAN (API) && DIAG (API))
	{	diagnose_object (API,(unsigned int) oid_region,obj_region);
		diagnose_object (API,(unsigned int) oid_brush, obj_brush );
	}

	if ((obj_region->type != OBJ_REGION) || (obj_brush->type != OBJ_BRUSH))
	{	WMF_ERROR (API,"libwmf: have lost track of the objects in this metafile");
		WMF_ERROR (API,"        please send it to us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
		return (changed);
	}

	region = &(obj_region->obj.rgn);
	brush = &(obj_brush->obj.brush);

	if (Record->function == META_FRAMEREGION)
	{	par_U16_x = ParU16 (API,Record,3);
		par_U16_y = ParU16 (API,Record,2);

		width  = (float) ((double) par_U16_x * ABS (P->dc->pixel_width ));
		height = (float) ((double) par_U16_y * ABS (P->dc->pixel_height));
	}
	else
	{	width  = 0;
		height = 0;
	}

	if (SCAN (API))
	{	d_pt = region->extents.TL;
		d_pt.x -= width;
		d_pt.y -= height;
		D_Coord_Register (API,d_pt,0);

		d_pt = region->extents.BR;
		d_pt.x += width;
		d_pt.y += height;
		D_Coord_Register (API,d_pt,0);

		return (changed);
	}

	polyrect.dc = P->dc;

	polyrect.TL = 0;
	polyrect.BR = 0;

	polyrect.count = 0;

	polyrect.width  = 0;
	polyrect.height = 0;

	if (FR->region_clip) FR->region_clip (API,&polyrect); /* i.e., none */

	clip = (wmfRegion*) P->dc->clip;

	polyrect.count = MAX (clip->numRects,region->numRects + 1);

	polyrect.TL = (wmfD_Coord*) wmf_malloc (API,polyrect.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.BR = (wmfD_Coord*) wmf_malloc (API,polyrect.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.count = region->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = region->rects[i].TL;
		polyrect.BR[i] = region->rects[i].BR;
	}
	i = polyrect.count;
	polyrect.TL[i] = region->extents.TL;
	polyrect.BR[i] = region->extents.BR;

	polyrect.width  = width;
	polyrect.height = height;

	switch (Record->function)
	{
	case META_FRAMEREGION:
		if (FR->region_frame)
		{	temp_brush = WMF_DC_BRUSH (polyrect.dc); /* ultimately redundant ?? */

			WMF_DC_SET_BRUSH (polyrect.dc,brush);

			FR->region_frame (API,&polyrect);

			WMF_DC_SET_BRUSH (polyrect.dc,temp_brush); /* ultimately redundant ?? */
		}
	break;

	case META_FILLREGION:
		if (FR->region_paint)
		{	temp_brush = WMF_DC_BRUSH (polyrect.dc); /* ultimately redundant ?? */

			WMF_DC_SET_BRUSH (polyrect.dc,brush);

			FR->region_paint (API,&polyrect);

			WMF_DC_SET_BRUSH (polyrect.dc,temp_brush); /* ultimately redundant ?? */
		}
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	polyrect.count = clip->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = clip->rects[i].TL;
		polyrect.BR[i] = clip->rects[i].BR;
	}

	polyrect.width  = 0;
	polyrect.height = 0;

	if (FR->region_clip) FR->region_clip (API,&polyrect);

	wmf_free (API,polyrect.TL);
	wmf_free (API,polyrect.BR);

	return (changed);
}

static int meta_rgn_paint (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfObject* objects;
	wmfObject* obj_region;

	wmfRegion* region;
	wmfRegion* clip;

	wmfD_Coord d_pt;

	wmfPolyRectangle_t polyrect;

	U16 oid_region;

	U16 temp_rop;

	unsigned int i;

	objects = P->objects;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	oid_region = ParU16 (API,Record,0);

	if (oid_region >= NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	obj_region = objects + oid_region;

	if (SCAN (API) && DIAG (API))
	{	diagnose_object (API,(unsigned int) oid_region,obj_region);
	}

	if (obj_region->type != OBJ_REGION)
	{	WMF_ERROR (API,"libwmf: have lost track of the objects in this metafile");
		WMF_ERROR (API,"        please send it to us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
		return (changed);
	}

	region = &(obj_region->obj.rgn);

	if (SCAN (API))
	{	d_pt = region->extents.TL;
		D_Coord_Register (API,d_pt,0);

		d_pt = region->extents.BR;
		D_Coord_Register (API,d_pt,0);

		return (changed);
	}

	polyrect.dc = P->dc;

	polyrect.TL = 0;
	polyrect.BR = 0;

	polyrect.count = 0;

	polyrect.width  = 0;
	polyrect.height = 0;

	if (FR->region_clip) FR->region_clip (API,&polyrect); /* i.e., none */

	clip = (wmfRegion*) P->dc->clip;

	polyrect.count = MAX (clip->numRects,region->numRects + 1);

	polyrect.TL = (wmfD_Coord*) wmf_malloc (API,polyrect.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.BR = (wmfD_Coord*) wmf_malloc (API,polyrect.count * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.count = region->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = region->rects[i].TL;
		polyrect.BR[i] = region->rects[i].BR;
	}
	i = polyrect.count;
	polyrect.TL[i] = region->extents.TL;
	polyrect.BR[i] = region->extents.BR;

	switch (Record->function)
	{
	case META_INVERTREGION:
		if (FR->region_paint)
		{	temp_rop = WMF_DC_ROP (polyrect.dc); /* ultimately redundant ?? */

			WMF_DC_SET_ROP (polyrect.dc,R2_NOT);

			FR->region_paint (API,&polyrect);

			WMF_DC_SET_ROP (polyrect.dc,temp_rop); /* ultimately redundant ?? */
		}
	break;

	case META_PAINTREGION:
		if (FR->region_paint) FR->region_paint (API,&polyrect);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	polyrect.count = clip->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = clip->rects[i].TL;
		polyrect.BR[i] = clip->rects[i].BR;
	}

	if (FR->region_clip) FR->region_clip (API,&polyrect);

	wmf_free (API,polyrect.TL);
	wmf_free (API,polyrect.BR);

	return (changed);
}

static int meta_rgn_create (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfRecord start;
	wmfRecord end;

	wmfObject* objects;
	wmfObject* obj_region;

	wmfRegion* region;
	wmfRegion  temp_region;

	wmfD_Rect d_r;

	U16 i;
	U16 oid_region;

	U16 x1;
	U16 x2;
	U16 y1;
	U16 y2;

	U16 band;
	U16 num_band;
	U16 num_pair;

	U16 count;

	unsigned long max_index;

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	oid_region = i;
	obj_region = objects + oid_region;

	obj_region->type = OBJ_REGION;

	region = &(obj_region->obj.rgn);

	region->rects = (wmfD_Rect*) wmf_malloc (API,8 * sizeof (wmfD_Rect));
	region->size = 8;

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	WmfSetRectRgn (region,0);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; index 0-4,6-10 skipped; max. index = 5",Record->size);

		diagnose_object (API,(unsigned int) oid_region,obj_region);
	}

	num_band = ParU16 (API,Record,5);

	if (num_band == 0) return (changed);

	temp_region.rects = (wmfD_Rect*) wmf_malloc (API,8 * sizeof (wmfD_Rect));
	temp_region.size = 8;

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	WmfSetRectRgn (&temp_region,0);

	end = OffsetRecord (API,Record,10);
	max_index = 10;
	for (band = 0; band < num_band; band++)
	{	max_index++;
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,",%lu",max_index);
		}

		start = OffsetRecord (API,&end,1);

		count = ParU16 (API,&start,0);

		if (count & 1)
		{	WMF_ERROR (API,"Delimiter not even!");
			API->err = wmf_E_BadFormat;
			break;
		}

		num_pair = count >> 1;

		max_index += count + 3;
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,",%lu",max_index);
		}

		end = OffsetRecord (API,&start,(unsigned long) (count + 3));

		if (ParU16 (API,&end,0) != count)
		{	WMF_ERROR (API,"Mismatched delimiters!");
			API->err = wmf_E_BadFormat;
			break;
		}

		y1 = ParU16 (API,&start,1);
		y2 = ParU16 (API,&start,2);

		for (i = 0; i < num_pair; i++)
		{	x1 = ParU16 (API,&start,(unsigned long) (3 + 2 * i));
			x2 = ParU16 (API,&start,(unsigned long) (4 + 2 * i));

			D_Rect (API,&d_r,x1,y1,x2,y2);

			WmfSetRectRgn (&temp_region,&d_r);
			WmfCombineRgn (API,region,region,&temp_region,RGN_OR);
		}
	}

	wmf_free (API,temp_region.rects);

	return (changed);
}

static int meta_clip_select (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfObject* objects;
	wmfObject* obj_region;

	wmfRegion* region;
	wmfRegion* clip;

	U16 oid_region;

	objects = P->objects;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	oid_region = ParU16 (API,Record,0);

	if (oid_region >= NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	obj_region = objects + oid_region;

	if (SCAN (API) && DIAG (API))
	{	diagnose_object (API,(unsigned int) oid_region,obj_region);
	}

	if (obj_region->type != OBJ_REGION)
	{	if (API->flags & WMF_OPT_IGNORE_NONFATAL)
		{	/* Some metafiles use this even though no region-objects have been defined,
			 * so I have some doubt about the correctness of this handler; perhaps
			 * this should reset the clip region to the entire space?
			 * 
			 * Anyway, I am making this particular error non-fatal.
			 */
			WMF_DEBUG (API,"libwmf: have lost track of the objects in this metafile");
			WMF_DEBUG (API,"        please send it to us at http://www.wvware.com/");
		}
		else
		{	WMF_ERROR (API,"libwmf: have lost track of the objects in this metafile");
			WMF_ERROR (API,"        please send it to us at http://www.wvware.com/");
			API->err = wmf_E_Glitch;
		}
		return (changed);
	}

	region = &(obj_region->obj.rgn);

	clip = (wmfRegion*) P->dc->clip;

	WmfCombineRgn (API,clip,region,0,RGN_COPY);

	return (changed);
}

static int meta_clip_offset (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfRegion* clip;

	wmfPolyRectangle_t polyrect;

	wmfL_Coord l_pt;

	U16 par_U16_x;
	U16 par_U16_y;

	unsigned int i;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 1",Record->size);
	}

	clip = (wmfRegion*) P->dc->clip;

	par_U16_x = ParU16 (API,Record,1);
	par_U16_y = ParU16 (API,Record,0);

	l_pt = L_Coord (par_U16_x,par_U16_y);

	for (i = 0; i < clip->numRects; i++)
	{	clip->rects[i].TL.x += l_pt.x;
		clip->rects[i].TL.y += l_pt.y;
		clip->rects[i].BR.x += l_pt.x;
		clip->rects[i].BR.y += l_pt.y;
	}
	clip->extents.TL.x += l_pt.x;
	clip->extents.TL.y += l_pt.y;
	clip->extents.BR.x += l_pt.x;
	clip->extents.BR.y += l_pt.y;
			
	if (SCAN (API)) return (changed);

	polyrect.TL = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.BR = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.count = clip->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = clip->rects[i].TL;
		polyrect.BR[i] = clip->rects[i].BR;
	}

	polyrect.dc = P->dc;

	polyrect.width  = 0;
	polyrect.height = 0;

	if (FR->region_clip) FR->region_clip (API,&polyrect);

	wmf_free (API,polyrect.TL);
	wmf_free (API,polyrect.BR);

	return (changed);
}

static int meta_clip_combine (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfRegion* visible;
	wmfRegion* clip;

	wmfPolyRectangle_t polyrect;

	wmfD_Rect d_r;

	U16 x1;
	U16 x2;
	U16 y1;
	U16 y2;

	unsigned int i;

	visible = &(P->visible);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
	}

	x1 = ParU16 (API,Record,3);
	y1 = ParU16 (API,Record,2);
	x2 = ParU16 (API,Record,1);
	y2 = ParU16 (API,Record,0);

	D_Rect (API,&d_r,x1,y1,x2,y2);

	clip = (wmfRegion*) P->dc->clip;

	switch (Record->function)
	{
	case META_EXCLUDECLIPRECT:
		Clipping (API,clip,visible,&d_r,CLIP_EXCLUDE);
	break;

	case META_INTERSECTCLIPRECT:
		Clipping (API,clip,visible,&d_r,CLIP_INTERSECT);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if (SCAN (API)) return (changed);

	polyrect.TL = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.BR = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	polyrect.count = clip->numRects;
	for (i = 0; i < polyrect.count; i++)
	{	polyrect.TL[i] = clip->rects[i].TL;
		polyrect.BR[i] = clip->rects[i].BR;
	}

	polyrect.dc = P->dc;

	polyrect.width  = 0;
	polyrect.height = 0;

	if (FR->region_clip) FR->region_clip (API,&polyrect);

	wmf_free (API,polyrect.TL);
	wmf_free (API,polyrect.BR);

	return (changed);
}

static int meta_dib_draw (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfRecord bmp_record;

	wmfBMP_Read_t bmp_read;
	wmfBMP_Draw_t bmp_draw;

	wmfL_Coord l_pt_TL;
	wmfL_Coord l_pt;
	wmfD_Coord d_pt;

	U16 par_U16_x = 0;
	U16 par_U16_y = 0;
	U16 par_U16_w = 0;
	U16 par_U16_h = 0;

	S32 width;
	S32 height;

	long pos_current;

	double stretch_x;
	double stretch_y;

	if ((Record->function == META_DIBBITBLT) && ((Record->size) == 9)) /* Special case... */
	{	changed = meta_rop_draw (API,Record);
		return (changed);
	}

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
	}

	switch (Record->function)
	{
	case META_SETDIBTODEV:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; index 0 skipped; max. index = 8",Record->size);
		}

		par_U16_x = ParU16 (API,Record,8);
		par_U16_y = ParU16 (API,Record,7);

		par_U16_w = ParU16 (API,Record,6);
		par_U16_h = ParU16 (API,Record,5);

		bmp_draw.crop.w = par_U16_w;
		bmp_draw.crop.h = par_U16_h;

		bmp_draw.crop.x = ParU16 (API,Record,4);
		bmp_draw.crop.y = ParU16 (API,Record,3);

		bmp_read.width  = ParU16 (API,Record,2); /* uncertain about this ?? */
		bmp_read.height = ParU16 (API,Record,1);

		bmp_draw.type = SRCCOPY;

		bmp_record = OffsetRecord (API,Record,9);
	break;

	case META_STRETCHDIB:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; index 2 skipped; max. index = 10",Record->size);
		}

		par_U16_x = ParU16 (API,Record,10);
		par_U16_y = ParU16 (API,Record,9);

		par_U16_w = ParU16 (API,Record,8);
		par_U16_h = ParU16 (API,Record,7);

		bmp_draw.crop.x = ParU16 (API,Record,6);
		bmp_draw.crop.y = ParU16 (API,Record,5);

		bmp_draw.crop.w = ParU16 (API,Record,4);
		bmp_draw.crop.h = ParU16 (API,Record,3);

		bmp_read.width  = 0;
		bmp_read.height = 0;

		bmp_draw.type = (U32) ParU16 (API,Record,0) + (((U32) ParU16 (API,Record,1)) << 16);

		bmp_record = OffsetRecord (API,Record,11);
	break;

	case META_DIBSTRETCHBLT:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; max. index = 9",Record->size);
		}

		par_U16_x = ParU16 (API,Record,9);
		par_U16_y = ParU16 (API,Record,8);

		par_U16_w = ParU16 (API,Record,7);
		par_U16_h = ParU16 (API,Record,6);

		bmp_draw.crop.x = ParU16 (API,Record,5);
		bmp_draw.crop.y = ParU16 (API,Record,4);

		bmp_draw.crop.w = ParU16 (API,Record,3);
		bmp_draw.crop.h = ParU16 (API,Record,2);

		bmp_read.width  = 0;
		bmp_read.height = 0;

		bmp_draw.type = (U32) ParU16 (API,Record,0) + (((U32) ParU16 (API,Record,1)) << 16);

		bmp_record = OffsetRecord (API,Record,10);
	break;

	case META_DIBBITBLT: WMF_DEBUG (API,"(play) META_DIBBITBLT");
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; max. index = 7",Record->size);
		}

		par_U16_x = ParU16 (API,Record,7);
		par_U16_y = ParU16 (API,Record,6);

		par_U16_w = ParU16 (API,Record,5);
		par_U16_h = ParU16 (API,Record,4);

		bmp_draw.crop.x = ParU16 (API,Record,3);
		bmp_draw.crop.y = ParU16 (API,Record,2);

		bmp_draw.crop.w = par_U16_w;
		bmp_draw.crop.h = par_U16_h;

		bmp_read.width  = 0;
		bmp_read.height = 0;

		bmp_draw.type = (U32) ParU16 (API,Record,0) + (((U32) ParU16 (API,Record,1)) << 16);

		bmp_record = OffsetRecord (API,Record,8);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if ((par_U16_w == 0) || (par_U16_h == 0) || (bmp_draw.crop.w == 0) || (bmp_draw.crop.h == 0))
	{	return (changed);
	}

	l_pt_TL = L_Coord (par_U16_x,par_U16_y);

	bmp_draw.pt = wmf_D_Coord_translate (API,l_pt_TL);

	l_pt = L_Coord (par_U16_w,par_U16_h);

	width  = ABS (l_pt.x);
	height = ABS (l_pt.y);

	if (SCAN (API))
	{	D_Coord_Register (API,bmp_draw.pt,0);

		l_pt.x = l_pt_TL.x + width;
		l_pt.y = l_pt_TL.y + height;
		d_pt = wmf_D_Coord_translate (API,l_pt);
		D_Coord_Register (API,d_pt,0);

		return (changed);
	}

	pos_current = WMF_TELL (API);
	if (pos_current < 0)
	{	WMF_ERROR (API,"API's tell() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (changed);
	}

	bmp_read.offset = bmp_record.position;
	bmp_read.buffer = bmp_record.parameter;
	bmp_read.length = (long) (bmp_record.size) * 2;

	bmp_read.bmp.data = 0;

	if (FR->bmp_read) FR->bmp_read (API,&bmp_read);

	if (ERR (API) || (bmp_read.bmp.data == 0))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if (WMF_SEEK (API,pos_current) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (changed);
	}

	bmp_draw.dc = P->dc;
	bmp_draw.bmp = bmp_read.bmp;
	if (bmp_draw.crop.x >= bmp_read.bmp.width ) bmp_draw.crop.x = 0;
	if (bmp_draw.crop.y >= bmp_read.bmp.height) bmp_draw.crop.y = 0;
	if (bmp_draw.crop.x + bmp_draw.crop.w >= bmp_read.bmp.width)
	{	bmp_draw.crop.w = bmp_read.bmp.width - bmp_draw.crop.x;
	}
	if (bmp_draw.crop.y + bmp_draw.crop.h >= bmp_read.bmp.height)
	{	bmp_draw.crop.h = bmp_read.bmp.height - bmp_draw.crop.y;
	}

	if ((bmp_draw.crop.w == 0) || (bmp_draw.crop.h == 0))
	{	if (FR->bmp_free) FR->bmp_free (API,&(bmp_read.bmp));
		return (changed);
	}

	stretch_x = (double) par_U16_w / (double) bmp_draw.crop.w;
	stretch_y = (double) par_U16_h / (double) bmp_draw.crop.h;

	bmp_draw.pixel_width  = ABS (P->dc->pixel_width ) * stretch_x;
	bmp_draw.pixel_height = ABS (P->dc->pixel_height) * stretch_y;

	if (FR->bmp_draw) FR->bmp_draw (API,&bmp_draw);

	if (FR->bmp_free) FR->bmp_free (API,&(bmp_draw.bmp));

	return (changed);
}

static int meta_dib_brush (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfRecord bmp_record;

	wmfObject* objects;
	wmfObject* obj_brush;

	wmfBrush* brush;

	wmfBMP_Read_t bmp_read;

	U16 oid_brush;

	unsigned int i;

	long pos_current;

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	oid_brush = i;
	obj_brush = objects + oid_brush;

	obj_brush->type = OBJ_BRUSH;

	brush = &(obj_brush->obj.brush);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = ?",Record->size);

		diagnose_object (API,(unsigned int) oid_brush,obj_brush);
	}

	bmp_record = OffsetRecord (API,Record,2);

	pos_current = WMF_TELL (API);
	if (pos_current < 0)
	{	WMF_ERROR (API,"API's tell() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (changed);
	}

	bmp_read.offset = bmp_record.position;
	bmp_read.buffer = bmp_record.parameter;
	bmp_read.length = (long) (bmp_record.size) * 2;

	bmp_read.width  = 0;
	bmp_read.height = 0;

	bmp_read.bmp.width  = 0;
	bmp_read.bmp.height = 0;
	bmp_read.bmp.data = 0;

	if (PLAY (API) && FR->bmp_read) FR->bmp_read (API,&bmp_read);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if (WMF_SEEK (API,pos_current) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (changed);
	}

	WMF_BRUSH_SET_STYLE (brush,BS_DIBPATTERN);

	WMF_BRUSH_SET_COLOR (brush,&wmf_black);

	WMF_BRUSH_SET_BITMAP (brush,&(bmp_read.bmp));

	if (SCAN (API)) wmf_ipa_color_add (API,WMF_BRUSH_COLOR (brush));

	WMF_DC_SET_BRUSH (P->dc,brush);

	return (changed);
}

static int meta_rop_draw (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfROP_Draw_t rop_draw;

	wmfL_Coord l_pt_TL;
	wmfL_Coord l_pt;

	U16 par_U16_x = 0;
	U16 par_U16_y = 0;
	U16 par_U16_w = 0;
	U16 par_U16_h = 0;

	S32 width;
	S32 height;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
	}

	switch (Record->function)
	{
	case META_DIBBITBLT: /* META_DIBBITBLT: Special case: Size = 12 */
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; index 2-4 skipped; max. index = 8",Record->size);
		}

		par_U16_x = ParU16 (API,Record,8);
		par_U16_y = ParU16 (API,Record,7);

		par_U16_w = ParU16 (API,Record,6);
		par_U16_h = ParU16 (API,Record,5);

		rop_draw.ROP = (U32) ParU16 (API,Record,0) + (((U32) ParU16 (API,Record,1)) << 16);
	break;

	case META_PATBLT:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t#par=%lu; max. index = 5",Record->size);
		}

		par_U16_x = ParU16 (API,Record,5);
		par_U16_y = ParU16 (API,Record,4);

		par_U16_w = ParU16 (API,Record,3);
		par_U16_h = ParU16 (API,Record,2);

		rop_draw.ROP = (U32) ParU16 (API,Record,0) + (((U32) ParU16 (API,Record,1)) << 16);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	l_pt_TL = L_Coord (par_U16_x,par_U16_y);

	rop_draw.TL = wmf_D_Coord_translate (API,l_pt_TL);

	l_pt = L_Coord (par_U16_w,par_U16_h);

	width  = ABS (l_pt.x);
	height = ABS (l_pt.y);

	l_pt.x = l_pt_TL.x + width;
	l_pt.y = l_pt_TL.y + height;

	rop_draw.BR = wmf_D_Coord_translate (API,l_pt);

	if (SCAN (API))
	{	D_Coord_Register (API,rop_draw.TL,0);
		D_Coord_Register (API,rop_draw.BR,0);
		return (changed);
	}

	rop_draw.dc = P->dc;

	rop_draw.pixel_width  = ABS (P->dc->pixel_width );
	rop_draw.pixel_height = ABS (P->dc->pixel_height);

	if (FR->rop_draw) FR->rop_draw (API,&rop_draw);

	return (changed);
}

static int meta_dc_set (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	U16 par_U16;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	par_U16 = ParU16 (API,Record,0);

	switch (Record->function)
	{
	case META_SETROP2:
		WMF_DC_SET_ROP (P->dc,par_U16);
	break;

	case META_SETTEXTJUSTIFICATION:
		WMF_DC_SET_BREAKEXTRA (P->dc,par_U16);
	break;

	case META_SETTEXTCHAREXTRA:
		WMF_DC_SET_CHAREXTRA (P->dc,par_U16);
	break;

	case META_SETPOLYFILLMODE:
		WMF_DC_SET_POLYFILL (P->dc,par_U16);
	break;

	case META_SETTEXTALIGN:
		WMF_DC_SET_TEXTALIGN (P->dc,par_U16);
	break;

	case META_SETBKMODE:
		if (par_U16 == TRANSPARENT)
		{	WMF_DC_SET_TRANSPARENT (P->dc);
		}
		else
		{	WMF_DC_SET_OPAQUE (P->dc);
			if (par_U16 != OPAQUE) { WMF_DEBUG (API,"unexpected background mode; assuming opaque..."); }
		}
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	return (changed);
}

static int meta_dc_color (wmfAPI* API,wmfRecord* Record,wmfAttributes* attrlist)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfRGB color;

	U16 par_U16_rg;
	U16 par_U16_b;

	const char * value = 0;
	char hash[8];
	unsigned long rgbhex;

	static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 1",Record->size);
	}

	if (API->flags & API_ENABLE_EDITING)
	{	if ((value = wmf_attr_query (API, attrlist, "color")))
		{	if ((*value) == '#')
			{	if (sscanf (value+1, "%lx", &rgbhex) == 1)
				{	par_U16_rg = (U16) ((rgbhex >> 8) & 0xffff);
					par_U16_b  = (U16) ( rgbhex       & 0x00ff);

					if (PutParU16 (API,Record,1,par_U16_b )) changed = 1;
					if (PutParU16 (API,Record,0,par_U16_rg)) changed = 1;
				}
				else
				{	value = 0; /* force a re-write below */
				}
			}
			else
			{	value = 0; /* force a re-write below */
			}
		}
	}

	par_U16_b  = ParU16 (API,Record,1);
	par_U16_rg = ParU16 (API,Record,0);

	color = rgb (par_U16_rg,par_U16_b);

	if ((API->flags & API_ENABLE_EDITING) && ((value == 0) || changed))
	{	hash[0] = '#';
		hash[1] = hex[(color.r >> 4) & 0x0f];
		hash[2] = hex[ color.r       & 0x0f];
		hash[3] = hex[(color.g >> 4) & 0x0f];
		hash[4] = hex[ color.g       & 0x0f];
		hash[5] = hex[(color.b >> 4) & 0x0f];
		hash[6] = hex[ color.b       & 0x0f];
		hash[7] = 0;
		wmf_attr_add (API, attrlist, "color", hash);
	}

	if (SCAN (API)) wmf_ipa_color_add (API,&color);

	switch (Record->function)
	{
	case META_SETTEXTCOLOR:
		WMF_DC_SET_TEXTCOLOR (P->dc,&color);
	break;

	case META_SETBKCOLOR:
		WMF_DC_SET_BACKGROUND (P->dc,&color);
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	return (changed);
}

static int meta_dc_select (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfObject* objects;
	wmfObject* obj;

	U16 oid;

	objects = P->objects;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	oid = ParU16 (API,Record,0);

	if (oid >= NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	obj = objects + oid;

	if (SCAN (API) && DIAG (API))
	{	diagnose_object (API,(unsigned int) oid,obj);
	}

	switch (obj->type)
	{
	case OBJ_BRUSH:
		WMF_DC_SET_BRUSH (P->dc,&(obj->obj.brush));
	break;

	case OBJ_PEN:
		WMF_DC_SET_PEN (P->dc,&(obj->obj.pen));
	break;

	case OBJ_FONT:
		WMF_DC_SET_FONT (P->dc,&(obj->obj.font));
	break;

	default:
		WMF_DEBUG (API,"unexpected object type!");
	break;
	}

	return (changed);
}

static int meta_dc_save (wmfAPI* API,wmfRecord* Record) /* complete ?? */
{	int changed = 0;

	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = ?",Record->size);
	}

	dc_stack_push (API,P->dc);

	P->dc = dc_copy (API,P->dc);

	return (changed);
}

static int meta_dc_restore (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfRegion* clip;

	wmfPolyRectangle_t polyrect;
	wmfUserData_t      userdata;

	unsigned int i;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = ?",Record->size);
	}

	/* for (i = PAR(0); i < 0; i++) *//* Implies PAR(0) is signed ?? */

	userdata.dc = P->dc;
	userdata.data = P->dc->userdata;

	if (PLAY (API) && FR->udata_free) FR->udata_free (API,&userdata);

	clip = (wmfRegion*) P->dc->clip;

	wmf_free (API,clip->rects);

	wmf_free (API,P->dc->clip);
	wmf_free (API,P->dc);

	P->dc = dc_stack_pop (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	if (SCAN (API)) return (changed);

	userdata.dc = P->dc;
	userdata.data = P->dc->userdata;

	if (FR->udata_set) FR->udata_set (API,&userdata);
			
	clip = (wmfRegion*) P->dc->clip;

	polyrect.dc = P->dc;

	polyrect.width  = 0;
	polyrect.height = 0;

	if (clip->numRects)
	{	polyrect.TL = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (changed);
		}

		polyrect.BR = (wmfD_Coord*) wmf_malloc (API,clip->numRects * sizeof (wmfD_Coord));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (changed);
		}

		polyrect.count = clip->numRects;
		for (i = 0; i < polyrect.count; i++)
		{	polyrect.TL[i] = clip->rects[i].TL;
			polyrect.BR[i] = clip->rects[i].BR;
		}

		if (FR->region_clip) FR->region_clip (API,&polyrect);

		wmf_free (API,polyrect.TL);
		wmf_free (API,polyrect.BR);
	}
	else
	{	polyrect.TL = 0;
		polyrect.BR = 0;

		polyrect.count = 0;
	
		if (FR->region_clip) FR->region_clip (API,&polyrect);
	}


	return (changed);
}

static int meta_text (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;
	wmfFontData*          FD = (wmfFontData*)          API->font_data;

	wmfRecord str_record;
	wmfRecord lpDx_record;

	wmfL_Coord l_pt;

	wmfD_Coord d_pt;
	wmfD_Coord t_pt;
	wmfD_Coord o_pt;

	wmfDrawText_t drawtext;

	wmfFont* font = 0;

	U16 par_U16;
	U16 par_U16_x;
	U16 par_U16_y;

	U16 i;
	U16 length = 0;

	U16 bbox_info = 0;

	U16 l_width;

	U16* lpDx = 0;

	char buffer[2];

	char* str_save;

	double theta;
	double ratio;

	float cos_theta;
	float sin_theta;

	float width;

	switch (Record->function)
	{
	case META_TEXTOUT:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t[0x%04x]",Record->function);
			fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
		}

		length = ParU16 (API,Record,0);

		if (length == 0) break;

		if (WMF_DC_TEXTALIGN (P->dc) & TA_UPDATECP)
		{	if ((Record->size) < (unsigned long)(1 + (length + 1) / 2))
			{	WMF_ERROR (API,"Record is too short!");
				API->err = wmf_E_BadFormat;
				break;
			}

			l_pt = P->current;
		}
		else
		{	if ((Record->size) < (unsigned long)(3 + (length + 1) / 2))
			{	WMF_ERROR (API,"Record is too short!");
				API->err = wmf_E_BadFormat;
				break;
			}

			if (SCAN (API) && DIAG (API))
			{	fprintf (stderr,",-2,-1");
			}

			par_U16_x  = ParU16 (API,Record,(Record->size)-1);
			par_U16_y  = ParU16 (API,Record,(Record->size)-2);

			l_pt = L_Coord (par_U16_x,par_U16_y);
		}

		drawtext.pt = wmf_D_Coord_translate (API,l_pt);

		drawtext.TL.x = 0;
		drawtext.TL.y = 0;

		drawtext.BR.x = 0;
		drawtext.BR.y = 0;

		str_record = OffsetRecord (API,Record,1);
	break;

	case META_EXTTEXTOUT:
		if (SCAN (API) && DIAG (API))
		{	fprintf (stderr,"\t[0x%04x]",Record->function);
		}

		if (Record->size < 4)
		{	WMF_ERROR (API,"Record is too short!");
			API->err = wmf_E_BadFormat;
			break;
		}

		if (WMF_DC_TEXTALIGN (P->dc) & TA_UPDATECP)
		{	if (SCAN (API) && DIAG (API))
			{	fprintf (stderr,"\t#par=%lu; index 0-1 ignored; max. index = 3",Record->size);
			}

			l_pt = P->current;
		}
		else
		{	if (SCAN (API) && DIAG (API))
			{	fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);
			}

			par_U16_x  = ParU16 (API,Record,1);
			par_U16_y  = ParU16 (API,Record,0);

			l_pt = L_Coord (par_U16_x,par_U16_y);
		}

		drawtext.pt = wmf_D_Coord_translate (API,l_pt);

		length = ParU16 (API,Record,2);

		if (length == 0) break;

		bbox_info = ParU16 (API,Record,3);
		if (bbox_info)
		{	if (Record->size < (unsigned long)(8 + (length + 1) / 2))
			{	WMF_ERROR (API,"Record is too short!");
				API->err = wmf_E_BadFormat;
				break;
			}

			if (SCAN (API) && DIAG (API))
			{	fprintf (stderr,",7");
			}

			par_U16_x  = ParU16 (API,Record,4); /* Is this right ?? */
			par_U16_y  = ParU16 (API,Record,5);

			l_pt = L_Coord (par_U16_x,par_U16_y);

			drawtext.TL = wmf_D_Coord_translate (API,l_pt);

			par_U16_x  = ParU16 (API,Record,6); /* Is this right ?? */
			par_U16_y  = ParU16 (API,Record,7);

			l_pt = L_Coord (par_U16_x,par_U16_y);

			drawtext.BR = wmf_D_Coord_translate (API,l_pt);

			if (SCAN (API))
			{	D_Coord_Register (API,drawtext.TL,0);
				D_Coord_Register (API,drawtext.BR,0);
			}

			str_record = OffsetRecord (API,Record,8);
		}
		else
		{	if (Record->size < (unsigned long)(4 + (length + 1) / 2))
			{	WMF_ERROR (API,"Record is too short!");
				API->err = wmf_E_BadFormat;
				break;
			}

			drawtext.TL.x = 0;
			drawtext.TL.y = 0;

			drawtext.BR.x = 0;
			drawtext.BR.y = 0;

			str_record = OffsetRecord (API,Record,4);
		}
	break;

	default:
		WMF_ERROR (API,"libwmf: erk! programmer's error...");
		WMF_ERROR (API,"        please contact us at http://www.wvware.com/");
		API->err = wmf_E_Glitch;
	break;
	}

	if (ERR (API)) return (changed);

	if (length == 0) return (changed);

	drawtext.str = (char*) wmf_malloc (API,length + 1);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	font = WMF_DC_FONT (P->dc);

        /* FIXME: bug here?  Negative font height is supposed to represent absolute font pointsize */
	drawtext.font_height = (double) WMF_FONT_HEIGHT (font) * ABS (P->dc->pixel_height);

        /* FIXME: bug here, WMF_FONT_WIDTH and  WMF_FONT_HEIGHT do not necessarily have same scale! */
	drawtext.font_ratio = (double) WMF_FONT_WIDTH (font) / (double) WMF_FONT_HEIGHT (font);

	par_U16 = 0;
	for (i = 0; i < length; i++)
	{	if (i & 1)
		{	drawtext.str[i] =  (par_U16 >> 8) & 0xff;
		}
		else
		{	par_U16 = ParU16 (API,&str_record,i/2);
			if (ERR (API)) break;
			drawtext.str[i] =  par_U16 & 0xff;
		}
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}
	drawtext.str[length] = '\0';

	width = FD->stringwidth (API,font,drawtext.str);

	width = (float) ((double) width * drawtext.font_height * drawtext.font_ratio);

	lpDx_record = OffsetRecord (API,&str_record,(length+1)/2);

	if ((Record->function == META_EXTTEXTOUT) && ((lpDx_record.size) >= length))
	{	lpDx = (U16*) wmf_malloc (API,length * sizeof (U16));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (changed);
		}

		l_width = 0;
		for (i = 0; i < length; i++)
		{	lpDx[i] = ParU16 (API,&lpDx_record,i);
			if (ERR (API)) break;
			l_width += lpDx[i];
		}
		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (changed);
		}

		l_pt = L_Coord (0,0);
		t_pt = wmf_D_Coord_translate (API,l_pt);
		l_pt = L_Coord (l_width,0);
		d_pt = wmf_D_Coord_translate (API,l_pt);

		width = d_pt.x - t_pt.x;
	}

	theta = - WMF_TEXT_ANGLE (font);

	cos_theta = (float) cos (theta);
	sin_theta = (float) sin (theta);

	/* Okay, have text reference point, string width & font height, & angle (in radians)
	 * Compute bounding box and adjust for alignment:
	 */
	if (WMF_DC_TEXTALIGN (P->dc) & TA_BASELINE)
	{	/* */
	}
	else if (WMF_DC_TEXTALIGN (P->dc) & TA_BOTTOM)
	{	d_pt.y = - drawtext.font_height / 3; /* This is only approximate... */
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.pt.x += t_pt.x;
		drawtext.pt.y += t_pt.y;
	}
	else /* if (WMF_DC_TEXTALIGN (P->dc) & TA_TOP) */
	{	d_pt.y = drawtext.font_height;       /* This is only approximate... */
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.pt.x += t_pt.x;
		drawtext.pt.y += t_pt.y;
	}

	if (WMF_DC_TEXTALIGN (P->dc) & TA_CENTER)
	{	d_pt.x = width / 2;
		d_pt.y = - drawtext.font_height * 0.77;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.TR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.TR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TR,0);

		drawtext.bbox.BL.x = drawtext.pt.x - t_pt.x;
		drawtext.bbox.BL.y = drawtext.pt.y - t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BL,0);

		d_pt.x = width / 2;
		d_pt.y = drawtext.font_height * 0.23;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.BR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.BR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BR,0);

		drawtext.bbox.TL.x = drawtext.pt.x - t_pt.x;
		drawtext.bbox.TL.y = drawtext.pt.y - t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TL,0);

		d_pt.x = - width / 2;
		t_pt.x = d_pt.x * cos_theta;
		t_pt.y = d_pt.x * sin_theta;
		drawtext.pt.x += t_pt.x;
		drawtext.pt.y += t_pt.y;
	}
	else if (WMF_DC_TEXTALIGN (P->dc) & TA_RIGHT)
	{	d_pt.y = - drawtext.font_height * 0.77;
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.bbox.TR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.TR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TR,0);

		d_pt.y = drawtext.font_height * 0.23;
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.bbox.BR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.BR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BR,0);

		d_pt.x = - width;
		d_pt.y = - drawtext.font_height * 0.77;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.TL.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.TL.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TL,0);

		d_pt.x = - width;
		d_pt.y = drawtext.font_height * 0.23;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.BL.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.BL.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BL,0);

		d_pt.x = - width;
		t_pt.x = d_pt.x * cos_theta;
		t_pt.y = d_pt.x * sin_theta;
		d_pt.x = drawtext.pt.x + t_pt.x;
		d_pt.y = drawtext.pt.y + t_pt.y;

		drawtext.pt = d_pt;

		if (WMF_DC_TEXTALIGN (P->dc) & TA_UPDATECP)
		{	P->current = wmf_L_Coord_translate (API,drawtext.pt);
		}
	}
	else /* if (WMF_DC_TEXTALIGN (P->dc) & TA_LEFT) */
	{	d_pt.y = - drawtext.font_height * 0.77;
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.bbox.TL.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.TL.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TL,0);

		d_pt.y = drawtext.font_height * 0.23;
		t_pt.x = - d_pt.y * sin_theta;
		t_pt.y =   d_pt.y * cos_theta;
		drawtext.bbox.BL.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.BL.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BL,0);

		d_pt.x = width;
		d_pt.y = - drawtext.font_height * 0.77;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.TR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.TR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.TR,0);

		d_pt.x = width;
		d_pt.y = drawtext.font_height * 0.23;
		t_pt.x = d_pt.x * cos_theta - d_pt.y * sin_theta;
		t_pt.y = d_pt.x * sin_theta + d_pt.y * cos_theta;
		drawtext.bbox.BR.x = drawtext.pt.x + t_pt.x;
		drawtext.bbox.BR.y = drawtext.pt.y + t_pt.y;
		D_Coord_Register (API,drawtext.bbox.BR,0);

		if (WMF_DC_TEXTALIGN (P->dc) & TA_UPDATECP)
		{	d_pt.x = width;
			t_pt.x = d_pt.x * cos_theta;
			t_pt.y = d_pt.x * sin_theta;
			d_pt.x = drawtext.pt.x + t_pt.x;
			d_pt.y = drawtext.pt.y + t_pt.y;

			P->current = wmf_L_Coord_translate (API,drawtext.pt);
		}
	}

	if (SCAN (API))
	{	wmf_free (API,drawtext.str);
		if (lpDx) wmf_free (API,lpDx);
		return (changed);
	}

	drawtext.dc = P->dc;

	drawtext.flags = bbox_info;

	if (lpDx)
	{	str_save = drawtext.str;
		drawtext.str = buffer;
		t_pt = drawtext.pt;
		ratio = drawtext.font_ratio;
		l_width = 0;
		for (i = 0; i < length; i++)
		{	buffer[0] = str_save[i];
			buffer[1] = 0;

			l_pt = L_Coord (0,0);
			o_pt = wmf_D_Coord_translate (API,l_pt);
			l_pt = L_Coord (l_width,0);
			d_pt = wmf_D_Coord_translate (API,l_pt);
			d_pt.x -= o_pt.x;
			d_pt.y -= o_pt.y;
			drawtext.pt.x = t_pt.x + d_pt.x * cos_theta;
			drawtext.pt.y = t_pt.y + d_pt.x * sin_theta;

			l_pt = L_Coord (0,0);
			o_pt = wmf_D_Coord_translate (API,l_pt);
			l_pt = L_Coord (lpDx[i],0);
			d_pt = wmf_D_Coord_translate (API,l_pt);
			d_pt.x -= o_pt.x;
			d_pt.y -= o_pt.y;
			width = FD->stringwidth (API,font,drawtext.str);
			width = (float) ((double) width * drawtext.font_height * ratio);
			if (d_pt.x < width)
			{	drawtext.font_ratio = ratio * (d_pt.x / width);
			}
			else
			{	drawtext.font_ratio = ratio;
			}

			if (FR->draw_text) FR->draw_text (API,&drawtext);

			l_width += lpDx[i];

			drawtext.bbox.TL.x = 0;
			drawtext.bbox.TL.y = 0;
			drawtext.bbox.TR.x = 0;
			drawtext.bbox.TR.y = 0;
			drawtext.bbox.BL.x = 0;
			drawtext.bbox.BL.y = 0;
			drawtext.bbox.BR.x = 0;
			drawtext.bbox.BR.y = 0;
		}
		wmf_free (API,lpDx);
	}
	else
	{	if (FR->draw_text) FR->draw_text (API,&drawtext);
	}

	wmf_free (API,drawtext.str);

	return (changed);
}

static int meta_pen_create (wmfAPI* API,wmfRecord* Record,wmfAttributes* attrlist) /* complete, I think */
{	int changed = 0;

	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;

	wmfObject* objects = 0;
	wmfObject* obj_pen = 0;

	wmfRGB color;

	wmfPen* pen = 0;

	U16 par_U16_rg;
	U16 par_U16_b;
	U16 par_U16_w;
	U16 par_U16_s;

	U16 oid_pen;

	U16 i;

	const char * value = 0;
	char hash[8];
	unsigned long rgbhex;

	static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	oid_pen = i;
	obj_pen = objects + oid_pen;

	obj_pen->type = OBJ_PEN;

	pen = &(obj_pen->obj.pen);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; index 2 skipped; max. index = 4",Record->size);

		diagnose_object (API,(unsigned int) oid_pen,obj_pen);
	}

	par_U16_s = ParU16 (API,Record,0);

	WMF_PEN_SET_STYLE  (pen,par_U16_s); /* Need to do all of these separately... */
	WMF_PEN_SET_ENDCAP (pen,par_U16_s);
	WMF_PEN_SET_JOIN   (pen,par_U16_s);
	WMF_PEN_SET_TYPE   (pen,par_U16_s);

	par_U16_w = ParU16 (API,Record,1);

	par_U16_w = MAX (1,par_U16_w);

	WMF_PEN_SET_WIDTH  (pen,(double) par_U16_w * ABS (P->dc->pixel_width ));
	WMF_PEN_SET_HEIGHT (pen,(double) par_U16_w * ABS (P->dc->pixel_height));

	if (API->flags & API_ENABLE_EDITING)
	{	if ((value = wmf_attr_query (API, attrlist, "color")))
		{	if ((*value) == '#')
			{	if (sscanf (value+1, "%lx", &rgbhex) == 1)
				{	par_U16_rg = (U16) ((rgbhex >> 8) & 0xffff);
					par_U16_b  = (U16) ( rgbhex       & 0x00ff);

					if (PutParU16 (API,Record,4,par_U16_b )) changed = 1;
					if (PutParU16 (API,Record,3,par_U16_rg)) changed = 1;
				}
				else
				{	value = 0; /* force a re-write below */
				}
			}
			else
			{	value = 0; /* force a re-write below */
			}
		}
	}

	par_U16_b  = ParU16 (API,Record,4);
	par_U16_rg = ParU16 (API,Record,3);

	color = rgb (par_U16_rg,par_U16_b);

	if ((API->flags & API_ENABLE_EDITING) && ((value == 0) || changed))
	{	hash[0] = '#';
		hash[1] = hex[(color.r >> 4) & 0x0f];
		hash[2] = hex[ color.r       & 0x0f];
		hash[3] = hex[(color.g >> 4) & 0x0f];
		hash[4] = hex[ color.g       & 0x0f];
		hash[5] = hex[(color.b >> 4) & 0x0f];
		hash[6] = hex[ color.b       & 0x0f];
		hash[7] = 0;
		wmf_attr_add (API, attrlist, "color", hash);
	}

	WMF_PEN_SET_COLOR (pen,&color);

	if (SCAN (API)) wmf_ipa_color_add (API,&color);

	if (WMF_PEN_STYLE (pen) != PS_NULL) { WMF_DEBUG (API,"Non-null pen style..."); }

	WMF_DC_SET_PEN (P->dc,pen);

	return (changed);
}

static int meta_brush_create (wmfAPI* API,wmfRecord* Record,wmfAttributes* attrlist) /* unfinished */
{	int changed = 0;

	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;

	wmfObject* objects = 0;
	wmfObject* obj_brush = 0;

	wmfRGB color;

	wmfBrush* brush = 0;

	wmfBMP bmp;

	U16 par_U16_rg;
	U16 par_U16_b;

	U16 oid_brush;

	U16 i;

	const char * value = 0;
	char hash[8];
	unsigned long rgbhex;

	static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	oid_brush = i;
	obj_brush = objects + oid_brush;

	obj_brush->type = OBJ_BRUSH;

	brush = &(obj_brush->obj.brush);

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 3",Record->size);

		diagnose_object (API,(unsigned int) oid_brush,obj_brush);
	}

	WMF_BRUSH_SET_STYLE (brush,ParU16 (API,Record,0));

	if (API->flags & API_ENABLE_EDITING)
	{	if ((value = wmf_attr_query (API, attrlist, "color")))
		{	if ((*value) == '#')
			{	if (sscanf (value+1, "%lx", &rgbhex) == 1)
				{	par_U16_rg = (U16) ((rgbhex >> 8) & 0xffff);
					par_U16_b  = (U16) ( rgbhex       & 0x00ff);

					if (PutParU16 (API,Record,2,par_U16_b )) changed = 1;
					if (PutParU16 (API,Record,1,par_U16_rg)) changed = 1;
				}
				else
				{	value = 0; /* force a re-write below */
				}
			}
			else
			{	value = 0; /* force a re-write below */
			}
		}
	}

	par_U16_b  = ParU16 (API,Record,2);
	par_U16_rg = ParU16 (API,Record,1);

	color = rgb (par_U16_rg,par_U16_b);

	if ((API->flags & API_ENABLE_EDITING) && ((value == 0) || changed))
	{	hash[0] = '#';
		hash[1] = hex[(color.r >> 4) & 0x0f];
		hash[2] = hex[ color.r       & 0x0f];
		hash[3] = hex[(color.g >> 4) & 0x0f];
		hash[4] = hex[ color.g       & 0x0f];
		hash[5] = hex[(color.b >> 4) & 0x0f];
		hash[6] = hex[ color.b       & 0x0f];
		hash[7] = 0;
		wmf_attr_add (API, attrlist, "color", hash);
	}

	WMF_BRUSH_SET_COLOR (brush,&color);

	WMF_BRUSH_SET_HATCH (brush,ParU16 (API,Record,3));

	bmp.width  = 0;
	bmp.height = 0;

	bmp.data = 0;

	WMF_BRUSH_SET_BITMAP (brush,&bmp);

	if (SCAN (API)) wmf_ipa_color_add (API,&color);

	WMF_DC_SET_BRUSH (P->dc,brush);

	return (changed);
}

static int meta_font_create (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;
	wmfFontData* FD = (wmfFontData*) API->font_data;

	wmfRecord name_record;

	wmfObject* objects = 0;
	wmfObject* obj_font = 0;

	wmfFont* font = 0;

	U16 oid_font;

	U16 par_U16;

	S16 par_S16_w;
	S16 par_S16_h;

	unsigned long i;
	unsigned long length;

	char* fontname = 0;

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	oid_font = i;
	obj_font = objects + oid_font;

	obj_font->type = OBJ_FONT;

	font = &(obj_font->obj.font);
	font->lfFaceName = 0;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 8",Record->size);

		diagnose_object (API,(unsigned int) oid_font,obj_font);
	}

	par_S16_w = ParS16 (API,Record,1);
	par_S16_h = ParS16 (API,Record,0);

	WMF_FONT_SET_HEIGHT (font,ABS (par_S16_h));
	WMF_FONT_SET_WIDTH  (font,ABS (par_S16_w));

	WMF_FONT_SET_ESCAPEMENT  (font,ParS16 (API,Record,2)); /* text angle */
	WMF_FONT_SET_ORIENTATION (font,ParS16 (API,Record,3));

	WMF_FONT_SET_WEIGHT (font,ParU16 (API,Record,4));

	par_U16 = ParU16 (API,Record,5);
	WMF_FONT_SET_ITALIC (font,par_U16 & 0xff);

	WMF_TEXT_SET_UNDERLINE (font,(par_U16 >> 8) & 0xff);

	par_U16 = ParU16 (API,Record,6);
	WMF_TEXT_SET_STRIKEOUT (font,par_U16 & 0xff);

	WMF_FONT_SET_CHARSET (font,(par_U16 >> 8) & 0xff);

	par_U16 = ParU16 (API,Record,7);
	WMF_FONT_SET_OUT (font,par_U16 & 0xff);

	WMF_FONT_SET_CLIP (font,(par_U16 >> 8) & 0xff);

	par_U16 = ParU16 (API,Record,8);
	WMF_FONT_SET_QUALITY (font,par_U16 & 0xff);

	WMF_FONT_SET_PITCH (font,(par_U16 >> 8) & 0xff);

	if (WMF_FONT_WIDTH (font) == 0)
	{	WMF_FONT_SET_WIDTH (font,WMF_FONT_HEIGHT (font));
	}

	name_record = OffsetRecord (API,Record,9);

	length = (name_record.size) * 2;

	fontname = (char*) wmf_malloc (API,length + 1);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	par_U16 = 0;
	for (i = 0; i < length; i++)
	{	if (i & 1)
		{	fontname[i] = (par_U16 >> 8) & 0xff;
		}
		else
		{	par_U16 = ParU16 (API,&name_record,i/2);
			if (ERR (API)) break;
			fontname[i] = par_U16 & 0xff;
		}
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}
	fontname[length] = '\0';

	WMF_FONT_SET_NAME (font,fontname); /* must not free fontname */

	font->user_data = 0;

	FD->map (API,font); /* Determine/Load freetype face */

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (changed);
	}

	WMF_DC_SET_FONT (P->dc,font);

	return (changed);
}

static int meta_palette_create (wmfAPI* API)
{	int changed = 0;

	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;

	wmfObject* objects = 0;

	unsigned long i;

	objects = P->objects;

	i = 0;
	while ((i < NUM_OBJECTS (API)) && objects[i].type) i++;

	if (i == NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	objects[i].type = OBJ_PAL;

	return (changed);
}

static int meta_delete (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfObject* objects;
	wmfObject* obj;

	U16 oid;

	objects = P->objects;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = 0",Record->size);
	}

	oid = ParU16 (API,Record,0);

	if (oid >= NUM_OBJECTS (API))
	{	WMF_ERROR (API,"Object out of range!");
		API->err = wmf_E_BadFormat;
		return (changed);
	}

	obj = objects + oid;

	if (SCAN (API) && DIAG (API))
	{	diagnose_object (API,(unsigned int) oid,obj);
	}

	if (obj->type == OBJ_BRUSH)
	{	if (obj->obj.brush.lbStyle == BS_DIBPATTERN)
		{	if (PLAY (API) && FR->bmp_free) FR->bmp_free (API,&(obj->obj.brush.bmp));
		}
	}
	else if (obj->type == OBJ_REGION)
	{	wmf_free (API,obj->obj.rgn.rects);
	}
	else if (obj->type == OBJ_FONT)
	{	wmf_free (API,obj->obj.font.lfFaceName);
	}

	obj->type = 0;

	return (changed);
}

static int meta_unused (wmfAPI* API,wmfRecord* Record)
{	int changed = 0;

	if (SCAN (API) && DIAG (API))
	{	fprintf (stderr,"\t[0x%04x]",Record->function);
		fprintf (stderr,"\t#par=%lu; max. index = ?",Record->size);
	}

	/* META_SETSTRETCHBLTMODE: not all that important really */

	/* META_SETMAPPERFLAGS:
extract from http://www.melander.dk/lib/windows/gdi/fontmap.htm
					{
					Windows Font Mapping

					Ron Gery 
					Microsoft Developer Network Technology Group 

					Created: June 8, 1992 

					Filters

An application can, to some extent, filter which physical fonts are examined by the font mapper. 
Aspect-ratio filtering, which is available in both Windows versions 3.0 and 3.1, allows an 
application to specify that only fonts designed for the device's aspect ratio should be considered 
by the font mapper. An application enables and disables this filter by using the SetMapperFlags 
function. Because nonscaling raster fonts are designed with a certain aspect ratio in mind, it is 
sometimes desirable to ensure that only fonts actually designed for the device are used. When this 
filter is enabled, the font mapper does not consider any physical fonts whose design aspect ratio 
does not match that of the device. Aspect-ratio filtering does not affect TrueType fonts because 
they can scale to match any aspect ratio. This filter affects all font selections to the DC until 
the filter is turned off. Aspect-ratio filtering is a holdover from earlier times and is not a 
recommended approach in today's font world. 
					}

	So we're going to ignore this call entirely */

	/* META_REALIZEPALETTE:
	   META_SELECTPALETTE:
	   META_SETPALENTRIES:
	as these set and fiddle with the palette i don't think
	they have much relevence to our converter, we will
	do our own color management elsewhere (if we do it
	at all), so i think we can safely ignore these things. */

	/* META_ESCAPE: ?? */

	return (changed);
}

static void diagnose_object (wmfAPI* API,unsigned int oid,wmfObject* obj)
{	fprintf (stderr,"\toid=%u / %u ",(unsigned) oid,(unsigned) NUM_OBJECTS (API));

	switch (obj->type)
	{
	case OBJ_BRUSH:
		fprintf (stderr,"(brush)");
	break;

	case OBJ_PEN:
		fprintf (stderr,"(pen)");
	break;

	case OBJ_FONT:
		fprintf (stderr,"(font)");
	break;

	case OBJ_REGION:
		fprintf (stderr,"(region)");
	break;

	case OBJ_PAL:
		fprintf (stderr,"(palette)");
	break;

	default:
		fprintf (stderr,"(other [%u])",(unsigned) obj->type);
	break;
	}
}

#endif /* ! WMFPLAYER_META_H */
