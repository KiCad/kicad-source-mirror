/* libwmf (font.c): library for wmf conversion
   Copyright (C) 2000,2001 Francis James Franklin

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
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "wmfdefs.h"

#include "font.h"

#if defined (_WIN32) && !defined (LIBWMF_STATIC)

extern char* _libwmf_get_fontdir();

static char* _libwmf_get_xtra_fontmap (void)
{
	static char retval[1000] = "";

	if (retval[0] == '\0')
	{	snprintf (retval, sizeof (retval), "%s\\share\\libwmf\\fonts\\fontmap",
			  _libwmf_get_fontdir ());
	}

	return retval;
}

#undef WMF_XTRA_FONTMAP
#define WMF_XTRA_FONTMAP _libwmf_get_xtra_fontmap ()

static char* remap_font_file_name(wmfAPI* API,char* filename)
{
	/* If filename is prefixed with the compile-time WMF_FONTDIR,
	 * substitute the run-time font directory.
	 */
	char* retval;
	if (strncmp (filename, WMF_FONTDIR "/", strlen (WMF_FONTDIR "/")) == 0)
	{	retval = wmf_malloc (API,strlen (filename) - strlen (WMF_FONTDIR) + strlen (_libwmf_get_fontdir ()) + 1);
		strcpy (retval,_libwmf_get_fontdir ());
		strcat (retval,filename + strlen (WMF_FONTDIR));
	}
	else
	{	retval = wmf_strdup (API,filename);
	}

	return retval;
}

#endif

static void ipa_font_add_wmf (wmfAPI* API,wmfFontMap* mapping)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfFontMap* more;

	unsigned int i = 0;

	while (font_data->wmf[i].name)
	{	if (strcmp (font_data->wmf[i].name,mapping->name) == 0) break;
		i++;
	}

	if (font_data->wmf[i].name) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (wmfFontMap*) wmf_realloc (API,font_data->wmf,(i + 0x11) * sizeof (wmfFontMap));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		font_data->wmf = more;
	}

	font_data->wmf[i].name       = wmf_strdup (API,mapping->name      );
	font_data->wmf[i].normal     = wmf_strdup (API,mapping->normal    );
	font_data->wmf[i].italic     = wmf_strdup (API,mapping->italic    );
	font_data->wmf[i].bold       = wmf_strdup (API,mapping->bold      );
	font_data->wmf[i].bolditalic = wmf_strdup (API,mapping->bolditalic);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	i++;
	font_data->wmf[i].name = 0;
}

static void ipa_font_add_sub (wmfAPI* API,wmfMapping* mapping)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfMapping* more;

	unsigned int i = 0;

	while (font_data->sub[i].name)
	{	if (strcmp (font_data->sub[i].name,mapping->name) == 0) break;
		i++;
	}

	if (font_data->sub[i].name) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (wmfMapping*) wmf_realloc (API,font_data->sub,(i + 0x11) * sizeof (wmfMapping));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		font_data->sub = more;
	}

	font_data->sub[i].name    = wmf_strdup (API,mapping->name   );
	font_data->sub[i].mapping = wmf_strdup (API,mapping->mapping);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	i++;
	font_data->sub[i].name = 0;
}

static void ipa_font_add_ps (wmfAPI* API,wmfMapping* mapping)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfFT_Mapping* more;

	unsigned int i = 0;

	while (font_data->ps[i].name)
	{	if (strcmp (font_data->ps[i].name,mapping->name) == 0) break;
		i++;
	}

	if (font_data->ps[i].name) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (wmfFT_Mapping*) wmf_realloc (API,font_data->ps,(i + 0x11) * sizeof (wmfFT_Mapping));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		font_data->ps = more;
	}

	font_data->ps[i].name     = wmf_strdup (API,mapping->name   );
	font_data->ps[i].mapping  = wmf_strdup (API,mapping->mapping);
	font_data->ps[i].encoding = mapping->encoding;

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	font_data->ps[i].face = 0;

	i++;
	font_data->ps[i].name = 0;
}

static void ipa_font_add_cache (wmfAPI* API,wmfFT_CacheEntry* entry)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfFT_CacheEntry* more;

	unsigned int i = 0;

	while (font_data->cache[i].name)
	{	if (strcmp (font_data->cache[i].path,entry->path) == 0) break;
		i++;
	}

	if (font_data->cache[i].name) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (wmfFT_CacheEntry*) wmf_realloc (API,font_data->cache,(i + 0x11) * sizeof (wmfFT_CacheEntry));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		font_data->cache = more;
	}

	font_data->cache[i].name = wmf_strdup (API,entry->name);
	font_data->cache[i].path = wmf_strdup (API,entry->path);
	font_data->cache[i].face = entry->face;

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	i++;
	font_data->cache[i].name = 0;
}

char* wmf_ipa_font_lookup (wmfAPI* API,char* ps_name)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	unsigned int i = 0;

	while (font_data->cache[i].name)
	{	if (strcmp (font_data->cache[i].name,ps_name) == 0) break;
		i++;
	}

	if (font_data->cache[i].name == 0) return (0);

	return (font_data->cache[i].path);
}

