/* libwmf (<libwmf/font.h>): library for wmf conversion
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


#ifndef LIBWMF_FONT_H
#define LIBWMF_FONT_H

#include <libwmf/ipa.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _wmfFT_Mapping          wmfFT_Mapping;
typedef struct _wmfFT_CacheEntry       wmfFT_CacheEntry;

typedef struct _wmfGS_FontData         wmfGS_FontData;
typedef struct _wmfGS_FontInfo         wmfGS_FontInfo;

typedef struct _wmfXML_FontData        wmfXML_FontData;
typedef struct _wmfXML_FontInfo        wmfXML_FontInfo;

typedef struct _wmfFontmapData         wmfFontmapData;

typedef struct _wmfIPAFont             wmfIPAFont;

typedef void  (*wmfCharDrawer)  (wmfAPI*,wmfDrawText_t*);

extern void  wmf_ipa_font_init (wmfAPI*,wmfAPI_Options*);
extern void  wmf_ipa_font_map_gs (wmfAPI*,wmfGS_FontData*,char*);
extern void  wmf_ipa_font_map_xml (wmfAPI*,wmfXML_FontData*,char*);
extern void  wmf_ipa_font_map_set (wmfAPI*,wmfMap);
extern void  wmf_ipa_font_map (wmfAPI*,wmfFont*);
extern void  wmf_ipa_font_dir (wmfAPI*,char*);
extern float wmf_ipa_font_stringwidth (wmfAPI*,wmfFont*,char*);
extern char* wmf_ipa_font_lookup (wmfAPI*,char*);

extern void wmf_ipa_draw_text (wmfAPI*,wmfDrawText_t*,wmfCharDrawer);

struct _wmfFT_Mapping
{	char* name;
	char* mapping;

	FT_Encoding encoding;
	FT_Face     face;
};

struct _wmfFT_CacheEntry
{	char* name;
	char* path;

	FT_Face face;
};

struct _wmfGS_FontData
{	unsigned int max;
	unsigned int len;

	wmfGS_FontInfo* FI;
};

struct _wmfGS_FontInfo
{	char* name;
	char* alias;
};

struct _wmfXML_FontData
{	unsigned int max;
	unsigned int len;

	wmfXML_FontInfo* FI;
};

struct _wmfXML_FontInfo
{	char* format;
	char* metrics;
	char* glyphs;
	char* name;
	char* fullname;
	char* familyname;
	char* weight;
	char* version;
	char* alias;
};

struct _wmfFontmapData
{	char** fontdirs;

	wmfFontMap*    wmf; /* {0,*}-terminated list: wmf-font-name -> ps-font-name */
	wmfMapping*    sub; /* {0,*}-terminated list: wmf-font-name substring equiv */
	wmfFT_Mapping* ps;  /* {0,*}-terminated list: ps-font-name -> pfb-file-root */

	wmfFT_CacheEntry* cache; /* {0,*}-terminated list: path / font-face cache */

	wmfGS_FontData  GS; /* structure for ghostscript font info */
	wmfXML_FontData FD; /* structure for system font info */

	FT_Library Library;
};

struct _wmfIPAFont
{	char* ps_name;

	FT_Face ft_face;
};

/**
 * WMF_FONT_PSNAME(wmfFont* F) -> (char*) font name to use in postscript output
 */
#define WMF_FONT_PSNAME(F)      ((F)->user_data ? ((wmfIPAFont*) (F)->user_data)->ps_name : 0)

/**
 * WMF_FONT_FTFACE(wmfFont* F) -> (FT_Face) freetype(2) font face
 */
#define WMF_FONT_FTFACE(F)      ((F)->user_data ? ((wmfIPAFont*) (F)->user_data)->ft_face : 0)

#ifdef __cplusplus
}
#endif

#endif /* ! LIBWMF_FONT_H */
