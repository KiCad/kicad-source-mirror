/* libwmf (<libwmf/types.h>): library for wmf conversion
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


#ifndef LIBWMF_TYPES_H
#define LIBWMF_TYPES_H

#include <stdio.h>
#include <sys/types.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <libwmf/fund.h>

#ifdef __cplusplus
extern "C" {
#endif

/* API Enumeration defs
 */

/**
 * \b wmf_bool_t is the (enumeration) type used for boolean
 */
typedef enum _wmf_bool_t
{	wmf_false = 0,	/**< False */
	wmf_true	/**< True */
} wmf_bool_t;

/**
 * \b wmf_error_t is the (enumeration) type used for the library error state.
 */
typedef enum _wmf_error_t
{	wmf_E_None = 0,		/**< No error. */
	wmf_E_InsMem,		/**< An attempt to allocate memory has failed. */
	wmf_E_BadFile,		/**< Attempt to open an unreadable file, or to read from an unopened file. */
	wmf_E_BadFormat,	/**< The metafile, if indeed it is a metafile, has been corrupted. */
	wmf_E_EOF,		/**< An unexpected end-of-file has been reached. */
	wmf_E_DeviceError,	/**< Device-layer error. */
	wmf_E_Glitch,		/**< Programmer's error. Sorry. */
	wmf_E_Assert,		/**< Internally forced error. */
	wmf_E_UserExit		/**< The status function has returned non-zero; exit is premature. */
} wmf_error_t;

/**
 * \b wmf_page_t is the (enumeration) type used to indicate page size.
 */
typedef enum _wmf_page_t
{	wmf_P_A5,	/**< A5 (420 x 595) */
	wmf_P_A4,	/**< A4 (595 x 842) */
	wmf_P_A3,	/**< A3 (842 x 1191) */
	wmf_P_A2,	/**< A2 (1191 x 1684) */
	wmf_P_A1,	/**< A1 (1684 x 2384) */
	wmf_P_A0,	/**< A0 (2384 x 3370) */
	wmf_P_B5,	/**< B5 (516 x 729) */
	wmf_P_Letter,	/**< Letter (612 x 792) */
	wmf_P_Legal,	/**< Legal (612 x 1008) */
	wmf_P_Ledger,	/**< Ledger (1224 x 792) */
	wmf_P_Tabloid	/**< Tabloid (792 x 1224) */
} wmf_page_t;

/**
 * \b wmf_image_t is the (enumeration) type used to indicate image type.
 */
typedef enum _wmf_image_t
{	wmf_I_gd
} wmf_image_t;

/**
 * User redefinable function for input-stream access:
 * 
 * wmfRead: returns unsigned char cast to int, or EOF
 * 
 * See \b wmf_bbuf_input().
 */
typedef int  (*wmfRead) (void*);

/**
 * User redefinable function for input-stream access:
 * 
 * wmfSeek: returns (-1) on error, else 0
 * 
 * See \b wmf_bbuf_input().
 */
typedef int  (*wmfSeek) (void*,long);

/**
 * User redefinable function for input-stream access:
 * 
 * wmfTell: returns (-1) on error, else pos
 * 
 * See \b wmf_bbuf_input().
 */
typedef long (*wmfTell) (void*);

/**
 * User redefinable function for character-based output-stream:
 * 
 * wmfSPutS: returns EOF on error, else 0
 * 
 * See \b wmfStream.
 */
typedef int (*wmfSPutS) (char*,void*);

/**
 * User redefinable function for character-based output-stream:
 * 
 * wmfReset: resets stream to start; returns non-zero on failure
 * 
 * See \b wmfStream.
 */
typedef int (*wmfReset) (void*);

/**
 * User definable function indicating progress.
 * 
 * @param context  handle for user data
 * @param fraction fraction of metafile parsed so far
 * 
 * @return Should return non-zero only if premature exit is required for whatever reason.
 * 
 * See \b wmf_status_function().
 */
