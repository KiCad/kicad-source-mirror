/* libwmf (meta.c): library for wmf conversion
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
#endif

#include <stdio.h>

#include "wmfdefs.h"

static void write_header (wmfAPI*,long,long);

static void write_str (wmfAPI*,const char*);
static void write_b64 (wmfAPI*,const unsigned char*,unsigned long);

/**
 * @internal
 */
U16 wmf_read_16 (wmfAPI* API)
{	int i1;
	int i2;

	U16 bb = 0;

	if (API->bbuf.read == 0)
	{	WMF_ERROR (API,"wmf_read_16: no input stream open!");
		API->err = wmf_E_BadFile;
	}
	else
	{	i1 = WMF_READ (API);
		i2 = WMF_READ (API);

		if ((i1 == EOF) || (i2 == EOF))
		{	WMF_DEBUG (API,"wmf_read_16: (EOF)");
			API->err = wmf_E_EOF;
		}
		else bb = (((U16) i2) << 8) + ((U16) i1);
	}

	return (bb);
}

/**
 * @internal
 */
U32 wmf_read_32 (wmfAPI* API,U16* u16a,U16* u16b)
{	U16 w1;
	U16 w2;

	U32 ww = 0;

	w1 = wmf_read_16 (API);
	w2 = wmf_read_16 (API);

	if (u16a) (*u16a) = w1;
	if (u16b) (*u16b) = w2;

	if (API->err == wmf_E_None) ww = WMF_U16_U16_to_U32 (w1,w2);

	return (ww);
}

/**
 * Compute the Aldus checksum of the metafile's header.
 * 
 * @param API the API handle
 * 
 * (Must read the header first, either via wmf_scan() or by calling wmf_header_read() directly.)
 * 
 * @return Returns the checksum.
 */
U16 wmf_aldus_checksum (wmfAPI* API)
{	U16 Checksum = 0;

	Checksum ^=  API->File->pmh->Key & 0x0000FFFFUL;
	Checksum ^= (API->File->pmh->Key & 0xFFFF0000UL) >> 16;
	Checksum ^=  API->File->pmh->Handle;
	Checksum ^=  API->File->pmh->Left;
	Checksum ^=  API->File->pmh->Top;
	Checksum ^=  API->File->pmh->Right;
	Checksum ^=  API->File->pmh->Bottom;
	Checksum ^=  API->File->pmh->Inch;
	Checksum ^=  API->File->pmh->Reserved & 0x0000FFFFUL;
	Checksum ^= (API->File->pmh->Reserved & 0xFFFF0000UL) >> 16;

	return (Checksum);
} 

/**
 * Read the metafile's header.
 * 
 * @param API the API handle
 * 
 * (Must read the header first, either via wmf_scan() or by calling wmf_header_read() directly.)
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error states of \b wmf_E_Glitch, \b wmf_E_BadFormat (bad header),
 *         \b wmf_E_BadFile (no open input stream) and \b wmf_E_EOF (premature end-of-file).
 */
