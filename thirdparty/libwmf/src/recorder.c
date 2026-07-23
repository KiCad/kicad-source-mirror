/* libwmf (recorder.c): library for wmf conversion
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


#ifdef HAVE_CONFIG_H
#include "wmfconfig.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wmfdefs.h"
#include "metadefs.h"

#include "libwmf/canvas.h"

#include "recorder.h"

static void s_convert_color (wmfRGB * color, unsigned short * rg, unsigned short * b)
{
  if (color && rg && b)
    {
      *rg = ((unsigned short) (color->g) << 8) | (unsigned short) (color->r);
      *b  =  (unsigned short) (color->b);
    }
}

static void s_rbox_set (wmfAPI * API, wmfConstruct * construct,
			wmfRecordBox * rbox, unsigned long n)
{
  if (n >= construct->rec_count)
    {
      WMF_ERROR (API, "Hmm. Record out of range...");
      API->err = wmf_E_Glitch;
      rbox->start = NULL;
      rbox->end   = NULL;
      rbox->ptr   = NULL;
      return;
    }
  rbox->start = construct->buffer + construct->rec_offset[n  ];
  rbox->end   = construct->buffer + construct->rec_offset[n+1];
  rbox->ptr   = rbox->start;
}

static void s_rbox_skip_w (wmfAPI * API, wmfRecordBox * rbox)
{
  if (rbox->end - rbox->ptr < 2)
    {
      WMF_ERROR (API, "Hmm. Record out of range...");
      API->err = wmf_E_Glitch;
      return;
    }
  rbox->ptr++;
  rbox->ptr++;
}

static void s_rbox_ushort (wmfAPI * API, wmfRecordBox * rbox, unsigned long us)
{
  if (rbox->end - rbox->ptr < 2)
    {
      WMF_ERROR (API, "Hmm. Record out of range...");
      API->err = wmf_E_Glitch;
      return;
    }
  *rbox->ptr++ = (unsigned char) ( us       & 0xff);
  *rbox->ptr++ = (unsigned char) ((us >> 8) & 0xff);
}

static void s_rbox_ulong (wmfAPI * API, wmfRecordBox * rbox, unsigned long ul)
{
  if (rbox->end - rbox->ptr < 4)
    {
      WMF_ERROR (API, "Hmm. Record out of range...");
      API->err = wmf_E_Glitch;
      return;
    }
  *rbox->ptr++ = (unsigned char) ( ul        & 0xff);
  *rbox->ptr++ = (unsigned char) ((ul >>  8) & 0xff);
  *rbox->ptr++ = (unsigned char) ((ul >> 16) & 0xff);
  *rbox->ptr++ = (unsigned char) ((ul >> 24) & 0xff);
}

static void s_update_header (wmfAPI * API, wmfConstruct * construct)
{
  wmfRecordBox rbox;
  
  s_rbox_set (API, construct, &rbox, 1);

  s_rbox_skip_w (API, &rbox);
  s_rbox_skip_w (API, &rbox);
  s_rbox_skip_w (API, &rbox);

  s_rbox_ulong  (API, &rbox, construct->length / 2);
  s_rbox_ushort (API, &rbox, construct->NoObjects);
  s_rbox_ulong  (API, &rbox, construct->MaxRecord);
}

static void s_append_record (wmfAPI * API, wmfConstruct * construct, 
			     wmfRecordBox * rbox, unsigned long length)
{
  unsigned char * uc_more = 0;
  unsigned long * ul_more = 0;
  unsigned long bytes;

  if (length & 1)
    {
      WMF_DEBUG (API, "odd length? records should have even length. adding 1...");
      length++;
    }

  if (construct->length + length > construct->max)
    {
      bytes = (construct->length + length) - construct->max;
      if (bytes & 0x7f)
	{
	  bytes |= 0x7f;
	  bytes++;
	}
      uc_more = (unsigned char *) wmf_realloc (API, construct->buffer, construct->max + bytes);
      if (ERR (API))
	{
	  WMF_DEBUG (API, "bailing...");
	  return;
	}
      construct->buffer = uc_more;
      construct->max += bytes;
    }
  if (construct->rec_count == construct->rec_max)
    {
      bytes = (construct->rec_max + 1 + 16) * sizeof (unsigned long);
      ul_more = (unsigned long *) wmf_realloc (API, construct->rec_offset, bytes);
      if (ERR (API))
	{
	  WMF_DEBUG (API, "bailing...");
	  return;
	}
      construct->rec_offset = ul_more;
      construct->rec_max += 16;
    }

  if (construct->MaxRecord < (length / 2)) construct->MaxRecord = length / 2;

  construct->length += length;

  construct->rec_count++;
  construct->rec_offset[construct->rec_count] = construct->length;

  s_rbox_set (API, construct, rbox, construct->rec_count-1);
}

/**
 * Get a handle for creating a new metafile.
 * 
 * @param API     the API handle
 * @param width   width in 'dots' (a.k.a. 'twips'?)
 * @param height  height in 'dots' (a.k.a. 'twips'?)
 * @param dpi     dots per inch: one of 1440, 2880, 720, or 360
 * 
 * In case you want to use simple drawing routines to create your own metafile; all
 * drawing commands expect a wmfCanvas handle as well as the API handle.
 * 
 * @return Returns 0 on failure.
 */
