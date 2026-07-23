/* libwmf (ipa/gd.c): library for wmf conversion
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


#ifdef HAVE_CONFIG_H
#include "wmfconfig.h"
#endif /* HAVE_CONFIG_H */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "wmfdefs.h"

#ifdef HAVE_GD

#include "libwmf/gd.h"

static void wmf_gd_device_open (wmfAPI*);
static void wmf_gd_device_close (wmfAPI*);
static void wmf_gd_device_begin (wmfAPI*);
static void wmf_gd_device_end (wmfAPI*);
static void wmf_gd_flood_interior (wmfAPI*,wmfFlood_t*);
static void wmf_gd_flood_exterior (wmfAPI*,wmfFlood_t*);
static void wmf_gd_draw_pixel (wmfAPI*,wmfDrawPixel_t*);
static void wmf_gd_draw_pie (wmfAPI*,wmfDrawArc_t*);
static void wmf_gd_draw_chord (wmfAPI*,wmfDrawArc_t*);
static void wmf_gd_draw_arc (wmfAPI*,wmfDrawArc_t*);
static void wmf_gd_draw_ellipse (wmfAPI*,wmfDrawArc_t*);
static void wmf_gd_draw_line (wmfAPI*,wmfDrawLine_t*);
static void wmf_gd_poly_line (wmfAPI*,wmfPolyLine_t*);
static void wmf_gd_draw_polygon (wmfAPI*,wmfPolyLine_t*);
static void wmf_gd_draw_rectangle (wmfAPI*,wmfDrawRectangle_t*);
static void wmf_gd_rop_draw (wmfAPI*,wmfROP_Draw_t*);
static void wmf_gd_bmp_draw (wmfAPI*,wmfBMP_Draw_t*);
static void wmf_gd_bmp_read (wmfAPI*,wmfBMP_Read_t*);
static void wmf_gd_bmp_free (wmfAPI*,wmfBMP*);
static void wmf_gd_draw_text (wmfAPI*,wmfDrawText_t*);
static void wmf_gd_udata_init (wmfAPI*,wmfUserData_t*);
static void wmf_gd_udata_copy (wmfAPI*,wmfUserData_t*);
static void wmf_gd_udata_set (wmfAPI*,wmfUserData_t*);
static void wmf_gd_udata_free (wmfAPI*,wmfUserData_t*);
static void wmf_gd_region_frame (wmfAPI*,wmfPolyRectangle_t*);
static void wmf_gd_region_paint (wmfAPI*,wmfPolyRectangle_t*);
static void wmf_gd_region_clip (wmfAPI*,wmfPolyRectangle_t*);

#include "ipa/xgd.h"
#include "ipa/xgd/bmp.h"
#include "ipa/xgd/device.h"
#include "ipa/xgd/draw.h"
#include "ipa/xgd/font.h"
#include "ipa/xgd/region.h"

#endif /* HAVE_GD */

