/* libwmf (xml.c): library for wmf conversion
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

#include "wmfdefs.h"

#include "xml.h"

static int s_value (char c)
{	int v = -1;

	if ((c >= 'A') && (c <= 'Z')) v = c - 'A';
	else if ((c >= 'a') && (c <= 'z')) v = 26 + c - 'a';
	else if ((c >= '0') && (c <= '9')) v = 52 + c - '0';
	else if (c == '+') v = 62;
	else if (c == '/') v = 63;

	return v;
}

static void cd_append (wmfXML_InputData * data, const char * buffer, unsigned long length)
{	unsigned long i;
	unsigned long size; 

	int v;

	char c;

	char * more = 0;

	const char * ptr = buffer;

	if ((length == 0) || (buffer == 0)) return;

	if (data->cd_buffer == 0)
	{	more = (char *) wmf_malloc (data->API, length);

		if (ERR (data->API))
		{	WMF_DEBUG (data->API,"bailing...");
			return;
		}

		data->cd_buffer = more;
		data->cd_bufptr = data->cd_buffer;
		data->cd_max = length;
	}
	if (data->cd_length + length > data->cd_max)
	{	size = data->cd_length + length;
		more = (char *) wmf_realloc (data->API, data->cd_buffer, size);

		if (ERR (data->API))
		{	WMF_DEBUG (data->API,"bailing...");
			return;
		}

		data->cd_buffer = more;
		data->cd_bufptr = data->cd_buffer + data->cd_length;
		data->cd_max = size;
	}

	for (i = 0; i < length; i++)
	{	c = *ptr++;
		v = s_value (c);
		if (v >= 0)
		{	*(data->cd_bufptr++) = c;
			  data->cd_length++;
		}
	}
}

static void mem_append (wmfXML_InputData * data, unsigned char * buffer, unsigned long length)
{	unsigned char * more = 0;

	unsigned long size; 

	if ((length == 0) || (buffer == 0)) return;

	if (data->mem_buffer == 0)
	{	more = (unsigned char *) wmf_malloc (data->API, length);

		if (ERR (data->API))
		{	WMF_DEBUG (data->API,"bailing...");
			return;
		}

		data->mem_buffer = more;
		data->mem_bufptr = data->mem_buffer;
		data->mem_max = length;
	}
	if (data->mem_length + length > data->mem_max)
	{	size = data->mem_length + length;
		more = (unsigned char *) wmf_realloc (data->API, data->mem_buffer, size);

		if (ERR (data->API))
		{	WMF_DEBUG (data->API,"bailing...");
			return;
		}

		data->mem_buffer = more;
		data->mem_bufptr = data->mem_buffer + data->mem_length;
		data->mem_max = size;
	}

	memcpy (data->mem_bufptr, buffer, length);

	data->mem_bufptr += length;
	data->mem_length += length;
}

static void store_clear (wmfAPI * API)
{	unsigned long i;

	for (i = 0; i < API->store.count; i++) wmf_attr_free (API, API->store.attrlist + i);
	API->store.count = 0;
}

static void store_atts (wmfAPI * API, const char * element_name, const char ** atts)
{	unsigned long size;

	const char ** attr = 0;

	const char * name  = 0;
	const char * value = 0;

	wmfAttributes * more = 0;

	if (API->store.attrlist == 0)
	{	more = (wmfAttributes *) wmf_malloc (API, 16 * sizeof (wmfAttributes));

		if (ERR (API))
		{	WMF_DEBUG (API, "bailing...");
			return;
		}

		API->store.attrlist = more;

		API->store.max   = 16;
		API->store.count =  0;
	}
	if (API->store.count == API->store.max)
	{	size = (API->store.max + 16) * sizeof (wmfAttributes);
		more = (wmfAttributes *) wmf_realloc (API, API->store.attrlist, size);

		if (ERR (API))
		{	WMF_DEBUG (API, "bailing...");
			return;
		}

		API->store.attrlist = more;

		API->store.max += 16;
	}

	more = API->store.attrlist + API->store.count++;

	wmf_attr_new (API, more);
	more->name = wmf_strdup (API, element_name);

	if (atts == 0) return;

	attr = atts;
	while (*attr)
	{	name  = *attr++;
		value = *attr++;
		wmf_attr_add (API, more, name, value);
	}
}

static void xml_start (void * user_data, const char * name, const char ** atts)
{	wmfXML_InputData * input_data = (wmfXML_InputData *) user_data;

	wmfAPI* API = input_data->API;

	if (ERR (API)) return;

	if (input_data->wmfxml == 0)
	{	if (strcmp (name, "wmfxml") == 0)
		{	input_data->wmfxml =  1;
		}
		else
		{	input_data->wmfxml = -1;
		}
		return;
	}
	if (input_data->wmfxml != 1) return;

	store_atts (API, name, atts);

	input_data->cd_bufptr = input_data->cd_buffer;
	input_data->cd_length = 0;
}

static void xml_data (void * user_data, const char * buffer, int length)
{	wmfXML_InputData * input_data = (wmfXML_InputData *) user_data;

	if (ERR (input_data->API)) return;

	if (input_data->wmfxml != 1) return;

	cd_append (input_data, buffer, (unsigned long) ((unsigned) length));
}

static void xml_end (void * user_data, const char * name)
{	unsigned char buffer[54];

	unsigned char * bufptr = 0;

	char * cd_ptr = 0;

	unsigned long i;
	unsigned long b32;
	unsigned long length;
	unsigned long remaining;

	wmfXML_InputData * input_data = (wmfXML_InputData *) user_data;

	wmfAttributes * attr = 0;

	wmfAPI * API = input_data->API;

	if (ERR (API)) return;

	if (input_data->wmfxml != 1) return;

	if (strcmp (name, "wmfxml") == 0)
	{	for (i = 0; i < API->store.count; i++)
		{	attr = API->store.attrlist + i;
			attr->buffer = input_data->mem_buffer + attr->offset;
		}
		return;
	}
	attr = API->store.attrlist + (API->store.count - 1);

	attr->length = 0;
	attr->offset = (unsigned long) (input_data->mem_bufptr - input_data->mem_buffer);

	cd_ptr = input_data->cd_buffer;
	remaining = input_data->cd_length;
	while (remaining >= 72)
	{	bufptr = buffer;
		for (i = 0; i < 18; i++)
		{	b32 =              (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
			b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
			b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
			b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);

			*bufptr++ = (unsigned char) ((b32 >> 16)       );
			*bufptr++ = (unsigned char) ((b32 >>  8) & 0xff);
			*bufptr++ = (unsigned char) ((b32      ) & 0xff);
		}
		mem_append (input_data, buffer, 54);
		remaining -= 72;
		attr->length += 54;
	}

	bufptr = buffer;
	length = 0;
	while (remaining >= 4)
	{	b32 =              (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);

		*bufptr++ = (unsigned char) ((b32 >> 16)       );
		*bufptr++ = (unsigned char) ((b32 >>  8) & 0xff);
		*bufptr++ = (unsigned char) ((b32      ) & 0xff);

		remaining -= 4;
		length += 3;
	}
	if (remaining == 3)
	{	b32 =              (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 6);

		*bufptr++ = (unsigned char) ((b32 >> 16)       );
		*bufptr++ = (unsigned char) ((b32 >>  8) & 0xff);

		remaining -= 3;
		length += 2;
	}
	if (remaining == 2)
	{	b32 =               (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 <<  6) | (unsigned long) ((unsigned) s_value (*(cd_ptr++)) & 0x3F);
		b32 = (b32 << 12);

		*bufptr++ = (unsigned char) ((b32 >> 16)       );

		remaining -= 2;
		length++;
	}
	if (remaining == 1) { WMF_DEBUG (API, "xml_end: WMF Record has unexpected length!"); }

	if (length)
	{	mem_append (input_data, buffer, length);
		attr->length += length;
	}

	input_data->cd_bufptr = input_data->cd_buffer;
	input_data->cd_length = 0;
}

#ifdef HAVE_EXPAT

#include <expat.h>

#define EXML_BUFSIZE 1024

wmf_error_t wmf_wmfxml_import (wmfAPI * API, const char * wmfxml_filename)
{	char buffer[EXML_BUFSIZE];

	int status;
	int length;

	FILE * in = 0;

	XML_Parser parser = 0;

	wmfXML_InputData input_data;

	input_data.API = API;

	input_data.cd_buffer = 0;
	input_data.cd_bufptr = 0;

	input_data.cd_length = 0;
	input_data.cd_max    = 0;

	input_data.mem_buffer = 0;
	input_data.mem_bufptr = 0;

	input_data.mem_length = 0;
	input_data.mem_max    = 0;

	input_data.wmfxml = 0;

	if (ERR (API)) return API->err;

	store_clear (API);

	if ( wmfxml_filename == 0) return wmf_E_BadFile;
	if (*wmfxml_filename == 0) return wmf_E_BadFile;

	in = fopen (wmfxml_filename, "r");

	if (in == 0)
	{	WMF_DEBUG (API, "wmf_wmfxml_import: unable to open file");
		return wmf_E_BadFile;
	}

	parser = XML_ParserCreate (0);

	if (parser == 0)
	{	WMF_DEBUG (API, "wmf_wmfxml_import: error creating expat-xml parser");
		fclose (in);
		return wmf_E_InsMem;
	}

	XML_SetUserData (parser, (void *) (&input_data));

	XML_SetStartElementHandler  (parser, xml_start);
	XML_SetEndElementHandler    (parser, xml_end);
	XML_SetCharacterDataHandler (parser, xml_data);

	status = 0;
	while (fgets (buffer, EXML_BUFSIZE, in) && (API->err == wmf_E_None))
	{	length = (int) strlen (buffer);

		if (XML_Parse (parser, buffer, length, 0) == 0)
		{	WMF_DEBUG (API,"wmf_wmfxml_import: error parsing file");
			status = 1;
			break;
		}
	}
	if ((status == 0) && (API->err == wmf_E_None)) XML_Parse (parser, buffer, 0, 1);

	XML_ParserFree (parser);

	fclose (in);

	if (input_data.cd_buffer) wmf_free (API, input_data.cd_buffer);

	if (ERR (API))
	{	if (input_data.mem_buffer) wmf_free (API, input_data.mem_buffer);
		return API->err;
	}

	API->flags |= API_ENABLE_EDITING;

	return wmf_mem_open (API, input_data.mem_buffer, (long) input_data.mem_length);
}
#endif /* HAVE_EXPAT */

