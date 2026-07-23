/* libwmf (<libwmf/svg.h>): library for wmf conversion
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


#ifndef LIBWMF_SVG_H
#define LIBWMF_SVG_H

#include <stdio.h>

#include <libwmf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _wmf_svg_t wmf_svg_t;

struct _wmf_svg_t
{	/* other */

	wmfD_Rect bbox;

	wmfStream* out; /* Output stream */

	char* Description;

	unsigned int width;
	unsigned int height;

	struct _wmf_svg_image /* SVG device layer writes raster images as PNG */
	{	void* context;
		char* (*name) (void*); /* takes context; returns file name */
	} image;

	unsigned long flags;
};

#define WMF_SVG_GetData(Z) ((wmf_svg_t*)((Z)->device_data))

#define WMF_SVG_INLINE_IMAGES (1 << 0)

extern void wmf_svg_function (wmfAPI*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_SVG_H */
