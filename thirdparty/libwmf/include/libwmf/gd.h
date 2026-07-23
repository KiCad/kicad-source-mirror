/* libwmf (<libwmf/gd.h>): library for wmf conversion
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


#ifndef LIBWMF_GD_H
#define LIBWMF_GD_H

#include <libwmf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _wmf_gd_subtype
{	wmf_gd_png = 0,
	wmf_gd_jpeg,
	wmf_gd_image
} wmf_gd_subtype;

typedef struct _wmf_gd_t wmf_gd_t;

struct _wmf_gd_t
{	wmf_gd_subtype type;

	void* gd_data;

	FILE* file;

	char* memory;

	struct _wmf_gd_sink
	{	void* context;
		int (*function) (void* context,char* buffer,int length);
	} sink;

	/* pointer to gdImage; null prior to wmf_play ()
	 */
	void* gd_image;

	unsigned int width;
	unsigned int height;

	wmfD_Rect bbox;

	unsigned long flags;
};

#define WMF_GD_SUPPORTS_PNG  (1 <<  0) /* Set by wmf_api_create () if PNG  supported */
#define WMF_GD_SUPPORTS_JPEG (1 <<  1) /* Set by wmf_api_create () if JPEG supported */

#define WMF_GD_OUTPUT_FILE   (1 << 16)
#define WMF_GD_OUTPUT_MEMORY (1 << 17)
#define WMF_GD_OWN_BUFFER    (1 << 18) /* To be used in conjuction with WMF_GD_OUTPUT_MEMORY */

#define WMF_GD_GetData(Z) ((wmf_gd_t*)((Z)->device_data))

extern void wmf_gd_function (wmfAPI*);

/**
 * If using the wmf_gd_image option to write to a GD image, the true-color
 * pixels (stored in a contiguous array of height * width integers) of the
 * image can be retrieved using wmf_gd_get_image_pixels(). The returned
 * buffer is owned by the API and is released by wmf_api_destroy(); do not
 * free() it.
 */
extern int * wmf_gd_get_image_pixels (wmfAPI* API);

/**
 * Deprecated: use wmf_gd_get_image_pixels() instead. When libwmf is built
 * against system libgd this function returns NULL, as the public libgd API
 * exposes no contiguous backing store to borrow.
 */
extern int * wmf_gd_image_pixels (void * gd_image);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_GD_H */
