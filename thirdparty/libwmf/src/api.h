/* libwmf ("api.h"): library for wmf conversion
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


#ifndef WMFAPI_API_H
#define WMFAPI_API_H

typedef struct _wmfMemoryManager  wmfMemoryManager;

struct _wmfMemoryManager
{	U32 count;
	U32 max;

	void** list;

	void* context;

	void* (*malloc)  (void* context,size_t size);
	void* (*realloc) (void* context,void* mem,size_t size);
	void  (*free)    (void* context,void* mem);
};

#endif /* ! WMFAPI_API_H */
