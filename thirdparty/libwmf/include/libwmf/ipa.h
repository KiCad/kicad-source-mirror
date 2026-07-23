/* libwmf (<libwmf/ipa.h>): library for wmf conversion
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
#ifndef LIBWMF_IPA_H
#define LIBWMF_IPA_H

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
  #ifdef LIBWMF_EXPORTS
    #define LIBWMF_EXPORT __declspec(dllexport)
  #else
    #define LIBWMF_EXPORT __declspec(dllimport)
  #endif
#else
  #define LIBWMF_EXPORT
#endif

#include <libwmf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type declarations
 */
typedef struct _wmfRGB                 wmfRGB;
typedef struct _wmfBMP                 wmfBMP;

typedef struct _wmfBrush               wmfBrush;
typedef struct _wmfPen                 wmfPen;
typedef struct _wmfFont                wmfFont;

typedef struct _wmfFontData            wmfFontData;
typedef struct _wmfColorData           wmfColorData;

typedef struct _wmfDC                  wmfDC;

typedef struct _wmfFlood_t             wmfFlood_t;
typedef struct _wmfDrawPixel_t         wmfDrawPixel_t;
typedef struct _wmfDrawArc_t           wmfDrawArc_t;
typedef struct _wmfDrawLine_t          wmfDrawLine_t;
typedef struct _wmfPolyLine_t          wmfPolyLine_t;
typedef struct _wmfPolyPoly_t          wmfPolyPoly_t;
typedef struct _wmfDrawRectangle_t     wmfDrawRectangle_t;
typedef struct _wmfPolyRectangle_t     wmfPolyRectangle_t;
typedef struct _wmfBMP_Read_t          wmfBMP_Read_t;
typedef struct _wmfBMP_Draw_t          wmfBMP_Draw_t;
typedef struct _wmfROP_Draw_t          wmfROP_Draw_t;
typedef struct _wmfDrawText_t          wmfDrawText_t;
typedef struct _wmfUserData_t          wmfUserData_t;

typedef struct _wmfFunctionReference   wmfFunctionReference;

typedef float (*wmfStringWidth) (wmfAPI*,wmfFont*,char*);
typedef void  (*wmfMap)         (wmfAPI*,wmfFont*);

/* Device-layer device-independent default functions
 */
extern void   wmf_ipa_bmp_b64 (wmfAPI*,wmfBMP_Draw_t*,wmfStream*);
extern void   wmf_ipa_bmp_png (wmfAPI*,wmfBMP_Draw_t*,char*);
extern void   wmf_ipa_bmp_jpg (wmfAPI*,wmfBMP_Draw_t*,char*);
extern void   wmf_ipa_bmp_eps (wmfAPI*,wmfBMP_Draw_t*,char*);
extern void   wmf_ipa_bmp_read (wmfAPI*,wmfBMP_Read_t*);
extern void   wmf_ipa_bmp_free (wmfAPI*,wmfBMP*);
extern wmfBMP wmf_ipa_bmp_copy (wmfAPI*,wmfBMP*,unsigned int,unsigned int);
extern int    wmf_ipa_bmp_color (wmfAPI*,wmfBMP*,wmfRGB*,unsigned int,unsigned int);
extern void   wmf_ipa_bmp_setcolor (wmfAPI*,wmfBMP*,wmfRGB*,unsigned char,unsigned int,unsigned int);
extern int    wmf_ipa_bmp_interpolate (wmfAPI*,wmfBMP*,wmfRGB*,float,float);

extern LIBWMF_EXPORT void          wmf_ipa_color_init (wmfAPI*);
extern LIBWMF_EXPORT void          wmf_ipa_color_add (wmfAPI*,wmfRGB*);
extern LIBWMF_EXPORT unsigned long wmf_ipa_color_index (wmfAPI*,wmfRGB*);
extern LIBWMF_EXPORT unsigned long wmf_ipa_color_count (wmfAPI*);
extern LIBWMF_EXPORT wmfRGB*       wmf_ipa_color (wmfAPI*,unsigned long);

extern char*        wmf_ipa_page_format (wmfAPI*,wmf_page_t);
extern unsigned int wmf_ipa_page_width  (wmfAPI*,wmf_page_t);
extern unsigned int wmf_ipa_page_height (wmfAPI*,wmf_page_t);

