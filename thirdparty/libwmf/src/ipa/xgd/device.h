/* libwmf ("ipa/xgd/device.h"): library for wmf conversion
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


/* This is called by wmf_play() the *first* time the meta file is played
 */
static void wmf_gd_device_open (wmfAPI* API)
{	
#if !defined(HAVE_LIBPNG) || !defined(HAVE_LIBJPEG)
	wmf_gd_t* ddata = WMF_GD_GetData (API);
#endif

	WMF_DEBUG (API,"wmf_[gd_]device_open");

#ifndef HAVE_LIBPNG
	if (ddata->type == wmf_gd_png)
	{	WMF_ERROR (API,"Sorry! libwmf[gd] does not have PNG support!");
		API->err = wmf_E_DeviceError;
		return;
	}
#endif /* HAVE_LIBPNG */
#ifndef HAVE_LIBJPEG
	if (ddata->type == wmf_gd_jpeg)
	{	WMF_ERROR (API,"Sorry! libwmf[gd] does not have JPEG support!");
		API->err = wmf_E_DeviceError;
		return;
	}
#endif /* HAVE_LIBJPEG */
}

/* This is called by wmf_api_destroy()
 */
static void wmf_gd_device_close (wmfAPI* API)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	WMF_DEBUG (API,"wmf_[gd_]device_close");

	if (gd->brush.hatch) gdImageDestroy (gd->brush.hatch);
	if (gd->brush.image) gdImageDestroy (gd->brush.image);
	if (gd->pen.image) gdImageDestroy (gd->pen.image);
	if (gd->image) gdImageDestroy (gd->image);
	if (gd->flat_pixels) wmf_free (API,gd->flat_pixels);
}

/* This is called from the beginning of each play for initial page setup
 */
static void wmf_gd_device_begin (wmfAPI* API)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	WMF_DEBUG (API,"wmf_[gd_]device_begin");

	if ((ddata->width == 0) || (ddata->height == 0))
	{	WMF_ERROR (API,"Image has bad size!");
		API->err = wmf_E_Glitch;
		return;
	}

	gd->image = gdImageCreateTrueColor ((int) ddata->width,(int) ddata->height);

	if (gd->image == 0)
	{	WMF_ERROR (API,"Unable to create image!");
		API->err = wmf_E_DeviceError;
		return;
	}

	gd->brush.hatch = 0;
	gd->brush.image = 0;
	gd->pen.image = 0;

	gd->white = gdImageColorAllocate (gd->image,255,255,255); /* background color */

	gdImageFilledRectangle (gd->image,0,0,(int) ddata->width - 1,(int) ddata->height - 1,gd->white);
}

/* This is called from the end of each play for page termination
 */
static void wmf_gd_device_end (wmfAPI* API)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	gdSink sink;

	WMF_DEBUG (API,"wmf_[gd_]device_end");

	switch (ddata->type)
	{
#ifdef HAVE_LIBPNG
	case (wmf_gd_png):
		if (ddata->flags & WMF_GD_OUTPUT_FILE)
		{	if (ddata->file == 0)
			{	WMF_ERROR (API,"Attempt to write to null stream!");
				API->err = wmf_E_BadFile;
				break;
			}
			gdImagePng (gd->image,ddata->file);
		}
		else if (ddata->flags & WMF_GD_OUTPUT_MEMORY)
		{	sink.context = (void*) API;
			sink.sink = gd_sink;

			gd->buffer = 0;

			gdImagePngToSink (gd->image,&sink);

			if (ERR (API))
			{	WMF_ERROR (API,"PNG Write failed!");
				break;
			}

			ddata->memory = gd->buffer;
		}
	break;
#endif
#ifdef HAVE_LIBJPEG
	case (wmf_gd_jpeg):
		if (ddata->flags & WMF_GD_OUTPUT_FILE)
		{	if (ddata->file == 0)
			{	WMF_ERROR (API,"Attempt to write to null stream!");
				API->err = wmf_E_BadFile;
				break;
			}
			gdImageJpeg (gd->image,ddata->file,-1); /* Default quality. */
		}
		else if (ddata->flags & WMF_GD_OUTPUT_MEMORY)
		{	WMF_ERROR (API,"JPEG Output to memory not supported!"); /* Or is it ?? */
			API->err = wmf_E_Glitch;
		}
	break;
#endif
	case (wmf_gd_image): /* no output, but don't destroy image (see below) */
	break;

	default:
		WMF_ERROR (API,"Output format is not supported!");
		API->err = wmf_E_Glitch;
	break;
	}

	if (gd->brush.hatch) gdImageDestroy (gd->brush.hatch);
	if (gd->brush.image) gdImageDestroy (gd->brush.image);
	if (gd->pen.image) gdImageDestroy (gd->pen.image);

	if (ddata->type == wmf_gd_image)
	{	wmf_gd_clip_reset (gd->image); /* Remove any clipping rectangles */
		ddata->gd_image = (void*) gd->image;
	}
	else
	{	gdImageDestroy (gd->image);
	}

	gd->brush.hatch = 0;
	gd->brush.image = 0;
	gd->pen.image = 0;
	gd->image = 0;
}