wmf_error_t wmf_header_read (wmfAPI* API)
{	U16 u16a;
	U16 u16b;

	long header_start = WMF_TELL (API);
	long header_end = 0;

	if ((API->File->pmh->Key = wmf_read_32 (API,&u16a,&u16b)) == 0x9ac6cdd7)
	{	API->File->placeable = 1;

		API->File->pmh->Handle	= wmf_read_16 (API);

		u16a = wmf_read_16 (API);  API->File->pmh->Left   = U16_2_S16 (u16a);
		u16a = wmf_read_16 (API);  API->File->pmh->Top    = U16_2_S16 (u16a);
		u16a = wmf_read_16 (API);  API->File->pmh->Right  = U16_2_S16 (u16a);
		u16a = wmf_read_16 (API);  API->File->pmh->Bottom = U16_2_S16 (u16a);

		API->File->pmh->Inch     = wmf_read_16 (API);
		API->File->pmh->Reserved = wmf_read_32 (API,0,0);
		API->File->pmh->Checksum = wmf_read_16 (API);

		API->File->wmfheader->FileType   = wmf_read_16 (API);
		API->File->wmfheader->HeaderSize = wmf_read_16 (API);
	}
	else
	{	API->File->placeable = 0;

		API->File->pmh->Key = 0;

		API->File->pmh->Handle   = 0;
		API->File->pmh->Left     = 0;
		API->File->pmh->Top      = 0;
		API->File->pmh->Right    = 0; /* was 6000 */
		API->File->pmh->Bottom   = 0; /* was 6000 */
		API->File->pmh->Inch     = 0;
		API->File->pmh->Reserved = 0;
		API->File->pmh->Checksum = 0;

		API->File->wmfheader->FileType   = u16a; /* from key-check above */
		API->File->wmfheader->HeaderSize = u16b;
	}

	if (ERR (API)) return (API->err);

	if (API->File->wmfheader->HeaderSize == 9)
	{	API->File->wmfheader->Version       = wmf_read_16 (API);
		API->File->wmfheader->FileSize      = wmf_read_32 (API,0,0);
		API->File->wmfheader->NumOfObjects  = wmf_read_16 (API);
		API->File->wmfheader->MaxRecordSize = wmf_read_32 (API,0,0);
		API->File->wmfheader->NumOfParams   = wmf_read_16 (API);

		API->File->pos = WMF_TELL (API);
		header_end = API->File->pos;
		write_header (API, header_start, header_end);
	}
	else
	{	WMF_ERROR (API,"wmf_header_read: this isn't a wmf file");
		API->err = wmf_E_BadFormat;
	}

	return (API->err);
}

