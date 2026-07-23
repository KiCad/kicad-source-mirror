/* libwmf ("ipa/ipa/bmp.h"): library for wmf conversion
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


#ifndef HAVE_GD
/* png stuff below adapted from stuff sent to me [fjf] by Lennard D. Rosenthal;
 * probably used in ImageMagick and may not be covered, therefore, by the LGPL - (fjf)
 * 
 * utility routine for saving a pixbuf to a png file.
 * This was adapted from Iain Holmes' code in gnome-iconedit, and probably
 * should be in a utility library, possibly in gdk-pixbuf itself.
 */
static void ldr_bmp_png (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,FILE* out)
{
#ifdef HAVE_LIBPNG
	wmfRGB rgb;

	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	png_text text[2];

	int i;
	int j;
	int x;
	int y;
	int width  = (int) bmp_draw->crop.w;
	int height = (int) bmp_draw->crop.h;
	int depth = 8; /* ?? bit depth (bits per sample) */
	int opacity;

	char* ptr = 0;
	char* buffer = (char*) wmf_malloc (API,4 * width);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,0,0,0);
	if (png_ptr == 0) 
	{	WMF_DEBUG (API,"Failed to write bitmap as PNG! (png_create_write_struct failed)");
		wmf_free (API,buffer);
		return;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == 0)
	{	WMF_DEBUG (API,"Failed to write bitmap as PNG! (png_create_info_struct failed)");
		png_destroy_write_struct (&png_ptr,0);
		wmf_free (API,buffer);
		return;
	}

	if (setjmp (png_ptr->jmpbuf))
	{	WMF_DEBUG (API,"Failed to write bitmap as PNG! (setjmp failed)");
		png_destroy_write_struct (&png_ptr,&info_ptr);
		wmf_free (API,buffer);
		return;
	}

	png_init_io (png_ptr,out);

	png_set_IHDR (png_ptr,info_ptr,width,height,depth,
		      PNG_COLOR_TYPE_RGB_ALPHA,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

	/* Some text to go with the png image */
	text[0].key = "Title";
	text[0].text = "A converted bitmap";
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key = "Software";
	text[1].text = "libwmf2";
	text[1].compression = PNG_TEXT_COMPRESSION_NONE;

	png_set_text (png_ptr,info_ptr,text,2);

	/* Write header data
	 */
	png_write_info (png_ptr,info_ptr);

	/* pump the raster data into libpng, one scan line at a time
	 */	
	x = (int) bmp_draw->crop.x;
	y = (int) bmp_draw->crop.y;

	for (j = 0; j < height; j++)
	{	ptr = buffer;
		for (i = 0; i < width; i++)
		{	opacity = wmf_ipa_bmp_color (API,&(bmp_draw->bmp),&rgb,x+i,y+j);
			*ptr++ = (char) rgb.r;
			*ptr++ = (char) rgb.g;
			*ptr++ = (char) rgb.b;
			*ptr++ = (char) ((unsigned char) (opacity & 0xff));
		}
		png_write_row (png_ptr,(png_bytep) buffer);		
	}
	
	png_write_end (png_ptr,info_ptr);
	png_destroy_write_struct (&png_ptr,&info_ptr);

	wmf_free (API,buffer);
#else /* HAVE_LIBPNG */
	WMF_ERROR (API,"Glitch? No PNG support!");
	API->err = wmf_E_Glitch;
#endif /* HAVE_LIBPNG */
	return;
}
#endif /* ! HAVE_GD */
#ifdef HAVE_GD
static int ipa_b64_sink (void* context,const char* buffer,int length)
{	ipa_b64_t* b64 = (ipa_b64_t*) context;

	int i = 0;

	while (i < length)
	{	while (i < length && b64->length < IPA_B64_BUFLEN)
			b64->buffer[b64->length++] = buffer[i++];

		if (b64->length == IPA_B64_BUFLEN)
			ipa_b64_flush (context);
		else
			break; /* input consumed */
	}

	return (i);
}
#endif /* HAVE_GD */
#ifdef HAVE_GD
static void ipa_b64_flush (void* context)
{	ipa_b64_t* b64 = (ipa_b64_t*) context;

	static char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char buffer[IPA_B64_LEQUIV];

	unsigned long ulc1;
	unsigned long ulc2;
	unsigned long ulc3;
	unsigned long ulc;

	int triplets = (b64->length + 2) / 3;
	int triplen;
	int i;

	char* ptr = b64->buffer;
	char* btr = buffer;

	if (b64->length == 0) return;

	triplen = triplets * 3;
	for (i = b64->length; i < triplen; i++) b64->buffer[i] = 0;

	(*btr) = '\n';
	btr++;

	for (i = 0; i < triplets; i++)
	{	ulc1 = (unsigned long) ((unsigned char) (*ptr));
		ptr++;
		ulc2 = (unsigned long) ((unsigned char) (*ptr));
		ptr++;
		ulc3 = (unsigned long) ((unsigned char) (*ptr));
		ptr++;

		ulc = (ulc1 << 16) | (ulc2 << 8) | ulc3;

		ulc1 = (ulc & (0x3f << 18)) >> 18;
		ulc2 = (ulc & (0x3f << 12)) >> 12;
		ulc3 = (ulc & (0x3f <<  6)) >>  6;

		ulc &= 0x3f;

		(*btr) = B64[ulc1];
		btr++;
		(*btr) = B64[ulc2];
		btr++;
		(*btr) = B64[ulc3];
		btr++;
		(*btr) = B64[ulc ];
		btr++;
	}

	if ((triplen - b64->length) > 1) (*(btr-2)) = '=';
	if ((triplen - b64->length) > 0) (*(btr-1)) = '=';

	(*btr) = '\0';

	wmf_stream_printf (b64->API,b64->out,buffer);

	b64->length = 0;
}
#endif /* HAVE_GD */

