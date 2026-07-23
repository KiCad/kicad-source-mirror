/* libwmf (<libwmf/x.h>): library for wmf conversion
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


#ifndef LIBWMF_X_H
#define LIBWMF_X_H

#include <X11/Xlib.h>
#include <X11/X.h>

#include <libwmf/types.h>

typedef struct _wmf_x_t wmf_x_t;

struct _wmf_x_t
{	/* other */

	char* display_name;
	char* window_name;
	char* icon_name;

	Display* display;

	Window root;
	Window window;
	Pixmap pixmap;
	Pixmap hatch;
	Pixmap brush;

	Visual*        visual;
	int            depth;
	int            class;
	Colormap       colormap;
	unsigned long  black;
	unsigned long  white;
	unsigned long* color;
	struct
	{	unsigned long red;
		unsigned long green;
		unsigned long blue;
	} mask;

	GC gc;

	unsigned int x_width;
	unsigned int x_height;

	wmfD_Rect bbox;

	unsigned long flags;
};

#define WMF_X_GetData(Z) ((wmf_x_t*)(((wmfAPI*)Z)->device_data))

#define WMF_X_DISPLAY_OPEN  (1 << 0) /* Open display; sets WMF_X_DISPLAY_CLOSE *
                                      * WMF_X_WINDOW_OPEN & WMF_X_PIXMAP_OPEN. */
#define WMF_X_DISPLAY_CLOSE (1 << 1) /* Close display */
#define WMF_X_WINDOW_OPEN   (1 << 2) /* Open window; sets WMF_X_WINDOW_CLOSE */
#define WMF_X_WINDOW_CLOSE  (1 << 3) /* Close window */
#define WMF_X_PIXMAP_OPEN   (1 << 4) /* Open pixmap; sets WMF_X_PIXMAP_CLOSE */
#define WMF_X_PIXMAP_CLOSE  (1 << 5) /* Close pixmap */

#define WMF_X_CMAP_DESTROY  (1 << 7) /* Destroy colormap */

#ifdef __cplusplus
extern "C" {
#endif

extern void wmf_x_function (wmfAPI*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_X_H */