static void write_header (wmfAPI* API, long header_start, long header_end)
{	unsigned char * header = 0;

	long i; 
	long header_size = header_end - header_start;

	int byte;

	wmfAttributes attrlist;

	if (API->write_data == 0) return;

	if (header_size <= 0)
	{	WMF_ERROR (API, "Glitch!");
		API->err = wmf_E_Glitch;
		return;
	}

	header = (unsigned char *) wmf_malloc (API, header_size);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	wmf_attr_new (API, &attrlist);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	if (WMF_SEEK (API,header_start) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return;
	}

	for (i = 0; i < header_size; i++)
	{	byte = WMF_READ (API);
		if (byte == EOF)
		{	WMF_ERROR (API, "Glitch!");
			API->err = wmf_E_Glitch;
			break;
		}
		header[i] = (unsigned char) (byte & 0xff);
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	wmf_write (API, 0, 0, "header", attrlist.atts, header, header_size);

	wmf_attr_free (API, &attrlist);
	wmf_free (API, header);
}

typedef enum _wmf_write_t
{	wmf_W_WMF = 0,
	wmf_W_XML
} wmf_write_t;

typedef struct _wmfWriteFileInfo wmfWriteFileInfo;

struct _wmfWriteFileInfo
{	FILE* file;
	wmf_write_t type;
};

void wmf_write (wmfAPI * API, unsigned long Size, unsigned int Function, const char * name,
		char ** atts, const unsigned char * buffer, unsigned long length)
{	char ** attr = 0;

	unsigned char prefix[6]; 

	wmfWriteFileInfo * WFI = (wmfWriteFileInfo *) API->write_data;

	if (WFI == 0) return;

	prefix[0] = (unsigned char) ( Size        & 0xff);
	prefix[1] = (unsigned char) ((Size >>  8) & 0xff);
	prefix[2] = (unsigned char) ((Size >> 16) & 0xff);
	prefix[3] = (unsigned char) ((Size >> 24) & 0xff);

	prefix[4] = (unsigned char) ( Function       & 0xff);
	prefix[5] = (unsigned char) ((Function >> 8) & 0xff);

	if (WFI->type == wmf_W_WMF)
	{	if (strcmp (name, "header")) fwrite (prefix, 1, 6, WFI->file);
		if (buffer && length) fwrite (buffer, 1, length, WFI->file);
		return;
	}

	if (name == 0) return;
	fprintf (WFI->file, " <%s", name);

	if (atts)
	{	attr = atts;
		while (*attr)
		{	fprintf (WFI->file, " %s=\"", *attr++);
			if (*attr) fputs (*attr, WFI->file);
			fputs ("\"", WFI->file);
			attr++;
		}
	}

	fputs (">\n", WFI->file);

	if (strcmp (name, "header")) write_b64 (API, prefix, 6);

	if (buffer && length) write_b64 (API, buffer, length);

	fprintf (WFI->file, " </%s>\n", name);
}

void wmf_write_begin (wmfAPI * API, const char * filename)
{	int length;

	wmfWriteFileInfo * WFI = (wmfWriteFileInfo *) wmf_malloc (API, sizeof(wmfWriteFileInfo));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	if (filename == 0)
	{	WMF_ERROR (API,"Glitch!");
		API->err = wmf_E_Glitch;
		return;
	}

	length = strlen (filename);
	if (length < 5)
	{	WMF_ERROR (API,"Bad [--wmf-write] filename! expected *.wmf or *.xml");
		API->err = wmf_E_BadFile;
		return;
	}
	if ((strcmp (filename + length - 4, ".wmf") == 0) ||
	    (strcmp (filename + length - 4, ".WMF") == 0))
	{	WFI->type = wmf_W_WMF;
	}
	else if ((strcmp (filename + length - 4, ".xml") == 0) ||
		 (strcmp (filename + length - 4, ".XML") == 0))
	{	WFI->type = wmf_W_XML;
	}
	else
	{	WMF_ERROR (API,"Bad [--wmf-write] filename! expected *.wmf or *.xml");
		API->err = wmf_E_BadFile;
		return;
	}

	if (WFI->type == wmf_W_WMF) WFI->file = fopen (filename, "wb");
	if (WFI->type == wmf_W_XML) WFI->file = fopen (filename, "w");

	if (WFI->file == 0)
	{	WMF_ERROR (API,"Unable to open [--wmf-write] file for writing!");
		API->err = wmf_E_BadFile;
		return;
	}

	API->write_data = (void *) WFI;

	if (WFI->type == wmf_W_XML)
	{	API->flags |= API_ENABLE_EDITING;

		write_str (API, "<?xml version=\"1.0\"?>\n");
		write_str (API, "<wmfxml>\n");
	}
}

void wmf_write_end (wmfAPI * API)
{	wmfWriteFileInfo * WFI = (wmfWriteFileInfo *) API->write_data;

	if (WFI == 0) return;

	if (WFI->type == wmf_W_XML) write_str (API, "</wmfxml>\n");

	fclose (WFI->file);
	wmf_free (API, API->write_data);
	API->write_data = 0;
}

static void write_str (wmfAPI * API, const char * str)
{	wmfWriteFileInfo * WFI = (wmfWriteFileInfo *) API->write_data;

	if (WFI == 0) return;
	if (WFI->type == wmf_W_WMF) return;

	if ( str == 0) return;
	if (*str == 0) return;

	fputs (str, WFI->file);
}

static void write_b64 (wmfAPI * API, const unsigned char * buffer, unsigned long length)
{	static char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned long i;
	unsigned long b32;
	unsigned long remaining = length;

	const unsigned char * bufptr = buffer;

	char buf[74];
	char * ptr = 0;

	wmfWriteFileInfo * WFI = (wmfWriteFileInfo *) API->write_data;

	if (WFI == 0) return;
	if (WFI->type == wmf_W_WMF) return;

	if (length == 0) return;

	if (buffer == 0)
	{	WMF_ERROR (API, "Glitch!");
		API->err = wmf_E_Glitch;
		return;
	}

	buf[72] = '\n';
	buf[73] = 0;
	while (remaining >= 54)
	{	ptr = buf;
		for (i = 0; i < 18; i++)
		{	b32 = (unsigned long) *bufptr++;
			b32 = (b32 << 8) | (unsigned long) *bufptr++;
			b32 = (b32 << 8) | (unsigned long) *bufptr++;
			*ptr++ = B64[(b32 >> 18)       ];
			*ptr++ = B64[(b32 >> 12) & 0x3f];
			*ptr++ = B64[(b32 >>  6) & 0x3f];
			*ptr++ = B64[ b32        & 0x3f];
		}
		fputs (buf, WFI->file);
		remaining -= 54;
	}

	ptr = buf;
	while (remaining >= 3)
	{	b32 = (unsigned long) *bufptr++;
		b32 = (b32 << 8) | (unsigned long) *bufptr++;
		b32 = (b32 << 8) | (unsigned long) *bufptr++;
		*ptr++ = B64[(b32 >> 18)       ];
		*ptr++ = B64[(b32 >> 12) & 0x3f];
		*ptr++ = B64[(b32 >>  6) & 0x3f];
		*ptr++ = B64[ b32        & 0x3f];
		remaining -= 3;
	}
	if (remaining == 2)
	{	b32 = (unsigned long) *bufptr++;
		b32 = (b32 << 8) | (unsigned long) *bufptr++;
		b32 = (b32 << 8);
		*ptr++ = B64[(b32 >> 18)       ];
		*ptr++ = B64[(b32 >> 12) & 0x3f];
		*ptr++ = B64[(b32 >>  6) & 0x3f];
		remaining -= 2;
	}
	if (remaining == 1)
	{	b32 = (unsigned long) *bufptr++;
		b32 = (b32 << 16);
		*ptr++ = B64[(b32 >> 18)       ];
		*ptr++ = B64[(b32 >> 12) & 0x3f];
	}

	*ptr++ = '\n';
	*ptr++ = 0;
	fputs (buf, WFI->file);
}

void wmf_attr_new (wmfAPI * API, wmfAttributes * list)
{	if (list == 0) return;

	list->name = 0;

	list->buffer = 0;
	list->length = 0;
	list->offset = 0;

	list->count = 0;
	list->max = 0;
	list->atts = (char **) wmf_malloc (API, 2 * (8 + 1) * sizeof (char *));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return;
	}

	list->max = 8;
	list->atts[list->count] = 0;
}