void wmf_ipa_bmp_b64 (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,wmfStream* out)
{
#ifdef HAVE_GD
	gdImage* image = 0;

	gdSink sink;

	ipa_b64_t b64;

	WMF_DEBUG (API,"~~~~~~~~wmf_ipa_bmp_b64");
#ifndef HAVE_LIBPNG
	WMF_DEBUG (API,"No support for PNG, sorry!");
	API->err = wmf_E_DeviceError;
	return;
#endif /* HAVE_LIBPNG */
	image = ipa_bmp_gd (API,bmp_draw);

	if (image == 0) return;

	b64.API = API;
	b64.out = out;

	b64.length = 0;

	sink.context = (void*) (&b64);
	sink.sink = ipa_b64_sink;

	gdImagePngToSink (image,&sink);
	gdImageDestroy (image);

	ipa_b64_flush (sink.context);
#endif /* HAVE_GD */
}

void wmf_ipa_bmp_png (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,char* name)
{	FILE* file = 0;
#ifdef HAVE_GD
	gdImage* image = 0;
#endif /* HAVE_GD */
	WMF_DEBUG (API,"~~~~~~~~wmf_ipa_bmp_png");
#ifndef HAVE_LIBPNG
	WMF_DEBUG (API,"No support for PNG, sorry!");
	API->err = wmf_E_DeviceError;
	return;
#endif /* HAVE_LIBPNG */
	file = fopen (name,"wb");

	if (file == 0)
	{	WMF_ERROR (API,"Failed to open file to write GD image!");
		return;
	}
#ifdef HAVE_GD
	image = ipa_bmp_gd (API,bmp_draw);

	if (image)
	{	gdImagePng (image,file);
		gdImageDestroy (image);
	}
#else /* HAVE_GD */
	ldr_bmp_png (API,bmp_draw,file);	
#endif /* HAVE_GD */

	fclose (file);
}

void wmf_ipa_bmp_jpg (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,char* name)
{
#ifdef HAVE_GD
	FILE* file = 0;

	gdImage* image = 0;

	WMF_DEBUG (API,"~~~~~~~~wmf_ipa_bmp_jpg");
#ifndef HAVE_LIBJPEG
	WMF_DEBUG (API,"No support for JPEG, sorry!");
	API->err = wmf_E_DeviceError;
	return;
#endif /* HAVE_LIBJPEG */
	file = fopen (name,"wb");

	if (file == 0)
	{	WMF_ERROR (API,"Failed to open file to write GD image!");
		return;
	}

	image = ipa_bmp_gd (API,bmp_draw);

	if (image)
	{	gdImageJpeg (image,file,-1); /* Default quality. */
		gdImageDestroy (image);
	}

	fclose (file);
#endif /* HAVE_GD */
}
#ifdef HAVE_GD
static gdImage* ipa_bmp_gd (wmfAPI* API,wmfBMP_Draw_t* bmp_draw)
{	wmfRGB rgb;

	int color;

	unsigned int ui_x;
	unsigned int ui_y;

	unsigned int i;
	unsigned int j;

	gdImage* image = 0;

	WMF_DEBUG (API,"~~~~~~~~ipa_bmp_gd");

	if (bmp_draw->bmp.data == 0)
	{	WMF_ERROR (API,"Glitch! Attempt to write non-existant bitmap.");
		API->err = wmf_E_Glitch;
		return (0);
	}

	image = gdImageCreateTrueColor ((int) bmp_draw->crop.w,(int) bmp_draw->crop.h);

	if (image == 0)
	{	WMF_ERROR (API,"Failed to create GD image!");
		API->err = wmf_E_DeviceError;
		return (0);
	}

	ui_x = (unsigned int) bmp_draw->crop.x;
	ui_y = (unsigned int) bmp_draw->crop.y;

	for (j = 0; j < (unsigned int) bmp_draw->crop.h; j++)
	{	for (i = 0; i < (unsigned int) bmp_draw->crop.w; i++)
		{	wmf_ipa_bmp_color (API,&(bmp_draw->bmp),&rgb,ui_x+i,ui_y+j);

			color = gdImageColorResolve (image,rgb.r,rgb.g,rgb.b);

			gdImageSetPixel (image,(int) i,(int) (bmp_draw->crop.h-1-j),color);
		}
	}

	return (image);
}
#endif /* HAVE_GD */
void wmf_ipa_bmp_eps (wmfAPI* API,wmfBMP_Draw_t* bmp_draw,char* name)
{	wmfRGB rgb;

	unsigned int i;
	unsigned int j;
	unsigned int k;

	unsigned int ui_x;
	unsigned int ui_y;

	unsigned int width;
	unsigned int height;

	static char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

	char buffer[80];

	FILE* file = 0;

	WMF_DEBUG (API,"~~~~~~~~wmf_ipa_bmp_eps");

	if (bmp_draw->bmp.data == 0)
	{	WMF_ERROR (API,"Glitch! Attempt to write non-existant bitmap.");
		API->err = wmf_E_Glitch;
		return;
	}

	file = fopen (name,"w");

	if (file == 0)
	{	WMF_ERROR (API,"Failed to open file to write EPS image!");
		API->err = wmf_E_BadFile;
		return;
	}

	ui_x = (unsigned int) bmp_draw->crop.x;
	ui_y = (unsigned int) bmp_draw->crop.y;

	width  = (unsigned int) bmp_draw->crop.w;
	height = (unsigned int) bmp_draw->crop.h;

	/* Output as an embedded eps */

	fputs ("%!PS-Adobe-2.0 EPSF-2.0\n",file);
	fputs ("%%BoundingBox: ",file);
	fprintf (file," 0 0 %u %u\n",width,height);
	
	fprintf (file," 0 %d translate\n",1);
	fprintf (file," %u %u scale\n",width,height);

	/* I'm going to assume it's a color image - TODO: monochrome */

	fprintf (file," /picstr %u 3 mul string def\n",width);

	fprintf (file," %u %u 8\n",width,height);

	fprintf (file," [ %u 0 0 %u 0 0 ]\n",width,height);

	fputs (" { currentfile picstr readhexstring pop } false 3\n",file);

	fputs (" colorimage\n",file);
			
	for (j = 0; j < height; j++)
	{	k = 0;
		for (i = 0; i < width; i++)
		{	if (k == 78)
			{	buffer[k++] = '\n';
				buffer[k] = 0;
				fputs (buffer,file);
				k = 0;
			}

			wmf_ipa_bmp_color (API,&(bmp_draw->bmp),&rgb,ui_x+i,ui_y+j);

			buffer[k++] = hex[(rgb.r & 0xf0) >> 4];
			buffer[k++] = hex[ rgb.r & 0x0f      ];
			buffer[k++] = hex[(rgb.g & 0xf0) >> 4];
			buffer[k++] = hex[ rgb.g & 0x0f      ];
			buffer[k++] = hex[(rgb.b & 0xf0) >> 4];
			buffer[k++] = hex[ rgb.b & 0x0f      ];
		}
		if (k > 0)
		{	buffer[k++] = '\n';
			buffer[k] = 0;
			fputs (buffer,file);
		}
	}

	fputs ("showpage\n",file);

	fclose (file);

	return;
}