wmfCanvas * wmf_canvas (wmfAPI * API, unsigned short width, unsigned short height,
			unsigned short dpi)
{
  unsigned short checksum;

  wmfRecordBox rbox;
  
  wmfConstruct * construct = 0;

  if ((width & 0x7fff) != width)
    {
      WMF_ERROR (API, "Image too large! (width > 0x7fff)");
      return 0;
    }
  if ((height & 0x7fff) != height)
    {
      WMF_ERROR (API, "Image too large! (height > 0x7fff)");
      return 0;
    }
  if ((dpi != 1440) && (dpi != 2880) && (dpi != 720) && (dpi != 360))
    {
      WMF_ERROR (API, "invalid dpi! expected one of 1440, 2880, 720, or 360");
      return 0;
    }

  construct = (wmfConstruct *) wmf_malloc (API, sizeof (wmfConstruct));
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return 0;
    }

  construct->buffer = (unsigned char *) wmf_malloc (API, 128);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      wmf_free (API, construct);
      return 0;
    }
  construct->length = 2 * 11; /* size in bytes of placeable meta header */
  construct->max = 128;

  construct->rec_offset = (unsigned long *) wmf_malloc (API, 16);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      wmf_free (API, construct->buffer);
      wmf_free (API, construct);
      return 0;
    }
  construct->rec_offset[0] = 0;
  construct->rec_count = 1;
  construct->rec_max = 15;
  construct->rec_offset[construct->rec_count] = construct->length;

  construct->width  = width;
  construct->height = height;
  construct->dpi    = dpi;

  s_rbox_set (API, construct, &rbox, 0);

  checksum = 0xcdd7 ^ 0x9ac6 ^ width ^ height ^ dpi; /* I think... */

  s_rbox_ulong  (API, &rbox, 0x9ac6cdd7); /* Key */
  s_rbox_ushort (API, &rbox, 0         ); /* Handle */
  s_rbox_ushort (API, &rbox, 0         ); /* Left */
  s_rbox_ushort (API, &rbox, 0         ); /* Top */
  s_rbox_ushort (API, &rbox, width     ); /* Right */
  s_rbox_ushort (API, &rbox, height    ); /* Bottom */
  s_rbox_ushort (API, &rbox, dpi       ); /* Inch */
  s_rbox_ulong  (API, &rbox, 0         ); /* Reserved */
  s_rbox_ushort (API, &rbox, checksum  ); /* Checksum */

  s_append_record (API, construct, &rbox, 2 * 9);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      wmf_free (API, construct->rec_offset);
      wmf_free (API, construct->buffer);
      wmf_free (API, construct);
      return 0;
    }
  s_rbox_ushort (API, &rbox,      2); /* Type */
  s_rbox_ushort (API, &rbox,      9); /* HeaderSize */
  s_rbox_ushort (API, &rbox, 0x0300); /* Version */
  s_rbox_ulong  (API, &rbox,      0); /* Size */
  s_rbox_ushort (API, &rbox,      0); /* NoObjects */
  s_rbox_ulong  (API, &rbox,      0); /* MaxRecord */
  s_rbox_ushort (API, &rbox,      0); /* NoParameters */

  construct->NoObjects =  3;
  construct->MaxRecord = 11;

  s_update_header (API, construct);

  construct->new_pen.style = PS_NULL;
  construct->new_pen.width = 1;
  construct->new_pen.color = wmf_black;

  construct->pen = construct->new_pen;

  construct->new_brush.style = BS_NULL;
  construct->new_brush.hatch = HS_HORIZONTAL;
  construct->new_brush.color = wmf_black;

  construct->brush = construct->new_brush;

  strcpy (construct->new_font.name, "Times New Roman");

  construct->new_font.height      = 100; /* size (in tenths of points) */
  construct->new_font.width       =   0; /* Do we care? */
  construct->new_font.escapement  =   0; /* Do we care? */
  construct->new_font.orientation =   0; /* text rotation (in tenths of a degree ??) */
  construct->new_font.weight      = 400; /* bold, normal, light, etc. */
  construct->new_font.italic      =   0; /* italic, oblique, etc. */
  construct->new_font.underline   =   0; /* underline */
  construct->new_font.strikeout   =   0; /* strike-through */
  construct->new_font.charset     =   0; /* character set, encoding,... */
  construct->new_font.out         =   0; /* font outlining */
  construct->new_font.clip        =   0; /* Do we care? */
  construct->new_font.quality     =   0; /* Do we care? */
  construct->new_font.pitch       =  16; /* italics angle ?? */

  construct->font = construct->new_font;

  s_create_pen   (API, construct); /* order of object creation is important! */
  s_create_brush (API, construct);
  s_create_font  (API, construct);

  s_select (API, construct, 0);
  s_select (API, construct, 1);
  s_select (API, construct, 2);

  construct->polyfill = ALTERNATE;
  s_set_polyfill (API, construct);

  construct->background = TRANSPARENT;
  s_set_background (API, construct);

  construct->bgcolor = wmf_white;
  s_set_bgcolor (API, construct);

  construct->textcolor = wmf_black;
  s_set_textcolor (API, construct);

  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      wmf_free (API, construct->rec_offset);
      wmf_free (API, construct->buffer);
      wmf_free (API, construct);
      return 0;
    }
  return (wmfCanvas *) construct;
}

