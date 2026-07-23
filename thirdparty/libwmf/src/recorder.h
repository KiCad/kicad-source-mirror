/* libwmf ("recorder.h"): library for wmf conversion
   Copyright (C) 2002 Francis James Franklin <fjf@alinameridon.com>

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


#ifndef WMFRECORDER_H
#define WMFRECORDER_H

typedef struct _wmfCanvasPen    wmfCanvasPen;
typedef struct _wmfCanvasBrush  wmfCanvasBrush;
typedef struct _wmfCanvasFont   wmfCanvasFont;

typedef struct _wmfConstruct    wmfConstruct;
typedef struct _wmfRecordBox    wmfRecordBox;

struct _wmfCanvasPen
{
  unsigned short style;
  unsigned short width;

  wmfRGB color;
};

struct _wmfCanvasBrush
{
  unsigned short style;
  unsigned short hatch;

  wmfRGB color;
};

#ifdef CanvasFontBufferSize
#undef CanvasFontBufferSize
#endif
#define CanvasFontBufferSize 64

struct _wmfCanvasFont
{
  char name[CanvasFontBufferSize];

  unsigned short height;
  unsigned short width;
  unsigned short escapement;
  unsigned short orientation;
  unsigned short weight;
  unsigned short italic;
  unsigned short underline;
  unsigned short strikeout;
  unsigned short charset;
  unsigned short out;
  unsigned short clip;
  unsigned short quality;
  unsigned short pitch;
};

struct _wmfConstruct
{
  unsigned char * buffer;

  unsigned long length;
  unsigned long max;

  unsigned long * rec_offset;

  unsigned long rec_count;
  unsigned long rec_max;

  unsigned short NoObjects;
  unsigned long  MaxRecord; /* Maximum record size in *words* */

  unsigned short width;
  unsigned short height;
  unsigned short dpi;

  wmfCanvasPen   pen;
  wmfCanvasBrush brush;
  wmfCanvasFont  font;

  wmfCanvasPen   new_pen;
  wmfCanvasBrush new_brush;
  wmfCanvasFont  new_font;

  unsigned short polyfill;
  unsigned short background;

  wmfRGB bgcolor;
  wmfRGB textcolor;
};

struct _wmfRecordBox
{
  unsigned char * start;
  unsigned char * end;
  unsigned char * ptr;
};

static void s_convert_color (wmfRGB *, unsigned short *, unsigned short *);

static void s_rbox_set (wmfAPI *, wmfConstruct *, wmfRecordBox *, unsigned long);

static void s_rbox_skip_w (wmfAPI *, wmfRecordBox *);
static void s_rbox_ushort (wmfAPI *, wmfRecordBox *, unsigned long);
static void s_rbox_ulong  (wmfAPI *, wmfRecordBox *, unsigned long);

static void s_update_header (wmfAPI *, wmfConstruct *);
static void s_append_record (wmfAPI *, wmfConstruct *, wmfRecordBox *, unsigned long);

static int s_setnew_pen   (wmfAPI *, wmfConstruct *);
static int s_setnew_brush (wmfAPI *, wmfConstruct *);
static int s_setnew_font  (wmfAPI *, wmfConstruct *);

static int s_create_pen   (wmfAPI *, wmfConstruct *); /* use via s_setnew_pen   */
static int s_create_brush (wmfAPI *, wmfConstruct *); /* use via s_setnew_brush */
static int s_create_font  (wmfAPI *, wmfConstruct *); /* use via s_setnew_font  */

static int s_delete (wmfAPI *, wmfConstruct *, unsigned short); /* use via s_setnew_* */
static int s_select (wmfAPI *, wmfConstruct *, unsigned short); /* use via s_setnew_* */

static int s_set_polyfill   (wmfAPI *, wmfConstruct *);
static int s_set_background (wmfAPI *, wmfConstruct *);
static int s_set_bgcolor    (wmfAPI *, wmfConstruct *);
static int s_set_textcolor  (wmfAPI *, wmfConstruct *);

static int s_moveto (wmfAPI *, wmfConstruct *, unsigned short, unsigned short);
static int s_lineto (wmfAPI *, wmfConstruct *, unsigned short, unsigned short);

static unsigned long s_bmp_query (void);

#endif /* ! WMFRECORDER_H */