void wmf_ipa_font_init (wmfAPI* API,wmfAPI_Options* options)
{	wmfFontData* font_data = 0;

	wmfFontmapData* fontmap_data = 0;

	unsigned int i;
	unsigned int count;

	API->font_data = wmf_malloc (API,sizeof (wmfFontData));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	font_data = (wmfFontData*) API->font_data;

	font_data->map = wmf_ipa_font_map;

	font_data->stringwidth = wmf_ipa_font_stringwidth;

	font_data->user_data = wmf_malloc (API,sizeof (wmfFontmapData));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data = (wmfFontmapData*) font_data->user_data;

	API->fonts = (char**) wmf_malloc (API,16 * sizeof (char*));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	API->fonts[0] = 0;

	fontmap_data->fontdirs = (char**) wmf_malloc (API,16 * sizeof (char*));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->fontdirs[0] = 0;

	fontmap_data->wmf = (wmfFontMap*) wmf_malloc (API,16 * sizeof (wmfFontMap));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->wmf[0].name = 0;

	if ((API->flags & WMF_OPT_FONTMAP) && options->font.wmf)
	{	i = 0;
		while (options->font.wmf[i].name)
		{	ipa_font_add_wmf (API,&(options->font.wmf[i]));

			if (ERR (API)) break;

			i++;
		}
		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	count = sizeof WMFFontMap / sizeof (wmfFontMap);

	for (i = 0; i < count; i++)
	{	ipa_font_add_wmf (API,&(WMFFontMap[i]));

		if (ERR (API)) break;
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->sub = (wmfMapping*) wmf_malloc (API,16 * sizeof (wmfMapping));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->sub[0].name = 0;

	if ((API->flags & WMF_OPT_FONTMAP) && options->font.sub)
	{	i = 0;
		while (options->font.sub[i].name)
		{	ipa_font_add_sub (API,&(options->font.sub[i]));

			if (ERR (API)) break;

			i++;
		}
		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	count = sizeof SubFontMap / sizeof (wmfMapping);

	for (i = 0; i < count; i++)
	{	ipa_font_add_sub (API,&(SubFontMap[i]));

		if (ERR (API)) break;
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->ps = (wmfFT_Mapping*) wmf_malloc (API,16 * sizeof (wmfFT_Mapping));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->ps[0].name = 0;

	fontmap_data->cache = (wmfFT_CacheEntry*) wmf_malloc (API,16 * sizeof (wmfFT_CacheEntry));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	fontmap_data->cache[0].name = 0;

	if ((API->flags & WMF_OPT_FONTMAP) && options->font.ps)
	{	i = 0;
		while (options->font.ps[i].name)
		{	ipa_font_add_ps (API,&(options->font.ps[i]));

			if (ERR (API)) break;

			i++;
		}
		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	count = 13;

	for (i = 0; i < count; i++)
	{	ipa_font_add_ps (API,&(PSFontMap[i]));

		if (ERR (API)) break;
	}
	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	if (FT_Init_FreeType (&(fontmap_data->Library)) != 0)
	{	WMF_ERROR (API,"Failed to initialize freetype...");
		API->err = wmf_E_DeviceError;
		fontmap_data->Library = 0;
	}

	API->flags |= API_FTLIBRARY_OPEN;

	fontmap_data->FD.max = 0;
	fontmap_data->FD.len = 0;

	fontmap_data->FD.FI = 0;

	if (API->flags & WMF_OPT_SYS_FONTS)
	{	if (API->flags & WMF_OPT_SYS_FONTMAP)
		{	wmf_ipa_font_map_xml (API,&(fontmap_data->FD),options->sys_fontmap_file);
		}
		else
		{	wmf_ipa_font_map_xml (API,&(fontmap_data->FD),WMF_SYS_FONTMAP);
		}
	}

	if (API->flags & WMF_OPT_XTRA_FONTS)
	{	if (API->flags & WMF_OPT_XTRA_FONTMAP)
		{	wmf_ipa_font_map_xml (API,&(fontmap_data->FD),options->xtra_fontmap_file);
		}
		else
		{	wmf_ipa_font_map_xml (API,&(fontmap_data->FD),WMF_XTRA_FONTMAP);
		}
	}

	fontmap_data->GS.max = 0;
	fontmap_data->GS.len = 0;

	fontmap_data->GS.FI = 0;

	if (API->flags & WMF_OPT_GS_FONTMAP)
	{	wmf_ipa_font_map_gs (API,&(fontmap_data->GS),options->gs_fontmap_file);
	}
	else
	{	wmf_ipa_font_map_gs (API,&(fontmap_data->GS),WMF_GS_FONTMAP);
	}
}

void wmf_ipa_font_map_set (wmfAPI* API,wmfMap map)
{	wmfFontData* font_data = (wmfFontData*) API->font_data;

	font_data->map = map;
}

void wmf_ipa_font_dir (wmfAPI* API,char* fontdir)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	char** more;

	unsigned int i = 0;

	while (font_data->fontdirs[i])
	{	if (strcmp (font_data->fontdirs[i],fontdir) == 0) break;
		i++;
	}

	if (font_data->fontdirs[i]) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (char**) wmf_realloc (API,font_data->fontdirs,(i + 0x11) * sizeof (char*));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		font_data->fontdirs = more;
	}

	font_data->fontdirs[i] = wmf_strdup (API,fontdir);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	i++;
	font_data->fontdirs[i] = 0;
}

static void ipa_font_add_api (wmfAPI* API,char* name)
{	char** more;

	unsigned int i = 0;

	while (API->fonts[i])
	{	if (strcmp (API->fonts[i],name) == 0) break;
		i++;
	}

	if (API->fonts[i]) return; /* Entry exists already */

	if ((i & 0x0f) == (unsigned int) 0x0f)
	{	more = (char**) wmf_realloc (API,API->fonts,(i + 0x11) * sizeof (char*));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		API->fonts = more;
	}

	API->fonts[i] = wmf_strdup (API,name);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	i++;
	API->fonts[i] = 0;
}

/* Returns width of string in points, assuming (unstretched) font size of 1pt
 * WARNING: This is not very accurate. If anyone has a better idea, then...
 */
float wmf_ipa_font_stringwidth (wmfAPI* API,wmfFont* font,char* str)
{	FT_Face face = WMF_FONT_FTFACE (font);

	if (!face) return 0.0;

	FT_Vector delta;

	FT_Bool use_kerning;

	FT_UInt glyph_index;
	FT_UInt previous;

	float width;

	int i;
	int length;

	if ((face == 0) || (str == 0))
	{	WMF_DEBUG (API,"wmf_ipa_font_stringwidth: NULL face or str - ??");
		return 0;
	}

	FT_Set_Char_Size (face,0,12 * 64,300,100);

	FT_Set_Transform (face,0,0);

	use_kerning = FT_HAS_KERNING (face);

	previous = 0;

	width = 0;

	length = strlen (str);

	for (i = 0; i < length; i++)
	{	/* convert character code to glyph index */
		/* ==== Should this be unsigned ??  ==== */

		glyph_index = FT_Get_Char_Index (face,str[i]);

		if (use_kerning && previous && glyph_index)
		{	FT_Get_Kerning (face,previous,glyph_index,0,&delta);

			width += (float) (delta.x >> 6);
		}

		if (glyph_index)
		{	/* load glyph image into the slot.  DO NOT RENDER IT! */
			FT_Load_Glyph (face,glyph_index,FT_LOAD_DEFAULT);

			if (face->glyph) width += (float) (face->glyph->advance.x >> 6);
		}

		previous = glyph_index;
	}

	return (width*72/(300*12));
}

static float ipa_char_position (wmfFont* font,char* str,char* last)
{	FT_Face face = WMF_FONT_FTFACE (font);

	if (!face) return 0.0;

	FT_Vector delta;

	FT_Bool use_kerning;

	FT_UInt glyph_index;
	FT_UInt previous;

	float width;

	char* ptr;

	FT_Set_Char_Size (face,0,12 * 64,300,100);

	FT_Set_Transform (face,0,0);

	use_kerning = FT_HAS_KERNING (face);

	previous = 0;

	width = 0;

	for (ptr = str; ptr <= last; ptr++)
	{	/* convert character code to glyph index */ /* === Should this be unsigned ?? ==== */
		glyph_index = FT_Get_Char_Index (face,(*ptr));

		if (use_kerning && previous && glyph_index)
		{	FT_Get_Kerning (face,previous,glyph_index,0,&delta);

			width += (float) (delta.x >> 6);
		}

		if (ptr == last) break;

		/* load glyph image into the slot.  DO NOT RENDER IT! */
		FT_Load_Glyph (face,glyph_index,FT_LOAD_DEFAULT);

		width += (float) (face->glyph->advance.x >> 6);

		previous = glyph_index;
	}

	return (width*72/(300*12));
}

void wmf_ipa_draw_text (wmfAPI* API,wmfDrawText_t* draw_text,wmfCharDrawer ipa_draw_text)
{	wmfDrawText_t draw_char;

	size_t i;
	size_t length;

	char buffer[2];

	float text_width;

	float cos_theta;
	float sin_theta;

	double theta;

	length = strlen (draw_text->str);

	theta = - WMF_TEXT_ANGLE (draw_text->dc->font);

	cos_theta = (float) cos (theta);
	sin_theta = (float) sin (theta);

	for (i = 0; i < length; i++)
	{	draw_char.dc = draw_text->dc;

		buffer[0] = draw_text->str[i];
		buffer[1] = 0;

		text_width = ipa_char_position (draw_text->dc->font,draw_text->str,draw_text->str+i);

		text_width = (float) ((double) text_width * draw_text->font_height * draw_text->font_ratio);

		draw_char.pt.x = draw_text->pt.x + text_width * cos_theta;
		draw_char.pt.y = draw_text->pt.y + text_width * sin_theta;

		draw_char.TL.x = 0;
		draw_char.TL.y = 0;
		draw_char.BR.x = 0;
		draw_char.BR.y = 0;

		draw_char.bbox.TL.x = 0;
		draw_char.bbox.TL.y = 0;
		draw_char.bbox.TR.x = 0;
		draw_char.bbox.TR.y = 0;
		draw_char.bbox.BR.x = 0;
		draw_char.bbox.BR.y = 0;
		draw_char.bbox.BL.x = 0;
		draw_char.bbox.BL.y = 0;

		draw_char.str = buffer;

		draw_char.flags = draw_text->flags;

		draw_char.font_height = draw_text->font_height;
		draw_char.font_ratio  = draw_text->font_ratio;

		ipa_draw_text (API,&draw_char);
	}
}

void wmf_ipa_font_map_xml (wmfAPI* API,wmfXML_FontData* FD,char* xmlfontmap)
{	FD->max = 0;
	FD->len = 0;

	FD->FI = 0;

#ifdef HAVE_EXPAT
	exmlfontmap_read (API,FD,xmlfontmap);
	return;
#endif /* HAVE_EXPAT */
#ifdef HAVE_LIBXML2
	xml2fontmap_read (API,FD,xmlfontmap);
	return;
#endif /* HAVE_LIBXML2 */
	WMF_DEBUG (API,"warning: no XML support!");
}

#ifdef HAVE_EXPAT

#include <expat.h>

#define EXML_BUFSIZE 1024

static void exml_start (void* user_data,const char* tag,const char** attributes)
{	const char** attr;

	EXML_FontData* exml_data = (EXML_FontData*) user_data;

	wmfAPI* API = exml_data->API;

	wmfXML_FontData* FD = exml_data->FD;

	wmfXML_FontInfo* more = 0;

	wmfXML_FontInfo FI;

	if ((tag == 0) || (attributes == 0)) return;

	if (strcmp (tag,"font")) return;

	FI.format = 0;
	FI.metrics = 0;
	FI.glyphs = 0;
	FI.name = 0;
	FI.fullname = 0;
	FI.familyname = 0;
	FI.weight = 0;
	FI.version = 0;
	FI.alias = 0;

	attr = attributes;
	while (*attr)
	{	if (strcmp ((*attr),"format") == 0)
		{	attr++;
			FI.format = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"metrics") == 0)
		{	attr++;
			FI.metrics = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"glyphs") == 0)
		{	attr++;
			FI.glyphs = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"name") == 0)
		{	attr++;
			FI.name = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"fullname") == 0)
		{	attr++;
			FI.fullname = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"familyname") == 0)
		{	attr++;
			FI.familyname = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"weight") == 0)
		{	attr++;
			FI.weight = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"version") == 0)
		{	attr++;
			FI.version = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"alias") == 0)
		{	attr++;
			FI.alias = wmf_strdup (API,(char*) (*attr));
		}
		else attr++;
		attr++;
	}

	if (FD->len == FD->max)
	{	more = wmf_realloc (API,FD->FI,(FD->max + 32) * sizeof (wmfXML_FontInfo));
		if (more)
		{	FD->FI = more;
			FD->max += 32;
		}
	}
	if (FD->len < FD->max)
	{	FD->FI[FD->len] = FI;
		FD->len++;
	}
}