/**
 * Last command when creating a new metafile; returns the metafile you have drawn.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param buffer    metafile buffer return
 * @param length    metafile length return
 * 
 * Cleans up memory etc. associated with the canvas process, except of course for the
 * new metafile itself which gets returned.
 * 
 * @return Returns 0 on failure, *buffer on success
 */
unsigned char * wmf_canvas_done (wmfAPI * API, wmfCanvas * canvas,
				 unsigned char ** buffer, unsigned long * length)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (buffer == 0) || (length == 0)) return 0;

  /* TODO: s_delete (API, construct, 2); */
  s_delete (API, construct, 1);
  s_delete (API, construct, 0);

  s_update_header (API, construct);

  /* TODO */

  *buffer = construct->buffer;
  *length = construct->length;

  wmf_free (API, construct->rec_offset);
  wmf_free (API, construct);

  return *buffer;
}

/**
 * Change current pen style.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param line      one of PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT, PS_DASHDOTDOT, PS_NULL, PS_INSIDEFRAME, PS_USERSTYLE, or PS_ALTERNATE
 * @param endcap    one of PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE, or PS_ENDCAP_FLAT
 * @param join      one of PS_JOIN_ROUND, PS_JOIN_BEVEL, or PS_JOIN_MITER
 * @param width     stroke width
 * @param color     stroke color
 * 
 * Pen settings to be used with next drawing command.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_pen (wmfAPI * API, wmfCanvas * canvas,
			unsigned short line, unsigned short endcap, unsigned short join,
			unsigned short width, wmfRGB color)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  line &= 0x0f;
  if (line > 8)
    {
      WMF_DEBUG (API, "Attempt to set unsupported line style.");
      line = 0;
    }
  endcap &= 0x0f00;
  if (endcap > 0x0200)
    {
      WMF_DEBUG (API, "Attempt to set unsupported endcap style.");
      endcap = 0;
    }
  join &= 0xf000;
  if (join > 0x2000)
    {
      WMF_DEBUG (API, "Attempt to set unsupported join style.");
      join = 0;
    }
  construct->new_pen.style = line | endcap | join;
  if (width == 0)
    {
      WMF_DEBUG (API, "Pen width is 0? Setting to 1.");
      width = 1;
    }
  construct->new_pen.width = width;
  construct->new_pen.color = color;

  return 0;
}

/**
 * Change current brush style.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param style     one of BS_SOLID, BS_NULL, or BS_HATCHED
 * @param hatch     one of HS_HORIZONTAL, HS_VERTICAL, HS_FDIAGONAL, HS_BDIAGONAL, HS_CROSS or HS_DIAGCROSS
 * @param color     stroke color
 * 
 * Brush settings to be used with next drawing command.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_brush (wmfAPI * API, wmfCanvas * canvas,
			  unsigned short style, unsigned short hatch, wmfRGB color)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (style > 2)
    {
      WMF_DEBUG (API, "Attempt to set unsupported brush style.");
      style = 0;
    }
  construct->new_brush.style = style;

  if (hatch > 5)
    {
      WMF_DEBUG (API, "Attempt to set unsupported hatch brush.");
      hatch = 0;
    }
  construct->new_brush.hatch = hatch;
  construct->new_brush.color = color;

  return 0;
}

/**
 * Change current font.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param name      font name
 * @param size      font height (in twips)
 * @param orient    text rotation in tenths of degrees
 * @param weight    400 for normal text, 700 for bold
 * @param italic    0 for normal text, 1 for oblique/italic
 * @param strike    0 for normal text, 1 for strike-through
 * @param uscore    0 for normal text, 1 for underscore (I think; and 2 for double uscore ??)
 * @param encode    encoding; 0 for most fonts, 2 for symbol fonts; presumably others...
 * @param pitch     italic angle; usually 0 (webdings), 16 (times), 32 (arial) or 48 (courier)
 * 
 * Font settings to be used with next drawing command.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_font (wmfAPI * API, wmfCanvas * canvas, const char * name,
			 unsigned short size,
			 unsigned short orient, unsigned short weight, unsigned short italic,
			 unsigned short strike, unsigned short uscore, unsigned short encode,
			 unsigned short pitch)
{
  int length;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (name == 0)) return -1;

  if (orient >= 3600)
    {
      WMF_DEBUG (API, "orientation exceeds 360 degrees; reducing to 0.");
      orient = 0;
    }
  if (italic > 1)
    {
      WMF_DEBUG (API, "italic set unusually high; reducing to 1.");
      italic = 1;
    }
  if (strike > 1)
    {
      WMF_DEBUG (API, "strike-through set unusually high; reducing to 1.");
      strike = 1;
    }
  if (uscore > 2)
    {
      WMF_DEBUG (API, "underscore set unusually high; reducing to 2.");
      uscore = 2;
    }
  if (pitch > 48)
    {
      WMF_DEBUG (API, "pitch set unusually high; reducing to 48.");
      pitch = 48;
    }

  length = strlen (name);
  if (length >= CanvasFontBufferSize)
    {
      WMF_DEBUG (API, "font name is too long; reducing.");
      length = CanvasFontBufferSize - 1;
    }
  memset (construct->new_font.name, 0, CanvasFontBufferSize);
  strncpy (construct->new_font.name, name, length);
  construct->new_font.name[length] = 0;

  construct->new_font.height      = size;
  construct->new_font.orientation = orient;
  construct->new_font.weight      = weight;
  construct->new_font.italic      = italic;
  construct->new_font.underline   = uscore;
  construct->new_font.strikeout   = strike;
  construct->new_font.charset     = encode;
  construct->new_font.pitch       = pitch;

  return 0;
}

static int s_setnew_pen (wmfAPI * API, wmfConstruct * construct)
{
  if (ERR (API)) return -1;

  if (memcmp (&(construct->pen), &(construct->new_pen), sizeof (wmfCanvasPen)) == 0)
    {
      return -1;
    }
  construct->pen = construct->new_pen;

  if (s_delete (API, construct, 0) == 0)
    if (s_create_pen (API, construct) == 0)
      if (s_select (API, construct, 0) == 0)
	return 0;

  return -1;
}

static int s_setnew_brush (wmfAPI * API, wmfConstruct * construct)
{
  if (ERR (API)) return -1;

  if (memcmp (&(construct->brush), &(construct->new_brush), sizeof (wmfCanvasBrush)) == 0)
    {
      return -1;
    }
  construct->brush = construct->new_brush;

  if (s_delete (API, construct, 1) == 0)
    if (s_create_brush (API, construct) == 0)
      if (s_select (API, construct, 1) == 0)
	return 0;

  return -1;
}

static int s_setnew_font (wmfAPI * API, wmfConstruct * construct)
{
  if (ERR (API)) return -1;

  if (memcmp (&(construct->font), &(construct->new_font), sizeof (wmfCanvasFont)) == 0)
    {
      return -1;
    }
  construct->font = construct->new_font;

  if (s_delete (API, construct, 2) == 0)
    if (s_create_font (API, construct) == 0)
      if (s_select (API, construct, 2) == 0)
	return 0;

  return -1;
}

static int s_create_pen (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 5;

  unsigned short rg = 0;
  unsigned short b = 0;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_convert_color (&(construct->pen.color), &rg, &b);

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_CREATEPENINDIRECT);

  s_rbox_ushort (API, &rbox, construct->pen.style);
  s_rbox_ushort (API, &rbox, construct->pen.width);
  s_rbox_ushort (API, &rbox, construct->pen.width); /* this should be ignored */
  s_rbox_ushort (API, &rbox, rg);
  s_rbox_ushort (API, &rbox, b);

  return 0;
}

