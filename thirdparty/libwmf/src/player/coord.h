/* libwmf ("player/coord.h"): library for wmf conversion
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


/* File to be included from player/player.c
 */

#ifndef WMFPLAYER_COORD_H
#define WMFPLAYER_COORD_H

/**
 * Set the device origin coordinate.
 * 
 * @param API  the API handle
 * @param d_pt origin in device coordinates
 * 
 * Not really recommended.
 */
void wmf_set_viewport_origin (wmfAPI* API,wmfD_Coord d_pt)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	P->Viewport_Origin = d_pt;
}

static wmfL_Coord L_Coord (U16 u16_x,U16 u16_y)
{	wmfL_Coord l_pt;

	l_pt.x = U16_2_S32 (u16_x);
	l_pt.y = U16_2_S32 (u16_y);

	return (l_pt);
}

static void D_Rect (wmfAPI* API,wmfD_Rect* d_r,U16 u16_x1,U16 u16_y1,U16 u16_x2,U16 u16_y2)
{	wmfL_Coord l_pt_1;
	wmfL_Coord l_pt_2;

	wmfD_Coord d_pt_1;
	wmfD_Coord d_pt_2;

	l_pt_1.x = U16_2_S32 (u16_x1);
	l_pt_1.y = U16_2_S32 (u16_y1);
	l_pt_2.x = U16_2_S32 (u16_x2);
	l_pt_2.y = U16_2_S32 (u16_y2);

	d_pt_1 = wmf_D_Coord_translate (API,l_pt_1);
	d_pt_2 = wmf_D_Coord_translate (API,l_pt_2);

	if (d_pt_1.x <= d_pt_2.x)
	{	d_r->TL.x = d_pt_1.x;
		d_r->BR.x = d_pt_2.x;
	}
	else
	{	d_r->TL.x = d_pt_2.x;
		d_r->BR.x = d_pt_1.x;
	}

	if (d_pt_1.y <= d_pt_2.y)
	{	d_r->TL.y = d_pt_1.y;
		d_r->BR.y = d_pt_2.y;
	}
	else
	{	d_r->TL.y = d_pt_2.y;
		d_r->BR.y = d_pt_1.y;
	}
}

static void D_Coord_Register (wmfAPI* API,wmfD_Coord d_pt,float scope)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	if ((P->flags & PLAYER_TLBR_D_SET) == 0)
	{	P->D_TL = d_pt;
		P->D_BR = d_pt;

		P->flags |= PLAYER_TLBR_D_SET;
	}

	scope = ABS (scope);

	if (P->D_TL.x > (d_pt.x - scope)) P->D_TL.x = d_pt.x - scope;
	if (P->D_TL.y > (d_pt.y - scope)) P->D_TL.y = d_pt.y - scope;
	if (P->D_BR.x < (d_pt.x + scope)) P->D_BR.x = d_pt.x + scope;
	if (P->D_BR.y < (d_pt.y + scope)) P->D_BR.y = d_pt.y + scope;
}

static wmfL_Coord wmf_L_Coord_translate (wmfAPI* API,wmfD_Coord d_pt)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfL_Coord l_pt;

	d_pt.x += P->Viewport_Origin.x;
	d_pt.y += P->Viewport_Origin.y;

	l_pt.x = (float) ((double) d_pt.x / PixelWidth (API));
	l_pt.y = (float) ((double) d_pt.y / PixelHeight (API));

	l_pt.x += P->dc->Window.Ox;
	l_pt.y += P->dc->Window.Oy;

	return (l_pt);
}

static wmfD_Coord wmf_D_Coord_translate (wmfAPI* API,wmfL_Coord l_pt)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfD_Coord d_pt;

	l_pt.x -= P->dc->Window.Ox;
	l_pt.y -= P->dc->Window.Oy;

	d_pt.x = (float) ((double) l_pt.x * PixelWidth (API));
	d_pt.y = (float) ((double) l_pt.y * PixelHeight (API));

	d_pt.x -= P->Viewport_Origin.x;
	d_pt.y -= P->Viewport_Origin.y;

	return (d_pt);
}

