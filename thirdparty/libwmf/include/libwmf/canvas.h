/* libwmf (<libwmf/canvas.h>): library for wmf conversion
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


/* Interface Programmable des Applications ??
 */
#ifndef LIBWMF_CANVAS_H
#define LIBWMF_CANVAS_H

#include <libwmf/ipa.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void * wmfCanvas;

/**
 * Make a canvas for drawing to, to build a metafile in memory.
 */
extern LIBWMF_EXPORT wmfCanvas * wmf_canvas (wmfAPI*,unsigned short,unsigned short,unsigned short);

/**
 * Final canvas call: finish off the metafile, free canvas etc., and return the metafile buffer.
 */
extern LIBWMF_EXPORT unsigned char * wmf_canvas_done (wmfAPI*,wmfCanvas*,unsigned char**,unsigned long*);

/**
 * Set current pen (stroke) attributes.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_pen (wmfAPI*,wmfCanvas*,
			       unsigned short,unsigned short,unsigned short,
			       unsigned short,wmfRGB);

/**
 * Set current brush (fill) attributes.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_brush (wmfAPI*,wmfCanvas*,unsigned short,unsigned short,wmfRGB);

/**
 * Change current font.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_font (wmfAPI*,wmfCanvas*,const char*,
				unsigned short,unsigned short,unsigned short,unsigned short,
				unsigned short,unsigned short,unsigned short,unsigned short);

/**
 * Set polygon fill mode.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_polyfill (wmfAPI*,wmfCanvas*,unsigned short);

/**
 * Set background mode.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_background (wmfAPI*,wmfCanvas*,unsigned short);

/**
 * Set background color.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_bgcolor (wmfAPI*,wmfCanvas*,wmfRGB);

/**
 * Set text color.
 */
extern LIBWMF_EXPORT int wmf_canvas_set_textcolor (wmfAPI*,wmfCanvas*,wmfRGB);

/**
 * Draw a line.
 */
extern LIBWMF_EXPORT int wmf_canvas_line (wmfAPI*,wmfCanvas*,
			    unsigned short,unsigned short,
			    unsigned short,unsigned short);

/**
 * Draw a rounded rectangle.
 */
extern LIBWMF_EXPORT int wmf_canvas_roundrect (wmfAPI*,wmfCanvas*,
				 unsigned short,unsigned short,
				 unsigned short,unsigned short,
				 unsigned short,unsigned short);

/**
 * Draw a rectangle.
 */
extern LIBWMF_EXPORT int wmf_canvas_rect (wmfAPI*,wmfCanvas*,
			    unsigned short,unsigned short,
			    unsigned short,unsigned short);

/**
 * Draw an ellipse in the given bounding box.
 */
extern LIBWMF_EXPORT int wmf_canvas_ellipse (wmfAPI*,wmfCanvas*,
			       unsigned short,unsigned short,
			       unsigned short,unsigned short);

/**
 * \b wmf_canvas_arc_t is the (enumeration) type used to distinguish arc type.
 */
typedef enum _wmf_canvas_arc_t
{
  wmf_CA_open = 0, /**< drawn arc segment (no fill) */
  wmf_CA_chord,    /**< start & end of arc joined together */
  wmf_CA_pie       /**< start & end of arc joined to centre */
} wmf_canvas_arc_t;

/**
 * Draw an elliptic arc in the given bounding box.
 */
extern LIBWMF_EXPORT int wmf_canvas_arc (wmfAPI*,wmfCanvas*,
			   unsigned short,unsigned short,
			   unsigned short,unsigned short,
			   unsigned short,unsigned short,
			   unsigned short,unsigned short,wmf_canvas_arc_t);

/**
 * Draw a line connecting a sequence of points.
 */
extern LIBWMF_EXPORT int wmf_canvas_polyline (wmfAPI*,wmfCanvas*,
				unsigned short*,unsigned short*,unsigned short);

/**
 * Draw a polygon.
 */
extern LIBWMF_EXPORT int wmf_canvas_polygon (wmfAPI*,wmfCanvas*,
			       unsigned short*,unsigned short*,unsigned short);

/**
 * Draw a set of polygons.
 */
extern LIBWMF_EXPORT int wmf_canvas_polygons (wmfAPI*,wmfCanvas*,unsigned short,
				unsigned short**,unsigned short**,unsigned short*);

/**
 * Draw text.
 */
extern LIBWMF_EXPORT int wmf_canvas_text (wmfAPI*,wmfCanvas*,unsigned short,unsigned short,const char*);

/**
 * Place a bitmap.
 */
extern LIBWMF_EXPORT int wmf_canvas_bitmap (wmfAPI*,wmfCanvas*,unsigned short,unsigned short,
			      unsigned short,unsigned short,const unsigned char*,unsigned long);

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_CANVAS_H */
