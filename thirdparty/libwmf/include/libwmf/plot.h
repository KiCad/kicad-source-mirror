/* libwmf (<libwmf/plot.h>): library for wmf conversion
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


#ifndef LIBWMF_PLOT_H
#define LIBWMF_PLOT_H

#include <libwmf/types.h>

typedef enum _wmf_plot_subtype
{	wmf_plot_generic = 0,
	wmf_plot_bitmap,
	wmf_plot_meta,
	wmf_plot_tek,
	wmf_plot_regis,
	wmf_plot_hpgl,
	wmf_plot_pcl,
	wmf_plot_fig,
	wmf_plot_cgm,
	wmf_plot_ps,
	wmf_plot_ai,
	wmf_plot_svg,
	wmf_plot_gif,
	wmf_plot_pnm,
	wmf_plot_png,
	wmf_plot_Xdrawable,
	wmf_plot_X
} wmf_plot_subtype;

typedef struct _wmf_plot_t wmf_plot_t;

struct _wmf_plot_t
{	wmf_plot_subtype type;

	void* plot_data;

	FILE* file;

	unsigned int width;
	unsigned int height;

	wmfD_Rect bbox;

	unsigned long flags;
};

#define WMF_PLOT_GetData(Z) ((wmf_plot_t*)((Z)->device_data))

#ifdef __cplusplus
extern "C" {
#endif

extern void wmf_plot_function (wmfAPI*);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_PLOT_H */