typedef int (*wmfStatus) (void* context,float fraction);

/* API types
 */

typedef struct _wmfD_Coord             wmfD_Coord;
typedef struct _wmfD_Rect              wmfD_Rect;

typedef struct _wmfMapping             wmfMapping;
typedef struct _wmfFontMap             wmfFontMap;

typedef struct _wmfHead                wmfHead; /* Bit nonsensical having these three at all ?? */
typedef struct _wmfMetaHeader          wmfMetaHeader,*wmfFile;
typedef struct _wmfPlaceableMetaHeader wmfPlaceableMetaHeader;

typedef struct _wmfAPI_Options         wmfAPI_Options;
typedef struct _wmfAPI                 wmfAPI;

typedef struct _wmfStream              wmfStream;

typedef struct _wmfImage               wmfImage;

typedef struct _wmfAttributes          wmfAttributes;
typedef struct _wmfAttributeStore      wmfAttributeStore;

/* API Structure defs
 */

/**
 * Device coordinate.
 */
struct _wmfD_Coord
{	float x;
	float y;
};

/**
 * Device rectangle.
 */
struct _wmfD_Rect
{	/**
	 * Device coordinate of top left corner (TL.x < BR.x, TL.y < BR.y).
	 */
	wmfD_Coord TL;

	/**
	 * Device coordinate of bottom right corner.
	 */
	wmfD_Coord BR;
};

struct _wmfMapping
{	char* name;
	char* mapping;

	/* I had been hoping to keep FT out of this file, but
	 * it seems easier just to use the FT encoding defs
	 * rather than create some kind of wrapper...
	 */
	FT_Encoding encoding;
};

struct _wmfFontMap
{	char* name;       /* wmf font name */

	char* normal;     /* postscript font names */
	char* italic;
	char* bold;
	char* bolditalic;
};

/**
 * Structure containing list of XML attributes
 */
struct _wmfAttributes
{	char * name;

	char ** atts;

	unsigned long count;
	unsigned long max;

	unsigned char * buffer;

	unsigned long length;
	unsigned long offset;
};

/**
 * Structure containing list of lists of XML attributes
 */
struct _wmfAttributeStore
{	wmfAttributes * attrlist;

	unsigned long count;
	unsigned long max;
};

/**
 * @internal
 */
struct _wmfPlaceableMetaHeader
{	U32 Key;      /* Magic number (always 9AC6CDD7h) */
	U16 Handle;   /* Metafile HANDLE number (always 0) */
	S16 Left;     /* Left coordinate in metafile units */
	S16 Top;      /* Top coordinate in metafile units */
	S16 Right;    /* Right coordinate in metafile units */
	S16 Bottom;   /* Bottom coordinate in metafile units */
	U16 Inch;     /* Number of metafile units per inch */
	U32 Reserved; /* Reserved (always 0) */
	U16 Checksum; /* Checksum value for previous 10 U16s */
};

/**
 * @internal
 */
struct _wmfHead
{	U16 FileType;      /* Type of metafile (0=memory, 1=disk) */
	U16 HeaderSize;    /* Size of header in U16S (always 9) */
	U16 Version;       /* Version of Microsoft Windows used */
	U32 FileSize;      /* Total size of the metafile in U16s */
	U16 NumOfObjects;  /* Number of objects in the file */
	U32 MaxRecordSize; /* The size of largest record in U16s */
	U16 NumOfParams;   /* Not Used (always 0) */
};

/**
 * @internal
 */
struct _wmfMetaHeader
{	wmfHead*                wmfheader;
	wmfPlaceableMetaHeader* pmh;

	FILE* filein;

	long pos;

	int placeable;
};

struct _wmfAPI_Options
{	void* context;

	void* (*malloc)  (void* context,size_t size);
	void* (*realloc) (void* context,void* mem,size_t size);
	void  (*free)    (void* context,void* mem);

	int    argc;
	char** argv;

