/* libwmf (<libwmf/api.h>): library for wmf conversion
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


#ifndef LIBWMF_API_H
#define LIBWMF_API_H

#if defined (_WIN32) && !defined (LIBWMF_STATIC)
  #ifdef LIBWMF_EXPORTS
    #define LIBWMF_EXPORT __declspec(dllexport)
  #else
    #define LIBWMF_EXPORT __declspec(dllimport)
  #endif
#else
  #define LIBWMF_EXPORT
#endif

#include <zlib.h>

#include <libwmf/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes library
 */
extern wmf_error_t wmf_api_create (wmfAPI**,unsigned long,wmfAPI_Options*);

/**
 * Possibly completes output, and otherwise frees all allocated memory
 */
extern wmf_error_t wmf_api_destroy (wmfAPI*);

/**
 * Initializes library - 'lite' interface only
 */
extern LIBWMF_EXPORT wmf_error_t wmf_lite_create (wmfAPI**,unsigned long,wmfAPI_Options*);

/**
 * Possibly completes output, and otherwise frees all allocated memory - 'lite' interface only
 */
extern LIBWMF_EXPORT wmf_error_t wmf_lite_destroy (wmfAPI*);

/**
 * Reads the header of the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_header_read (wmfAPI*);

/**
 * Scans the current metafile to determine bounding box and resources
 */
extern LIBWMF_EXPORT wmf_error_t wmf_scan (wmfAPI*,unsigned long,wmfD_Rect*);

/**
 * Plays the current metafile, calling exporter graphics procedures
 */
extern LIBWMF_EXPORT wmf_error_t wmf_play (wmfAPI*,unsigned long,wmfD_Rect*);

/**
 * Supplies a width and height for the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_size (wmfAPI*,float*,float*);

/**
 * Supplies a display (integer-) width and height for the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_display_size (wmfAPI*,unsigned int*,unsigned int*,double,double);

/**
 * Sets user defines input stream functions for reading a metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_bbuf_input (wmfAPI*,wmfRead,wmfSeek,wmfTell,void*);

/**
 * Opens a file as the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_file_open (wmfAPI*,const char*);

/**
 * Closes the file corresponding to the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_file_close (wmfAPI*);

/**
 * Specifies an array of unsigned char as the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_mem_open (wmfAPI*,unsigned char*,long);

/**
 * Disassociates array corresponding to the current metafile
 */
extern LIBWMF_EXPORT wmf_error_t wmf_mem_close (wmfAPI*);

/* wmf_stream_create: set FILE stream to 0 to write to memory;
 * wmf_stream_destroy: returns pointer to memory, if not a FILE stream
 */

/**
 * Creates compressed character output file stream
 */
extern wmfStream* wmf_ztream_create (wmfAPI*,gzFile);

/**
 * Creates uncompressed character output file or memory stream
 */
extern wmfStream* wmf_stream_create (wmfAPI*,FILE*);

/**
 * Finalizes compressed character output file stream
 */
extern void wmf_ztream_destroy (wmfAPI*,wmfStream*,char**,unsigned long*);

/**
 * Finalizes uncompressed character output file stream
 */
extern void wmf_stream_destroy (wmfAPI*,wmfStream*,char**,unsigned long*);

/**
 * Formatted print to character output file stream
 */
extern int wmf_stream_printf (wmfAPI*,wmfStream*,char*,...);

/**
 * malloc() & attach to library's memory manager
 */
extern LIBWMF_EXPORT void* wmf_malloc (wmfAPI*,size_t);

/**
 * calloc() & attach to library's memory manager
 */
extern LIBWMF_EXPORT void* wmf_calloc (wmfAPI*,size_t,size_t);

/**
 * realloc() memory attached to library's memory manager
 */
extern LIBWMF_EXPORT void* wmf_realloc (wmfAPI*,void*,size_t);

/**
 * free() memory attached to library's memory manager
 */
extern LIBWMF_EXPORT void wmf_free (wmfAPI*,void*);

/**
 * Detach memory from library's memory manager
 */
extern LIBWMF_EXPORT void  wmf_detach (wmfAPI*,void*);

/**
 * strdup() & attach to library's memory manager
 */
extern LIBWMF_EXPORT char* wmf_strdup (wmfAPI*,const char*);

/**
 * create concatenation of two strings and attach to library's memory manager
 */
extern LIBWMF_EXPORT char* wmf_str_append (wmfAPI*,char*,char*);

/**
 * strstr()
 */
extern LIBWMF_EXPORT char* wmf_strstr (const char*,const char*);

/**
 * Increases size of API's string buffer; returns new size or 0 on error
 */
