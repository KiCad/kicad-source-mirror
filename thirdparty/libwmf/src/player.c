/* libwmf (player.c): library for wmf conversion
   Copyright (C) 2000,2001 - various; see CREDITS, ChangeLog, and sources

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


#ifdef HAVE_CONFIG_H
#include "wmfconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "wmfdefs.h"
#include "metadefs.h"

#include <stdint.h>
#include "player.h"
#include "player/region.h"   /* Provides: REGION manipulation functions  */
#include "player/clip.h"     /* Provides: clip function                  */
#include "player/color.h"    /* Provides: color stuff                    */
#include "player/coord.h"    /* Provides: coordinate translations        */
#include "player/dc.h"       /* Provides: dc stack & other dc functions  */
#include "player/defaults.h" /* Provides: default settings               */
#include "player/record.h"   /* Provides: parameter mechanism            */
#include "player/meta.h"     /* Provides: record interpreters            */

/**
 * @internal
 */
wmf_error_t wmf_player_init (wmfAPI* API)
{	wmfPlayer_t* P = 0;

	WMF_DEBUG (API,"~~~~~~~~wmf_player_init");

	API->player_data = wmf_malloc (API,sizeof (wmfPlayer_t));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	P = (wmfPlayer_t*) API->player_data;

	P->flags = 0;

	return (API->err);
}

/**
 * Scan the metafile.
 * 
 * @param API   the API handle
 * @param flags (unused)
 * @param d_r   for bounding-box return
 * 
 * Before the metafile can be played, it must be scanned. This is because metafile headers do not always
 * provide image size information. Although the device layer (the graphics exporter) is initialized in
 * wmf_api_create(), the output device is not opened until after the metafile is scanned. By scanning,
 * therefore, the device layer can be provided on opening not only with size information but also a list
 * of colors to expect (not including those in raster images) and of other required resources, if necessary.
 * Finally, if scanning fails, then there's certainly no point in playing the metafile.
 * 
 * The bounding box, in device coordinates, is returned in \p *d_r.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 */
