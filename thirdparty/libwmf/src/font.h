/* libwmf ("font.h"): library for wmf conversion
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


#ifndef WMF_FONT_H
#define WMF_FONT_H

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_GLYPH_H

static void ipa_font_add_wmf (wmfAPI*,wmfFontMap*);
static void ipa_font_add_sub (wmfAPI*,wmfMapping*);
static void ipa_font_add_ps (wmfAPI*,wmfMapping*);
static void ipa_font_add_cache (wmfAPI*,wmfFT_CacheEntry*);
static void ipa_font_add_api (wmfAPI*,char*);

static float ipa_char_position (wmfFont*,char*,char*);

static wmfXML_FontInfo* ipa_font_sys_map (wmfAPI*,wmfFont*);
static FT_Face          ipa_font_sys_face (wmfAPI*,wmfFont*,wmfXML_FontInfo*);

#ifdef HAVE_EXPAT
static void exml_start (void*,const char*,const char**);
static void exmlfontmap_read (wmfAPI*,wmfXML_FontData*,char*);

typedef struct _EXML_FontData EXML_FontData;

struct _EXML_FontData
{	wmfAPI* API;

	wmfXML_FontData* FD;
};
#endif /* HAVE_EXPAT */

#ifdef HAVE_LIBXML2
static void xml2_start (void*,const char*,const char**);
static void xml2fontmap_read (wmfAPI*,wmfXML_FontData*,char*);

typedef struct _XML2_FontData XML2_FontData;

struct _XML2_FontData
{	wmfAPI* API;

	wmfXML_FontData* FD;
};
#endif /* HAVE_LIBXML2 */

static void            ipa_font_gs_file (wmfAPI* API,wmfGS_FontData* FD,char* file);
static void            ipa_font_gs_add (wmfAPI* API,wmfGS_FontData* FD,char* name,char* alias);
static char*           ipa_font_gs_alias (wmfGS_FontData* FD,char* name);
static char*           ipa_font_gs_readline (wmfAPI* API,FILE* in);
static wmfGS_FontInfo* ipa_font_gs_map (wmfAPI* API,wmfFont* font);
static FT_Face         ipa_font_gs_face (wmfAPI* API,wmfFont* font,wmfGS_FontInfo* FI);

static FT_Face ipa_font_face (wmfAPI* API,wmfFont* font,char* ps_name);
static FT_Face ipa_font_face_open (wmfAPI* API,char* ps_name,char* glyphs,char* metrics);
static FT_Face ipa_font_face_cached (wmfAPI* API,wmfFont* font,char* ps_name);

static char* ipa_font_path_find (wmfAPI* API,char* file_name);

static char* ipa_font_std (wmfAPI* API,wmfFont* font);

/* postscript (standard 13) font name mapping to type 1 font file
 */
static wmfMapping PSFontMap[13] = {
	{	"Courier",		"NimbusMonoPS-Regular.t1",	ft_encoding_adobe_standard	},
	{	"Courier-Oblique",	"NimbusMonoPS-Italic.t1",	ft_encoding_adobe_standard	},
	{	"Courier-Bold",		"NimbusMonoPS-Bold.t1",	ft_encoding_adobe_standard	},
	{	"Courier-BoldOblique",	"NimbusMonoPS-BoldItalic.t1",	ft_encoding_adobe_standard	},
	{	"Helvetica",		"NimbusSans-Regular.t1",	ft_encoding_adobe_standard	},
	{	"Helvetica-Oblique",	"NimbusSans-Italic.t1",	ft_encoding_adobe_standard	},
	{	"Helvetica-Bold",	"NimbusSans-Bold.t1",	ft_encoding_adobe_standard	},
	{	"Helvetica-BoldOblique","NimbusSans-BoldItalic.t1",	ft_encoding_adobe_standard	},
	{	"Times-Roman",		"NimbusRoman-Regular.t1",	ft_encoding_adobe_standard	},
	{	"Times-Italic",		"NimbusRoman-Italic.t1",	ft_encoding_adobe_standard	},
	{	"Times-Bold",		"NimbusRoman-Bold.t1",	ft_encoding_adobe_standard	},
	{	"Times-BoldItalic",	"NimbusRoman-BoldItalic.t1",	ft_encoding_adobe_standard	},
	{	"Symbol",		"StandardSymbolsPS.t1",	ft_encoding_adobe_custom  	}};

/* exact WMF font name to postscript (standard 13) equivalent...
 * well, yeah, I know. :-(
 */
static wmfFontMap WMFFontMap[] = {
{ "Courier",		"Courier",     "Courier-Oblique",   "Courier-Bold",   "Courier-BoldOblique"   },
{ "Helvetica",		"Helvetica",   "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique" },
{ "Modern",		"Courier",     "Courier-Oblique",   "Courier-Bold",   "Courier-BoldOblique"   },
{ "Monotype Corsiva",	"Courier",     "Courier-Oblique",   "Courier-Bold",   "Courier-BoldOblique"   },
{ "News Gothic",	"Helvetica",   "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique" },
{ "Symbol",		"Symbol",      "Symbol",            "Symbol",         "Symbol"                },
{ "System",		"Courier",     "Courier-Oblique",   "Courier-Bold",   "Courier-BoldOblique"   },
{ "Times",		"Times-Roman", "Times-Italic",      "Times-Bold",     "Times-BoldItalic"      }};

/* Sub-string match if not in the above list;
 */
static wmfMapping SubFontMap[] = {
	{	"Arial",		"Helvetica", ft_encoding_adobe_standard	},
	{	"Courier",		"Courier", ft_encoding_adobe_standard	},
	{	"Fixed",		"Courier", ft_encoding_adobe_standard	},
	{	"Helvetica",		"Helvetica", ft_encoding_adobe_standard	},
	{	"Sans",			"Helvetica", ft_encoding_adobe_standard	},
	{	"Sym",			"Symbol", ft_encoding_adobe_standard	},
	{	"Terminal",		"Courier", ft_encoding_adobe_standard	},
	{	"Times",		"Times", ft_encoding_adobe_standard	},
	{	"Wingdings",		"Symbol", ft_encoding_adobe_standard	}};

/* If all else fails, assume Times
 */
static char* DefaultFontMapping = "Times";

#endif /* ! WMF_FONT_H */