extern LIBWMF_EXPORT unsigned long wmf_strbuf_grow (wmfAPI*);

/**
 * Initializes the metafile player (called by wmf_api_create())
 */
extern LIBWMF_EXPORT wmf_error_t wmf_player_init (wmfAPI*);

/**
 * Returns the Aldus Checksum of the metafile's header
 */
extern LIBWMF_EXPORT U16 wmf_aldus_checksum (wmfAPI*);

/**
 * Reads a two-byte sequence from the current metafile, returns U16
 */
extern LIBWMF_EXPORT U16 wmf_read_16 (wmfAPI*);

/**
 * Reads a four-byte sequence from the current metafile, returns U32
 */
extern LIBWMF_EXPORT U32 wmf_read_32 (wmfAPI*,U16*,U16*);

/**
 * file input (wmf_file_open): fgetc()
 */
extern LIBWMF_EXPORT int wmf_file_read (void*);

/**
 * file input (wmf_file_open): fseek()
 */
extern LIBWMF_EXPORT int wmf_file_seek (void*,long);

/**
 * file input (wmf_file_open): ftell()
 */
extern LIBWMF_EXPORT long wmf_file_tell (void*);

/**
 * memory input (wmf_mem_open): fgetc() equiv.
 */
extern LIBWMF_EXPORT int wmf_mem_read (void*);

/**
 * memory input (wmf_mem_open): fseek() equiv.
 */
extern LIBWMF_EXPORT int wmf_mem_seek (void*,long);

/**
 * memory input (wmf_mem_open): ftell() equiv.
 */
extern LIBWMF_EXPORT long wmf_mem_tell (void*);

/* default ztream functions; NOT to be used directly! */

/**
 * compressed char output (wmf_ztream_create): fputs()
 */
extern int wmf_stream_zputs (char*,void*);

/**
 * compressed char output (wmf_ztream_create): rewind()
 */
extern int wmf_stream_rezet (void*);

/**
 * uncompressed char output (wmf_stream_create): fputs()
 */
extern int wmf_stream_sputs (char*,void*); /* default stream functions; NOT to be used directly! */

/**
 * uncompressed char output (wmf_stream_create): rewind()
 */
extern int wmf_stream_reset (void*);

/**
 * Writes message to error stream (use WMF_ERROR macro)
 */
extern LIBWMF_EXPORT void wmf_error (wmfAPI*,char*,int,char*);

/**
 * Writes message to debug stream (use WMF_DEBUG macro)
 */
extern LIBWMF_EXPORT void wmf_debug (wmfAPI*,char*,int,char*);

/**
 * Formatted print to debug stream
 */
extern LIBWMF_EXPORT void wmf_printf (wmfAPI*,char*,...);

/**
 * Asserts on zero expression (use WMF_ASSERT macro)
 */
extern LIBWMF_EXPORT void wmf_assert (wmfAPI*,char*,int);

/**
 * Outputs library-specific command-line options
 */
extern char* wmf_help (void);

/**
 * Sets drawing origin in device coordinates
 */
extern LIBWMF_EXPORT void wmf_set_viewport_origin (wmfAPI*,wmfD_Coord);

/**
 * Sets call-back function, called after every metafile record
 */
extern LIBWMF_EXPORT void wmf_status_function (wmfAPI*,void*,wmfStatus);

/**
 * Writes to --wmf-write file (which may be WMF or home-made wmfxml)
 */
extern LIBWMF_EXPORT void wmf_write (wmfAPI*,unsigned long,unsigned int,const char*,
		       char**,const unsigned char*,unsigned long);

/**
 * Open --wmf-write file (which may be WMF or home-made wmfxml)
 */
extern LIBWMF_EXPORT void wmf_write_begin (wmfAPI*,const char*);

/**
 * Close --wmf-write file (which may be WMF or home-made wmfxml)
 */
extern LIBWMF_EXPORT void wmf_write_end (wmfAPI*);

/**
 * Initialize a wmfAttributes structure
 */
extern LIBWMF_EXPORT void wmf_attr_new (wmfAPI*,wmfAttributes*);

/**
 * Clear/Empty a wmfAttributes structure
 */
extern LIBWMF_EXPORT void wmf_attr_clear (wmfAPI*,wmfAttributes*);

/**
 * Free memory associated with a wmfAttributes structure
 */
extern LIBWMF_EXPORT void wmf_attr_free (wmfAPI*,wmfAttributes*);

/**
 * Add an name&value to a wmfAttributes structure; returns ptr to value-in-list
 */