static int s_create_brush (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 4;

  unsigned short rg = 0;
  unsigned short b = 0;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_convert_color (&(construct->brush.color), &rg, &b);

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_CREATEBRUSHINDIRECT);

  s_rbox_ushort (API, &rbox, construct->brush.style);
  s_rbox_ushort (API, &rbox, rg);
  s_rbox_ushort (API, &rbox, b);
  s_rbox_ushort (API, &rbox, construct->brush.hatch);
  /* ?? something missing? */

  return 0;
}

static int s_create_font (wmfAPI * API, wmfConstruct * construct)
{
  short size;

  unsigned short i;
  unsigned short cc;
  unsigned short length = (strlen (construct->font.name) + 1) / 2;

  unsigned long Size = 3 + 9 + length;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_CREATEFONTINDIRECT);

  size = - (short) (construct->font.height);
  s_rbox_ushort (API, &rbox, (unsigned short) size);

  s_rbox_ushort (API, &rbox, construct->font.width);
  s_rbox_ushort (API, &rbox, construct->font.escapement);
  s_rbox_ushort (API, &rbox, construct->font.orientation);
  s_rbox_ushort (API, &rbox, construct->font.weight);
  s_rbox_ushort (API, &rbox, construct->font.italic    | (construct->font.underline << 8));
  s_rbox_ushort (API, &rbox, construct->font.strikeout | (construct->font.charset   << 8));
  s_rbox_ushort (API, &rbox, construct->font.out       | (construct->font.clip      << 8));
  s_rbox_ushort (API, &rbox, construct->font.quality   | (construct->font.pitch     << 8));

  for (i = 0; i < length; i++)
    {
      cc  =  (unsigned short) construct->font.name[2*i  ];
      cc |= ((unsigned short) construct->font.name[2*i+1]) << 8;

      s_rbox_ushort (API, &rbox, cc);
    }
  return 0;
}

