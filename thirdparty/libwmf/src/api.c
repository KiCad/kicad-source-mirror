/* libwmf (api.c): library for wmf conversion
   Copyright (C) 2001 - Francis James Franklin

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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "wmfdefs.h"

#include "api.h"

/**
 * Creates and initializes an instance of the \b libwmf library (lite interface) for a specified device layer.
 * 
 * @param API_return pointer to a wmfAPI* (the API handle use henceforth)
 * @param flags      bitwise OR of WMF_OPT_ options
 * @param options    pointer to wmfAPI_Options structure
 * 
 * This is the first and necessary step when using \b libwmf. Options are passed via the wmfAPI_Options
 * structure and \p flags. wmf_api_create allocates the wmfAPI structure and initializes the color tables,
 * the metafile player, and the device layer. If successful then the pointer to the wmfAPI structure is
 * returned via \p API_return, otherwise all allocated memory is released and the library exits with an
 * appropriate error.
 * 
 * wmf_lite_create () ignores command line arguments, if any are given, and does \b not attempt to set up
 * font mapping.
 * 
 * The library should be closed using the corresponding wmf_lite_destroy () function.
 * 
 * @return The error state of the library: \b wmf_E_None indicates successful creation and initialization
 *         of the library, and \p *API_return will be non-zero. For any other error value \p *API_return
 *         will be zero.
 */
wmf_error_t wmf_lite_create (wmfAPI** API_return,unsigned long flags,wmfAPI_Options* options)
{	wmfAPI* API = 0;

	wmfMemoryManager* MM = 0;
	wmfFunctionReference* FR = 0;

	wmf_error_t err = wmf_E_None;

	(*API_return) = 0;

/* Initialize memory management array & allocate
 */
	if (flags & WMF_OPT_ALLOC)
	{	MM = (wmfMemoryManager*) options->malloc (options->context,sizeof (wmfMemoryManager));
	}
	else
	{	MM = (wmfMemoryManager*) malloc (sizeof (wmfMemoryManager));
	}
	if (MM == 0)
	{	if ((flags & WMF_OPT_NO_ERROR) == 0)
		{	fputs ("wmf_api_create: insufficient memory!\n",stderr);
		}
		return (wmf_E_InsMem);
	}
	MM->count = 0;
	MM->max = 32;
	if (flags & WMF_OPT_ALLOC)
	{	MM->list = (void**) options->malloc (options->context,MM->max * sizeof (void*));
	}
	else
	{	MM->list = (void**) malloc (MM->max * sizeof (void*));
	}
	if (MM->list == 0)
	{	if ((flags & WMF_OPT_NO_ERROR) == 0)
		{	fputs ("wmf_api_create: insufficient memory!\n",stderr);
		}
		if (flags & WMF_OPT_ALLOC)
		{	options->free (options->context,MM);
		}
		else
		{	free (MM);
		}
		return (wmf_E_InsMem);
	}
	if (flags & WMF_OPT_ALLOC)
	{	MM->context = options->context;

		MM->malloc  = options->malloc;
		MM->realloc = options->realloc;
		MM->free    = options->free;
	}
	else
	{	MM->context = 0;

		MM->malloc  = 0;
		MM->realloc = 0;
		MM->free    = 0;
	}
/* Allocate wmfAPI structure
 */
	if (flags & WMF_OPT_ALLOC)
	{	API = (wmfAPI*) options->malloc (options->context,sizeof (wmfAPI));
	}
	else
	{	API = (wmfAPI*) malloc (sizeof (wmfAPI));
	}
	if (API == 0)
	{	if ((flags & WMF_OPT_NO_ERROR) == 0)
		{	fputs ("wmf_api_create: insufficient memory!\n",stderr);
		}
		if (flags & WMF_OPT_ALLOC)
		{	options->free (options->context,MM->list);
			options->free (options->context,MM);
		}
		else
		{	free (MM->list);
			free (MM);
		}
		return (wmf_E_InsMem);
	}

	API->memory_data = (void*) MM;

/* Initialize debug, error & other streams
 */
	if (flags & WMF_OPT_NO_DEBUG) API->debug_out = 0;
	else
	{	if (flags & WMF_OPT_LOG_DEBUG) API->debug_out = options->debug_out;
		else
		{
			API->debug_out = stdout;
		}
	}

	if (flags & WMF_OPT_NO_ERROR) API->error_out = 0;
	else
	{	if (flags & WMF_OPT_LOG_ERROR) API->error_out = options->error_out;
		else
		{
			API->error_out = stderr;
		}
	}

	API->write_data = 0;

	API->MetaHeader.pmh = &(API->PlaceableMetaHeader);
	API->MetaHeader.wmfheader = &(API->Head);
	API->File = &(API->MetaHeader);

	API->File->filein = 0; /* Input stream: file (unused) */

	API->buffer_data = 0;  /* Input stream */

	API->bbuf.read = 0;
	API->bbuf.seek = 0;
	API->bbuf.tell = 0;

/* Status function & context
 */
	API->status.context = 0;
	API->status.function = 0;

/* General purpose string buffer
 */
	API->string_buffer.length = 0;
	API->string_buffer.buffer = 0;

/* Zero some unset pointers 
 */
	API->function_reference = 0;

	API->font_data = 0; /* non-lite option TODO */
	API->fonts = 0;

	API->color_data = 0;

	API->store.attrlist = 0;
	API->store.count = 0;
	API->store.max = 0;

/* Library error state:
 */
	API->err = wmf_E_None;

/* Finally: flags, etc.
 */
	API->flags = flags;

/* ---- Henceforth all allocation to be done via api ---- */

/* General purpose string buffer
 */
	API->string_buffer.length = 64;
	API->string_buffer.buffer = wmf_malloc (API,API->string_buffer.length * sizeof (char));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_lite_destroy (API);
		return (err);
	}