void wmf_ipa_bmp_read (wmfAPI* API,wmfBMP_Read_t* bmp_read)
{	wmfBMP* bmp = 0;

	BMPSource source;

	BMPData* data;

	WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_read");

	bmp = &(bmp_read->bmp);

	bmp->data = 0;

	data = (BMPData*) wmf_malloc (API,sizeof (BMPData));

	if (ERR (API))
	{	if (API->flags & WMF_OPT_IGNORE_NONFATAL)
		{	API->err = wmf_E_None;
			bmp->data = 0;
		}
		WMF_DEBUG (API,"bailing...");
		return;
	}

	data->NColors = 0;
	data->rgb = 0;
	data->image = 0;

	bmp->width  = bmp_read->width;
	bmp->height = bmp_read->height;

	bmp->data = (void*) data;

	source.begin = bmp_read->buffer;
	source.end   = bmp_read->buffer + bmp_read->length;
	source.ptr   = bmp_read->buffer;

	ReadBMPImage (API,bmp,&source);

	if (ERR (API))
	{	if (API->flags & WMF_OPT_IGNORE_NONFATAL)
		{	API->err = wmf_E_None;
			bmp->data = 0;
		}
	}
}

void wmf_ipa_bmp_free (wmfAPI* API,wmfBMP* bmp)
{	BMPData* data;

	WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_free");

	if (bmp->data == 0) return;

	data = (BMPData*) bmp->data;

	if (data->rgb)   wmf_free (API,(void*) data->rgb  );
	if (data->image) wmf_free (API,(void*) data->image);

	wmf_free (API,bmp->data);

	bmp->data = 0;
}

wmfBMP wmf_ipa_bmp_copy (wmfAPI* API,wmfBMP* bmp,unsigned int width,unsigned int height)
{	wmfBMP copy;

	wmfRGB rgb;

	BMPData* copy_data = 0;
	BMPData* data = 0;

	float x;
	float y;

	unsigned int i;
	unsigned int j;

	int opacity;

	size_t size;

	WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_copy");

	copy.width  = width;
	copy.height = height;

	copy.data = 0;

	if (bmp->data == 0) return (copy);

	data = (BMPData*) bmp->data;

	copy.data = wmf_malloc (API,sizeof (BMPData));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (copy);
	}

	copy_data = (BMPData*) copy.data;

	if (data->rgb)
	{	copy_data->NColors = data->NColors;
		copy_data->rgb = (wmfRGB*) wmf_malloc (API,data->NColors * sizeof (wmfRGB));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			wmf_free (API,copy.data);
			copy.data = 0;
			return (copy);
		}
	}
	else
	{	copy_data->NColors = 0;
		copy_data->rgb = 0;
	}

	copy_data->bits_per_pixel = data->bits_per_pixel;

	copy_data->bytes_per_line = 4 * ((width * copy_data->bits_per_pixel + 31) / 32);

	size = height * copy_data->bytes_per_line * sizeof (unsigned char);

	copy_data->image = (unsigned char*) wmf_malloc (API,size);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		if (copy_data->rgb) wmf_free (API,copy_data->rgb);
		wmf_free (API,copy.data);
		copy.data = 0;
		return (copy);
	}

	copy_data->masked = data->masked;
	copy_data->flipped = data->flipped;

	/* Data structure is complete, now to copy the image... */

	for (j = 0; j < height; j++)
	{	y = (float) ((double) j * (double) bmp->height / (double) height);
		for (i = 0; i < width; i++)
		{	x = (float) ((double) i * (double) bmp->width / (double) width);

			opacity = wmf_ipa_bmp_interpolate (API,bmp,&rgb,x,y);

			if (opacity < 0) break; /* Shouldn't occur, I think */

			wmf_ipa_bmp_setcolor (API,&copy,&rgb,(unsigned char) opacity,i,j);
		}
	}

	return (copy);
}

