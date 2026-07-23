/* libwmf (bbuf.c): library for wmf conversion
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
#endif

#include <stdio.h>

#include "wmfdefs.h"

#include "bbuf.h"

/**
 * Set custom metafile input-stream handler functions.
 * 
 * @param API       the API handle
 * @param fp_read   function to read a byte
 * @param fp_seek   function to set position
 * @param fp_tell   function to get position
 * @param user_data handle for user data
 * 
 * \b libwmf has simple methods for reading either from file or from memory, but many applications will want
 * to use customized variants of these. wmf_bbuf_input() enables this.
 * 
 * @verbatim
typedef int  (*wmfRead) (void* user_data);
typedef int  (*wmfSeek) (void* user_data,long position);
typedef long (*wmfTell) (void* user_data);
@endverbatim
 * 
 * \b wmfRead returns unsigned char cast to int, or EOF (cf. fgetc())
 * 
 * \b wmfSeek returns (-1) on error, otherwise 0 (cf. fseek())
 * 
 * \b wmfTell returns (-1) on error, otherwise current position (cf. ftell())
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error state of \b wmf_E_Glitch, if any of the three functions is zero.
 */
wmf_error_t wmf_bbuf_input (wmfAPI* API,wmfRead fp_read,wmfSeek fp_seek,wmfTell fp_tell,void* user_data)
{	if (ERR (API)) return (API->err);

	if ((fp_read == 0) || (fp_seek == 0) || (fp_tell == 0))
	{	WMF_ERROR (API,"wmf_bbuf_input: null arg. given unexpectedly!");
		API->err = wmf_E_Glitch;
	}
	else
	{	API->buffer_data = user_data;

		API->bbuf.read = fp_read;
		API->bbuf.seek = fp_seek;
		API->bbuf.tell = fp_tell;
	}

	return (API->err);
}

/**
 * Open file as metafile.
 * 
 * @param API  the API handle
 * @param file file name
 * 
 * Simple method for reading from file.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error states of \b wmf_E_Glitch or \b wmf_E_BadFile.
 */