static int s_delete (wmfAPI * API, wmfConstruct * construct, unsigned short oid)
{
  unsigned long Size = 3 + 1;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_DELETEOBJECT);

  s_rbox_ushort (API, &rbox, oid);

  return 0;
}

static int s_select (wmfAPI * API, wmfConstruct * construct, unsigned short oid)
{
  unsigned long Size = 3 + 1;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_SELECTOBJECT);

  s_rbox_ushort (API, &rbox, oid);

  return 0;
}

/**
 * Set polygon fill mode.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param mode      one of ALTERNATE or WINDING
 * 
 * Set polygon fill mode.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_polyfill (wmfAPI * API, wmfCanvas * canvas, unsigned short mode)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (ERR (API)) return -1;

  if ((mode != ALTERNATE) && (mode != WINDING))
    {
      WMF_ERROR (API, "Unexpected polygon fill mode! Expected one of ALTERNATE or WINDING");
      return 0;
    }
  if (construct->polyfill == mode) return 0;

  construct->polyfill = mode;
  return s_set_polyfill (API, construct);
}

static int s_set_polyfill (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 1;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_SETPOLYFILLMODE);

  s_rbox_ushort (API, &rbox, construct->polyfill);

  return 0;
}

/**
 * Set background mode.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param mode      one of TRANSPARENT or OPAQUE
 * 
 * Set background mode.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_background (wmfAPI * API, wmfCanvas * canvas, unsigned short mode)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (ERR (API)) return -1;

  if ((mode != TRANSPARENT) && (mode != OPAQUE))
    {
      WMF_ERROR (API, "Unexpected background mode! Expected one of TRANSPARENT or OPAQUE");
      return -1;
    }
  if (construct->background == mode) return 0;

  construct->background = mode;
  return s_set_background (API, construct);
}

static int s_set_background (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 1;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_SETBKMODE);

  s_rbox_ushort (API, &rbox, construct->background);

  return 0;
}

/**
 * Set background color.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param color     background color
 * 
 * Set background color.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_bgcolor (wmfAPI * API, wmfCanvas * canvas, wmfRGB color)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (ERR (API)) return -1;

  if ((construct->bgcolor.r == color.r) &&
      (construct->bgcolor.g == color.g) &&
      (construct->bgcolor.b == color.b)) return 0;

  construct->bgcolor = color;
  return s_set_bgcolor (API, construct);
}

static int s_set_bgcolor (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 2;

  unsigned short rg = 0;
  unsigned short b = 0;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_convert_color (&(construct->bgcolor), &rg, &b);

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_SETBKCOLOR);

  s_rbox_ushort (API, &rbox, rg);
  s_rbox_ushort (API, &rbox, b);

  return 0;
}

/**
 * Set text color.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param color     text color
 * 
 * Set text color.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_set_textcolor (wmfAPI * API, wmfCanvas * canvas, wmfRGB color)
{
  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (ERR (API)) return -1;

  if ((construct->textcolor.r == color.r) &&
      (construct->textcolor.g == color.g) &&
      (construct->textcolor.b == color.b)) return 0;

  construct->textcolor = color;
  return s_set_textcolor (API, construct);
}

static int s_set_textcolor (wmfAPI * API, wmfConstruct * construct)
{
  unsigned long Size = 3 + 2;

  unsigned short rg = 0;
  unsigned short b = 0;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_convert_color (&(construct->textcolor), &rg, &b);

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_SETTEXTCOLOR);

  s_rbox_ushort (API, &rbox, rg);
  s_rbox_ushort (API, &rbox, b);

  return 0;
}

/**
 * Draw a line.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param x1        x-coordinate of start point
 * @param y1        y-coordinate of start point
 * @param x2        x-coordinate of end point
 * @param y2        y-coordinate of end point
 * 
 * Draw line from (x1,y1) to (x2,y2).
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_line (wmfAPI * API, wmfCanvas * canvas,
		     unsigned short x1, unsigned short y1,
		     unsigned short x2, unsigned short y2)
{
#if 0  
  unsigned long Size = 3 + 2;

  wmfRecordBox rbox;
#endif

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  if (ERR (API)) return -1;

  if (((x1 & 0x7fff) != x1) || ((x2 & 0x7fff) != x2))
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if (((y1 & 0x7fff) != y1) || ((y2 & 0x7fff) != y2))
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  if (s_moveto (API, construct, x1, y1) == 0)
    if (s_lineto (API, construct, x2, y2) == 0)
      return 0;

  return -1;
}

static int s_moveto (wmfAPI * API, wmfConstruct * construct, unsigned short x, unsigned short y)
{
  unsigned long Size = 3 + 2;

  wmfRecordBox rbox;

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_MOVETO);

  s_rbox_ushort (API, &rbox, y);
  s_rbox_ushort (API, &rbox, x);

  return 0;
}

static int s_lineto (wmfAPI * API, wmfConstruct * construct, unsigned short x, unsigned short y)
{
  unsigned long Size = 3 + 2;

  wmfRecordBox rbox;

  s_setnew_pen (API, construct);

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_LINETO);

  s_rbox_ushort (API, &rbox, y);
  s_rbox_ushort (API, &rbox, x);

  return 0;
}

/**
 * Draw a rounded rectangle.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param left      lower x-coordinate
 * @param top       lower y-coordinate
 * @param right     upper x-coordinate
 * @param bottom    upper y-coordinate
 * @param rx        x-axis of corner ellipse
 * @param ry        y-axis of corner ellipse
 * 
 * Draw a rounded rectangle. Coordinate origin is at the top-left, y increasing down.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_roundrect (wmfAPI * API, wmfCanvas * canvas,
			  unsigned short left,  unsigned short top,
			  unsigned short right, unsigned short bottom,
			  unsigned short rx,    unsigned short ry)
{
  unsigned long Size = 3 + 6;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  s_setnew_pen   (API, construct);
  s_setnew_brush (API, construct);

  if (ERR (API)) return -1;

  if (((left & 0x7fff) != left) || ((right & 0x7fff) != right) || ((rx & 0x7fff) != rx))
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if (((top & 0x7fff) != top) || ((bottom & 0x7fff) != bottom) || ((ry & 0x7fff) != ry))
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_ROUNDRECT);

  s_rbox_ushort (API, &rbox, ry);
  s_rbox_ushort (API, &rbox, rx);
  s_rbox_ushort (API, &rbox, bottom);
  s_rbox_ushort (API, &rbox, right);
  s_rbox_ushort (API, &rbox, top);
  s_rbox_ushort (API, &rbox, left);

  return 0;
}

/**
 * Draw a rectangle.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param left      lower x-coordinate
 * @param top       lower y-coordinate
 * @param right     upper x-coordinate
 * @param bottom    upper y-coordinate
 * 
 * Draw a rectangle. Coordinate origin is at the top-left, y increasing down.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_rect (wmfAPI * API, wmfCanvas * canvas,
		     unsigned short left,  unsigned short top,
		     unsigned short right, unsigned short bottom)
{
  unsigned long Size = 3 + 4;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  s_setnew_pen   (API, construct);
  s_setnew_brush (API, construct);

  if (ERR (API)) return -1;

  if (((left & 0x7fff) != left) || ((right & 0x7fff) != right))
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if (((top & 0x7fff) != top) || ((bottom & 0x7fff) != bottom))
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_RECTANGLE);

  s_rbox_ushort (API, &rbox, bottom);
  s_rbox_ushort (API, &rbox, right);
  s_rbox_ushort (API, &rbox, top);
  s_rbox_ushort (API, &rbox, left);

  return 0;
}

/**
 * Draw an ellipse.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param left      lower x-coordinate
 * @param top       lower y-coordinate
 * @param right     upper x-coordinate
 * @param bottom    upper y-coordinate
 * 
 * Draw an ellipse in the given bounding box. Coordinate origin is at the top-left,
 * y increasing down.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_ellipse (wmfAPI * API, wmfCanvas * canvas,
			unsigned short left,  unsigned short top,
			unsigned short right, unsigned short bottom)
{
  unsigned long Size = 3 + 4;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  s_setnew_pen   (API, construct);
  s_setnew_brush (API, construct);

  if (ERR (API)) return -1;

  if (((left & 0x7fff) != left) || ((right & 0x7fff) != right))
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if (((top & 0x7fff) != top) || ((bottom & 0x7fff) != bottom))
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_ELLIPSE);

  s_rbox_ushort (API, &rbox, bottom);
  s_rbox_ushort (API, &rbox, right);
  s_rbox_ushort (API, &rbox, top);
  s_rbox_ushort (API, &rbox, left);

  return 0;
}

/**
 * Draw an arc, open or fill as chord or pie.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param left      lower x-coordinate
 * @param top       lower y-coordinate
 * @param right     upper x-coordinate
 * @param bottom    upper y-coordinate
 * @param x1        x-coordinate of start of arc
 * @param y1        y-coordinate of start of arc
 * @param x2        x-coordinate of end of arc
 * @param y2        y-coordinate of end of arc
 * @param type      arc type (open, chord or pie)
 * 
 * Draw an arc in the given bounding box, from (x1,y1) to (x2,y2).
 * Coordinate origin is at the top-left, y increasing down.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_arc (wmfAPI * API, wmfCanvas * canvas,
		    unsigned short left,  unsigned short top,
		    unsigned short right, unsigned short bottom,
		    unsigned short x1,    unsigned short y1,
		    unsigned short x2,    unsigned short y2, wmf_canvas_arc_t type)
{
  unsigned long Size = 3 + 8;

  unsigned short Function;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if (construct == 0) return -1;

  switch (type)
    {
    default:
    case wmf_CA_open:
      Function = META_ARC;
      s_setnew_pen   (API, construct);
      break;
    case wmf_CA_chord:
      Function = META_CHORD;
      s_setnew_pen   (API, construct);
      s_setnew_brush (API, construct);
      break;
    case wmf_CA_pie:
      Function = META_PIE;
      s_setnew_pen   (API, construct);
      s_setnew_brush (API, construct);
      break;
    }

  if (ERR (API)) return -1;

  if (((left  & 0x7fff) != left ) ||
      ((right & 0x7fff) != right) ||
      ((x1    & 0x7fff) != x1   ) ||
      ((x2    & 0x7fff) != x2   ))
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if (((top    & 0x7fff) != top   ) ||
      ((bottom & 0x7fff) != bottom) ||
      ((y1     & 0x7fff) != y1    ) ||
      ((y2     & 0x7fff) != y2    ))
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, Function);

  s_rbox_ushort (API, &rbox, y1);
  s_rbox_ushort (API, &rbox, x1);
  s_rbox_ushort (API, &rbox, y2);
  s_rbox_ushort (API, &rbox, x2);

  s_rbox_ushort (API, &rbox, bottom);
  s_rbox_ushort (API, &rbox, right);
  s_rbox_ushort (API, &rbox, top);
  s_rbox_ushort (API, &rbox, left);

  return 0;
}

/**
 * Draw a line connecting a sequence of points.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param x         array of N x-coordinates
 * @param y         array of N y-coordinates
 * @param N         number of points on line
 * 
 * Draw a line connecting a sequence of points.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_polyline (wmfAPI * API, wmfCanvas * canvas,
			 unsigned short * x, unsigned short * y, unsigned short N)
{
  unsigned long Size = 3 + 1 + 2 * (unsigned long) N;

  unsigned short i;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (x == 0) || (y == 0) || (N < 2)) return -1;

  s_setnew_pen (API, construct);

  if (ERR (API)) return -1;

  for (i = 0; i < N; i++)
    {
      if ((x[i] & 0x7fff) != x[i])
	{
	  WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
	  Size = 0;
	  break;
	}
      if ((y[i] & 0x7fff) != y[i])
	{
	  WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
	  Size = 0;
	  break;
	}
    }
  if (Size == 0) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_POLYLINE);

  s_rbox_ushort (API, &rbox, N);
  for (i = 0; i < N; i++)
    {
      s_rbox_ushort (API, &rbox, x[i]);
      s_rbox_ushort (API, &rbox, y[i]);
    }
  return 0;
}

/**
 * Draw a polygon.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param x         array of N x-coordinates
 * @param y         array of N y-coordinates
 * @param N         number of points of polygon
 * 
 * Draw a polygon.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_polygon (wmfAPI * API, wmfCanvas * canvas,
			unsigned short * x, unsigned short * y, unsigned short N)
{
  unsigned long Size = 3 + 1 + 2 * (unsigned long) N;

  unsigned short i;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (x == 0) || (y == 0) || (N < 3)) return -1;

  s_setnew_pen   (API, construct);
  s_setnew_brush (API, construct);

  if (ERR (API)) return -1;

  for (i = 0; i < N; i++)
    {
      if ((x[i] & 0x7fff) != x[i])
	{
	  WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
	  Size = 0;
	  break;
	}
      if ((y[i] & 0x7fff) != y[i])
	{
	  WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
	  Size = 0;
	  break;
	}
    }
  if (Size == 0) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_POLYGON);

  s_rbox_ushort (API, &rbox, N);
  for (i = 0; i < N; i++)
    {
      s_rbox_ushort (API, &rbox, x[i]);
      s_rbox_ushort (API, &rbox, y[i]);
    }
  return 0;
}

/**
 * Draw a set of polygons.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param count     number of polygons [i = 0..count-1]
 * @param x         x[i] is array of N x-coordinates in ith polygon
 * @param y         y[i] is array of N y-coordinates in ith polygon
 * @param N         N[i] is number of points of ith polygon
 * 
 * Draw a set of polygons.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_polygons (wmfAPI * API, wmfCanvas * canvas, unsigned short count,
			 unsigned short ** x, unsigned short ** y, unsigned short * N)
{
  unsigned long Size = 3 + 1 + (unsigned long) count;

  unsigned short i;
  unsigned short j;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (x == 0) || (y == 0) || (count == 0)) return -1;

  for (j = 0; j < count; j++)
    {
      if ((x[j] == 0) || (y[j] == 0) || (N[j] < 3))
	{
	  Size = 0;
	  break;
	}
      Size += 2 * (unsigned long) N[j];
      if (Size > 0x7fffffff)
	{
	  Size = 0;
	  break;
	}
    }
  if (Size == 0) return -1;

  s_setnew_pen   (API, construct);
  s_setnew_brush (API, construct);

  if (ERR (API)) return -1;

  for (j = 0; j < count; j++)
    for (i = 0; i < N[j]; i++)
      {
	if ((x[j][i] & 0x7fff) != x[j][i])
	  {
	    WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
	    Size = 0;
	    break;
	  }
	if ((y[j][i] & 0x7fff) != y[j][i])
	  {
	    WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
	    Size = 0;
	    break;
	  }
      }
  if (Size == 0) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_POLYPOLYGON);

  s_rbox_ushort (API, &rbox, count);
  for (j = 0; j < count; j++) s_rbox_ushort (API, &rbox, N[j]);
  for (j = 0; j < count; j++)
    for (i = 0; i < N[j]; i++)
      {
	s_rbox_ushort (API, &rbox, x[j][i]);
	s_rbox_ushort (API, &rbox, y[j][i]);
      }
  return 0;
}

/**
 * Draw text.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param x         x-coordinate of string
 * @param y         y-coordinate of string
 * @param str       the text string to be drawn
 * 
 * Draw text.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_text (wmfAPI * API, wmfCanvas * canvas,
		     unsigned short x, unsigned short y, const char * str)
{
  unsigned short i;
  unsigned short length;

  unsigned short cc;

  unsigned long Size = 3 + 3;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (str == 0)) return -1;

  length = (strlen (str) + 1) / 2; /* TODO: do conversion from UTF-8 to... ?? */
  if (length == 0) return 0;

  Size += length;

  if ((x & 0x7fff) != x)
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if ((y & 0x7fff) != y)
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }

  s_setnew_font (API, construct);

  if (ERR (API)) return -1;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_TEXTOUT);

  s_rbox_ushort (API, &rbox, length);

  for (i = 0; i < length; i++)
    {
      cc  =  (unsigned short) str[2*i  ];
      cc |= ((unsigned short) str[2*i+1]) << 8;

      s_rbox_ushort (API, &rbox, cc);
    }
  s_rbox_ushort (API, &rbox, y);
  s_rbox_ushort (API, &rbox, x);

  return 0;
}