int wmf_ipa_bmp_color (wmfAPI* API,wmfBMP* bmp,wmfRGB* rgb,unsigned int x,unsigned int y)
{	int status = -1; /* error value */

/*	WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_color"); */

	rgb->r = 0;
	rgb->g = 0;
	rgb->b = 0;

	if (bmp->data && x < bmp->width && y < bmp->height)
	{	status = ExtractColor (API,bmp,rgb,x,y);
	}
	else if ((API->flags & WMF_OPT_IGNORE_NONFATAL) == 0)
	{	WMF_ERROR (API,"Point outside bitmap");
		API->err = wmf_E_Glitch;
	}

	return (status);
}

int wmf_ipa_bmp_interpolate (wmfAPI* API,wmfBMP* bmp,wmfRGB* rgb,float x,float y)
{	int status = -1; /* error value */

	unsigned int i1;
	unsigned int i2;
	unsigned int j1;
	unsigned int j2;

	unsigned char o_11;
	unsigned char o_12;
	unsigned char o_21;
	unsigned char o_22;

	float f_11;
	float f_12;
	float f_21;
	float f_22;

	wmfRGB rgb_11;
	wmfRGB rgb_12;
	wmfRGB rgb_21;
	wmfRGB rgb_22;

/*	WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_interpolate"); */

	i1 = (unsigned int) floor (x);
	i2 = (unsigned int) ceil (x);
	j1 = (unsigned int) floor (y);
	j2 = (unsigned int) ceil (y);

	if (i1 >= (unsigned int)(bmp->width - 2))
	{	i1 = bmp->width - 2;
		i2 = bmp->width - 1;
	}

	if (j1 >= (unsigned int)(bmp->height - 2))
	{	j1 = bmp->height - 2;
		j2 = bmp->height - 1;
	}

	if ((i1 == i2) && (j1 == j2)) return (wmf_ipa_bmp_color (API,bmp,rgb,i1,j1));

	i2 = i1 + 1;
	j2 = j1 + 1;

	rgb->r = 0;
	rgb->g = 0;
	rgb->b = 0;

	status = wmf_ipa_bmp_color (API,bmp,&rgb_11,i1,j1);
	if (status < 0) return (status);
	o_11 = (unsigned char) status;

	status = wmf_ipa_bmp_color (API,bmp,&rgb_12,i2,j1);
	if (status < 0) return (status);
	o_12 = (unsigned char) status;

	status = wmf_ipa_bmp_color (API,bmp,&rgb_21,i1,j2);
	if (status < 0) return (status);
	o_21 = (unsigned char) status;

	status = wmf_ipa_bmp_color (API,bmp,&rgb_22,i2,j2);
	if (status < 0) return (status);
	o_22 = (unsigned char) status;

	x -= (float) i1;
	y -= (float) j1;

	f_11 = (1 - x) * (1 - y);
	f_12 = x * (1 - y);
	f_22 = x * y;
	f_21 = (1 - x) * y;

	status = (int) (rgb_11.r * f_11 + rgb_21.r * f_21 + rgb_22.r * f_22 + rgb_12.r * f_12);
	rgb->r = (unsigned char) ((status < 0) ? 0 : ((status > 255) ? 255 : status));

	status = (int) (rgb_11.g * f_11 + rgb_21.g * f_21 + rgb_22.g * f_22 + rgb_12.g * f_12);
	rgb->g = (unsigned char) ((status < 0) ? 0 : ((status > 255) ? 255 : status));

	status = (int) (rgb_11.b * f_11 + rgb_21.b * f_21 + rgb_22.b * f_22 + rgb_12.b * f_12);
	rgb->b = (unsigned char) ((status < 0) ? 0 : ((status > 255) ? 255 : status));

	status = (int) (o_11 * f_11 + o_21 * f_21 + o_22 * f_22 + o_12 * f_12);
	status = ((status < 0) ? 0 : ((status > 255) ? 255 : status));

	return (status);
}

void wmf_ipa_bmp_setcolor (wmfAPI* API,wmfBMP* bmp,wmfRGB* rgb,unsigned char opacity,
                           unsigned int x,unsigned int y)
{
	/* WMF_DEBUG (API,"~~~~~~~~wmf_[ipa_]bmp_setcolor"); */

	if (bmp->data && x < bmp->width && y < bmp->height)
	{	SetColor (API,bmp,rgb,opacity,x,y);
	}
	else if ((API->flags & WMF_OPT_IGNORE_NONFATAL) == 0)
	{	WMF_ERROR (API,"Point outside bitmap");
		API->err = wmf_E_Glitch;
	}
}