wmf_error_t wmf_scan (wmfAPI* API,unsigned long flags,wmfD_Rect* d_r)
{	(void)flags;
	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;

	WMF_DEBUG (API,"~~~~~~~~wmf_scan");

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

/* wmf_scan doesn't allow re-scanning, but I suppose there may be a case
 * implementing a rescan function (or WMF_RESCAN flag?) at some point ??
 */
	if (P->flags & PLAYER_SCANNED)
	{	WMF_DEBUG (API,"already scanned; skipping...");
		return (API->err);
	}

	P->dc_stack_maxlen = 0;
	P->dc_stack = 0;

	P->objects = 0;

	P->D_TL.x = 0;
	P->D_TL.y = 0;
	P->D_BR.x = 0;
	P->D_BR.y = 0;

	d_r->TL = P->D_TL;
	d_r->BR = P->D_BR;

	P->flags &= ~PLAYER_PLAY; /* Set scan mode */

	wmf_header_read (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	if (API->File->wmfheader->NumOfObjects > 0)
	{	//each object needs a CREATE* record of at least 6 bytes
		if (wmf_can_supply (API, (long) NUM_OBJECTS (API) * 6) == (-1))
		{	WMF_DEBUG (API,"bailing...");
			return (API->err);
		}

		P->objects = (wmfObject*) wmf_malloc (API,NUM_OBJECTS (API) * sizeof (wmfObject));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (API->err);
		}
	}

	if (MAX_REC_SIZE(API) > UINT32_MAX / 2)
	{
		API->err = wmf_E_InsMem;
		WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	U32 nMaxRecordSize = (MAX_REC_SIZE(API)  ) * 2 * sizeof (unsigned char);
	if (nMaxRecordSize)
	{	if (wmf_can_supply (API, (long) nMaxRecordSize) == (-1))
		{	WMF_DEBUG (API,"bailing...");
			return (API->err);
		}
	}

 	P->Parameters = (unsigned char*) wmf_malloc (API, nMaxRecordSize);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	WmfPlayMetaFile (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	d_r->TL = P->D_TL;
	d_r->BR = P->D_BR;

	P->flags |= PLAYER_SCANNED;

	return (API->err);
}

/**
 * Get image size.
 * 
 * @param API    the API handle
 * @param width  width return
 * @param height height return
 * 
 * wmf_size() returns image width in \p *width and image height in \p *height. If supplied, the metafile
 * header values are used, otherwise the width and height found by wmf_scan() are used.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error state of \b wmf_E_Glitch (the metafile has not been scanned).
 */
wmf_error_t wmf_size (wmfAPI* API,float* width,float* height)
{	wmfPlayer_t* P  = (wmfPlayer_t*) API->player_data;

	S16 default_width;
	S16 default_height;

	WMF_DEBUG (API,"~~~~~~~~wmf_size");

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	if ((P->flags & PLAYER_SCANNED) == 0)
	{	WMF_ERROR (API,"attempt to determine size of unscanned metafile!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	default_width  = WMF_BBOX_RIGHT (API) - WMF_BBOX_LEFT (API);
	default_height = WMF_BBOX_TOP (API) - WMF_BBOX_BOTTOM (API);

	default_width  = ABS (default_width );
	default_height = ABS (default_height);

	if (default_width && default_height)
	{	(*width)  = (float) default_width;
		(*height) = (float) default_height;
	}
	else
	{	(*width)  = P->D_BR.x - P->D_TL.x;
		(*height) = P->D_BR.y - P->D_TL.y;
	}

	return (API->err);
}

/**
 * Get estimate of image display size.
 * 
 * @param API    the API handle
 * @param width  width return
 * @param height height return
 * @param res_x  x-resolution of display
 * @param res_y  y-resolution of display
 * 
 * wmf_display_size() returns image width in \p *width and image height in \p *height.
 * wmf_size() is used to get the calculated/estimate width and height of the image,
 * and these values are converted to integer width and height estimates for display.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error state of \b wmf_E_Glitch (the metafile has not been scanned).
 */
wmf_error_t wmf_display_size (wmfAPI* API,unsigned int* width,unsigned int* height,
			      double res_x,double res_y)
{	unsigned int units_per_inch = 1440;

	double disp_width;
	double disp_height;

	float est_width;
	float est_height;

	WMF_DEBUG (API,"~~~~~~~~wmf_display_size");

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	wmf_size (API, &est_width, &est_height);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	if (PLACEABLE (API))
	{	units_per_inch = DPI (API);
	}
	else if ((est_width * est_height) < (1024 * 1024))
	{	units_per_inch = 72;
	}

	disp_width  = ((double) est_width ) * res_x / (double) units_per_inch;
	disp_height = ((double) est_height) * res_y / (double) units_per_inch;

	if (width)  *width  = (unsigned int) disp_width;
	if (height) *height = (unsigned int) disp_height;

	return API->err;
}

/**
 * Play the metafile.
 * 
 * @param API   the API handle
 * @param flags (unused)
 * @param d_r   for bounding-box return
 * 
 * Before the metafile can be played, it must be scanned - see wmf_scan().
 * 
 * The first time (and only the first time) the metafile is played, it first calls \b device_open() for the
 * device layer specified (and initialized) in wmf_api_create(). Then, and also each subsequent time the
 * metafile is played, it calls \b device_begin(), plays the metafile (with calls to various other device
 * layer functions), and finally it calls \b device_end(). \b device_close() is only ever called via
 * wmf_api_destroy().
 * 
 * \p d_r is recomputed, but should not change.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 */
wmf_error_t wmf_play (wmfAPI* API,unsigned long flags,wmfD_Rect* d_r)
{	(void)flags;
	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	WMF_DEBUG (API,"~~~~~~~~wmf_play");

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	if ((P->flags & PLAYER_SCANNED) == 0)
	{	WMF_ERROR (API,"attempt to play unscanned metafile!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	if ((API->flags & API_DEVICE_OPEN) == 0)
	{	if (FR->device_open) FR->device_open (API);

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (API->err);
		}

		API->flags |= API_DEVICE_OPEN;
	}

	d_r->TL = P->D_TL;
	d_r->BR = P->D_BR;

	P->flags |= PLAYER_PLAY; /* Set play mode */

	WmfPlayMetaFile (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	d_r->TL = P->D_TL;
	d_r->BR = P->D_BR;

	return (API->err);
}

static wmf_error_t WmfPlayMetaFile (wmfAPI* API)
{	unsigned int i;
	int byte;
	int changed;

	unsigned char* Par;

	unsigned int Function;
	unsigned long Size;
	unsigned long number;

	long pos_params;
	long pos_current;
	long pos_max;

	wmfObject* objects = 0;

	wmfUserData_t userdata;

	wmfRegion* visible;

	wmfRecord Record;

	wmfPlayer_t*          P  = (wmfPlayer_t*)          API->player_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;
	wmfFontData*          FD = (wmfFontData*)          API->font_data;

	wmfAttributes   attrlist;
	wmfAttributes * atts = 0;

	WMF_DEBUG (API,"~~~~~~~~WmfPlayMetaFile");

	P->dc_stack_length = 0;

	visible = &(P->visible);

	visible->numRects = 0;
	visible->rects = 0;
	visible->size = 0;

	P->current.x = 0; /* Should this be in the DC ?? */
	P->current.y = 0;

	P->Viewport_Origin.x = 0; /* Should this be in the DC ?? */
	P->Viewport_Origin.y = 0;

	P->Viewport_Width  = 1024; /* Should this be in the DC ?? */
	P->Viewport_Height = 1024;

/* Can SetDefaults go in player_init ??
 */
	if (FD == 0)
	{	WMF_ERROR (API,"Glitch! No font engine interface!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	if (FD->map == 0)
	{	WMF_ERROR (API,"Glitch! No font-mapper!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	if (FD->stringwidth == 0)
	{	WMF_ERROR (API,"Glitch! No string width function!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	if ((API->bbuf.read == 0) || (API->bbuf.seek == 0) || (API->bbuf.tell == 0))
	{	WMF_ERROR (API,"WmfPlayMetaFile: input functions set improperly!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	P->dc = dc_copy (API,0);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	WmfSetMapMode (API,(U16)(PLACEABLE (API) ? MM_DPI : MM_TEXT));

	if (PLAY (API))
	{	if (FR->device_begin) FR->device_begin (API);

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return (API->err);
		}
	}

	Par = P->Parameters;

	objects = P->objects;

	for (i = 0; i < NUM_OBJECTS (API); i++) objects[i].type = 0;

	if (WMF_SEEK (API,API->File->pos) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (API->err);
	}

	wmf_attr_new (API, &attrlist);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	number = 0;
	do
	{	if (++number < API->store.count)
		{	atts = API->store.attrlist + number;
		}
		else
		{	atts = &attrlist;
			wmf_attr_clear (API, atts);
		}

		Size     = wmf_read_32 (API,0,0);
		Function = wmf_read_16 (API);

		/* Propagate any current error from the size/function reads as-ism
		 * so the cleanup at the end of the loop can recognise it. */
		if (ERR (API)) break;

		if ((Size == 3) && (Function == 0))
		{	if (SCAN (API)) wmf_write (API, Size, Function, "empty",
					atts->atts, 0, 0);
			break; /* Probably final record ?? */
		}

/*		if ((Size > MAX_REC_SIZE (API)) || (Size < 3))
 */		if (((Size - 3) > MAX_REC_SIZE (API)) || (Size < 3))
		{	WMF_ERROR (API,"libwmf: wmf with bizarre record size; bailing...");
			WMF_ERROR (API,"        please send it to us at http://www.wvware.com/");
			wmf_printf (API,"maximum record size = %u\n",(unsigned) MAX_REC_SIZE (API));
			wmf_printf (API,"record size = %u\n",(unsigned) Size);
			API->err = wmf_E_BadFormat;
			break;
		}

		pos_params = WMF_TELL (API);

		if (pos_params < 0)
		{	WMF_ERROR (API,"API's tell() failed on input stream!");
			API->err = wmf_E_BadFile;
			break;
		}

		for (i = 0; i < ((Size - 3) * 2); i++)
		{	byte = WMF_READ (API);
			if (byte == (-1))
			{	WMF_ERROR (API,"Unexpected EOF!");
				API->err = wmf_E_EOF;
				break;
			}
			Par[i] = (unsigned char) byte;
		}

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			break;
		}

		Record.size = Size - 3;
		Record.function = Function;
		Record.parameter = Par;
		Record.position = pos_params;

		switch (Function)
		{
		case META_SETMAPMODE:
			SCAN_DIAGNOSTIC (API,"New Record: SETMAPMODE");
			changed = meta_mapmode (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setmapmode",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETWINDOWORG:
			SCAN_DIAGNOSTIC (API,"New Record: SETWINDOWORG");
			changed = meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setwindoworg",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETVIEWPORTORG:
			SCAN_DIAGNOSTIC (API,"New Record: SETVIEWPORTORG");
			meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setviewportorg",
					atts->atts, Record.parameter, Record.size * 2);
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETVIEWPORTEXT:
			SCAN_DIAGNOSTIC (API,"New Record: SETVIEWPORTEXT");
			changed = meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setviewportext",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETWINDOWEXT:
			SCAN_DIAGNOSTIC (API,"New Record: SETWINDOWEXT");
			changed = meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setwindowext",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_OFFSETWINDOWORG:
			SCAN_DIAGNOSTIC (API,"New Record: OFFSETWINDOWORG");
			changed = meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "offsetwindoworg",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_OFFSETVIEWPORTORG:
			SCAN_DIAGNOSTIC (API,"New Record: OFFSETVIEWPORTORG");
			changed = meta_orgext (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "offsetviewportorg",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SCALEWINDOWEXT:
			SCAN_DIAGNOSTIC (API,"New Record: SCALEWINDOWEXT");
			changed = meta_scale (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "scalewindowext",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SCALEVIEWPORTEXT:
			SCAN_DIAGNOSTIC (API,"New Record: SCALEVIEWPORTEXT");
			changed = meta_scale (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "scaleviewportext",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* Following were originally play-only:
 * (a) basic draw
 * (b) REGION calls
 * (c) BMP & ROP stuff
 * (d) DC set
 * (e) text & font
 * (f) palette
 * (g) create & delete; save & restore; ...
 * (h) ==== other ====
 */

/* (a) basic draw
 */
		case META_MOVETO:
			SCAN_DIAGNOSTIC (API,"New Record: MOVETO");
			changed = meta_moveto (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "moveto",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_FLOODFILL:
			SCAN_DIAGNOSTIC (API,"New Record: FLOODFILL");
			changed = meta_flood (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "floodfill",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_EXTFLOODFILL:
			SCAN_DIAGNOSTIC (API,"New Record: EXTFLOODFILL");
			changed = meta_flood (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "extfloodfill",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETPIXEL:
			SCAN_DIAGNOSTIC (API,"New Record: SETPIXEL");
			changed = meta_pixel (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setpixel",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_PIE:
			SCAN_DIAGNOSTIC (API,"New Record: PIE");
			changed = meta_arc (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "pie",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_CHORD:
			SCAN_DIAGNOSTIC (API,"New Record: CHORD");
			changed = meta_arc (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "chord",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_ARC:
			SCAN_DIAGNOSTIC (API,"New Record: ARC");
			changed = meta_arc (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "arc",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_ELLIPSE:
			SCAN_DIAGNOSTIC (API,"New Record: ELLIPSE");
			changed = meta_ellipse (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "ellipse",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_LINETO:
			SCAN_DIAGNOSTIC (API,"New Record: LINETO");
			changed = meta_line (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "lineto",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_POLYLINE:
			SCAN_DIAGNOSTIC (API,"New Record: POLYLINE");
			changed = meta_lines (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "polyline",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_POLYGON:
			SCAN_DIAGNOSTIC (API,"New Record: POLYGON");
			changed = meta_polygon (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "polygon",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_POLYPOLYGON:
			SCAN_DIAGNOSTIC (API,"New Record: POLYPOLYGON");
			changed = meta_polygons (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "polypolygon",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_ROUNDRECT:
			SCAN_DIAGNOSTIC (API,"New Record: ROUNDRECT");
			changed = meta_round (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "roundrect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_RECTANGLE:
			SCAN_DIAGNOSTIC (API,"New Record: RECTANGLE");
			changed = meta_rect (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "rectangle",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (b) REGION calls
 */
		case META_FRAMEREGION:
			SCAN_DIAGNOSTIC (API,"New Record: FRAMEREGION");
			changed = meta_rgn_brush (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "frameregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_FILLREGION:
			SCAN_DIAGNOSTIC (API,"New Record: FILLREGION");
			changed = meta_rgn_brush (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "fillregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_INVERTREGION:
			SCAN_DIAGNOSTIC (API,"New Record: INVERTREGION");
			changed = meta_rgn_paint (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "invertregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_PAINTREGION:
			SCAN_DIAGNOSTIC (API,"New Record: PAINTREGION");
			changed = meta_rgn_paint (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "paintregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_CREATEREGION:
			SCAN_DIAGNOSTIC (API,"New Record: CREATEREGION");
			changed = meta_rgn_create (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "createregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SELECTCLIPREGION:
			SCAN_DIAGNOSTIC (API,"New Record: SELECTCLIPREGION");
			changed = meta_clip_select (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "selectclipregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_OFFSETCLIPRGN:
			SCAN_DIAGNOSTIC (API,"New Record: OFFSETCLIPREGION");
			changed = meta_clip_offset (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "offsetclipregion",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_EXCLUDECLIPRECT:
			SCAN_DIAGNOSTIC (API,"New Record: EXCLUDECLIPRECT");
			changed = meta_clip_combine (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "excludecliprect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_INTERSECTCLIPRECT:
			SCAN_DIAGNOSTIC (API,"New Record: INTERSECTCLIPRECT");
			changed = meta_clip_combine (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "intersectcliprect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (c) BMP & ROP stuff
 * Notes: (1) BMP width/height may be signed! ?? i.e., -width,-height ??
 *        (2) Check! Check! Check!
 */
		case META_SETDIBTODEV:
			SCAN_DIAGNOSTIC (API,"New Record: SETDIBTODEV");
			changed = meta_dib_draw (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setdibtodev",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_STRETCHDIB:
			SCAN_DIAGNOSTIC (API,"New Record: STRETCHDIB");
			changed = meta_dib_draw (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "stretchdib",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_DIBSTRETCHBLT:
			SCAN_DIAGNOSTIC (API,"New Record: DIBSTRETCHBLT");
			changed = meta_dib_draw (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "dibstretchblt",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_DIBBITBLT:
			SCAN_DIAGNOSTIC (API,"New Record: DIBBITBLT");
			changed = meta_dib_draw (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "dibbitblt",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_DIBCREATEPATTERNBRUSH:
			SCAN_DIAGNOSTIC (API,"New Record: DIBCREATEPATTERNBRUSH");
			changed = meta_dib_brush (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "dibcreatepatternbrush",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_PATBLT:
			SCAN_DIAGNOSTIC (API,"New Record: PATBLT");
			changed = meta_rop_draw (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "patblt",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (d) DC set
 */
		case META_SETROP2:
			SCAN_DIAGNOSTIC (API,"New Record: SETROP2");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setrop2",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETTEXTJUSTIFICATION:
			SCAN_DIAGNOSTIC (API,"New Record: SETTEXTJUSTIFICATION");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "settextjustification",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETTEXTCHAREXTRA:
			SCAN_DIAGNOSTIC (API,"New Record: SETTEXTCHAREXTRA");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "settextcharextra",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETPOLYFILLMODE:
			SCAN_DIAGNOSTIC (API,"New Record: SETPOLYFILLMODE");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setpolyfillmode",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETSTRETCHBLTMODE:
			SCAN_DIAGNOSTIC (API,"New Record: SETSTRETCHBLTMODE");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setstretchbltmode",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETTEXTALIGN:
			SCAN_DIAGNOSTIC (API,"New Record: SETTEXTALIGN");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "settextalign",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETTEXTCOLOR:
			SCAN_DIAGNOSTIC (API,"New Record: SETTEXTCOLOUR");
			changed = meta_dc_color (API,&Record,atts);
			if (SCAN (API)) wmf_write (API, Size, Function, "settextcolour",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETBKCOLOR:
			SCAN_DIAGNOSTIC (API,"New Record: SETBKCOLOR");
			changed = meta_dc_color (API,&Record,atts);
			if (SCAN (API)) wmf_write (API, Size, Function, "setbkcolor",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETBKMODE:
			SCAN_DIAGNOSTIC (API,"New Record: SETBKMODE");
			changed = meta_dc_set (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setbkmode",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SELECTOBJECT:
			SCAN_DIAGNOSTIC (API,"New Record: SELECTOBJECT");
			changed = meta_dc_select (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "selectobject",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (e) text & font
 */
		case META_TEXTOUT:
			SCAN_DIAGNOSTIC (API,"New Record: TEXTOUT");
			changed = meta_text (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "textout",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_EXTTEXTOUT:
			SCAN_DIAGNOSTIC (API,"New Record: EXTTEXTOUT");
			changed = meta_text (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "exttextout",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_CREATEFONTINDIRECT:
			SCAN_DIAGNOSTIC (API,"New Record: CREATEFONTINDIRECT");
			changed = meta_font_create (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "createfontindirect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETMAPPERFLAGS:
			SCAN_DIAGNOSTIC (API,"New Record: SETMAPPERFLAGS");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setmapperflags",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (f) palette
 */
		case META_CREATEPALETTE:
			SCAN_DIAGNOSTIC (API,"New Record: CREATEPALETTE");
			changed = meta_palette_create (API);
			if (SCAN (API)) wmf_write (API, Size, Function, "createpalette",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_REALIZEPALETTE:
			SCAN_DIAGNOSTIC (API,"New Record: REALIZEPALETTE");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "realizepalette",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SELECTPALETTE:
			SCAN_DIAGNOSTIC (API,"New Record: SELECTPALETTE");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "selectpalette",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_SETPALENTRIES:
			SCAN_DIAGNOSTIC (API,"New Record: SETPALENTRIES");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "setpalentries",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (g) create & delete; save & restore; ...
 */
		case META_SAVEDC:
			SCAN_DIAGNOSTIC (API,"New Record: SAVEDC");
			changed = meta_dc_save (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "savedc",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_RESTOREDC:
			SCAN_DIAGNOSTIC (API,"New Record: RESTOREDC");
			changed = meta_dc_restore (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "restoredc",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_CREATEPENINDIRECT:
			SCAN_DIAGNOSTIC (API,"New Record: CREATEPENINDIRECT");
			changed = meta_pen_create (API,&Record,atts);
			if (SCAN (API)) wmf_write (API, Size, Function, "createpenindirect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_CREATEBRUSHINDIRECT:
			SCAN_DIAGNOSTIC (API,"New Record: CREATEBRUSHINDIRECT");
			changed = meta_brush_create (API,&Record,atts);
			if (SCAN (API)) wmf_write (API, Size, Function, "createbrushindirect",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		case META_DELETEOBJECT:
			SCAN_DIAGNOSTIC (API,"New Record: DELETEOBJECT");
			changed = meta_delete (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "deleteobject",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

/* (h) ==== other ====
 */
		case META_ESCAPE:
			SCAN_DIAGNOSTIC (API,"New Record: ESCAPE");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "escape",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;

		default:
			SCAN_DIAGNOSTIC (API,"New Record: UNKNOWN");
			changed = meta_unused (API,&Record);
			if (SCAN (API)) wmf_write (API, Size, Function, "unknown",
					atts->atts, Record.parameter, Record.size * 2);
			if (changed && ((Record.size * 2 + 6) == atts->length))
			{	memcpy (atts->buffer + 6, Record.parameter, Record.size * 2);
			}
			SCAN_DIAGNOSTIC (API,"\n");
		break;
		}

		if (PLAY (API) && API->status.function)
		{	pos_current = WMF_TELL (API);
			if (pos_current < 0)
			{	WMF_ERROR (API,"API's tell() failed on input stream!");
				API->err = wmf_E_BadFile;
				break;
			}
			pos_current -= API->File->pos;
			pos_max = (FILE_SIZE(API) - API->File->pos)*2;
			if (API->status.function (API->status.context,(float) pos_current / (float) pos_max))
			{	API->err = wmf_E_UserExit;
			}
		}
		
	} while (API->err == wmf_E_None);

	if (Size == 0)
	{	WMF_DEBUG (API,"size was 0, giving up now silently...");
		if (API->err == wmf_E_EOF)
		{	WMF_DEBUG (API,"discarding an EOF error, however.");
			API->err = wmf_E_None;
		}
	}

	if (ERR (API)) /* Quit now ?? */
	{	WMF_DEBUG (API,"bailing...");
		return (API->err);
	}

	wmf_attr_free (API, &attrlist);

	if (SCAN (API)) wmf_write_end (API);

	while (P->dc_stack_length)
	{	if (P->dc)
		{	userdata.dc = P->dc;
			userdata.data = P->dc->userdata;

			if (FR->udata_free) FR->udata_free (API,&userdata);
			wmf_free (API,P->dc);
		}
		P->dc = dc_stack_pop (API);
	}

	if (P->dc)
	{	userdata.dc = P->dc;
		userdata.data = P->dc->userdata;

		if (PLAY (API) && FR->udata_free) FR->udata_free (API,&userdata);
		wmf_free (API,P->dc);
	}

	dc_stack_free (API);

	for (i = 0; i < NUM_OBJECTS(API); i++)
	{	if (objects[i].type == OBJ_BRUSH)
		{	if (objects[i].obj.brush.lbStyle == BS_DIBPATTERN)
			{	if (objects[i].obj.brush.bmp.data && FR->bmp_free)
				{	FR->bmp_free (API,&(objects[i].obj.brush.bmp));
				}
			}
		}
		else if (objects[i].type == OBJ_REGION)
		{	wmf_free (API,objects[i].obj.rgn.rects);
		}
		else if (objects[i].type == OBJ_FONT)
		{	wmf_free (API,objects[i].obj.font.lfFaceName);
		}
	}

	if (PLAY (API) && FR->device_end) FR->device_end (API);

	return (API->err);
}
