/* libwmf (ipa/ipa.c): library for wmf conversion
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


#ifdef HAVE_CONFIG_H
#include "wmfconfig.h"
#endif /* HAVE_CONFIG_H */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Define WMF_API if this is module so that ipa headers are included via "wmfdefs.h" first
 */
#define WMF_IPA 1

#include "wmfdefs.h"

#include "ipa/ipa.h"
#include "ipa/ipa/bmp.h"   /* Provides default bitmap functionality */

#define NPageTypes 11

static ipa_page_info PageInfo[NPageTypes] = {
	{ wmf_P_A5,		"A5",		 420,  595 },
	{ wmf_P_A4,		"A4",		 595,  842 },
	{ wmf_P_A3,		"A3",		 842, 1191 },
	{ wmf_P_A2,		"A2",		1191, 1684 },
	{ wmf_P_A1,		"A1",		1684, 2384 },
	{ wmf_P_A0,		"A0",		2384, 3370 },
	{ wmf_P_B5,		"B5",		 516,  729 },
	{ wmf_P_Letter,		"Letter",	 612,  792 },
	{ wmf_P_Legal,		"Legal",	 612, 1008 },
	{ wmf_P_Ledger,		"Ledger",	1224,  792 },
	{ wmf_P_Tabloid,	"Tabloid",	 792, 1224 }};

char* wmf_ipa_page_format (wmfAPI* API,wmf_page_t type)
{	int i;

	char* format = 0;

	for (i = 0; i < NPageTypes; i++)
	{	if (PageInfo[i].type == type)
		{	format = PageInfo[i].format;
			break;
		}
	}

	if (format == 0)
	{	WMF_ERROR (API,"Glitch! unexpected page type!");
		API->err = wmf_E_Glitch;
	}

	return (format);
}

unsigned int wmf_ipa_page_width (wmfAPI* API,wmf_page_t type)
{	int i;

	unsigned int length = 0;

	for (i = 0; i < NPageTypes; i++)
	{	if (PageInfo[i].type == type)
		{	length = PageInfo[i].width;
			break;
		}
	}

	if (length == 0)
	{	WMF_ERROR (API,"Glitch! unexpected page type!");
		API->err = wmf_E_Glitch;
	}

	return (length);
}

unsigned int wmf_ipa_page_height (wmfAPI* API,wmf_page_t type)
{	int i;

	unsigned int length = 0;

	for (i = 0; i < NPageTypes; i++)
	{	if (PageInfo[i].type == type)
		{	length = PageInfo[i].height;
			break;
		}
	}

	if (length == 0)
	{	WMF_ERROR (API,"Glitch! unexpected page type!");
		API->err = wmf_E_Glitch;
	}

	return (length);
}