/* Create color data - must be done prior to IPA initialization
 */
	wmf_ipa_color_init (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_lite_destroy (API);
		return (err);
	}

/* Create ipa function interface
 */
	API->function_reference = wmf_malloc (API,sizeof (wmfFunctionReference));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_lite_destroy (API);
		return (err);
	}

	FR = (wmfFunctionReference*) API->function_reference;

	FR->device_open      = 0;
	FR->device_close     = 0;
	FR->device_begin     = 0;
	FR->device_end       = 0;
	FR->flood_interior   = 0;
	FR->flood_exterior   = 0;
	FR->draw_pixel       = 0;
	FR->draw_pie         = 0;
	FR->draw_chord       = 0;
	FR->draw_arc         = 0;
	FR->draw_ellipse     = 0;
	FR->draw_line        = 0;
	FR->poly_line        = 0;
	FR->draw_polygon     = 0;
	FR->draw_polypolygon = 0;
	FR->draw_rectangle   = 0;
	FR->rop_draw         = 0;
	FR->bmp_draw         = 0;
	FR->bmp_read         = 0;
	FR->bmp_free         = 0;
	FR->draw_text        = 0;
	FR->udata_init       = 0;
	FR->udata_copy       = 0;
	FR->udata_set        = 0;
	FR->udata_free       = 0;
	FR->region_frame     = 0;
	FR->region_paint     = 0;
	FR->region_clip	     = 0;

/* Create ipa-device data
 */
	if (flags & WMF_OPT_FUNCTION)
	{	options->function (API);
	}
	else if (flags & WMF_OPT_MODULE) /* TODO... TODO... TODO... */
	{	WMF_ERROR (API,"libwmf: module interface not implemented yet...");
		WMF_ERROR (API,"        unable to initialize device layer!");
		API->err = wmf_E_Glitch;
	}
	else
	{	WMF_ERROR (API,"libwmf: unable to initialize device layer!");
		API->err = wmf_E_Glitch;
	}

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_lite_destroy (API);
		return (err);
	}

/* Create player data
 */
	wmf_player_init (API);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		err = wmf_lite_destroy (API);
		return (err);
	}

/* Have successfully created the API...
 */
	(*API_return) = API;

	return (wmf_E_None);
}

/**
 * Close the device layer, if open, and release all allocated memory attached to the memory manager.
 * 
 * @param API the API handle
 * 
 * @return The final error state of the library.
 */