/* Other useful functions
 */
extern LIBWMF_EXPORT wmfRGB wmf_rgb_white (void);
extern LIBWMF_EXPORT wmfRGB wmf_rgb_black (void);
extern LIBWMF_EXPORT wmfRGB wmf_rgb_color (wmfAPI*,float,float,float);

/* Structure definitions
 */
struct _wmfRGB
{	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct _wmfBMP
{	U16 width;
	U16 height;

	void* data;
};

struct _wmfPen
{	U16 lopnStyle;

	double width;
	double height;

	wmfRGB lopnColor;
};

struct _wmfBrush
{	U16 lbStyle;
	U16 lbHatch;

	wmfRGB lbColor;
	wmfBMP bmp;
};

struct _wmfFont
{	U16 lfHeight;
	U16 lfWidth;

	S16 lfEscapement;
	S16 lfOrientation;

	U16 lfWeight;

	U8 lfItalic;
	U8 lfUnderline;
	U8 lfStrikeOut;
	U8 lfCharSet;
	U8 lfOutPrecision;
	U8 lfClipPrecision;
	U8 lfQuality;
	U8 lfPitchAndFamily;

	char* lfFaceName;

	void* user_data;
};

/**
 * API->font_data is a pointer to a wmfFontData. wmf_api_create () sets this up automatically, but
 * wmf_lite_create () does not. If you use wmf_lite_create () then you \b must create your own
 * wmfFontData. \b libwmflite requires you to define \p map and \p stringwidth functions but the
 * rest of these fields are ignored (they are only used by \b libwmf).
 */
struct _wmfFontData
{	/**
	 * Necessary field: exactly what the function does is irrelevant.
	 */
	wmfMap map;

	/**
	 * Necessary field: returns width of specified string in points, assuming (unstretched)
	 * font size of 1pt.
	 */
	wmfStringWidth stringwidth;

	/**
	 * A handle for data, unused by libwmflite
	 */
	void* user_data;
};

struct _wmfColorData
{	unsigned long max;
	unsigned long count;

	wmfRGB* rgb;
};

struct _wmfDC
{	void* userdata;

	wmfBrush* brush;
	wmfPen* pen;
	wmfFont* font;

	wmfRGB textcolor;
	wmfRGB bgcolor;

	U16 textalign;
	U16 bgmode;
	U16 polyfillmode;
	U16 charextra;
	U16 breakextra;

	U16 ROPmode;

	struct
	{	S32 Ox;
		S32 Oy;
		S32 width;
		S32 height;
	} Window;

	double pixel_width; /* Display pixel dimensions (inches) */
	double pixel_height;

	U16 map_mode;

	void* clip;
};

/* IPA info structures
 */
struct _wmfFlood_t
{	wmfDC* dc;

	wmfD_Coord pt;
	wmfRGB color;

	U16 type;

	double pixel_width;
	double pixel_height;
};

struct _wmfDrawPixel_t
{	wmfDC* dc;

	wmfD_Coord pt;
	wmfRGB color;

	double pixel_width;
	double pixel_height;
};

struct _wmfDrawArc_t
{	wmfDC* dc;

	wmfD_Coord TL;
	wmfD_Coord BR;

	wmfD_Coord start; /* draw_ellipse: (ignored) */
	wmfD_Coord end;
};

struct _wmfDrawLine_t
{	wmfDC* dc;

	wmfD_Coord from;
	wmfD_Coord to;
};

struct _wmfPolyLine_t
{	wmfDC* dc;

	wmfD_Coord* pt;

	U16 count;
};

struct _wmfPolyPoly_t
{	wmfDC* dc;

	wmfD_Coord** pt; /* pt[i][*] = points of ith polygon */

	U16* count;      /* points in ith polygon */
	U16  npoly;      /* number of polygons */
};

struct _wmfDrawRectangle_t
{	wmfDC* dc;

	wmfD_Coord TL;
	wmfD_Coord BR;

	float width; /* draw_rectangle: axes of corner ellipses; zero if un-rounded */
	float height;
};

struct _wmfPolyRectangle_t
{	wmfDC* dc;

