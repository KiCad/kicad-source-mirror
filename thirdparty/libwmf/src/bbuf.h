/* libwmf ("bbuf.h"): library for wmf conversion
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


#ifndef WMFBBUF_H
#define WMFBBUF_H

#include <stdio.h>

typedef struct _wmfBBufFileInfo wmfBBufFileInfo;
typedef struct _wmfBBufMemInfo  wmfBBufMemInfo;

struct _wmfBBufFileInfo
{	FILE* file;
};

struct _wmfBBufMemInfo
{	unsigned char* mem;
	unsigned char* ptr;

	long pos; /* redundant, but convenient ?? */
	long length;
};

#endif /* ! WMFBBUF_H */