extern LIBWMF_EXPORT const char * wmf_attr_add (wmfAPI*,wmfAttributes*,const char*,const char*);

/**
 * Return value of name in a wmfAttributes structure; returns 0 if name not found
 */
extern LIBWMF_EXPORT const char * wmf_attr_query (wmfAPI*,wmfAttributes*,const char*);

/**
 * Load wmfxml file and wmf_mem_open() it
 */
extern wmf_error_t wmf_wmfxml_import (wmfAPI*,const char*);

#ifdef __cplusplus
}
#endif

/* Useful defs & macros
 */

/**
 * @param Z the API handle
 * @param M string to send to the error stream via wmf_error()
 */
#define WMF_ERROR(Z,M) wmf_error (Z,__FILE__,__LINE__,M)

#ifdef DEBUG
#define WMF_DEBUG(Z,M) wmf_debug (Z,__FILE__,__LINE__,M)
#define WMF_ASSERT(Z,M) if (!(M)) wmf_assert (Z,__FILE__,__LINE__)
#else
/**
 * @param Z the API handle
 * @param M string to send to the debug stream via wmf_debug()
 * 
 * (debug build only)
 */
#define WMF_DEBUG(Z,M) (void)Z

/**
 * @param Z the API handle
 * @param M an <expr>, if zero then call wmf_assert()
 * 
 * (debug build only)
 */
#define WMF_ASSERT(Z,M)
#endif

/* Flags to be passed in wmf_api_create: flags to lie in range (1<<0) to (1<<19)
 */

/**
 * Option to wmf_api_create()
 * 
 * use provided [*]alloc/free functions
 */
#define WMF_OPT_ALLOC           (1<<0)

/**
 * Option to wmf_api_create()
 * 
 * check provided command line for --wmf-<option>
 */
#define WMF_OPT_ARGS            (1<<1)

/**
 * Option to wmf_api_create()
 * 
 * font directories specified
 */
#define WMF_OPT_FONTDIRS        (1<<2)

/**
 * Option to wmf_api_create()
 * 
 * font mappings specified
 */
#define WMF_OPT_FONTMAP         (1<<3)

/**
 * Option to wmf_api_create()
 * 
 * use system fonts, if found
 */
#define WMF_OPT_SYS_FONTS       (1<<4)

/**
 * Option to wmf_api_create()
 * 
 * use specified XML system fontmap file
 */
#define WMF_OPT_SYS_FONTMAP     (1<<5)

/**
 * Option to wmf_api_create()
 * 
 * use non-system fonts, if found
 */
#define WMF_OPT_XTRA_FONTS      (1<<6)

/**
 * Option to wmf_api_create()
 * 
 * use specified XML non-system fontmap file
 */
#define WMF_OPT_XTRA_FONTMAP    (1<<7)

/**
 * Option to wmf_api_create()
 * 
 * use specified ghostscript fontmap file
 */
#define WMF_OPT_GS_FONTMAP      (1<<8)

/**
 * Option to wmf_api_create()
 * 
 * write metafile to specified file
 */
#define WMF_OPT_WRITE           (1<<9)

/**
 * Option to wmf_api_create()
 * 
 * initialize device-layer with supplied function
 */
#define WMF_OPT_FUNCTION        (1<<10)

/**
 * Option to wmf_api_create()
 * 
 * initialize device-layer with specified module
 */
#define WMF_OPT_MODULE          (1<<11)

/**
 * Option to wmf_api_create()
 * 
 * check for module also in specified directories
 */
#define WMF_OPT_MODULE_DIRS     (1<<12)

/**
 * Option to wmf_api_create()
 * 
 * ignore (some) non-fatal errors --wmf-ignore-nonfatal
 */
#define WMF_OPT_IGNORE_NONFATAL (1<<14)

/**
 * Option to wmf_api_create()
 * 
 * suppress all error reports --wmf-error
 */
#define WMF_OPT_NO_ERROR        (1<<15)

/**
 * Option to wmf_api_create()
 * 
 * suppress all debug reports --wmf-debug
 */
#define WMF_OPT_NO_DEBUG        (1<<16)

/**
 * Option to wmf_api_create()
 * 
 * divert error reports to specified stream
 */
#define WMF_OPT_LOG_ERROR       (1<<17)

/**
 * Option to wmf_api_create()
 * 
 * divert debug reports to specified stream
 */
#define WMF_OPT_LOG_DEBUG       (1<<18)

/**
 * Option to wmf_api_create()
 * 
 * emit diagnostic information --wmf-diagnostics
 */
#define WMF_OPT_DIAGNOSTICS     (1<<19)

#endif /* ! LIBWMF_API_H */
