/* libwmf ("ipa/ipa.h"): library for wmf conversion
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


#ifndef WMFIPA_IPA_H
#define WMFIPA_IPA_H

/* bmp.h
 */
typedef struct _ipa_b64_t   ipa_b64_t;

typedef struct _BMPSource   BMPSource;
typedef struct _BMPData     BMPData;

typedef struct _PointInfo   PointInfo;
typedef struct _BMPInfo     BMPInfo;

#ifdef HAVE_GD
#include <gd.h> /* Use GD for BMP -> PNG conversion */

static int      ipa_b64_sink (void*,const char*,int);
static void     ipa_b64_flush (void*);
static gdImage* ipa_bmp_gd (wmfAPI*,wmfBMP_Draw_t*);
#else /* HAVE_GD */
#ifdef HAVE_LIBPNG
#include <png.h>
#endif /* HAVE_LIBPNG */
static void ldr_bmp_png (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,FILE* out);
#endif /* HAVE_GD */

static size_t         ReadBlob (BMPSource*,size_t,unsigned char*);
static int            ReadBlobByte (BMPSource*);
static unsigned short ReadBlobLSBShort (BMPSource*);
static unsigned int   ReadBlobLSBLong (BMPSource*);
static signed int     ReadBlobLSBSignedLong (BMPSource*);
static long           TellBlob (BMPSource*);
static int            DecodeImage (wmfBMP*,BMPSource*,unsigned int,unsigned char*,unsigned int number_pixels,unsigned int bytes_per_line);
static void           ReadBMPImage (wmfAPI*,wmfBMP*,BMPSource*);
static int            ExtractColor (wmfAPI*,wmfBMP*,wmfRGB*,unsigned int,unsigned int);
static void           SetColor (wmfAPI*,wmfBMP*,wmfRGB*,unsigned char,unsigned int,unsigned int);

#define IPA_B64_BUFLEN 57 /* must be multiple of three... */
#define IPA_B64_LEQUIV 78 /* (IPA_B64_BUFLEN * 4) / 3 + 2 */

struct _ipa_b64_t
{	wmfAPI* API;

	wmfStream* out;

	char buffer[IPA_B64_BUFLEN];

	int length;
};

struct _BMPSource
{	unsigned char* begin;
	unsigned char* end;

	unsigned char* ptr;
};

struct _BMPData
{	unsigned int NColors;

	wmfRGB* rgb;

	unsigned char* image;

	unsigned short bits_per_pixel;
	unsigned int   bytes_per_line;

	unsigned short masked;
	unsigned short flipped;
};

struct _PointInfo
{	double x;
	double y;
	double z;
};

struct _BMPInfo
{	unsigned long file_size;
	unsigned long ba_offset;
	unsigned long offset_bits;
	unsigned long size;

	long width;
	long height;

	unsigned short planes;
	unsigned short bits_per_pixel;

	unsigned long compression;
	unsigned long image_size;
	unsigned long x_pixels;
	unsigned long y_pixels;
	unsigned long number_colors;
	unsigned long colors_important;

	unsigned short red_mask;
	unsigned short green_mask;
	unsigned short blue_mask;
	unsigned short alpha_mask;

	long colorspace;

	PointInfo red_primary;
	PointInfo green_primary;
	PointInfo blue_primary;
	PointInfo gamma_scale;
};

/* ipa.c
 */
typedef struct _ipa_page_info ipa_page_info;

struct _ipa_page_info
{	wmf_page_t type;

	char* format;

	unsigned int width;
	unsigned int height;
};

#endif /* ! WMFIPA_IPA_H */