static void exmlfontmap_read (wmfAPI* API,wmfXML_FontData* FD,char* xmlfontmap)
{	int status = 0;
	int length;

	char buffer[EXML_BUFSIZE];

	void* user_data;

	FILE* in;

	EXML_FontData exml_data;

	XML_Parser exml;

	FD->max = 32;
	FD->len = 0;

	FD->FI = wmf_malloc (API,FD->max * sizeof (wmfXML_FontInfo));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		FD->max = 0;
		return;
	}

	in = fopen (xmlfontmap,"r");

	if (in == 0)
	{	WMF_DEBUG (API,"unable to open xml-fontmap");
		FD->max = 0;
		wmf_free (API,FD->FI);
		FD->FI = 0;
		return;
	}

	exml = XML_ParserCreate (0);

	if (exml == 0)
	{	WMF_DEBUG (API,"error creating expat-xml parser");
		FD->max = 0;
		wmf_free (API,FD->FI);
		FD->FI = 0;
		fclose (in);
		return;
	}

	exml_data.API = API;
	exml_data.FD = FD;

	user_data = (void*) (&exml_data);

	XML_SetUserData (exml,user_data);

	XML_SetStartElementHandler (exml,exml_start);

	while (fgets (buffer,EXML_BUFSIZE,in))
	{	length = (int) strlen (buffer);

		if (XML_Parse (exml,buffer,length,0) == 0)
		{	WMF_DEBUG (API,"expat-xml: error parsing xml fontmap");
			status = 1;
			break;
		}
	}
	if (status == 0) XML_Parse (exml,buffer,0,1);

	XML_ParserFree (exml);

	fclose (in);

	if (FD->len == 0)
	{	FD->max = 0;
		wmf_free (API,FD->FI);
		FD->FI = 0;
	}
}
#endif /* HAVE_EXPAT */

