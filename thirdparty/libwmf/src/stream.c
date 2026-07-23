/* libwmf (stream.c): library for wmf conversion
   Copyright (C) 2001 The Free Software Foundation

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

#include "stream.h"

/**
 * Open a compressed character output stream.
 * 
 * @param API the API handle
 * @param out descriptor for a zlib compressed file stream
 * 
 * wmf_ztream_create() creates a \b wmfStream (a character-based output stream) for output to a file opened
 * via \b zlib. (Writing compressed data to memory is not currently supported.)
 * 
 * @return Returns a \b wmfStream pointer, or zero on failure.
 *         Possible library error state of \b wmf_E_InsMem.
 */
wmfStream* wmf_ztream_create (wmfAPI* API,gzFile out)
{	wmfStream*        stream = 0;
	wmfDefaultZtream* defstr = 0;

	if (out == 0)
	{	WMF_DEBUG (API,"memory ztream (compressed stream) not supported");
		return (0);
	}

	defstr = wmf_malloc (API,sizeof (wmfDefaultZtream));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	defstr->API = API;

	defstr->out = out;

	defstr->offset = 0; /* Can't reset them anyway. */

	defstr->max = 0;
	defstr->len = 0;

	defstr->buf = 0;
	defstr->ptr = 0;

	stream = wmf_malloc (API,sizeof (wmfStream));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		if (defstr->buf) wmf_free (API,defstr->buf);
		wmf_free (API,defstr);
		return (0);
	}

	stream->context = (void*) defstr;

	stream->sputs = wmf_stream_zputs;
	stream->reset = wmf_stream_rezet;

	return (stream);
}

/**
 * Open an uncompressed character output stream.
 * 
 * @param API the API handle
 * @param out descriptor for a file stream; or zero to write to memory
 * 
 * wmf_stream_create() creates a \b wmfStream (a character-based output stream) for output to a file or, if
 * \p out is zero, to memory.
 * 
 * @return Returns a \b wmfStream pointer, or zero on failure.
 *         Possible library error states of \b wmf_E_InsMem or \b wmf_E_BadFile.
 */
wmfStream* wmf_stream_create (wmfAPI* API,FILE* out)
{	wmfStream*        stream = 0;
	wmfDefaultStream* defstr = 0;

	defstr = wmf_malloc (API,sizeof (wmfDefaultStream));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	defstr->API = API;

	if (out)
	{	defstr->out = out;

		defstr->offset = ftell (out);

		if ((defstr->offset < 0) && ((API->flags & WMF_OPT_IGNORE_NONFATAL) == 0))
		{	WMF_ERROR (API,"wmf_stream_create: ftell failed on output stream");
			API->err = wmf_E_BadFile;
			wmf_free (API,defstr);
			return (0);
		}

		defstr->max = 0;
		defstr->len = 0;

		defstr->buf = 0;
		defstr->ptr = 0;
	}
	else
	{	defstr->out = 0;

		defstr->offset = 0;

		defstr->max = 256;
		defstr->len = 0;

		defstr->buf = wmf_malloc (API,defstr->max * sizeof (char));
		defstr->ptr = defstr->buf;

		if (ERR (API))
		{	WMF_DEBUG (API,"bailing...");
			wmf_free (API,defstr);
			return (0);
		}
	}

	stream = wmf_malloc (API,sizeof (wmfStream));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		if (defstr->buf) wmf_free (API,defstr->buf);
		wmf_free (API,defstr);
		return (0);
	}

	stream->context = (void*) defstr;

	stream->sputs = wmf_stream_sputs;
	stream->reset = wmf_stream_reset;

	return (stream);
}

/**
 * Close a compressed character output stream.
 * 
 * @param API    the API handle
 * @param stream stream handle
 * @param buffer (unused)
 * @param length (unused)
 */
