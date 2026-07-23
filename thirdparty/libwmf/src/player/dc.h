/* libwmf ("player/dc.h"): library for wmf conversion
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


#ifndef WMFPLAYER_DC_H
#define WMFPLAYER_DC_H

static wmfDC* dc_copy (wmfAPI* API,wmfDC* dc)
{	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	wmfDC* dc_new = 0;

	wmfRegion* clip;
	wmfRegion* clip_new;

	wmfUserData_t userdata;

	dc_new = (wmfDC*) wmf_malloc (API,sizeof (wmfDC));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	dc_new->clip = wmf_malloc (API,sizeof (wmfRegion));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		wmf_free (API,dc_new);
		return (0);
	}

	clip_new = (wmfRegion*) dc_new->clip;

	clip_new->numRects = 0;
	clip_new->size = 8;
	clip_new->rects = (wmfD_Rect*) wmf_malloc (API,clip_new->size * sizeof (wmfD_Rect));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		wmf_free (API,dc_new->clip);
		wmf_free (API,dc_new);
		return (0);
	}

	WMF_DC_SET_BACKGROUND (dc_new,&wmf_white);
	WMF_DC_SET_TEXTCOLOR  (dc_new,&wmf_black);

	WMF_DC_SET_OPAQUE (dc_new);

	WMF_DC_SET_POLYFILL (dc_new,ALTERNATE);

	WMF_DC_SET_ROP (dc_new,R2_COPYPEN);

	WMF_DC_SET_TEXTALIGN (dc_new,TA_LEFT);

	WMF_DC_SET_CHAREXTRA  (dc_new,0);
	WMF_DC_SET_BREAKEXTRA (dc_new,0);

	if (dc)
	{	WMF_DC_SET_BRUSH (dc_new,dc->brush);
		WMF_DC_SET_PEN   (dc_new,dc->pen  );
		WMF_DC_SET_FONT  (dc_new,dc->font );

		clip = (wmfRegion*) dc->clip;

		REGION_CopyRegion (API,clip_new,clip);

		dc_new->Window.Ox = dc->Window.Ox;
		dc_new->Window.Oy = dc->Window.Oy;

		dc_new->Window.width  = dc->Window.width;
		dc_new->Window.height = dc->Window.height;

		dc_new->pixel_width  = dc->pixel_width;
		dc_new->pixel_height = dc->pixel_height;

		dc_new->map_mode = dc->map_mode;

		userdata.dc = P->dc;
		userdata.data = P->dc->userdata;

		if (PLAY (API) && FR->udata_copy) FR->udata_copy (API,&userdata);

		dc_new->userdata = userdata.data;
	}
	else
	{	SetDefaults (API,&(P->default_pen),&(P->default_brush),&(P->default_font));

		WMF_DC_SET_BRUSH (dc_new,&(P->default_brush));
		WMF_DC_SET_PEN   (dc_new,&(P->default_pen  ));
		WMF_DC_SET_FONT  (dc_new,&(P->default_font ));

		dc_new->Window.Ox = 0;
		dc_new->Window.Oy = 0;

		dc_new->Window.width  = 1024;
		dc_new->Window.height = 1024;

		dc_new->pixel_width  = 1;
		dc_new->pixel_height = 1;

		dc_new->map_mode = MM_TEXT;

		userdata.dc = dc_new;
		userdata.data = 0;

		if (PLAY (API) && FR->udata_init) FR->udata_init (API,&userdata);

		dc_new->userdata = userdata.data;
	}

	return (dc_new);
}

static void dc_stack_push (wmfAPI* API,wmfDC* dc)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	wmfDC** more = 0;

	if (ERR (API)) return;

	if (dc == 0)
	{	API->err = wmf_E_Glitch;
		return;
	}

	if (P->dc_stack_length >= 1024)
	{	WMF_ERROR (API,"DC stack depth limit exceeded!");
		API->err = wmf_E_BadFormat;
		return;
	}

	if (P->dc_stack == 0)
	{	P->dc_stack = (wmfDC**) wmf_malloc (API,8 * sizeof (wmfDC*));

		if (ERR (API)) return;

		P->dc_stack_maxlen = 8;
	}

	if (P->dc_stack_length == P->dc_stack_maxlen)
	{	more =  (wmfDC**) wmf_realloc (API,P->dc_stack,(P->dc_stack_maxlen + 8) * sizeof (wmfDC*));

		if (ERR (API)) return;

		P->dc_stack = more;
		P->dc_stack_maxlen += 8;
	}

	P->dc_stack[P->dc_stack_length] = dc;
	P->dc_stack_length++;
}

static wmfDC* dc_stack_pop (wmfAPI* API)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	if (ERR (API)) return (0);

	if (P->dc_stack_length == 0)
	{	API->err = wmf_E_Glitch;
		return (0);
	}

	P->dc_stack_length--;

	return (P->dc_stack[P->dc_stack_length]);
}

static void dc_stack_free (wmfAPI* API)
{	wmfPlayer_t* P = (wmfPlayer_t*) API->player_data;

	while (P->dc_stack_length)
	{	P->dc_stack_length--;
		wmf_free (API,P->dc_stack[P->dc_stack_length]);
	}

	wmf_free (API,P->dc_stack);

	P->dc_stack = 0;
	P->dc_stack_maxlen = 0;
}

#endif /* ! WMFPLAYER_DC_H */