#ifdef HAVE_LIBXML2

#include <libxml/parser.h>
#include <libxml/parserInternals.h>

static void xml2_start (void* user_data,const char* tag,const char** attributes)
{	const char** attr;

	XML2_FontData* xml2_data = (XML2_FontData*) user_data;

	wmfAPI* API = xml2_data->API;

	wmfXML_FontData* FD = xml2_data->FD;

	wmfXML_FontInfo* more = 0;

	wmfXML_FontInfo FI;

	if ((tag == 0) || (attributes == 0)) return;

	if (strcmp (tag,"font")) return;

	FI.format = 0;
	FI.metrics = 0;
	FI.glyphs = 0;
	FI.name = 0;
	FI.fullname = 0;
	FI.familyname = 0;
	FI.weight = 0;
	FI.version = 0;
	FI.alias = 0;

	attr = attributes;
	while (*attr)
	{	if (strcmp ((*attr),"format") == 0)
		{	attr++;
			FI.format = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"metrics") == 0)
		{	attr++;
			FI.metrics = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"glyphs") == 0)
		{	attr++;
			FI.glyphs = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"name") == 0)
		{	attr++;
			FI.name = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"fullname") == 0)
		{	attr++;
			FI.fullname = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"familyname") == 0)
		{	attr++;
			FI.familyname = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"weight") == 0)
		{	attr++;
			FI.weight = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"version") == 0)
		{	attr++;
			FI.version = wmf_strdup (API,(char*) (*attr));
		}
		else if (strcmp ((*attr),"alias") == 0)
		{	attr++;
			FI.alias = wmf_strdup (API,(char*) (*attr));
		}
		else attr++;
		attr++;
	}

	if (FD->len == FD->max)
	{	more = wmf_realloc (API,FD->FI,(FD->max + 32) * sizeof (wmfXML_FontInfo));
		if (more)
		{	FD->FI = more;
			FD->max += 32;
		}
	}
	if (FD->len < FD->max)
	{	FD->FI[FD->len] = FI;
		FD->len++;
	}
}

static void xml2fontmap_read (wmfAPI* API,wmfXML_FontData* FD,char* xmlfontmap)
{	XML2_FontData xml2_data;

	xmlParserCtxtPtr ctxt;

	xmlSAXHandler sax;

	memset ((void*) (&sax), 0, sizeof (xmlSAXHandler));

	sax.startElement = (startElementSAXFunc) xml2_start;

	xml2_data.API = API;
	xml2_data.FD = FD;

	ctxt = xmlCreateFileParserCtxt (xmlfontmap);

	if (ctxt == 0) return;

	ctxt->sax = &sax;
	ctxt->userData = (void*) (&xml2_data);

	xmlParseDocument (ctxt);

	ctxt->sax = 0;

	xmlFreeParserCtxt (ctxt);
}

#endif /* HAVE_LIBXML2 */

void wmf_ipa_font_map_gs (wmfAPI* API,wmfGS_FontData* FD,char* gsfontmap)
{	ipa_font_gs_file (API,FD,gsfontmap);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	/* TODO ?? */
}

static void ipa_font_gs_file (wmfAPI* API,wmfGS_FontData* FD,char* file)
{	char* line = 0;

	char* name = 0;
	char* alias = 0;

	FILE* in = 0;

	in = fopen (file,"r");

	if (in == 0)
	{	WMF_DEBUG (API,"ipa_font_gs_file: unable to read ghostscript fontmap!\n");
		return;
	}

	while (wmf_true)
	{	line = ipa_font_gs_readline (API,in);

		if (line == 0) break;

		if (*line)
		{	name = line;
			alias = line;

			while (!isspace ((int) (*alias)))
			{	if ((*alias) == 0) break;
				alias++;
			}
			if (*alias)
			{	(*alias) = 0;
				alias++;
			}

			ipa_font_gs_add (API,FD,name,alias);
		}

		wmf_free (API,line);
	}

	fclose (in);
}