void wmf_gd_function (wmfAPI* API)
{
#ifdef HAVE_GD
	wmf_gd_t* ddata = 0;

	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	gd_t* gd = 0;

	if ((API->flags & API_STANDARD_INTERFACE) == 0)
	{	WMF_ERROR (API,"Can't use this device layer with 'lite' interface!");
		API->err = wmf_E_DeviceError;
		return;
	}

/* IPA function reference links
 */
	FR->device_open    = wmf_gd_device_open;
	FR->device_close   = wmf_gd_device_close;
	FR->device_begin   = wmf_gd_device_begin;
	FR->device_end     = wmf_gd_device_end;
	FR->flood_interior = wmf_gd_flood_interior;
	FR->flood_exterior = wmf_gd_flood_exterior;
	FR->draw_pixel     = wmf_gd_draw_pixel;
	FR->draw_pie       = wmf_gd_draw_pie;
	FR->draw_chord     = wmf_gd_draw_chord;
	FR->draw_arc       = wmf_gd_draw_arc;
	FR->draw_ellipse   = wmf_gd_draw_ellipse;
	FR->draw_line      = wmf_gd_draw_line;
	FR->poly_line      = wmf_gd_poly_line;
	FR->draw_polygon   = wmf_gd_draw_polygon;
	FR->draw_rectangle = wmf_gd_draw_rectangle;
	FR->rop_draw       = wmf_gd_rop_draw;
	FR->bmp_draw       = wmf_gd_bmp_draw;
	FR->bmp_read       = wmf_gd_bmp_read;
	FR->bmp_free       = wmf_gd_bmp_free;
	FR->draw_text      = wmf_gd_draw_text;
	FR->udata_init     = wmf_gd_udata_init;
	FR->udata_copy     = wmf_gd_udata_copy;
	FR->udata_set      = wmf_gd_udata_set;
	FR->udata_free     = wmf_gd_udata_free;
	FR->region_frame   = wmf_gd_region_frame;
	FR->region_paint   = wmf_gd_region_paint;
	FR->region_clip    = wmf_gd_region_clip;

/* Allocate device data structure
 */
	ddata = (wmf_gd_t*) wmf_malloc (API,sizeof (wmf_gd_t));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	API->device_data = (void*) ddata;

/* Device data defaults
 */
	ddata->type = wmf_gd_png;

	ddata->gd_data = (void*) wmf_malloc (API,sizeof (gd_t));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	gd = (gd_t*) ddata->gd_data;

	gd->image = 0;
	gd->flat_pixels = 0;

	ddata->file   = 0;
	ddata->memory = 0;

	ddata->sink.context  = 0;
	ddata->sink.function = 0;

	ddata->gd_image = 0;

	ddata->width  = 0;
	ddata->height = 0;

	ddata->bbox.TL.x = 0;
	ddata->bbox.TL.y = 0;
	ddata->bbox.BR.x = 0;
	ddata->bbox.BR.y = 0;

	ddata->flags = 0;
#ifdef HAVE_LIBPNG
	ddata->flags |= WMF_GD_SUPPORTS_PNG;
#endif
#ifdef HAVE_LIBJPEG
	ddata->flags |= WMF_GD_SUPPORTS_JPEG;
#endif
#else /* HAVE_GD */
	API->device_data = 0;

	API->err = wmf_E_DeviceError;
#endif /* HAVE_GD */
}
#ifdef HAVE_GD
static void wmf_gd_udata_init (wmfAPI* API,wmfUserData_t* userdata)
{	(void)userdata;
	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]udata_init");

}

static void wmf_gd_udata_copy (wmfAPI* API,wmfUserData_t* userdata)
{	(void)userdata;
	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]udata_copy");

}

static void wmf_gd_udata_set (wmfAPI* API,wmfUserData_t* userdata)
{	(void)userdata;
	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]udata_set");

}

static void wmf_gd_udata_free (wmfAPI* API,wmfUserData_t* userdata)
{	(void)userdata;
	/* wmf_gd_t* ddata = WMF_GD_GetData (API); */

	/* gd_t* gd = (gd_t*) ddata->gd_data; */

	WMF_DEBUG (API,"wmf_[gd_]udata_free");

}