	wmfD_Coord* TL; /* region_frame & region_paint: TL[count],BR[count] give the */
	wmfD_Coord* BR; /* final `extents'... */

	unsigned int count;

	float width;  /* region_frame: border thickness; zero otherwise */
	float height;
};

struct _wmfBMP_Read_t          /* Two means available for accessing BMP image:        */
{	long offset;           /* (1) position in source file of start of BMP;        *
                                * use API->bbuf.seek to set pos(ition), etc.          */
	long length;           /* (2) buffer of length length containing image of BMP */
	unsigned char* buffer;

	U16 width;  /* WMF player may preset these values; zero otherwise. */
	U16 height; /* Use caution - may be buggy... ?? [TODO]             */

	wmfBMP bmp;
};

struct _wmfBMP_Draw_t
{	wmfDC* dc;

	wmfD_Coord pt;
	wmfBMP bmp;

	U32 type;

	struct
	{	U16 x;
		U16 y;
		U16 w;
		U16 h;
	} crop;

	double pixel_width;
	double pixel_height;
};

struct _wmfROP_Draw_t
{	wmfDC* dc;

	wmfD_Coord TL;
	wmfD_Coord BR;

	U32 ROP;

	double pixel_width;
	double pixel_height;
};

struct _wmfDrawText_t
{	wmfDC* dc;

	wmfD_Coord pt;

	wmfD_Coord TL; /* Clip zone */
	wmfD_Coord BR;

	struct /* An estimated surround zone */
	{	wmfD_Coord TL;
		wmfD_Coord TR;
		wmfD_Coord BL;
		wmfD_Coord BR;
	} bbox;

	char* str;

	U16 flags;

	double font_height;
	double font_ratio;  /* width to height ratio */
};

struct _wmfUserData_t	/* TODO: Need to be careful with usage here; not all these are set by the player! */
{	wmfDC* dc;          /* dc is guaranteed */

	void* data;         /* data also, except for init */
};

struct _wmfFunctionReference
{	void (*device_open) (wmfAPI*);
	void (*device_close) (wmfAPI*);
	void (*device_begin) (wmfAPI*);
	void (*device_end) (wmfAPI*);

	void (*flood_interior) (wmfAPI*,wmfFlood_t*);
	void (*flood_exterior) (wmfAPI*,wmfFlood_t*);

	void (*draw_pixel) (wmfAPI*,wmfDrawPixel_t*);
	void (*draw_pie) (wmfAPI*,wmfDrawArc_t*);
	void (*draw_chord) (wmfAPI*,wmfDrawArc_t*);
	void (*draw_arc) (wmfAPI*,wmfDrawArc_t*);
	void (*draw_ellipse) (wmfAPI*,wmfDrawArc_t*);
	void (*draw_line) (wmfAPI*,wmfDrawLine_t*);
	void (*poly_line) (wmfAPI*,wmfPolyLine_t*);
	void (*draw_polygon) (wmfAPI*,wmfPolyLine_t*);
	void (*draw_polypolygon) (wmfAPI*,wmfPolyPoly_t*);
	void (*draw_rectangle) (wmfAPI*,wmfDrawRectangle_t*);

	void (*rop_draw) (wmfAPI*,wmfROP_Draw_t*);
	void (*bmp_draw) (wmfAPI*,wmfBMP_Draw_t*);
	void (*bmp_read) (wmfAPI*,wmfBMP_Read_t*);
	void (*bmp_free) (wmfAPI*,wmfBMP*);

	void (*draw_text) (wmfAPI*,wmfDrawText_t*);

	void (*udata_init) (wmfAPI*,wmfUserData_t*);
	void (*udata_copy) (wmfAPI*,wmfUserData_t*);
	void (*udata_set) (wmfAPI*,wmfUserData_t*);
	void (*udata_free) (wmfAPI*,wmfUserData_t*);

	void (*region_frame) (wmfAPI*,wmfPolyRectangle_t*);
	void (*region_paint) (wmfAPI*,wmfPolyRectangle_t*);
	void (*region_clip) (wmfAPI*,wmfPolyRectangle_t*);
};

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_IPA_H */