static int gd_sink (void* context,const char* buffer,int length)
{	wmfAPI* API = (wmfAPI*) context;

	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	int i;

	char* more = 0;

	if ((ddata->flags & WMF_GD_OWN_BUFFER) && ddata->sink.function)
	{	return (ddata->sink.function (ddata->sink.context,(char*) buffer,length));
	}

	if (ERR (API)) return (0);

	if (length <= 0) return (length);

	if (gd->buffer == 0)
	{	gd->buffer = (char*) wmf_malloc (API,4096);

		if (ERR (API))
		{	gd->buffer = 0;
			return (0);
		}

		gd->max_length = 4096;
		gd->length = 0;
	}

	while ((gd->length + (long) length) > gd->max_length)
	{	more = (char*) wmf_realloc (API,gd->buffer,gd->max_length + 4096);

		if (ERR (API)) break;

		gd->buffer = more;
		gd->max_length += 4096;
	}

	if (ERR (API)) return (0);

	gd->ptr = gd->buffer + gd->length;

	for (i = 0; i < length; i++)
	{	(*(gd->ptr)) = (char) buffer[i];
		gd->ptr++;
		gd->length++;
	}

	return (length);
}

static gdPoint gd_translate (wmfAPI* API,wmfD_Coord d_pt)
{	return (gd_translate_ft64 (API,d_pt,0));
}

static gdPoint gd_translate_ft64 (wmfAPI* API,wmfD_Coord d_pt,FT_Vector* pen)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gdPoint g_pt;

	double x;
	double y;

	x = ((double) d_pt.x - (double) ddata->bbox.TL.x);
	x /= ((double) ddata->bbox.BR.x - (double) ddata->bbox.TL.x);
	x *= (double) ddata->width;

	y = ((double) d_pt.y - (double) ddata->bbox.TL.y);
	y /= ((double) ddata->bbox.BR.y - (double) ddata->bbox.TL.y);
	y *= (double) ddata->height;

	g_pt.x = (int) floor (x);
	g_pt.y = (int) floor (y);

	if (pen)
	{	pen->x = floor ((x - floor (x)) * 64);
		pen->y = floor ((y - floor (y)) * 64);
	}

	return (g_pt);
}

static float gd_width (wmfAPI* API,float wmf_width)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	double width;

	width = (double) wmf_width * (double) ddata->width;
	width /= ((double) ddata->bbox.BR.x - (double) ddata->bbox.TL.x);

	return ((float) width);
}

static float gd_height (wmfAPI* API,float wmf_height)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	double height;

	height = (double) wmf_height * (double) ddata->height;
	height /= ((double) ddata->bbox.BR.y - (double) ddata->bbox.TL.y);

	return ((float) height);
}
