/* libwmf ("ipa/gd.h"): library for wmf conversion
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


#ifndef WMFIPA_GD_H
#define WMFIPA_GD_H

/* I suppose I shouldn't call this file gd.h, but as long <gd.h> is included
 * before any other "gd*.h" there's no problem...
 */
#include <gd.h>

extern int gdImageBoundsSafe (gdImage*,int,int);

#ifdef HAVE_SYS_GD
static void wmf_gd_clip_reset (gdImage* image)
{	gdImageSetClip (image,0,0,gdImageSX (image) - 1,gdImageSY (image) - 1);
}
#else
extern void gdClipSetReset (gdImage*);
extern void gdClipSetAdd (gdImage*,gdClipRectangle*);
#define wmf_gd_clip_reset gdClipSetReset
#endif

typedef enum
{	gd_arc_ellipse = 0,
	gd_arc_open,
	gd_arc_pie,
	gd_arc_chord
} gd_arc_t;

typedef struct _gd_t gd_t;

/* extern char* gdft_draw_bitmap (gdImagePtr,int,FT_Bitmap,int,int); */

static void gd_draw_arc (wmfAPI*,wmfDrawArc_t*,gd_arc_t);

static int gd_sink (void*,const char*,int);

static gdPoint gd_translate (wmfAPI*,wmfD_Coord);
static gdPoint gd_translate_ft64 (wmfAPI*,wmfD_Coord,FT_Vector*);

static float gd_width (wmfAPI*,float);
static float gd_height (wmfAPI*,float);

static int setbrushstyle (wmfAPI*,wmfDC*);
static int setlinestyle (wmfAPI*,wmfDC*);

struct _gd_t
{	gdImagePtr image;

	int* flat_pixels; /* sys-gd flatten cache for wmf_gd_get_image_pixels */

	int white;

	/* Pen data */
	struct
	{	gdImagePtr image;

		int width;
		int height;
	} pen;

	struct
	{	gdImagePtr hatch; /* 8x8 only */
		gdImagePtr image;

		int width;
		int height;
	} brush;

	/* Sink data */
	long max_length;
	long length;

	char* buffer;
	char* ptr;
};

static unsigned char HS_BDIAGONAL_bits[] = {
   0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

static unsigned char HS_CROSS_bits[] = {
   0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08};

static unsigned char HS_DIAGCROSS_bits[] = {
   0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};

static unsigned char HS_FDIAGONAL_bits[] = {
   0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

static unsigned char HS_HORIZONTAL_bits[] = {
   0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};

static unsigned char HS_VERTICAL_bits[] = {
   0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

/* font.h
 */
static void gd_draw_ftbitmap (wmfAPI*,FT_Bitmap*,gdPoint,wmfRGB*,wmfRGB*);

#endif /* ! WMFIPA_GD_H */
