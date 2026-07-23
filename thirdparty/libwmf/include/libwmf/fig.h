/* libwmf (<libwmf/fig.h>): library for wmf conversion
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


#ifndef LIBWMF_FIG_H
#define LIBWMF_FIG_H

#include <stdio.h>

#include <libwmf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _wmf_fig_t wmf_fig_t;

struct _wmf_fig_t
{	/* other */

	void* fig_data;

	wmfD_Rect bbox;

	wmfStream* out;   /* Output stream */

	char* Title;
	char* Creator;
	char* Date;
	char* For;

	unsigned int fig_x;
	unsigned int fig_y;
	unsigned int fig_width;  /* 0 = use wmf width  */
	unsigned int fig_height; /* 0 = use wmf height */

	wmf_page_t format;

	unsigned int dpi;

	int depth;
	int ddec;

	struct _wmf_fig_image /* fig device layer writes raster images as EPS */
	{	void* context;
		char* (*name) (void*); /* takes context; returns file name */
	} image;

	unsigned long flags;
};

#define WMF_FIG_LANDSCAPE (1 << 0)

#define WMF_FIG_IMAGE_PNG  (2) /* Default is to write raster sub-images as EPS */
#define WMF_FIG_IMAGE_JPEG (6)
#define WMF_FIG_ImageIsEPS(Z) (((Z)->flags & WMF_FIG_IMAGE_JPEG) == 0)
#define WMF_FIG_ImageIsPNG(Z) (((Z)->flags & WMF_FIG_IMAGE_JPEG) == WMF_FIG_IMAGE_PNG )
#define WMF_FIG_ImageIsJPG(Z) (((Z)->flags & WMF_FIG_IMAGE_JPEG) == WMF_FIG_IMAGE_JPEG)

#define WMF_FIG_SUPPORTS_PNG  (1 << 3) /* Set by wmf_api_create () if PNG  supported */
#define WMF_FIG_SUPPORTS_JPEG (1 << 4) /* Set by wmf_api_create () if JPEG supported */

#define WMF_FIG_MAXPECT    (1 << 5) /* scale image to fit page */
#define WMF_FIG_NO_MARGINS (1 << 6) /* remove margins when scaling */

#define WMF_FIG_GetData(Z) ((wmf_fig_t*)((Z)->device_data))

extern void wmf_fig_function (wmfAPI*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_FIG_H */
