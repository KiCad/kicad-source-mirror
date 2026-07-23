/* libwmf (<libwmf/eps.h>): library for wmf conversion
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


#ifndef LIBWMF_EPS_H
#define LIBWMF_EPS_H

#include <stdio.h>

#include <libwmf/types.h>

typedef struct _wmf_eps_t wmf_eps_t;

struct _wmf_eps_t
{	/* other */

	wmfD_Rect bbox;

	wmfStream* out;   /* Output stream */

	char* Title;
	char* Creator;
	char* Date;
	char* For;

	int eps_x;   /* Desired location & size of eps output */
	int eps_y;

	unsigned int eps_width;  /* 0 = use wmf width  */
	unsigned int eps_height; /* 0 = use wmf height */

	unsigned int page_width; /* Page size if (style_eps) */
	unsigned int page_height;

	unsigned long flags;
};

#define WMF_EPS_STYLE_PS   (1 << 0)
#define WMF_EPS_LANDSCAPE  (1 << 1)

#define WMF_EPS_GetData(Z) ((wmf_eps_t*)((Z)->device_data))

#ifdef __cplusplus
extern "C" {
#endif

extern void wmf_eps_function (wmfAPI*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_EPS_H */
