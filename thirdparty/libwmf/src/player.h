/* libwmf ("player.h"): library for wmf conversion
   Copyright (C) 2000,2001 - various; see CREDITS, ChangeLog, and sources

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


#ifndef WMFPLAYER_H
#define WMFPLAYER_H

/* General defs
 */
typedef struct _wmfL_Coord  wmfL_Coord;
typedef struct _wmfRegion   wmfRegion;
typedef struct _wmfObject   wmfObject;
typedef struct _wmfPlayer_t wmfPlayer_t;
typedef struct _wmfRecord   wmfRecord;

typedef void (*pProcO)    (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,float,float);
typedef void (*pProcNonO) (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,float,float);

struct _wmfL_Coord
{	S32 x;
	S32 y;
};

struct _wmfRegion
{	unsigned int size;
	unsigned int numRects;

	U16 type;     /* NULL, SIMPLE or COMPLEX */

	wmfD_Rect* rects;
	wmfD_Rect  extents;
};

struct _wmfObject
{	int type;
	union
	{	wmfBrush brush;
		wmfPen	pen;
		wmfFont  font;

		int palette;

		wmfRegion rgn;
	} obj;
};

struct _wmfPlayer_t
{	wmfPen   default_pen;
	wmfBrush default_brush;
	wmfFont  default_font;

	wmfDC* dc; /* current dc */

	int dc_stack_maxlen;
	int dc_stack_length;
	wmfDC** dc_stack;

	wmfObject* objects;

	wmfRegion visible; /* I don't understand this... what is the purpose of `visible'? */

	wmfL_Coord current; /* Current position */

	wmfD_Coord D_TL;    /* Bounding box from Display perspective */
	wmfD_Coord D_BR;

	wmfD_Coord Viewport_Origin; /* Origin of Viewport */

	S32 Viewport_Width; /* Display extents	 */
	S32 Viewport_Height;

	unsigned char* Parameters; /* meta file parameter values */

	unsigned long flags;
};

struct _wmfRecord
{	unsigned long size;

	unsigned int function;

	unsigned char* parameter;

	long position;
};

/* In: defaults.h
 */
static void SetDefaults (wmfAPI*,wmfPen*,wmfBrush*,wmfFont*);

/* In: color.h
 */
static wmfRGB rgb (U16,U16);

/* In: coord.h
 */
static wmfL_Coord L_Coord (U16,U16);

static void D_Rect (wmfAPI*,wmfD_Rect*,U16,U16,U16,U16);

static void D_Coord_Register (wmfAPI*,wmfD_Coord,float);

static void WmfSetMapMode (wmfAPI*,U16 map_mode);

static double PixelWidth (wmfAPI*);
static double PixelHeight (wmfAPI*);

static wmfL_Coord wmf_L_Coord_translate (wmfAPI*,wmfD_Coord);
static wmfD_Coord wmf_D_Coord_translate (wmfAPI*,wmfL_Coord);

/* In: region.h
 */
static wmfD_Rect* rgn_memchk (wmfAPI*,wmfRegion*);

static unsigned int REGION_Coalesce (wmfRegion *pReg,unsigned int prevStart,unsigned int curStart);

static void REGION_RegionOp (wmfAPI*,wmfRegion*,wmfRegion*,wmfRegion*,pProcO,pProcNonO,pProcNonO);

static void REGION_SetExtents (wmfRegion*);

static void REGION_CopyRegion (wmfAPI*,wmfRegion*,wmfRegion*);

static void REGION_UnionRegion (wmfAPI*,wmfRegion*,wmfRegion*,wmfRegion*);
static void REGION_UnionO (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,float,float);
static void REGION_UnionNonO (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,float,float);

static void REGION_SubtractRegion (wmfAPI*,wmfRegion*,wmfRegion*,wmfRegion*);
static void REGION_SubtractO (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,float,float);
static void REGION_SubtractNonO1 (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,float,float);

static void REGION_IntersectRegion (wmfAPI*,wmfRegion*,wmfRegion*,wmfRegion*);
static void REGION_IntersectO (wmfAPI*,wmfRegion*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,wmfD_Rect*,float,float);

static void WmfSetRectRgn (wmfRegion*,wmfD_Rect*);
static void WmfCombineRgn (wmfAPI*,wmfRegion*,wmfRegion*,wmfRegion*,U16);

static void rect_merge (wmfAPI*,wmfRegion*,wmfD_Rect*,float,float);

/* In: clip.h
 */
static void Clipping (wmfAPI*,wmfRegion*,wmfRegion*,wmfD_Rect*,U16);

/* In: dc.h
 */
static wmfDC* dc_copy (wmfAPI*,wmfDC*);

static void   dc_stack_push (wmfAPI*,wmfDC*);
static wmfDC* dc_stack_pop (wmfAPI*);
static void   dc_stack_free (wmfAPI*);

/* In: record.h
 */