/* Following source adapted from ImageMagick-5.2.7's coders/bmp.c, hence:
%
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            BBBB   M   M  PPPP                               %
%                            B   B  MM MM  P   P                              %
%                            BBBB   M M M  PPPP                               %
%                            B   B  M   M  P                                  %
%                            BBBB   M   M  P                                  %
%                                                                             %
%                                                                             %
%                    Read/Write ImageMagick Image Format.                     %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright (C) 2001 ImageMagick Studio, a non-profit organization dedicated %
%  to making software imaging solutions freely available.                     %
%                                                                             %
%  Permission is hereby granted, free of charge, to any person obtaining a    %
%  copy of this software and associated documentation files ("ImageMagick"),  %
%  to deal in ImageMagick without restriction, including without limitation   %
%  the rights to use, copy, modify, merge, publish, distribute, sublicense,   %
%  and/or sell copies of ImageMagick, and to permit persons to whom the       %
%  ImageMagick is furnished to do so, subject to the following conditions:    %
%                                                                             %
%  The above copyright notice and this permission notice shall be included in %
%  all copies or substantial portions of ImageMagick.                         %
%                                                                             %
%  The software is provided "as is", without warranty of any kind, express or %
%  implied, including but not limited to the warranties of merchantability,   %
%  fitness for a particular purpose and noninfringement.  In no event shall   %
%  ImageMagick Studio be liable for any claim, damages or other liability,    %
%  whether in an action of contract, tort or otherwise, arising from, out of  %
%  or in connection with ImageMagick or the use or other dealings in          %
%  ImageMagick.                                                               %
%                                                                             %
%  Except as contained in this notice, the name of the ImageMagick Studio     %
%  shall not be used in advertising or otherwise to promote the sale, use or  %
%  other dealings in ImageMagick without prior written authorization from the %
%  ImageMagick Studio.                                                        %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

static size_t ReadBlob (BMPSource* src,size_t length,unsigned char* buffer)
{	size_t count = 0;

	for (count = 0; count < length; count++)
	{	if (src->ptr >= src->end) break;

		buffer[count] = (*(src->ptr));
		src->ptr++;
	}

	return (count);
}

static int ReadBlobByte (BMPSource* src)
{	int byte;

	if (src->ptr >= src->end) return (EOF);

	byte = (int) (*(src->ptr));
	src->ptr++;

	return (byte);
}

static unsigned short ReadBlobLSBShort (BMPSource* src)
{	unsigned short value;

	unsigned char buffer[2];

	value = ReadBlob (src,2,buffer);

	if (value < 2) return (~value);

	value  = buffer[1] << 8;
	value |= buffer[0];

	return (value);
}

static unsigned int ReadBlobLSBLong (BMPSource* src)
{	unsigned int value;

	unsigned char buffer[4];

	value = ReadBlob (src,4,buffer);

	if (value < 4) return (~value); /* i.e., = -1 */

	value  = buffer[3] << 24;
	value |= buffer[2] << 16;
	value |= buffer[1] <<  8;
	value |= buffer[0];

	return (value);
}

static signed int ReadBlobLSBSignedLong (BMPSource* src)
{	union
	{	unsigned int unsigned_value;
		signed int   signed_value;
	} quantum;

	quantum.unsigned_value = ReadBlobLSBLong (src);

	return (quantum.signed_value);
}