static int setbrushstyle (wmfAPI* API,wmfDC* dc)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	wmfBrush* brush = 0;

	wmfBMP* bmp = 0;

	wmfRGB* rgb = 0;

	wmfRGB pixel;

	int i;
	int j;

	int fg;
	int bg;

	int brushstyle;

	unsigned int x;
	unsigned int y;

	unsigned char* bits = 0;

	brush = WMF_DC_BRUSH (dc);

	rgb = WMF_BRUSH_COLOR (brush);
	brushstyle = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	switch (WMF_BRUSH_STYLE (brush))
	{
	case BS_NULL:
		WMF_ERROR (API,"Attempt to set null fill-style!");
		API->err = wmf_E_Glitch;
	break;

	case BS_HATCHED:
		switch (WMF_BRUSH_HATCH (brush))
		{
		case HS_HORIZONTAL:
			bits = HS_HORIZONTAL_bits;
		break;

		case HS_VERTICAL:
			bits = HS_VERTICAL_bits;
		break;

		case HS_FDIAGONAL:
			bits = HS_FDIAGONAL_bits;
		break;

		case HS_BDIAGONAL:
			bits = HS_BDIAGONAL_bits;
		break;

		case HS_CROSS:
			bits = HS_CROSS_bits;
		break;

		case HS_DIAGCROSS:
			bits = HS_DIAGCROSS_bits;
		break;

		default:
			if (API->flags & WMF_OPT_IGNORE_NONFATAL)
			{	WMF_DEBUG (API,"Unsupported brush/hatch style!");
				bits = HS_DIAGCROSS_bits;
			}
			else
			{	WMF_ERROR (API,"Unsupported brush/hatch style!");
				API->err = wmf_E_Glitch;
				bits = 0;
			}
		break;
		}
		if (bits == 0) break;

		if (gd->brush.hatch == 0) gd->brush.hatch = gdImageCreateTrueColor (8,8);

		if (gd->brush.hatch)
		{	rgb = WMF_DC_BACKGROUND (dc);
			bg = gdImageColorResolve (gd->brush.hatch,rgb->r,rgb->g,rgb->b);

			if (!WMF_DC_OPAQUE (dc))
			{	gdImageColorTransparent (gd->brush.hatch,bg);
			}

			rgb = WMF_BRUSH_COLOR (brush);
			fg = gdImageColorResolve (gd->brush.hatch,rgb->r,rgb->g,rgb->b);

			for (j = 0; j < 8; j++)
			{	for (i = 0; i < 8; i++)
				{	if (bits[j] & (1<<(7-i)))
					{	gdImageSetPixel (gd->brush.hatch,i,j,fg);
					}
					else
					{	gdImageSetPixel (gd->brush.hatch,i,j,bg);
					}
				}
			}

			gdImageSetTile (gd->image,gd->brush.hatch);

			brushstyle = gdTiled;
		}
	break;

	case BS_DIBPATTERN:
		bmp = WMF_BRUSH_BITMAP (brush);

		if (bmp->data == 0)
		{	if (API->flags & WMF_OPT_IGNORE_NONFATAL)
			{	WMF_DEBUG (API,"Attempt to fill with non-existent pattern!");
				break;
			}
			else
			{	WMF_ERROR (API,"Attempt to fill with non-existent pattern!");
				API->err = wmf_E_Glitch;
				break;
			}
		}
		if (gd->brush.image)
		{	if ((gd->brush.width  != (int) bmp->width )
			 || (gd->brush.height != (int) bmp->height))
			{	gdImageDestroy (gd->brush.image);
				gd->brush.image = 0;
			}
		}
		if (gd->brush.image == 0)
		{	gd->brush.width  = (int) bmp->width;
			gd->brush.height = (int) bmp->height;

			gd->brush.image = gdImageCreateTrueColor (gd->brush.width,gd->brush.height);
		}
		if (gd->brush.image)
		{	for (y = 0; y < (unsigned int) gd->brush.height; y++)
			{	for (x = 0; x < (unsigned int) gd->brush.width; x++)
				{	wmf_ipa_bmp_color (API,bmp,&pixel,x,y);

					fg = gdImageColorResolve (gd->brush.image,pixel.r,pixel.g,pixel.b);

					gdImageSetPixel (gd->brush.image,(int) x,(int) y,fg);
				}
			}

			gdImageSetTile (gd->image,gd->brush.image);

			brushstyle = gdTiled;
		}
	break;

	default:
		if (API->flags & WMF_OPT_IGNORE_NONFATAL)
		{	WMF_DEBUG (API,"Unsupported brush style!");
			/* no break here */
		}
		else
		{	WMF_ERROR (API,"Unsupported brush style!");
			API->err = wmf_E_Glitch;
			break;
		}
	case BS_SOLID:
	break;
	}

	return (brushstyle);
}