wmf_error_t wmf_file_open (wmfAPI* API,const char* file)
{	wmfBBufFileInfo* file_info = 0;

	if (ERR (API)) return (API->err);

	if (API->buffer_data)
	{	WMF_ERROR (API,"wmf_file_open: input stream already open!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	file_info = (wmfBBufFileInfo*) wmf_malloc (API,sizeof (wmfBBufFileInfo));

	if (ERR (API)) return (API->err);

	if ((file_info->file = fopen (file,"rb")) == 0)
	{	WMF_ERROR (API,"wmf_file_open: unable to open file for reading.");
		wmf_free (API,file_info);
		API->err = wmf_E_BadFile;
		return (API->err);
	}

	wmf_bbuf_input (API,wmf_file_read,wmf_file_seek,wmf_file_tell,(void*) file_info);

	if (ERR (API))
	{	wmf_file_close (API);
		return (API->err);
	}

	API->flags |= API_FILE_OPEN;

	return (API->err);
}

/**
 * Close metafile input file stream.
 * 
 * @param API the API handle
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error state of \b wmf_E_Glitch.
 */
wmf_error_t wmf_file_close (wmfAPI* API)
{	wmfBBufFileInfo* file_info = (wmfBBufFileInfo*) (API->buffer_data);

	if ((API->buffer_data == 0) || ((API->flags & API_FILE_OPEN) == 0))
	{	WMF_ERROR (API,"wmf_file_close: attempt to close unopened stream!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	fclose (file_info->file);

	API->flags &= ~API_FILE_OPEN;

	wmf_free (API,API->buffer_data);

	API->buffer_data = 0;

	API->bbuf.read = 0;
	API->bbuf.seek = 0;
	API->bbuf.tell = 0;

	return (API->err);
}

/**
 * @internal
 */
int wmf_file_read (void* user_data)
{	wmfBBufFileInfo* file_info = (wmfBBufFileInfo*) user_data;

	return (fgetc (file_info->file));
}

/**
 * @internal
 */
int wmf_file_seek (void* user_data,long pos)
{	wmfBBufFileInfo* file_info = (wmfBBufFileInfo*) user_data;

	return (fseek (file_info->file,pos,SEEK_SET));
}

/**
 * @internal
 */
long wmf_file_tell (void* user_data)
{	wmfBBufFileInfo* file_info = (wmfBBufFileInfo*) user_data;

	return (ftell (file_info->file));
}

/**
 * Open metafile in memory.
 * 
 * @param API    the API handle
 * @param mem    the metafile in memory
 * @param length the length in bytes of metafile data
 * 
 * Simple method for reading from memory as array of unsigned char.
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 *         Possible library error state of \b wmf_E_Glitch.
 */
wmf_error_t wmf_mem_open (wmfAPI* API,unsigned char* mem,long length)
{	wmfBBufMemInfo* mem_info = 0;

	if (ERR (API)) return (API->err);

	if (API->buffer_data)
	{	WMF_ERROR (API,"wmf_mem_open: input stream already open!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	if ((mem == 0) || (length <= 0))
	{	WMF_ERROR (API,"wmf_mem_open: null or improper buffer!");
		API->err = wmf_E_Glitch;
		return (API->err);
	}

	mem_info = (wmfBBufMemInfo*) wmf_malloc (API,sizeof (wmfBBufMemInfo));

	if (ERR (API)) return (API->err);

	mem_info->mem = mem;
	mem_info->ptr = mem;

	mem_info->pos = 0;
	mem_info->length = length;

	wmf_bbuf_input (API,wmf_mem_read,wmf_mem_seek,wmf_mem_tell,(void*) mem_info);

	if (ERR (API))
	{	wmf_mem_close (API);
		return (API->err);
	}

	return (API->err);
}

/**
 * Close metafile input memory stream.
 * 
 * @param API the API handle
 * 
 * @return Returns the library error state (\b wmf_E_None on success).
 */
wmf_error_t wmf_mem_close (wmfAPI* API)
{	wmf_free (API,API->buffer_data);

	API->buffer_data = 0;

	API->bbuf.read = 0;
	API->bbuf.seek = 0;
	API->bbuf.tell = 0;

	return (API->err);
}

/**
 * @internal
 */
int wmf_mem_read (void* user_data)
{	int byte = EOF;

	wmfBBufMemInfo* mem_info = (wmfBBufMemInfo*) user_data;

	if (mem_info->pos < mem_info->length)
	{	byte = (int) (*(mem_info->ptr));

		mem_info->ptr++;
		mem_info->pos++;
	}

	return (byte);
}

/**
 * @internal
 */
int wmf_mem_seek (void* user_data,long pos)
{	wmfBBufMemInfo* mem_info = (wmfBBufMemInfo*) user_data;

	if ((pos < 0) || (pos > mem_info->length)) return (-1);

	mem_info->ptr = mem_info->mem + pos;
	mem_info->pos = pos;

	return (0);
}

/**
 * @internal
 */
long wmf_mem_tell (void* user_data)
{	wmfBBufMemInfo* mem_info = (wmfBBufMemInfo*) user_data;

	return (mem_info->pos);
}

/**
 * @internal
 *
 * Before allocating memory do a sanity check on size by seeking to
 * claimed end to see if its possible. We're constrained here by the
 * api and existing implementations to not simply seeking to SEEK_END.
 * So use what we have to skip to the last byte and try and read it.
 */
int wmf_can_supply (wmfAPI* API, long requested)
{	long pos;

	if (requested < 0)
	{	WMF_ERROR (API,"wmf_can_supply: negative request!");
		API->err = wmf_E_BadFile;
		return (-1);
	}

	if (requested == 0) return (0);

	pos = WMF_TELL (API);
	if (pos < 0)
	{	WMF_ERROR (API,"API's tell() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (-1);
	}

	if (WMF_SEEK (API, pos + requested - 1) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (-1);
	}

	if (WMF_READ (API) == (-1))
	{	WMF_ERROR (API,"Unexpected EOF!");
		API->err = wmf_E_EOF;
		return (-1);
	}

	if (WMF_SEEK (API, pos) == (-1))
	{	WMF_ERROR (API,"API's seek() failed on input stream!");
		API->err = wmf_E_BadFile;
		return (-1);
	}

	return (0);
}