#ifdef HAVE_LIBXML2

#include <libxml/parser.h>
#include <libxml/parserInternals.h>

wmf_error_t wmf_wmfxml_import (wmfAPI * API, const char * wmfxml_filename)
{	xmlParserCtxtPtr ctxt = 0;

	xmlSAXHandler sax;

	wmfXML_InputData input_data;

	input_data.API = API;

	input_data.cd_buffer = 0;
	input_data.cd_bufptr = 0;

	input_data.cd_length = 0;
	input_data.cd_max    = 0;

	input_data.mem_buffer = 0;
	input_data.mem_bufptr = 0;

	input_data.mem_length = 0;
	input_data.mem_max    = 0;

	input_data.wmfxml = 0;

	if (ERR (API)) return API->err;

	store_clear (API);

	if ( wmfxml_filename == 0) return wmf_E_BadFile;
	if (*wmfxml_filename == 0) return wmf_E_BadFile;

	memset ((void*) (&sax), 0, sizeof (xmlSAXHandler));

	sax.startElement = (startElementSAXFunc)xml_start;
	sax.endElement   = (endElementSAXFunc)xml_end;
	sax.characters   = (charactersSAXFunc)xml_data;

	ctxt = xmlCreateFileParserCtxt (wmfxml_filename);

	if (ctxt == 0) return wmf_E_InsMem;

	ctxt->sax = &sax;
	ctxt->userData = (void*) (&input_data);

	xmlParseDocument (ctxt);

	ctxt->sax = 0;

	xmlFreeParserCtxt (ctxt);

	if (input_data.cd_buffer) wmf_free (API, input_data.cd_buffer);

	if (ERR (API))
	{	if (input_data.mem_buffer) wmf_free (API, input_data.mem_buffer);
		return API->err;
	}

	API->flags |= API_ENABLE_EDITING;

	return wmf_mem_open (API, input_data.mem_buffer, (long) input_data.mem_length);
}

#endif /* HAVE_LIBXML2 */