static long TellBlob (BMPSource* src)
{	return ((long) (src->ptr - src->begin));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method DecodeImage unpacks the packed image pixels into runlength-encoded
%  pixel packets.
%
%  A description of each parameter follows:
%
%    o compression:  A value of 1 means the compressed pixels are runlength
%      encoded for a 256-color bitmap.  A value of 2 means a 16-color bitmap.
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the decoding process.
%
%
*/
static int DecodeImage (wmfBMP* bmp,BMPSource* src,unsigned int compression,unsigned char* pixels,unsigned int number_pixels,unsigned int bytes_per_line)
{	int byte;
	int count;
	int i;

	U16 x;
	U16 y;

	U32 u;

	unsigned char* q;
	unsigned char* end;

	for (u = 0; u < number_pixels; u++) pixels[u] = 0;

	byte = 0;
	x = 0;
	q = pixels;
	end = pixels + number_pixels;

	for (y = 0; y < bmp->height; )
	{	count = ReadBlobByte (src);
		if (count == EOF) break;
		if (count != 0)
		{	/* Encoded mode. */
			byte = ReadBlobByte (src);
			for (i = 0; i < count; i++)
			{	
				if (q == end)
					return 0;
			 	if (compression == 1)
				{	(*(q++)) = (unsigned char) byte;
				}
				else
				{	(*(q++)) = ((i & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
					x++;
				}
			}
		}
		else
		{	/* Escape mode. */
			count = ReadBlobByte (src);
			if (count == 0x01) return 1;
			switch (count)
			{
			case 0x00:
			 {	/* End of line. */
				x = 0;
				y++;
				if (y >= bmp->height)
					return 0;
				q = pixels + y * bytes_per_line;
				break;
			 }
			case 0x02:
			 {	/* Delta mode. */
				x += ReadBlobByte (src);
				y += ReadBlobByte (src);
				if (y >= bmp->height)
					return 0;
				if (x >= bmp->width)
					return 0;
				q = pixels + y * bytes_per_line + x;
				break;
			 }
			default:
			 {	/* Absolute mode. */
				for (i = 0; i < count; i++)
				{
					if (q == end)
						return 0;
					if (compression == 1)
					{	(*(q++)) = ReadBlobByte (src);
					}
					else
					{	if ((i & 0x01) == 0) byte = ReadBlobByte (src);
						(*(q++)) = ((i & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
					}
					x++;
				}
				/* Read pad byte. */
				if (compression == 1)
				{	if (count & 0x01) byte = ReadBlobByte (src);
				}
				else
				{	if (((count & 0x03) == 1) || ((count & 0x03) == 2))
					{	byte = ReadBlobByte (src);
					}
				}
				break;
			 }
			}
		}
/* ?? TODO 	if (QuantumTick (y,image->rows)) MagickMonitor (LoadImageText,y,image->rows); */
	}
	ReadBlobByte (src);  /* end of line */
	ReadBlobByte (src);

	return 1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d B M P I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBMPImage reads a Microsoft Windows bitmap image file and
%  returns it.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadBMPImage method is:
%
%      image=ReadBMPImage(image_info)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadBMPImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to an ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static void ReadBMPImage (wmfAPI* API,wmfBMP* bmp,BMPSource* src)
{	BMPInfo bmp_info;

	BMPData* data = 0;

	long start_position = 0;

	unsigned char packet[4];

	unsigned int bytes_per_line;
	unsigned int image_size;
	unsigned int packet_size;
	unsigned int i;

	unsigned long u;

	size_t bytes_read;

	data = (BMPData*) bmp->data;

	memset (&bmp_info,0,sizeof (BMPInfo));

	bmp_info.ba_offset = 0;

	bmp_info.size = ReadBlobLSBLong (src);

	if (bmp_info.size == 12)
	{	/* OS/2 BMP image file. */
		bmp_info.width  = ReadBlobLSBShort (src);
		bmp_info.height = ReadBlobLSBShort (src);
		bmp_info.planes = ReadBlobLSBShort (src);

		bmp_info.bits_per_pixel = ReadBlobLSBShort (src);

		bmp_info.x_pixels = 0;
		bmp_info.y_pixels = 0;
		bmp_info.number_colors = 0;
		bmp_info.compression = 0;
		bmp_info.image_size = 0;
	}
	else
	{	/* Microsoft Windows BMP image file. */
		if (bmp_info.size < 40)
		{	WMF_ERROR (API,"BMP has invalid info-header size");
			API->err = wmf_E_BadFormat;
			return;
		}

		bmp_info.width  = ReadBlobLSBSignedLong (src);
		bmp_info.height = ReadBlobLSBSignedLong (src);
		bmp_info.planes = ReadBlobLSBShort (src);

		bmp_info.bits_per_pixel = ReadBlobLSBShort (src);

		bmp_info.compression = ReadBlobLSBLong (src);
		bmp_info.image_size  = ReadBlobLSBLong (src);

		bmp_info.x_pixels = ReadBlobLSBLong (src);
		bmp_info.y_pixels = ReadBlobLSBLong (src);

		bmp_info.number_colors = ReadBlobLSBLong (src);

		bmp_info.colors_important = ReadBlobLSBLong (src);

		for (u = 0; u < (bmp_info.size - 40); u++)
		{	if (ReadBlobByte (src) == EOF) break;
		}

		if ( (bmp_info.compression == 3)
		  && ((bmp_info.bits_per_pixel == 16) || (bmp_info.bits_per_pixel == 32)) )
		{	bmp_info.red_mask   = ReadBlobLSBShort (src);
			bmp_info.green_mask = ReadBlobLSBShort (src);
			bmp_info.blue_mask  = ReadBlobLSBShort (src);

			if (bmp_info.size > 40)
			{	/* Read color management information. */
				bmp_info.alpha_mask = ReadBlobLSBShort (src);
				bmp_info.colorspace = ReadBlobLSBLong (src);

				bmp_info.red_primary.x = ReadBlobLSBLong (src);
				bmp_info.red_primary.y = ReadBlobLSBLong (src);
				bmp_info.red_primary.z = ReadBlobLSBLong (src);

				bmp_info.green_primary.x = ReadBlobLSBLong (src);
				bmp_info.green_primary.y = ReadBlobLSBLong (src);
				bmp_info.green_primary.z = ReadBlobLSBLong (src);

				bmp_info.blue_primary.x = ReadBlobLSBLong (src);
				bmp_info.blue_primary.y = ReadBlobLSBLong (src);
				bmp_info.blue_primary.z = ReadBlobLSBLong (src);

				bmp_info.gamma_scale.x = ReadBlobLSBShort (src);
				bmp_info.gamma_scale.y = ReadBlobLSBShort (src);
				bmp_info.gamma_scale.z = ReadBlobLSBShort (src);
			}
		}
	}

	if (bmp_info.width <= 0)
	{	WMF_ERROR (API,"BMP has invalid width");
		API->err = wmf_E_BadFormat;
		return;
	}

	if ((bmp_info.height == 0) || (bmp_info.height < -INT_MAX))
	{	WMF_ERROR (API,"BMP has invalid height");
		API->err = wmf_E_BadFormat;
		return;
	}

	if (bmp_info.height < 0)
	{	bmp_info.height = - bmp_info.height;
		data->flipped = 1;
	}
	else
	{	data->flipped = 0;
	}

	/* WMF may change bitmap size without changing bitmap header
	 */
	if (bmp->width  == 0) bmp->width  = (U16) bmp_info.width;
	if (bmp->height == 0) bmp->height = (U16) bmp_info.height;

	data->NColors = 0;
	if ((bmp_info.number_colors != 0) || (bmp_info.bits_per_pixel <= 8))
	{	unsigned int max_colors = 0;

		if (bmp_info.bits_per_pixel <= 8)
			max_colors = 1u << (bmp_info.bits_per_pixel & 0x1F);

		data->NColors = (unsigned int) bmp_info.number_colors;

		if ((max_colors > 0) && (data->NColors > max_colors))
			data->NColors = max_colors;
	}

	if (data->NColors > 0)
	{	/* Read BMP raster colormap. */
		data->rgb = (wmfRGB*) wmf_malloc (API,data->NColors * sizeof (wmfRGB));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		if (bmp_info.size == 12) packet_size = 3;
		else                     packet_size = 4;

		for (i = 0; i < data->NColors; i++)
		{	bytes_read = ReadBlob (src,packet_size,packet);
			if (bytes_read < packet_size)
			{	WMF_ERROR (API,"Unexpected EOF");
				API->err = wmf_E_EOF;
				break;
			}
			data->rgb[i].b = packet[0];
			data->rgb[i].g = packet[1];
			data->rgb[i].r = packet[2];
		}

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	while (TellBlob (src) < (long) (start_position + bmp_info.offset_bits))
	{	ReadBlobByte (src);
	}

	/* Read image data. */
	if (bmp_info.bits_per_pixel > 32)
	{	WMF_ERROR (API,"BMP image has invalid bits_per_pixel");
		API->err = wmf_E_BadFormat;
		return;
	}

	if (bmp_info.compression == 2)
		bmp_info.bits_per_pixel = (unsigned short) ((bmp_info.bits_per_pixel & 0x3F) << 1);

	bytes_per_line = 4 * ((bmp->width * bmp_info.bits_per_pixel + 31) / 32);

	if ((bmp->width == 0) || (bmp->height == 0))
	{	WMF_ERROR (API,"BMP image has zero width or height");
		API->err = wmf_E_BadFormat;
		return;
	}

	if (bytes_per_line > UINT_MAX / bmp->height)
	{	WMF_ERROR (API,"BMP image dimensions too large");
		API->err = wmf_E_BadFormat;
		return;
	}

	image_size = bytes_per_line * bmp->height;

	data->image = (unsigned char*) wmf_malloc (API,image_size);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	if ((bmp_info.compression == 0) || (bmp_info.compression == 3))
	{	bytes_read = ReadBlob (src,image_size,data->image);
		if (bytes_read < image_size)
		{	WMF_ERROR (API,"Unexpected EOF");
			API->err = wmf_E_EOF;
		}
	}
	else
	{
		if (bmp_info.bits_per_pixel == 8)	/* Convert run-length encoded raster pixels. */
		{
			if (!DecodeImage (bmp,src,(unsigned int) bmp_info.compression,data->image,image_size,bytes_per_line))
			{	WMF_ERROR (API,"corrupt bmp");
				API->err = wmf_E_BadFormat;
			}
		}
		else
		{	WMF_ERROR (API,"Unexpected pixel depth");
			API->err = wmf_E_BadFormat;
		}
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	data->bits_per_pixel = bmp_info.bits_per_pixel;
	data->bytes_per_line = bytes_per_line;

	data->masked = bmp_info.red_mask;
}

static int ExtractColor (wmfAPI* API,wmfBMP* bmp,wmfRGB* rgb,unsigned int x,unsigned int y)
{	int status = 0;

	BMPData* data = 0;

	int bit;

	unsigned int color;

	unsigned char opacity = WMF_BMP_OPAQUE;

	unsigned char* p;

	unsigned short word;

	data = (BMPData*) bmp->data;

	if (data->flipped) y = (bmp->height - 1) - y;

	switch (data->bits_per_pixel)
	{
	case 1:
	 {	p = data->image + (y * data->bytes_per_line) + (x >> 3);
		bit = 0x80 >> (x & 0x07);

		if ((*p) & bit) color = 1;
		else            color = 0;

		if (data->rgb && (color < data->NColors))
		{	(*rgb) = data->rgb[color];
		}
		else
		{	if (color)
			{	rgb->r = 0;
				rgb->g = 0;
				rgb->b = 0;
			}
			else
			{	rgb->r = 0xff;
				rgb->g = 0xff;
				rgb->b = 0xff;
			}
		}
		break;
	 }
	case 4:
	 {	p = data->image + (y * data->bytes_per_line) + (x >> 1);

	 	if (x & 1) color =  (*p)       & 0x0f;
	 	else       color = ((*p) >> 4) & 0x0f;

		if (data->rgb && (color < data->NColors))
		{	(*rgb) = data->rgb[color];
		}
		else
		{	rgb->r = color << 4;
			rgb->g = color << 4;
			rgb->b = color << 4;
		}
		break;
	 }
	case 8:
	 {	p = data->image + (y * data->bytes_per_line) + x;

		color = (*p);

		if (data->rgb && (color < data->NColors))
		{	(*rgb) = data->rgb[color];
		}
		else
		{	rgb->r = color;
			rgb->g = color;
			rgb->b = color;
		}
		break;
	 }
	case 16:
	 {	p = data->image + (y * data->bytes_per_line) + (x << 1);

		word = (unsigned short) p[0] | (((unsigned short) p[1]) << 8);

		if (data->masked == 0)
		{	rgb->r = ((word >> 10) & 0x1f) << 3;
			rgb->g = ((word >>  5) & 0x1f) << 3;
			rgb->b = ( word        & 0x1f) << 3;
		}
		else
		{	rgb->r = ((word >> 11) & 0x1f) << 3;
			rgb->g = ((word >>  5) & 0x3f) << 2;
			rgb->b = ( word        & 0x1f) << 3;
		}
		break;
	 }
	case 24:
	 {	p = data->image + (y * data->bytes_per_line) + (x + (x << 1));

	 	rgb->b = p[0];
	 	rgb->g = p[1];
	 	rgb->r = p[2];

		break;
	 }
	case 32:
	 {	p = data->image + (y * data->bytes_per_line) + (x << 2);

	 	rgb->b = p[0];
	 	rgb->g = p[1];
	 	rgb->r = p[2];

	 	opacity = p[3];

		break;
	 }
	default:
		if ((API->flags & WMF_OPT_IGNORE_NONFATAL) == 0)
		{	WMF_ERROR (API,"Bitmap has bad format (illegal color depth)");
			API->err = wmf_E_BadFormat;
		}
		status = -1;
	break;
	}

	if (status == 0) status = (int) opacity;

	return (status);
}

static void SetColor (wmfAPI* API,wmfBMP* bmp,wmfRGB* rgb,unsigned char opacity,unsigned int x,unsigned int y)
{	BMPData* data = 0;

	unsigned int i;
	unsigned int r_diff;
	unsigned int g_diff;
	unsigned int b_diff;
	unsigned int diff;
	unsigned int min_diff;
	unsigned int color;

	unsigned char bit;

	unsigned char* p;

	unsigned short word;

	data = (BMPData*) bmp->data;

	if (data->flipped) y = (bmp->height - 1) - y;

	switch (data->bits_per_pixel)
	{
	case 1:
	 {	p = data->image + (y * data->bytes_per_line) + (x >> 3);
		bit = 0x80 >> (x & 0x07);

		if (rgb->r || rgb->g || rgb->b)
		{	(*p) |= ( bit & 0xff);
		}
		else
		{	(*p) &= (~bit & 0xff);
		}

		break;
	 }
	case 4:
	 {	p = data->image + (y * data->bytes_per_line) + (x >> 1);

	 	if (data->rgb == 0) break;

	 	min_diff = 766;
	 	color = 0;
	 	for (i = 0; i < data->NColors; i++)
	 	{	r_diff = (unsigned int) ABS ((int) rgb->r - (int) data->rgb[i].r);
			g_diff = (unsigned int) ABS ((int) rgb->g - (int) data->rgb[i].g);
			b_diff = (unsigned int) ABS ((int) rgb->b - (int) data->rgb[i].b);
			diff = r_diff + g_diff + b_diff;
			if (min_diff > diff)
			{	min_diff = diff;
				color = i;
			}
	 	}

	 	if (x & 1)
		{	(*p) = ( ((unsigned char) color)       | ((*p) & 0x0f));
		}
	 	else
		{	(*p) = ((((unsigned char) color) << 4) | ((*p) & 0xf0));
		}

		break;
	 }
	case 8:
	 {	p = data->image + (y * data->bytes_per_line) + x;

	 	if (data->rgb == 0) break;

	 	min_diff = 766;
	 	color = 0;
	 	for (i = 0; i < data->NColors; i++)
	 	{	r_diff = (unsigned int) ABS ((int) rgb->r - (int) data->rgb[i].r);
			g_diff = (unsigned int) ABS ((int) rgb->g - (int) data->rgb[i].g);
			b_diff = (unsigned int) ABS ((int) rgb->b - (int) data->rgb[i].b);
			diff = r_diff + g_diff + b_diff;
			if (min_diff > diff)
			{	min_diff = diff;
				color = i;
			}
	 	}

		(*p) = (unsigned char) color;

		break;
	 }
	case 16:
	 {	p = data->image + (y * data->bytes_per_line) + (x << 1);

		word = 0;

		if (data->masked == 0)
		{	word |= (rgb->r >> 3) << 10;
			word |= (rgb->g >> 3) <<  5;
			word |=  rgb->b >> 3;
		}
		else
		{	word |= (rgb->r >> 3) << 11;
			word |= (rgb->g >> 2) <<  5;
			word |=  rgb->b >> 3;
		}

		p[0] = (unsigned char) ( word & 0x00ff      );
		p[1] = (unsigned char) ((word & 0xff00) >> 8);

		break;
	 }
	case 24:
	 {	p = data->image + (y * data->bytes_per_line) + (x + (x << 1));

	 	p[0] = rgb->b;
	 	p[1] = rgb->g;
	 	p[2] = rgb->r;

		break;
	 }
	case 32:
	 {	p = data->image + (y * data->bytes_per_line) + (x << 2);

	 	p[0] = rgb->b;
	 	p[1] = rgb->g;
	 	p[2] = rgb->r;

	 	p[3] = opacity;

		break;
	 }
	default:
		if ((API->flags & WMF_OPT_IGNORE_NONFATAL) == 0)
		{	WMF_ERROR (API,"Bitmap has bad format (illegal color depth)");
			API->err = wmf_E_BadFormat;
		}
	break;
	}
}