static U16 ParU16 (wmfAPI*,wmfRecord*,unsigned long);
static S16 ParS16 (wmfAPI*,wmfRecord*,unsigned long);
static S32 ParS32 (wmfAPI*,wmfRecord*,unsigned long);

static int PutParU16 (wmfAPI*,wmfRecord*,unsigned long,U16);

static wmfRecord OffsetRecord (wmfAPI*,wmfRecord*,unsigned long);

/* In: meta.h
 */

static int meta_mapmode (wmfAPI*,wmfRecord*);
static int meta_orgext (wmfAPI*,wmfRecord*);
static int meta_scale (wmfAPI*,wmfRecord*);
static int meta_moveto (wmfAPI*,wmfRecord*);
static int meta_flood (wmfAPI*,wmfRecord*);
static int meta_pixel (wmfAPI*,wmfRecord*);
static int meta_arc (wmfAPI*,wmfRecord*);
static int meta_ellipse (wmfAPI*,wmfRecord*);
static int meta_line (wmfAPI*,wmfRecord*);
static int meta_lines (wmfAPI*,wmfRecord*);
static int meta_polygon (wmfAPI*,wmfRecord*);
static int meta_polygons (wmfAPI*,wmfRecord*);
static int meta_round (wmfAPI*,wmfRecord*);
static int meta_rect (wmfAPI*,wmfRecord*);
static int meta_rgn_brush (wmfAPI*,wmfRecord*);
static int meta_rgn_paint (wmfAPI*,wmfRecord*);
static int meta_rgn_create (wmfAPI*,wmfRecord*);
static int meta_clip_select (wmfAPI*,wmfRecord*);
static int meta_clip_offset (wmfAPI*,wmfRecord*);
static int meta_clip_combine (wmfAPI*,wmfRecord*);
static int meta_dib_draw (wmfAPI*,wmfRecord*);
static int meta_dib_brush (wmfAPI*,wmfRecord*);
static int meta_rop_draw (wmfAPI*,wmfRecord*);
static int meta_dc_set (wmfAPI*,wmfRecord*);
static int meta_dc_color (wmfAPI*,wmfRecord*,wmfAttributes*);
static int meta_dc_select (wmfAPI*,wmfRecord*);
static int meta_dc_save (wmfAPI*,wmfRecord*);
static int meta_dc_restore (wmfAPI*,wmfRecord*);
static int meta_text (wmfAPI*,wmfRecord*);
static int meta_pen_create (wmfAPI*,wmfRecord*,wmfAttributes*);
static int meta_brush_create (wmfAPI*,wmfRecord*,wmfAttributes*);
static int meta_font_create (wmfAPI*,wmfRecord*);
static int meta_palette_create (wmfAPI*);
static int meta_delete (wmfAPI*,wmfRecord*);
static int meta_unused (wmfAPI*,wmfRecord*);

static void polypoly_construct (wmfAPI*,wmfPolyPoly_t*,wmfPolyLine_t*,U16);

static void diagnose_object (wmfAPI*,unsigned int,wmfObject*);

/* In: player.c
 */

static wmf_error_t WmfPlayMetaFile (wmfAPI*);

static U16 ParU16 (wmfAPI*,wmfRecord*,unsigned long);
static S16 ParS16 (wmfAPI*,wmfRecord*,unsigned long);
static S32 ParS32 (wmfAPI*,wmfRecord*,unsigned long);

static wmfRecord OffsetRecord (wmfAPI*,wmfRecord*,unsigned long);

#define PLAYER_SCANNED          (1 << 0)
#define PLAYER_PLAY             (1 << 1)
#define PLAYER_TLBR_D_SET       (1 << 2)

#define SCAN(API) ((((wmfPlayer_t*)((API)->player_data))->flags & PLAYER_PLAY) == 0)
#define PLAY(API)  (((wmfPlayer_t*)((API)->player_data))->flags & PLAYER_PLAY)

#define PLACEABLE(API)    ((API)->File->placeable)
#define DPI(API)          ((API)->File->pmh->Inch)
#define NUM_OBJECTS(API)  ((API)->File->wmfheader->NumOfObjects)
#define MAX_REC_SIZE(API) ((API)->File->wmfheader->MaxRecordSize)
#define FILE_SIZE(API)    ((API)->File->wmfheader->FileSize)

#define WMF_BBOX_LEFT(API)   ((API)->File->pmh->Left  )
#define WMF_BBOX_TOP(API)    ((API)->File->pmh->Top   )
#define WMF_BBOX_RIGHT(API)  ((API)->File->pmh->Right )
#define WMF_BBOX_BOTTOM(API) ((API)->File->pmh->Bottom)

#define POINT_TO_INCH(X) ((double) (X) / (double) 72  )
#define INCH_TO_POINT(X) ((double) (X) * (double) 72  )
#define MM_TO_INCH(X)    ((double) (X) / (double) 25.4)

#define SCAN_DIAGNOSTIC(API,MESG) if (SCAN ((API)) && DIAG ((API))) fputs (MESG,stderr);

#endif /* ! WMFPLAYER_H */