static void ipa_font_gs_add (wmfAPI* API,wmfGS_FontData* FD,char* name,char* alias)
{	wmfGS_FontInfo* more = 0;

	wmf_bool_t fMatched;

	unsigned int i;

	if ((name == 0) || (alias == 0)) return;

	if (name[0] != '/') return;

	if ((alias[0] != '/') && (alias[0] != '(')) return;

	if ((alias[0] == '(') && (alias[strlen (alias) - 1] != ')')) return;

	if (FD->FI == 0)
	{	FD->FI = (wmfGS_FontInfo*) wmf_calloc (API,0x20,sizeof (wmfGS_FontInfo));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		FD->max = 0x20;
		FD->len = 0;
	}

	fMatched = wmf_false;
	for (i = 0; i < FD->len; i++)
	{	if (strcmp (FD->FI[i].name,name+1) == 0)
		{	wmf_free (API,FD->FI[i].alias);
			FD->FI[i].alias = wmf_strdup (API,alias);
			fMatched = wmf_true;
			break;
		}
	}
	if (fMatched) return;

	if (FD->len == FD->max)
	{	more = (wmfGS_FontInfo*) wmf_realloc (API,FD->FI,(FD->max+0x20) * sizeof (wmfGS_FontInfo));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}

		FD->FI = more;

		FD->max += 0x20;
	}

	FD->FI[FD->len].name  = wmf_strdup (API,name+1);
	FD->FI[FD->len].alias = wmf_strdup (API,alias);

	FD->len++;
}

static char* ipa_font_gs_alias (wmfGS_FontData* FD,char* name)
{	char* alias = 0;

	unsigned int i;

	for (i = 0; i < FD->len; i++)
	{	if (strcmp (FD->FI[i].name,name) == 0)
		{	alias = FD->FI[i].alias;
			break;
		}
	}

	return (alias);
}

static char* ipa_font_gs_readline (wmfAPI* API,FILE* in)
{	char buf[128];

	char* line = 0;
	char* more = 0;
	char* ptr = 0;
	char* ptr1 = 0;
	char* ptr2 = 0;

	wmf_bool_t fBackSlash;
	wmf_bool_t fContinue;
	wmf_bool_t fReadExtra = wmf_false;

	if (fgets (buf,128,in) == 0) return (0);

	fBackSlash = wmf_false;
	fContinue  = wmf_true;
	ptr = buf;
	while (*ptr)
	{	if (((*ptr) == '%') && (!fBackSlash))
		{	fContinue = wmf_false;
			if (buf[strlen(buf)-1] != '\n')
			{	fReadExtra = wmf_true;
			}
			else
			{	fReadExtra = wmf_false;
			}
			(*ptr) = 0;
			break;
		}
		if ((*ptr) == '\n')
		{	if (fBackSlash)
			{	fContinue = wmf_true;
				ptr--;
			}
			else
			{	fContinue = wmf_false;
			}
			(*ptr) = 0;
			break;
		}
		if ((*ptr) == '\\')
		{	fBackSlash = wmf_true;
		}
		else
		{	fBackSlash = wmf_false;
		}
		ptr++;
	}

	line = wmf_strdup (API,buf);

	if (line == 0) return (line);

	while (fContinue)
	{	if (fgets (buf,10,in) == 0)
		{	wmf_free (API,line);
			return (0);
		}

		fBackSlash = wmf_false;
		fContinue  = wmf_true;
		ptr = buf;
		while (*ptr)
		{	if (((*ptr) == '%') && (!fBackSlash))
			{	fContinue = wmf_false;
				fReadExtra = wmf_true;
				(*ptr) = 0;
				break;
			}
			if ((*ptr) == '\n')
			{	if (fBackSlash)
				{	fContinue = wmf_true;
					ptr--;
				}
				else
				{	fContinue = wmf_false;
				}
				(*ptr) = 0;
				break;
			}
			if ((*ptr) == '\\')
			{	fBackSlash = wmf_true;
			}
			else
			{	fBackSlash = wmf_false;
			}
			ptr++;
		}

		more = wmf_str_append (API,line,buf);

		wmf_free (API,line);

		line = more;

		if (line == 0)
		{
			break;
		}
	}

	if (line == 0) return (0);

	if (fReadExtra)
	{
		while (buf[strlen(buf)-1] != '\n')
			if (fgets (buf,128,in) == 0)
				break;
	}

	/* Strip the string */

	ptr1 = line;
	ptr2 = line;
	while (*ptr2)
	{	if (!isspace ((int) (*ptr2))) break;
		ptr2++;
	}
	while (*ptr2)
	{	if (isspace ((int) (*ptr2))) break;
		(*ptr1) = (*ptr2);
		ptr1++;
		ptr2++;
	}
	if (*ptr2)
	{	(*ptr1) = (*ptr2);
		ptr1++;
		ptr2++;
	}
	while (*ptr2)
	{	if (!isspace ((int) (*ptr2))) break;
		ptr2++;
	}
	while (*ptr2)
	{	if (isspace ((int) (*ptr2))) break;
		(*ptr1) = (*ptr2);
		ptr1++;
		ptr2++;
	}
	(*ptr1) = 0;

	while ((--ptr1) >= line)
	{	if (!isspace ((int) (*ptr1))) break;
		(*ptr1) = 0;
	}

	ptr1 = line;
	ptr2 = line;
	while (*ptr2)
	{	if ((*ptr2) == '%') ptr1--;
		(*ptr1) = (*ptr2);
		ptr1++;
		ptr2++;
	}
	(*ptr1) = 0;

	return (line);
}