void wmf_attr_clear (wmfAPI * API, wmfAttributes * list)
{	unsigned long i;

	if (list == 0) return;

	for (i = 0; i < (2 * list->count); i++) wmf_free (API, list->atts[i]);
	list->count = 0;

	list->atts[list->count] = 0;

	if (list->name)
	{	wmf_free (API, list->name);
		list->name = 0;
	}
	list->buffer = 0;
	list->length = 0;
}

void wmf_attr_free (wmfAPI * API, wmfAttributes * list)
{	if (list == 0) return;

	wmf_attr_clear (API, list);

	if (list->atts)
	{	wmf_free (API, list->atts);
		list->atts = 0;
	}
	list->max = 0;
}

const char * wmf_attr_add (wmfAPI * API, wmfAttributes * list,
			   const char * name, const char * value)
{	unsigned long i;

	char ** more = 0;

	char * copy_name = 0;
	char * copy_value = 0;

	if (list == 0) return 0;
	if (list->atts == 0) return 0;

	copy_value = wmf_strdup (API, value);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return 0;
	}

	for (i = 0; i < (2 * list->count); i += 2)
	{	if (strcmp (list->atts[i], name) == 0)
		{	copy_name = list->atts[i];
			wmf_free (API, list->atts[i+1]);
			list->atts[i+1] = copy_value;
			break;
		}
	}
	if (copy_name) return copy_value;

	copy_name = wmf_strdup (API, name);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return 0;
	}

	if (list->count == list->max)
	{	more = (char **) wmf_realloc (API, list->atts, 2 * (list->max + 8 + 1) * sizeof (char *));

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			return 0;
		}

		list->atts = more;
		list->max += 8;
	}
	list->atts[list->count++] = copy_name;
	list->atts[list->count++] = copy_value;

	list->atts[list->count] = 0;

	return copy_value;
}

const char * wmf_attr_query (wmfAPI * API, wmfAttributes * list, const char* name)
{	(void)API;
	unsigned long i;

	const char * value = 0;

	if (list == 0) return 0;
	if (list->atts == 0) return 0;

	for (i = 0; i < (2 * list->count); i += 2)
	{	if (strcmp (list->atts[i], name) == 0)
		{	value = list->atts[i+1];
			break;
		}
	}
	return value;
}