wmf_error_t wmf_lite_destroy (wmfAPI* API) /* Basically free all alloced memory */
{	wmf_error_t err;                   /* associated with the API */

	wmfMemoryManager*     MM = (wmfMemoryManager*)     API->memory_data;
	wmfFunctionReference* FR = (wmfFunctionReference*) API->function_reference;

	if (FR)
	{	/* FR->device_close must be the first action of wmf_lite_destroy in case
		 * FR->device_close decides, for whatever reason, to re-play the meta file
		 * or to acquire API resources...
		 */
		if ((API->flags & API_DEVICE_OPEN) && FR->device_close) FR->device_close (API);
	}

	if (API->flags & API_FILE_OPEN) wmf_file_close (API);

	err = API->err;

	while (MM->count)
	{	MM->count--;
		if (MM->free)
		{	MM->free (MM->context,MM->list[MM->count]);
		}
		else
		{	free (MM->list[MM->count]);
		}
	}

	if (MM->free)
	{	MM->free (MM->context,API);
		MM->free (MM->context,MM->list);
		MM->free (MM->context,MM);
	}
	else
	{	free (API);
		free (MM->list);
		free (MM);
	}

	return (err);
}

/* ERROR & DEBUG Reporting
 * =======================
 */

/**
 * Set the error state of the library to wmf_E_Assert.
 * 
 * @param API  the API handle
 * @param file file name
 * @param line line number
 * 
 * This should only be called via the macro WMF_ASSERT(API,<expr>) which is defined (for debug builds only)
 * as:
 * @verbatim
#define WMF_ASSERT(Z,M) if (!(M)) wmf_assert (Z,__FILE__,__LINE__)
@endverbatim
 * i.e., if <expr> evaluates to 0 then call wmf_assert() with current file name and line number.
 */
void wmf_assert (wmfAPI* API,char* file,int line)
{	wmf_error (API,file,line,"Assertion failed!");

	API->err = wmf_E_Assert;
}

/**
 * Print message to error stream.
 * 
 * @param API  the API handle
 * @param file file name
 * @param line line number
 * @param msg  message to print
 * 
 * This should only be called via the macro WMF_ERROR(API,msg) which calls wmf_error() with the current file
 * name and line number.
 */
void wmf_error (wmfAPI* API,char* file,int line,char* msg)
{	if (API->error_out == 0) return;

	fprintf (API->error_out,"ERROR: %s (%d): %s\n",file,line,msg);
	fflush (API->error_out);
}

/**
 * Print message to debug stream.
 * 
 * @param API  the API handle
 * @param file file name
 * @param line line number
 * @param msg  message to print
 * 
 * This should only be called via the macro WMF_DEBUG(API,msg) which (in debug builds only) calls
 * wmf_debug() with the current file name and line number.
 */
void wmf_debug (wmfAPI* API,char* file,int line,char* msg)
{	if (API->debug_out == 0) return;

	fprintf (API->debug_out,"%s (%d): %s\n",file,line,msg);
	fflush (API->debug_out);
}

/**
 * Print formatted message to debug stream.
 * 
 * @param API the API handle
 * @param msg message to print
 * 
 * With syntax similar to printf(), wmf_printf() prints formatted output to the debug stream.
 */
void wmf_printf (wmfAPI* API,char* msg,...)
{	va_list argp;

	va_start (argp,msg);

	if (API->debug_out)
	{	vfprintf (API->debug_out,msg,argp);
		fflush (API->debug_out);
	}

	va_end (argp);
}

/* Memory management interface
 * ===========================
 */

/**
 * Allocate memory of specified size and attach to the API's memory manager's internal list.
 * 
 * @param API  the API handle
 * @param size size in bytes of memory required
 * 
 * With syntax similar to malloc(), wmf_malloc() allocates \p size bytes of memory and adds a reference to
 * it in the memory manager's list. To free the memory, use wmf_free().
 * 
 * @return Pointer to new memory, or zero on failure.
 *         Sets error state \b wmf_E_InsMem on failure.
 */