void wmf_ztream_destroy (wmfAPI* API,wmfStream* stream,char** buffer,unsigned long* length)
{	wmfDefaultZtream* defstr = 0;

	if (stream == 0) return;

	defstr = (wmfDefaultZtream*) stream->context;

	(*(defstr->ptr)) = 0;

	if (buffer) (*buffer) = defstr->buf;
	if (length) (*length) = defstr->len;

	wmf_free (API,stream->context);
	wmf_free (API,(void*) stream);
}

/**
 * Close an uncompressed character output stream.
 * 
 * @param API    the API handle
 * @param stream stream handle
 * @param buffer for memory return
 * @param length for length return
 * 
 * In the case of write-to-memory, on return \p *buffer is an array of length \p length.
 */
void wmf_stream_destroy (wmfAPI* API,wmfStream* stream,char** buffer,unsigned long* length)
{	wmfDefaultStream* defstr = 0;

	if (stream == 0) return;

	defstr = (wmfDefaultStream*) stream->context;

	(*(defstr->ptr)) = 0;

	if (buffer) (*buffer) = defstr->buf;
	if (length) (*length) = defstr->len;

	wmf_free (API,stream->context);
	wmf_free (API,(void*) stream);
}

/**
 * Formatted print to character output stream.
 * 
 * @param API    the API handle
 * @param stream stream handle
 * @param format print format
 * 
 * With syntax similar to printf(), wmf_stream_printf() prints a formatted message to a \b wmfStream
 * character stream.
 * 
 * @return Returns the number of characters written, or zero on failure.
 *         Possible library error state of \b wmf_E_InsMem.
 */
int wmf_stream_printf (wmfAPI* API,wmfStream* stream,char* format,...)
{	int length;

	va_list argp;

	va_start (argp,format);

	for (;;)
	{	va_list argp_copy;
		va_copy (argp_copy,argp);
		length = vsnprintf (API->string_buffer.buffer,API->string_buffer.length,format,argp_copy);
		va_end (argp_copy);

		if ((length >= 0) && ((unsigned int)length < (API->string_buffer.length - 1))) break; /* i.e., success */

		if (wmf_strbuf_grow (API) == 0) break; /* i.e., probably a memory failure */
	}

	va_end (argp);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	stream->sputs (API->string_buffer.buffer,stream->context);

	return (length);
}

/**
 * @internal
 */
int wmf_stream_zputs (char* str,void* context)
{	wmfDefaultZtream* defstr = (wmfDefaultZtream*) context;

	return (gzputs (defstr->out,str));
}

/**
 * @internal
 */
int wmf_stream_sputs (char* str,void* context)
{	char* more = 0;
	char* sptr = 0;

	unsigned long max=0;

	wmfDefaultStream* defstr = (wmfDefaultStream*) context;

	wmfAPI* API = defstr->API;

	if (defstr->out)
	{	return (fputs (str,defstr->out));
	}

	sptr = str;
	while (*sptr)
	{	if ((defstr->len + 1) == defstr->max)
		{	more = (char*) wmf_realloc (API,defstr->buf,(defstr->max + 256) * sizeof (char));

			if (ERR (API))
			{	WMF_DEBUG (API,"bailing...");
				defstr->max = max;
				return (EOF);
			}

			defstr->max += 256;

			defstr->buf = more;
			defstr->ptr = defstr->buf + defstr->len;
		}

		(*(defstr->ptr)) = (*sptr);

		defstr->ptr++;
		defstr->len++;

		sptr++;
	}

	return (0);
}

/**
 * @internal
 */
int wmf_stream_rezet (void* context)
{	
	WMF_DEBUG (((wmfDefaultZtream*) context)->API,
		"cannot reset ztream (compressed stream)");

	return (0);
}

/**
 * @internal
 */
int wmf_stream_reset (void* context)
{	wmfDefaultStream* defstr = (wmfDefaultStream*) context;

	if (defstr->out)
	{	return (fseek (defstr->out,defstr->offset,SEEK_SET));
	}
	else
	{	defstr->len = 0;

		defstr->ptr = defstr->buf;

		return (0);
	}
}