/**
 * This is the fun bit.
 * 
 * @param API  the API handle
 * @param font WMF font object: font's \b family \b name, \b italics flag, and the \b weight
 * 
 * Other inputs include: a ghostscript-style Fontmap, and up to two XML RedHat-style fontmaps. (These are
 * all parsed earlier.) There are also the fallback 13 standard postscript fonts. And then there is a list
 * of font directories to search for fonts.
 * 
 * \b libwmf requires fonts to have a postscript name, and maintains an internal cache of glyphs path and
 * \b freetype font face, keyed to postscript font name. Unfortunately, although the XML fontmaps do have
 * font \b family \b name, the GS fontmap does not. In the latter, therefore, matching WMF font info to
 * postscript font name is hit-and-miss.
 */
void wmf_ipa_font_map (wmfAPI* API,wmfFont* font)
{	wmfIPAFont* ipa_font = 0;

	char* mapping = 0;

	if (font == 0) return;
	if (font->user_data == 0)
	{	font->user_data = wmf_malloc (API,sizeof (wmfIPAFont));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return;
		}
	}

	ipa_font = (wmfIPAFont*) font->user_data;

	ipa_font->ps_name = 0;
	ipa_font->ft_face = 0;

/* Check system fonts for match and load font face if found...
 */
	if (ipa_font_sys_face (API,font,ipa_font_sys_map (API,font))) return;

/* Check GS fontmap for match and load font face if found...
 */
	if (ipa_font_gs_face (API,font,ipa_font_gs_map (API,font))) return;

/* otherwise, load one of standard 13...
 */
	mapping = ipa_font_std (API,font);

	if (mapping)
	{	if (ipa_font_face (API,font,mapping)) return;
	}

	WMF_ERROR (API,"wmf_ipa_font_map: failed to load *any* font!");
	API->err = wmf_E_BadFile;
}

/**
 * Searches XML Fontmap for font-name matching WMF's font name.
 * 
 * Basically, this is matching a font's family name + italic + weight parameters to a postscript name.
 * Font weight is problematic.
 */
static wmfXML_FontInfo* ipa_font_sys_map (wmfAPI* API,wmfFont* font)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfXML_FontInfo* FI = 0;

	unsigned int i;

	for (i = 0; i < font_data->FD.len; i++)
	{	if (strcmp (font->lfFaceName,font_data->FD.FI[i].familyname) == 0)
		{	if ( wmf_strstr (font_data->FD.FI[i].fullname,"Italic")
			  || wmf_strstr (font_data->FD.FI[i].fullname,"Oblique"))
			{	if (font->lfItalic != 1) continue;
			}
			else
			{	if (font->lfItalic == 1) continue;
			}
			if ( wmf_strstr (font_data->FD.FI[i].weight,"Bold")
			  || wmf_strstr (font_data->FD.FI[i].weight,"Demi")) /* or "Medium" ?? */
			{	if (font->lfWeight <= 550) continue;
			}
			else
			{	if (font->lfWeight >  550) continue;
			}
			FI = &(font_data->FD.FI[i]);
			break;
		}
	}

	return (FI);
}

/**
 * Checks XML Fontmap entry is valid before loading face.
 */
static FT_Face ipa_font_sys_face (wmfAPI* API,wmfFont* font,wmfXML_FontInfo* FI)
{	
#if 0
	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;
	struct stat stat_buf;
#endif

	FT_Face face = 0;

	if (FI == 0) return (0);

	if (FI->name == 0)
	{	WMF_DEBUG (API,"no postscript name in system-font record?");
		return (0);
	}

	if (FI->glyphs == 0)
	{	WMF_DEBUG (API,"no glyphs in system-font record?");
		return (0);
	}

	face = ipa_font_face (API,font,FI->name);

	return (face);
}

/**
 * Searches GS Fontmap for font-name matching WMF's font name.
 * 
 * Basically, this is matching a font's family name + italic + weight parameters to a postscript name.
 * I have decided to test only the first word of the family name, so incorrect matches are probably.
 * Font weight is problematic as well.
 */
static wmfGS_FontInfo* ipa_font_gs_map (wmfAPI* API,wmfFont* font)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfGS_FontData* GS = &(font_data->GS);
	wmfGS_FontInfo* FI = 0;

	wmf_bool_t require_italic;
	wmf_bool_t require_bold;
	wmf_bool_t found_italic;
	wmf_bool_t found_bold;

	unsigned int name_length = 0;
	unsigned int i;

	char* name = 0;
	char* ptr = 0;

	if (GS->len == 0) return (0);

	name = font->lfFaceName;

	if (name == 0 || name[0] == 0)
		name = "Times";

	/* Find first white-space character or eol
	 */
	ptr = name;
	while (!isspace (*ptr))
	{	if ((*ptr) == 0) break;
		ptr++;
		name_length++;
	}

	if (name_length == 0)
	{	WMF_DEBUG (API,"Unexpectedly short font name?");
		API->err = wmf_E_Glitch;
		return (0);
	}

	require_italic = ((font->lfItalic == 1) ? wmf_true : wmf_false);

	require_bold = ((font->lfWeight > 550) ? wmf_true : wmf_false);

	/* Search for match in GS Fontmap list
	 */
	for (i = 0; i < GS->len; i++)
	{	if (strncmp (name,GS->FI[i].name,name_length) == 0)
		{	found_italic = wmf_false;
			found_bold   = wmf_false;
			if ( wmf_strstr (GS->FI[i].name,"Ital")
			  || wmf_strstr (GS->FI[i].name,"Obli"))
			{	found_italic = wmf_true;
			}
			if ( wmf_strstr (GS->FI[i].name,"Bold")
			  || wmf_strstr (GS->FI[i].name,"Demi")) /* or "Medi" ?? */
			{	found_bold   = wmf_true;
			}
			if ((require_italic == found_italic) && (require_bold == found_bold))
			{	FI = &(GS->FI[i]);
				break;
			}
		}
	}

	if (FI == 0) return (0);

	return (FI);
}