void* wmf_malloc (wmfAPI* API,size_t size)
{	wmfMemoryManager* MM = (wmfMemoryManager*) API->memory_data;

	void*  mem = 0;
	void** more = 0;

	if (size == 0)
	{	WMF_DEBUG (API,"wmf_[*]alloc: attempt to allocate zero-sized memory!");
		return (0);
	}

	if (MM->count == MM->max)
	{	if (MM->realloc)
		{	more = (void**) MM->realloc (MM->context,MM->list,(MM->max+32) * sizeof (void*));
		}
		else
		{	more = (void**) realloc (MM->list,(MM->max+32) * sizeof (void*));
		}
		if (more == 0)
		{	WMF_ERROR (API,"wmf_[*]alloc: insufficient memory!");
			API->err = wmf_E_InsMem;
			return (0);
		}
		MM->max += 32;
		MM->list = more;
	}

	if (MM->malloc)
	{	mem = MM->malloc (MM->context,size);
	}
	else
	{	mem = malloc (size);
	}
	if (mem == 0)
	{	WMF_ERROR (API,"wmf_[*]alloc: insufficient memory!");
		API->err = wmf_E_InsMem;
		return (0);
	}

	MM->list[MM->count] = mem;
	MM->count++;

	return (mem);
}

/**
 * Allocate memory of specified size and attach to the API's memory manager's internal list.
 * 
 * @param API    the API handle
 * @param number number or elements
 * @param size   size in bytes of memory required by one element
 * 
 * With syntax similar to calloc(), wmf_calloc() allocates \p number * \p size bytes of memory and adds a
 * reference to it in the memory manager's list. To free the memory, use wmf_free().
 * 
 * @return Pointer to new memory, or zero on failure.
 *         Sets error state \b wmf_E_InsMem on failure.
 */
void* wmf_calloc (wmfAPI* API,size_t number,size_t size)
{	void* mem;

	if (size != 0 && number > SIZE_MAX / size)
	{	WMF_ERROR (API,"wmf_calloc: number * size overflows!");
		API->err = wmf_E_InsMem;
		return (0);
	}

	mem = wmf_malloc (API, number * size);
	if (mem) memset (mem, 0, number * size);
	return (mem);
}

/**
 * (Re)Allocate memory of specified size and attach to the API's memory manager's internal list.
 * 
 * @param API  the API handle
 * @param mem  pointer to memory previously allocated via the API
 * @param size new size in bytes of memory required
 * 
 * With syntax similar to realloc(), wmf_realloc() allocates \p size bytes of memory and adds a reference
 * to it in the memory manager's list. To free the memory, use wmf_free(). If \p mem is zero, this is
 * equivalent to a call to wmf_malloc(). If \p size is zero, the memory is released via wmf_free().
 * 
 * @return Pointer to new memory, or zero on failure.
 *         Sets error state \b wmf_E_InsMem on failure.
 */
void* wmf_realloc (wmfAPI* API,void* mem,size_t size)
{	wmfMemoryManager* MM = (wmfMemoryManager*) API->memory_data;

	void* more = 0;

	unsigned int i;

	if (mem == 0) return (wmf_malloc (API,size));

	if (size == 0)
	{	WMF_DEBUG (API,"wmf_realloc: attempt to allocate zero-sized memory!");
		wmf_free (API,mem);
		return (0);
	}

	for (i = 0; i < MM->count; i++)
		if (MM->list[i] == mem)
		{	if (MM->realloc)
			{	more = MM->realloc (MM->context,mem,size);
			}
			else
			{	more = realloc (mem,size);
			}
			if (more == 0)
			{	WMF_ERROR (API,"wmf_[*]alloc: insufficient memory!");
				API->err = wmf_E_InsMem;
			}
			else
			{	MM->list[i] = more;
			}
			break;
		}

	return (more);
}

/**
 * Frees memory attached to the API's memory manager's internal list.
 * 
 * @param API the API handle
 * @param mem pointer to memory previously allocated via the API
 * 
 * Syntax is similar to free().
 */
void wmf_free (wmfAPI* API,void* mem)
{	wmfMemoryManager* MM = (wmfMemoryManager*) API->memory_data;

	unsigned int i;

	for (i = 0; i < MM->count; i++)
		if (MM->list[i] == mem)
		{	if (MM->free)
			{	MM->free (MM->context,mem);
			}
			else
			{	free (mem);
			}
			MM->count--;
			MM->list[i] = MM->list[MM->count];
			break;
		}
}

/**
 * Detach memory attached to the API's memory manager's internal list.
 * 
 * @param API the API handle
 * @param mem pointer to memory previously allocated via the API
 * 
 * This removes the reference in the API's memory manager's internal list, and the memory will not,
 * therefore, be released by wmf_api_destroy(). To free subsequently, use free().
 */
