/* libwmf (<libwmf/foreign.h>): library for wmf conversion
   Copyright (C) 2001 Francis James Franklin

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


#ifndef LIBWMF_FOREIGN_H
#define LIBWMF_FOREIGN_H

#include <stdio.h>

#include <libwmf/types.h>

typedef struct _wmf_foreign_t wmf_foreign_t;

struct _wmf_foreign_t
{	/* other */

	unsigned long flags;
};

#define WMF_FOREIGN_SUPPORTS_PNG  (1 <<  0) /* Set by wmf_api_create () if PNG  supported */
#define WMF_FOREIGN_SUPPORTS_JPEG (1 <<  1) /* Set by wmf_api_create () if JPEG supported */

#define WMF_FOREIGN_GetData(Z) ((wmf_foreign_t*)((Z)->device_data))

#ifdef __cplusplus
extern "C" {
#endif

extern void wmf_foreign_function (wmfAPI* API);

extern int wmf_image_load_png (wmfAPI*,FILE*,wmfImage*);
extern int wmf_image_load_jpg (wmfAPI*,FILE*,wmfImage*);

extern int wmf_image_save_eps (wmfAPI*,FILE*,wmfImage*);

extern void wmf_image_free (wmfAPI*,wmfImage*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_FOREIGN_H */