/**
 * Takes a [name]/[alias] pair from the GS Fontmap and tries to load [name] as a font,
 * and then, provided [alias] is aliased to a font-file & not a 3rd font-name, [alias].
 */
static FT_Face ipa_font_gs_face (wmfAPI* API,wmfFont* font,wmfGS_FontInfo* FI)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	FT_Face face = 0;

	char* aalias = 0;

	if (FI == 0) return (0);

	face = ipa_font_face (API,font,FI->name);

	if (face) return (face);

	if (FI->alias[0] != '/') return (0);

	aalias = ipa_font_gs_alias (&(font_data->GS),FI->alias+1);

	if (!aalias) return 0;

	if (aalias[0] != '(')
	{	WMF_DEBUG (API,"font lookup is too complicated! Giving up...");
		return (0);
	}

	face = ipa_font_face (API,font,FI->alias+1);

	return (face);
}

/**
 * Takes a postscript font name and:
 * 
 * (1) Check to see whether font has been cached; if so: return (ipa_font_face_cached ())
 * (2) Check for font in XML table; if found:
 *    (a) ipa_font_face_open ()
 *    (b) return (ipa_font_face_cached ())
 * (3) Check for font with file in GS table; if so:
 *    (a) search for font file in font search path; if found:
 *       ( i) ipa_font_face_open ()
 *       (ii) return (ipa_font_face_cached ())
 * (4) Check for font in internal table; if found:
 *    (a) ipa_font_face_open ()
 *    (b) return (ipa_font_face_cached ())
 * (5) return (0)
 */
static FT_Face ipa_font_face (wmfAPI* API,wmfFont* font,char* ps_name)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	FT_Face face = 0;

	unsigned int i;

	char* name = 0;
	char* suffix = 0;
	char* glyphs = 0;
	char* metrics = 0;

	/* (1) Check to see whether font has been cached; if so: return (ipa_font_face_cached ())
	 */
	face = ipa_font_face_cached (API,font,ps_name);

	if (face) return (face);

	/* (2) Check for font in XML table
	 */
	glyphs = 0;
	metrics = 0;
	for (i = 0; i < font_data->FD.len; i++)
	{	if (font_data->FD.FI[i].name == 0) continue;
		if (strcmp (ps_name,font_data->FD.FI[i].name) == 0)
		{	glyphs  = font_data->FD.FI[i].glyphs;
			metrics = font_data->FD.FI[i].metrics;
			break;
		}
	}

	if (glyphs)
	{	face = ipa_font_face_open (API,ps_name,glyphs,metrics);

		if (face) return (ipa_font_face_cached (API,font,ps_name));
	}

	/* (3) Check for font with file in GS table
	 */
	name = 0;
	for (i = 0; i < font_data->GS.len; i++)
	{	if (strcmp (ps_name,font_data->GS.FI[i].name) == 0)
		{	if (font_data->GS.FI[i].alias[0] == '(')
			{	name = wmf_strdup (API,font_data->GS.FI[i].alias + 1);
			}
			break;
		}
	}

	glyphs = 0;
	if (name)
	{	name[strlen (name) - 1] = 0;

		glyphs = ipa_font_path_find (API,name);
	}

	metrics = 0;
	if (glyphs)
	{	if (strlen (name) > 3)
		{	suffix = name + strlen (name) - 4;
			if ((strcmp (suffix,".pfa") == 0) || (strcmp (suffix,".pfb") == 0))
			{	strcpy (suffix,".afm");
				metrics = ipa_font_path_find (API,name);
			}
		}

		face = ipa_font_face_open (API,ps_name,glyphs,metrics);
	}

	if (name) wmf_free (API,name);

	if (glyphs)  wmf_free (API,glyphs);
	if (metrics) wmf_free (API,metrics);

	if (face) return (ipa_font_face_cached (API,font,ps_name));

	/* (4) Check for font in internal table
	 */
	name = 0;
	i = 0;
	while (font_data->ps[i].name)
	{	if (strcmp (ps_name,font_data->ps[i].name) == 0)
		{	name = wmf_strdup (API,font_data->ps[i].mapping);
			break;
		}
		i++;
	}

	glyphs = 0;
	if (name)
	{	glyphs = ipa_font_path_find (API,name);
	}

	metrics = 0;
	if (glyphs)
	{	if (strlen (name) > 3)
		{	suffix = name + strlen (name) - 4;
			if ((strcmp (suffix,".pfa") == 0) || (strcmp (suffix,".pfb") == 0))
			{	strcpy (suffix,".afm");
				metrics = ipa_font_path_find (API,name);
			}
		}

		face = ipa_font_face_open (API,ps_name,glyphs,metrics);
	}

	if (name) wmf_free (API,name);

	if (glyphs)  wmf_free (API,glyphs);
	if (metrics) wmf_free (API,metrics);

	if (face) return (ipa_font_face_cached (API,font,ps_name));

	/* (5) return (0)
	 */
	return (0);
}

/**
 * Opens the font with freetype and caches postscript name, glyphs path & FT font face
 */
static FT_Face ipa_font_face_open (wmfAPI* API,char* ps_name,char* glyphs,char* metrics)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfFT_CacheEntry entry;

	FT_Face face = 0;

	struct stat stat_buf;

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
	glyphs = remap_font_file_name (API,glyphs);
#endif
	if (stat (glyphs,&stat_buf))
	{	WMF_ERROR (API,"unable to stat font file:");
		WMF_ERROR (API,glyphs);
		API->err = wmf_E_BadFile;
#if defined (_WIN32) && !defined (LIBWMF_STATIC)
		wmf_free (API,glyphs);
#endif
		return (0);
	}

	if (FT_New_Face (font_data->Library,glyphs,0,&face) != 0)
	{	WMF_ERROR (API,"Failed to open font:");
		WMF_ERROR (API,glyphs);
		API->err = wmf_E_DeviceError;
#if defined (_WIN32) && !defined (LIBWMF_STATIC)
		wmf_free (API,glyphs);
#endif
		return (0);
	}

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
	if (metrics)
	{
		metrics = remap_font_file_name (API,metrics);
	}