static void WmfSetMapMode (wmfAPI* API,U16 map_mode)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	switch (map_mode)
	{
	case (MM_TEXT): /* each unit is 1pt */ WMF_DEBUG (API,"map_mode: MM_TEXT");
		P->dc->pixel_width  = 1;
		P->dc->pixel_height = 1;
	break;

	case (MM_LOMETRIC): /* each unit is 0.1mm */ WMF_DEBUG (API,"map_mode: MM_LOMETRIC");
		P->dc->pixel_width  = INCH_TO_POINT (MM_TO_INCH (0.1));
		P->dc->pixel_height = INCH_TO_POINT (MM_TO_INCH (0.1));
	break;

	case (MM_HIMETRIC): /* each unit is 0.01mm */ WMF_DEBUG (API,"map_mode: MM_HIMETRIC");
		P->dc->pixel_width  = INCH_TO_POINT (MM_TO_INCH (0.01));
		P->dc->pixel_height = INCH_TO_POINT (MM_TO_INCH (0.01));
	break;

	case (MM_LOENGLISH): /* each unit is 0.01 inch */ WMF_DEBUG (API,"map_mode: MM_LOENGLISH");
		P->dc->pixel_width  = INCH_TO_POINT (0.01);
		P->dc->pixel_height = INCH_TO_POINT (0.01);
	break;

	case (MM_HIENGLISH): /* each unit is 0.001 inch */ WMF_DEBUG (API,"map_mode: MM_HIENGLISH");
		P->dc->pixel_width  = INCH_TO_POINT (0.001);
		P->dc->pixel_height = INCH_TO_POINT (0.001);
	break;

	case (MM_TWIPS): /* each unit is 1/1440 inch */ WMF_DEBUG (API,"map_mode: MM_TWIPS");
		P->dc->pixel_width  = 0.05;
		P->dc->pixel_height = 0.05;
	break;

	case (MM_ISOTROPIC):
	case (MM_ANISOTROPIC): WMF_DEBUG (API,"map_mode: MM_[AN]ISOTROPIC");
		/* scale here depends on window & viewport extents */
		PixelWidth (API);
		PixelHeight (API);
	break;

	default:
		if (PLACEABLE (API))
		{	WMF_DEBUG (API,"map_mode: MM_DPI (placeable)");

			P->dc->pixel_width  = INCH_TO_POINT ((double) 1 / (double) DPI (API));
			P->dc->pixel_height = INCH_TO_POINT ((double) 1 / (double) DPI (API));

			map_mode = MM_DPI; /* [fjf] added this - uncertainly */
		}
		else
		{	WMF_ERROR (API,"unexpected mapping mode!");
			API->err = wmf_E_BadFormat;
		}
	break;
	}

	P->dc->map_mode = map_mode;
}

static double PixelWidth (wmfAPI* API)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	switch (P->dc->map_mode)
	{
	case (MM_ISOTROPIC):	/* scale here depends on window & viewport extents */
	case (MM_ANISOTROPIC):

		if (P->dc->Window.width == 0)
		{	WMF_ERROR (API,"PixelWidth: window has bad size!");
			API->err = wmf_E_BadFormat;
			break;
		}
		if (P->Viewport_Width == 0)
		{	WMF_ERROR (API,"PixelWidth: viewport has bad size!");
			API->err = wmf_E_BadFormat;
			break;
		}
		P->dc->pixel_width  = (double) P->Viewport_Width  / (double) P->dc->Window.width;

	break;

	case (MM_TEXT):      /* each unit is 1pt */
	case (MM_LOMETRIC):  /* each unit is 0.1mm */
	case (MM_HIMETRIC):  /* each unit is 0.01mm */
	case (MM_LOENGLISH): /* each unit is 0.01 inch */
	case (MM_HIENGLISH): /* each unit is 0.001 inch */
	case (MM_TWIPS):     /* each unit is 1/1440 inch */
	case (MM_DPI):       /* isotropic; placeable meta file */
		if (P->dc->Window.width < 0)
			return -P->dc->pixel_width;
	break;

	default:
		if (!ERR (API))
		{	WMF_ERROR (API,"unexpected mapping mode!");
			API->err = wmf_E_Glitch;
		}
	break;
	}

	if (ERR (API)) return (1);

	return (P->dc->pixel_width);
}

static double PixelHeight (wmfAPI* API)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	switch (P->dc->map_mode)
	{
	case (MM_ISOTROPIC):	/* scale here depends on window & viewport extents */
	case (MM_ANISOTROPIC):

		if (P->dc->Window.height == 0)
		{	WMF_ERROR (API,"PixelHeight: window has bad size!");
			API->err = wmf_E_BadFormat;
			break;
		}
		if (P->Viewport_Height == 0)
		{	WMF_ERROR (API,"PixelHeight: viewport has bad size!");
			API->err = wmf_E_BadFormat;
			break;
		}
		P->dc->pixel_height  = (double) P->Viewport_Height / (double) P->dc->Window.height;

	break;

	case (MM_TEXT):      /* each unit is 1pt */
	case (MM_LOMETRIC):  /* each unit is 0.1mm */
	case (MM_HIMETRIC):  /* each unit is 0.01mm */
	case (MM_LOENGLISH): /* each unit is 0.01 inch */
	case (MM_HIENGLISH): /* each unit is 0.001 inch */
	case (MM_TWIPS):     /* each unit is 1/1440 inch */
	case (MM_DPI):       /* isotropic; placeable meta file */
		if (P->dc->Window.height < 0)
			return -P->dc->pixel_height;
	break;

	default:
		if (!ERR (API))
		{	WMF_ERROR (API,"unexpected mapping mode!");
			API->err = wmf_E_Glitch;
		}
	break;
	}

	if (ERR (API)) return (1);

	return (P->dc->pixel_height);
}

#endif /* ! WMFPLAYER_COORD_H */