static int setlinestyle (wmfAPI* API,wmfDC* dc)
{	wmf_gd_t* ddata = WMF_GD_GetData (API);

	gd_t* gd = (gd_t*) ddata->gd_data;

	wmfPen* pen = 0;

	wmfRGB* rgb = 0;

	int pen_width;
	int pen_height;

	int fg;
	int bg;

	int linestyle;

	int i;

	int style[14];

	pen = WMF_DC_PEN (dc);

	/* Color
	 */
	rgb = WMF_PEN_COLOR (pen);
	fg = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	rgb = WMF_DC_BACKGROUND (dc);
	bg = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

	linestyle = fg;

	/* Size
	 */
	pen_width  = (int) ceil ((double) gd_width  (API,WMF_PEN_WIDTH  (pen)));
	pen_height = (int) ceil ((double) gd_height (API,WMF_PEN_HEIGHT (pen)));

	if (pen_width  < 1) pen_width  = 1;
	if (pen_height < 1) pen_height = 1;

	if ((pen_width == 1) && (pen_height == 1))
	{	/* Ordinary line */
	}
	else
	{	if (gd->pen.image == 0)
		{	/* Need to create a brush */
			gd->pen.image = gdImageCreateTrueColor (pen_width,pen_height);
		}
		else if ((pen_width == gd->pen.width) && (pen_height == gd->pen.height))
		{	/* Already have brush of correct size */
		}
		else
		{	/* Need to free current brush & create new one */
			gdImageDestroy (gd->pen.image);

			gd->pen.image = gdImageCreateTrueColor (pen_width,pen_height);
		}
		if (gd->pen.image)
		{	gd->pen.width  = pen_width;
			gd->pen.height = pen_height;

			rgb = WMF_PEN_COLOR (pen);
			fg = gdImageColorResolve (gd->pen.image,rgb->r,rgb->g,rgb->b);

			rgb = WMF_DC_BACKGROUND (dc);
			bg = gdImageColorResolve (gd->image,rgb->r,rgb->g,rgb->b);

			if (bg != fg) gdImageColorTransparent (gd->pen.image,bg);

			gdImageFilledRectangle (gd->pen.image,0,0,gd->pen.width,gd->pen.height,bg);

			/* `Square' nib
			 */
			gdImageFilledRectangle (gd->pen.image,0,0,gd->pen.width,gd->pen.height,fg);

			gdImageSetBrush (gd->image,gd->pen.image);

			linestyle = gdBrushed;
		}
	}

	switch (WMF_PEN_STYLE (pen))
	{
	case PS_DASH:
		for (i =  0; i <  5; i++) style[i] = fg;
		for (i =  5; i <  8; i++) style[i] = bg;

		gdImageSetStyle (gd->image,style, 8);

		if (linestyle == gdBrushed)
		{	linestyle = gdStyledBrushed;
		}
		else
		{	linestyle = gdStyled;
		}
	break;

	case PS_DOT:
		for (i =  0; i <  2; i++) style[i] = fg;
		for (i =  2; i <  4; i++) style[i] = bg;

		gdImageSetStyle (gd->image,style, 4);

		if (linestyle == gdBrushed)
		{	linestyle = gdStyledBrushed;
		}
		else
		{	linestyle = gdStyled;
		}
	break;

	case PS_DASHDOT:
		for (i =  0; i <  4; i++) style[i] = fg;
		for (i =  4; i <  7; i++) style[i] = bg;
		for (i =  7; i <  9; i++) style[i] = fg;
		for (i =  9; i < 12; i++) style[i] = bg;

		gdImageSetStyle (gd->image,style,12);

		if (linestyle == gdBrushed)
		{	linestyle = gdStyledBrushed;
		}
		else
		{	linestyle = gdStyled;
		}
	break;

	case PS_DASHDOTDOT:
		for (i =  0; i <  4; i++) style[i] = fg;
		for (i =  4; i <  6; i++) style[i] = bg;
		for (i =  6; i <  8; i++) style[i] = fg;
		for (i =  8; i < 10; i++) style[i] = bg;
		for (i = 10; i < 12; i++) style[i] = fg;
		for (i = 12; i < 14; i++) style[i] = bg;

		gdImageSetStyle (gd->image,style,14);

		if (linestyle == gdBrushed)
		{	linestyle = gdStyledBrushed;
		}
		else
		{	linestyle = gdStyled;
		}
	break;

	case PS_INSIDEFRAME: /* There is nothing to do in this case... */
	case PS_SOLID:
	default:
	break;
	}

	return (linestyle);
}
#endif /* HAVE_GD */

int * wmf_gd_get_image_pixels (wmfAPI* API)
{
	int * pixels = 0;
#ifdef HAVE_GD
	wmf_gd_t* ddata = WMF_GD_GetData (API);
	gdImagePtr img = (gdImagePtr) ddata->gd_image;
	if (img)
	{
#ifdef HAVE_SYS_GD
		gd_t* gd = (gd_t*) ddata->gd_data;

		int img_w = gdImageSX (img);
		int img_h = gdImageSY (img);
		size_t width;
		size_t height;
		size_t count;
		size_t row;
		size_t col;

		if (img_w <= 0 || img_h <= 0) return (0);

		width = (size_t) img_w;
		height = (size_t) img_h;

		if (width > SIZE_MAX / height) return (0);
		count = width * height;
		if (count > SIZE_MAX / sizeof (int)) return (0);

		if (gd->flat_pixels)
		{	wmf_free (API,gd->flat_pixels);
			gd->flat_pixels = 0;
		}

		gd->flat_pixels = (int*) wmf_malloc (API,count * sizeof (int));
		if (gd->flat_pixels == 0) return (0);

		for (row = 0; row < height; row++)
		{	for (col = 0; col < width; col++)
			{	gd->flat_pixels[(row * width) + col] = gdImageTrueColorPixel (img,col,row);
			}
		}

		pixels = gd->flat_pixels;
#else
		pixels = img->_tpixels;
#endif
	}
#endif /* HAVE_GD */
	return (pixels);
}

int * wmf_gd_image_pixels (void * gd_image)
{
	int * pixels = 0;
#ifdef HAVE_GD
	gdImagePtr img = (gdImagePtr) gd_image;
	if (img)
	{
#ifdef HAVE_SYS_GD
		/* Deprecated. The public libgd API exposes no contiguous backing
		 * store to borrow, so this returns NULL when libwmf is built
		 * against system libgd. Use wmf_gd_get_image_pixels() instead.
		 */
#else
		pixels = img->_tpixels;
#endif
	}
#endif /* HAVE_GD */
	return (pixels);
}