/**
 * Place a bitmap.
 * 
 * @param API       the API handle
 * @param canvas    the Canvas handle for the metafile you are drawing to
 * @param x         x-coordinate of bitmap
 * @param y         y-coordinate of bitmap
 * @param width     (scaled) width  of bitmap
 * @param height    (scaled) height of bitmap
 * @param buffer    buffer containing bitmap
 * @param length    length of buffer
 * 
 * Place a bitmap.
 * 
 * @return Returns 0 on success.
 */
int wmf_canvas_bitmap (wmfAPI * API, wmfCanvas * canvas,
		       unsigned short x,     unsigned short y,
		       unsigned short width, unsigned short height,
		       const unsigned char * buffer, unsigned long length)
{
  unsigned short bmp_width;
  unsigned short bmp_height;

  unsigned long bmp_size;
  unsigned long Size = 3 + 10;

  wmfRecordBox rbox;

  wmfConstruct * construct = (wmfConstruct *) canvas;

  if ((construct == 0) || (buffer == 0) || (length == 0)) return -1;

  if ((x & 0x7fff) != x)
    {
      WMF_ERROR (API, "Coordinate out of range! (x > 0x7fff)");
      return -1;
    }
  if ((y & 0x7fff) != y)
    {
      WMF_ERROR (API, "Coordinate out of range! (y > 0x7fff)");
      return -1;
    }
  if ((bmp_size = s_bmp_query ()) == 0)
    {
      WMF_ERROR (API, "Bad bitmap!");
      return -1;
    }

  if (ERR (API)) return -1;

  Size += bmp_size;

  s_append_record (API, construct, &rbox, 2 * Size);
  if (ERR (API))
    {
      WMF_DEBUG (API, "bailing...");
      return -1;
    }
  s_rbox_ulong  (API, &rbox, Size);
  s_rbox_ushort (API, &rbox, META_DIBSTRETCHBLT);

  s_rbox_ulong  (API, &rbox, 0); /* ?? */

  s_rbox_ushort (API, &rbox, bmp_height);
  s_rbox_ushort (API, &rbox, bmp_width);
  s_rbox_ushort (API, &rbox, 0);
  s_rbox_ushort (API, &rbox, 0);
  s_rbox_ushort (API, &rbox, height);
  s_rbox_ushort (API, &rbox, width);
  s_rbox_ushort (API, &rbox, y);
  s_rbox_ushort (API, &rbox, x);

  /* TODO */

  return 0;
}

static unsigned long s_bmp_query (void)
{
  return 0;
}