	char** fontdirs; /* NULL-terminated list of directories to search for font files */

	struct
	{	wmfFontMap* wmf; /* {0,*}-terminated list: wmf-font-name -> ps-font-name */
		wmfMapping* sub; /* {0,*}-terminated list: wmf-font-name substring equiv */
		wmfMapping* ps;  /* {0,*}-terminated list: ps-font-name -> pfb-file-name */
	} font;

	char* sys_fontmap_file;
	char* xtra_fontmap_file;
	char* gs_fontmap_file;

	char* write_file;

	void (*function) (wmfAPI*);

	char*  module;
	char** dirs;

	FILE* debug_out;
	FILE* error_out;
};

/**
 * @internal
 */
struct _wmfAPI
{	wmf_error_t err; /* current state of API; wmf_E_None, hopefully... */

	wmfHead                Head;  /* structures containing meta file general properties... */
	wmfPlaceableMetaHeader PlaceableMetaHeader;
	wmfMetaHeader          MetaHeader;
	wmfFile                File;

	FILE* debug_out; /* Output streams for debugger & error reports... */
	FILE* error_out;

	wmfAttributeStore store;

	void* write_data; /* Output stream data for --wmf-write=<file> */

	void* user_data; /* These are hooks for data to hang on to... */

	void* device_data;
	void* player_data;
	void* buffer_data;
	void* memory_data;

	void* function_reference;

	void* font_data;

	char** fonts; /* NULL-terminated list of fonts loaded during wmf_scan () */

	void* color_data;

	struct /* Input stream functions... */
	{	wmfRead read;
		wmfSeek seek;
		wmfTell tell;
	} bbuf;

	struct
	{	void* context;

		wmfStatus function; /* return non-zero if premature exit desired */
	} status;

	struct
	{	unsigned long length;

		char* buffer;
	} string_buffer; /* this is a general purpose char buffer */

	unsigned long flags; /* General flags... */
};

/**
 * WMF_ERROR_STATE(wmfAPI* API) -> (wmf_error_t) library error state
 */
#define WMF_ERROR_STATE(Z) (((wmfAPI*)(Z))->err)

/**
 * Structure describing user-definable character-based output stream.
 * 
 * \b wmf_stream_create() and \b wmf_ztream_create() both return pointers to \b wmfStream objects, but
 * an application can create its own implementation if preferred.
 * 
 * @verbatim
typedef int (*wmfSPutS) (char* str,void* context);
typedef int (*wmfReset) (void* context);
@endverbatim
 * 
 * \b wmfSPutS: writes string \p str; returns EOF on error, else 0.
 * 
 * \b wmfReset: resets stream to start; returns non-zero on failure.
 * 
 * \p context is a handle for user data
 */
struct _wmfStream
{	void* context;

	wmfSPutS sputs;
	wmfReset reset;
};

/**
 * @internal
 * Macro-wrapper for input stream function:
 * (int)  WMF_READ((wmfAPI*) API)                 - returns unsigned char cast to int, or EOF
 */
#define WMF_READ(Z)   ((Z)->bbuf.read ((Z)->buffer_data))

/**
 * @internal
 * Macro-wrapper for input stream function:
 * (int)  WMF_SEEK((wmfAPI*) API,(long) position) - returns (-1) on error, else 0
 */
#define WMF_SEEK(Z,P) ((Z)->bbuf.seek ((Z)->buffer_data,P))

/**
 * @internal
 * Macro-wrapper for input stream function:
 * (long) WMF_TELL((wmfAPI*) API)                 - returns (-1) on error, else current position
 */
#define WMF_TELL(Z)   ((Z)->bbuf.tell ((Z)->buffer_data))

#ifdef __cplusplus
}
#endif

/**
 * Structure referencing an image
 */
struct _wmfImage
{	wmf_image_t type;

	U16 width;
	U16 height;

	void* data;
};

#endif /* ! LIBWMF_TYPES_H */
