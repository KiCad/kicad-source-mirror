/* libwmf (ipa/foreign.c): library for wmf conversion
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

#include "wmfdefs.h"

#ifndef WITHOUT_LAYERS

#ifdef HAVE_GD
#include <gd.h>
#endif /* HAVE_GD */

#include "libwmf/foreign.h"

#else /* ! WITHOUT_LAYERS */

#ifdef HAVE_GD
#undef HAVE_GD
#endif

#endif /* ! WITHOUT_LAYERS */

void wmf_foreign_function (wmfAPI* API)
{
#ifndef WITHOUT_LAYERS
	wmf_foreign_t* ddata = 0;

	API->function_reference = 0;

/* Allocate device data structure
 */
	ddata = (wmf_foreign_t*) wmf_malloc (API,sizeof (wmf_foreign_t));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	API->device_data = (void*) ddata;

	ddata->flags = 0;
#ifdef HAVE_LIBPNG
	ddata->flags |= WMF_FOREIGN_SUPPORTS_PNG;
#endif
#ifdef HAVE_LIBJPEG
	ddata->flags |= WMF_FOREIGN_SUPPORTS_JPEG;
#endif
#else /* ! WITHOUT_LAYERS */
	API->device_data = 0;

	API->err = wmf_E_DeviceError;
#endif /* ! WITHOUT_LAYERS */
}

int wmf_image_load_png (wmfAPI* API,FILE* png_in,wmfImage* image)
{	(void)API;
	/* wmf_foreign_t* ddata = WMF_FOREIGN_GetData (API); */
#ifdef HAVE_GD
	gdImage* gd_image = 0;
#ifndef HAVE_LIBPNG
	return (-1);
#endif
	gd_image = gdImageCreateFromPng (png_in);

	if (gd_image == 0) return (-1);

	image->type = wmf_I_gd;

	image->width  = (U16) gdImageSX (gd_image);
	image->height = (U16) gdImageSY (gd_image);

	image->data = (void*) gd_image;

	return (0);
#else /* HAVE_GD */
	return (-1);
#endif /* HAVE_GD */
}

int wmf_image_load_jpg (wmfAPI* API,FILE* jpg_in,wmfImage* image)
{	(void)API;
	/* wmf_foreign_t* ddata = WMF_FOREIGN_GetData (API); */
#ifdef HAVE_GD
	gdImage* gd_image = 0;
#ifndef HAVE_LIBJPEG
	return (-1);
#endif
	gd_image = gdImageCreateFromJpeg (jpg_in);

	if (gd_image == 0) return (-1);

	image->type = wmf_I_gd;

	image->width  = (U16) gdImageSX (gd_image);
	image->height = (U16) gdImageSY (gd_image);

	image->data = (void*) gd_image;

	return (0);
#else /* HAVE_GD */
	return (-1);
#endif /* HAVE_GD */
}

int wmf_image_save_eps (wmfAPI* API,FILE* file,wmfImage* image)
{	/* wmf_foreign_t* ddata = WMF_FOREIGN_GetData(API); */
#ifdef HAVE_GD
	wmfRGB rgb;

	gdImage* gd_image = 0;

	int c;
	int i;
	int j;
	int k;
	int width;
	int height;

	static char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

	char buffer[80];

	gd_image = (gdImage*) image->data;

	if ((gd_image == 0) || (file == 0)) return (-1);

	if (image->type != wmf_I_gd)
	{	WMF_ERROR (API,"image type not supported!");
		API->err = wmf_E_DeviceError;
		return (-1);
	}

	width  = gdImageSX (gd_image);
	height = gdImageSY (gd_image);

	/* */
	fputs ("%!PS-Adobe-2.0 EPSF-2.0\n",file);
	fputs ("%%BoundingBox: ",file);
	fprintf (file," 0 0 %d %d\n",width,height);
	
	fprintf (file," 0 %d translate\n",1);
	fprintf (file," %d %d scale\n",width,height);

	/* I'm going to assume it's a color image - TODO: monochrome */

	fprintf (file," /picstr %d 3 mul string def\n",width);

	fprintf (file," %d %d 8\n",width,height);

	fprintf (file," [ %d 0 0 %d 0 0 ]\n",width,height);

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

			c = gdImageGetPixel (gd_image,i,j);
			if (gd_image->trueColor)
			{	rgb.r = gdTrueColorGetRed (c);
				rgb.g = gdTrueColorGetGreen (c);
				rgb.b = gdTrueColorGetBlue (c);
			}
			else
			{	rgb.r = gd_image->red[c];
				rgb.g = gd_image->green[c];
				rgb.b = gd_image->blue[c];
			}

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

	return (0);
#else /* HAVE_GD */
	return (-1);
#endif /* HAVE_GD */
}

void wmf_image_free (wmfAPI* API,wmfImage* image)
{	/* wmf_foreign_t* ddata = WMF_FOREIGN_GetData (API); */
#ifdef HAVE_GD
	gdImage* gd_image = (gdImage*) image->data;

	if (image->type != wmf_I_gd)
	{	WMF_ERROR (API,"image type not supported!");
		API->err = wmf_E_DeviceError;
		return;
	}

	if (gd_image)
	{	gdImageDestroy (gd_image);
	}

	image->data = 0;
#endif /* HAVE_GD */
}