void wmf_detach (wmfAPI* API,void* mem)
{	wmfMemoryManager* MM = (wmfMemoryManager*) API->memory_data;

	unsigned int i;

	for (i = 0; i < MM->count; i++)
		if (MM->list[i] == mem)
		{	MM->count--;
			MM->list[i] = MM->list[MM->count];
			break;
		}
}

/**
 * Duplicate string and attach to the API's memory manager's internal list.
 * 
 * @param API the API handle
 * @param str a string
 * 
 * With syntax similar to strdup(), wmf_strdup() allocates the necessary memory via wmf_malloc() and copies
 * the string. Use wmf_free() to free the string.
 * 
 * @return Pointer to new string, or zero on failure.
 *         Sets error state \b wmf_E_InsMem on failure, or \b wmf_E_Glitch if str is zero.
 */
char* wmf_strdup (wmfAPI* API,const char* str)
{	char* cpy = 0;

	if (str == 0)
	{	if (ERR (API)) return (0);

		WMF_ERROR (API,"wmf_strdup: attempt to copy non-existent string!");
		API->err = wmf_E_Glitch;
		return (0);
	}

	cpy = (char*) wmf_malloc (API,strlen (str) + 1);

	if (ERR (API)) return (0);

	strcpy (cpy,str);

	return (cpy);
}

/**
 * Create concatenatation of two strings and attach to the API's memory manager's internal list.
 * 
 * @param API  the API handle
 * @param pre  a string
 * @param post a string
 * 
 * wmf_str_append() allocates the necessary memory via wmf_malloc(), copies \p pre into the string and
 * appends \p post. Use wmf_free() to free the string.
 * 
 * @return Pointer to new string, or zero on failure.
 *         Sets error state \b wmf_E_InsMem on failure, or \b wmf_E_Glitch if str is zero.
 */
char* wmf_str_append (wmfAPI* API,char* pre,char* post)
{	char* cpy = 0;

	if ((pre == 0) && (post == 0)) return (0);

	if (pre  == 0) return (wmf_strdup (API,post));
	if (post == 0) return (wmf_strdup (API,pre));

	cpy = (char*) wmf_malloc (API,strlen (pre) + strlen (post) + 1);

	if (ERR (API)) return (0);

	strcpy (cpy,pre);
	strcat (cpy,post);

	return (cpy);
}

/**
 * Substring search.
 * 
 * @param haystack a string
 * @param needle   a substring to search for in haystack
 * 
 * With syntax identical to strstr(), wmf_strstr() searches for string \p needle in string \p haystack.
 * 
 * @return Pointer to substring \p needle found in \p haystack, or zero if not found.
 */
char* wmf_strstr (const char* haystack,const char* needle)
{
#ifdef HAVE_STRSTR
	return (strstr (haystack,needle));
#else /* HAVE_STRSTR */
	const char* ptr1;
	const char* ptr2;
	const char* ptr3;

	ptr1 = haystack;
	while (*ptr1)
	{	ptr2 = ptr1;
		ptr3 = needle;

		while ((*ptr3) == (*ptr2))
		{	if ((*ptr3) == 0) break;
			ptr2++;
			ptr3++;
		}

		if ((*ptr3) == 0) break;

		ptr1++;
	}

	return ((*ptr1) ? (char*) ptr1 : 0);
#endif /* HAVE_STRSTR */
}

/* Optional status call-back function
 * ==================================
 */

/**
 * Set a status call-back function.
 * 
 * @param API      the API handle
 * @param context  handle for user data
 * @param function call-back function
 * 
 * The metafile player calls the status function after each record.
 */
void wmf_status_function (wmfAPI* API,void* context,wmfStatus function)
{	API->status.context = context;
	API->status.function = function;
}

/**
 * Increase the size of the internal string buffer.
 * 
 * @param API the API handle
 * 
 * \b libwmf maintains an internal buffer for string operations. wmf_strbuf_grow() increases the size by 64.
 * 
 * @return Returns the new size of the buffer.
 *         Uses wmf_realloc(), so may set \b wmf_E_InsMem on failure.
 */
unsigned long wmf_strbuf_grow (wmfAPI* API)
{	char* more = 0;

	more = (char*) wmf_realloc (API,API->string_buffer.buffer,(API->string_buffer.length + 64) * sizeof (char));

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (0);
	}

	API->string_buffer.length += 64;

	API->string_buffer.buffer = more;

	return (API->string_buffer.length);
}