#endif
	if (metrics)
	{	if (stat (metrics,&stat_buf))
		{	WMF_DEBUG (API,"unable to stat font metrics file:");
			WMF_DEBUG (API,metrics);
		}
		else
		{	if (FT_Attach_File (face,metrics) != 0)
			{	WMF_DEBUG (API,"unable to load font metrics file:");
				WMF_DEBUG (API,metrics);
			}
		}
	}

	/* Select encoding (I'm very uncertain about this!): - [TODO] ??
	 */
	if (FT_Select_Charmap (face,ft_encoding_adobe_standard) == 0)
	{	WMF_DEBUG (API,glyphs);
		WMF_DEBUG (API,"Adobe Standard Encoding");
	}
	else if (FT_Select_Charmap (face,ft_encoding_adobe_custom) == 0)
	{	WMF_DEBUG (API,glyphs);
		WMF_DEBUG (API,"Adobe Custom Encoding");
	}
	else if (FT_Select_Charmap (face,ft_encoding_symbol) == 0)
	{	WMF_DEBUG (API,glyphs);
		WMF_DEBUG (API,"Symbol Encoding");
	}
	else if (FT_Select_Charmap (face,ft_encoding_unicode) == 0)
	{	WMF_DEBUG (API,glyphs);
		WMF_DEBUG (API,"Unicode Encoding");
	}
	else
	{	WMF_ERROR (API,"Bad encoding! (Please help me!)");
		API->err = wmf_E_DeviceError;
	}

	entry.name = ps_name;
	entry.path = glyphs;

	entry.face = face;

	ipa_font_add_cache (API,&entry); /* cache font name, path & face */

	ipa_font_add_api (API,ps_name); /* add font name to list of fonts used */

	return (face);
}

/**
 * Finds font info in cache and sets WMF's font entry appropriately.
 */
static FT_Face ipa_font_face_cached (wmfAPI* API,wmfFont* font,char* ps_name)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	wmfIPAFont* ipa_font = (wmfIPAFont*) font->user_data;

	FT_Face face = 0;

	unsigned int i;

	/* Check cache for font
	 */
	i = 0;
	while (font_data->cache[i].name)
	{	if (strcmp (font_data->cache[i].name,ps_name) == 0)
		{	face = font_data->cache[i].face;
			break;
		}
		i++;
	}

	if (face)
	{	ipa_font->ps_name = ps_name;
		ipa_font->ft_face = face;
	}

	return (face);
}

/**
 * Search for file_name in font path and return full path to file as wmf_malloc() string
 */
static char* ipa_font_path_find (wmfAPI* API,char* file_name)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	struct stat stat_buf;

	unsigned int length;
	unsigned int count;
	unsigned int i;

	char* path = 0;

	/* Determine length of string required to hold path name of font
	 */
	count = 0;
	i = 0;
	while (font_data->fontdirs[i])
	{	length = strlen (font_data->fontdirs[i]);
		if (count < length) count = length;
		i++;
	}
	count += 1 + strlen (file_name) + 1;

	path = (char*) wmf_malloc (API,count);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	/* Compose full font file path:
	 */
	i = 0;
	while (font_data->fontdirs[i])
	{	strcpy (path,font_data->fontdirs[i]);
		strcat (path,"/");
		strcat (path,file_name);
		WMF_DEBUG (API,path);
		if (stat (path,&stat_buf) == 0) break; /* file exists */
		i++;
	}

	if (font_data->fontdirs[i] == 0)
	{	WMF_DEBUG (API,"file not found in font path.");
		wmf_free (API,path);
		path = 0;
	}

	return (path);
}

/**
 * If after XML & GS fontmap searches the font still hasn't been matched, then use one of the 13 standard
 * postscript fonts (or any others that have been added via library options).
 */
static char* ipa_font_std (wmfAPI* API,wmfFont* font)
{	wmfFontmapData* font_data = (wmfFontmapData*) ((wmfFontData*) API->font_data)->user_data;

	char* mapping = 0;

	unsigned int i;
	unsigned int index = 0;

	/* First: check for an exact match:
	 */
	i = 0;
	while (font_data->wmf[i].name)
	{	if (strcmp (font->lfFaceName,font_data->wmf[i].name) == 0)
		{	mapping = font_data->wmf[i].name;
			break;
		}
		i++;
	}

	/* If no exact match, check for a close (i.e., sub-string) match:
	 */
	if (mapping == 0)
	{	i = 0;
		while (font_data->sub[i].name)
		{	if (wmf_strstr (font->lfFaceName,font_data->sub[i].name))
			{	mapping = font_data->sub[i].mapping;
				break;
			}
			i++;
		}
	}

	/* If still no match, use the default:
	 */
	if (mapping == 0) mapping = DefaultFontMapping;

	/* Check chosen mapping in list of exact names:
	 */
	i = 0;
	while (font_data->wmf[i].name)
	{	if (strcmp (mapping,font_data->wmf[i].name) == 0)
		{	index = i;
			break;
		}
		i++;
	}

	/* If not found then something odd has happened - probably a typo somewhere...
	 */
	if (font_data->wmf[i].name == 0)
	{	WMF_ERROR (API,"Glitch! Unmapped font...");
		API->err = wmf_E_Glitch;
		return (0);
	}

	/* Select a new mapping (the PS font name) on basis of weight & italic parameters
	 */
	if (font->lfWeight > 550) /* => Bold (?? Don't ask me...) */
	{	if (font->lfItalic == 1) /* Italic */
		{	mapping = font_data->wmf[index].bolditalic;
		}
		else
		{	mapping = font_data->wmf[index].bold;
		}
	}
	else
	{	if (font->lfItalic == 1) /* Italic */
		{	mapping = font_data->wmf[index].italic;
		}
		else
		{	mapping = font_data->wmf[index].normal;
		}
	}

	return (mapping);
}
