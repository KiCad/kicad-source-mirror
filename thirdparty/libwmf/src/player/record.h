/* libwmf ("player/record.h"): library for wmf conversion
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


#ifndef WMFPLAYER_RECORD_H
#define WMFPLAYER_RECORD_H

static U16 ParU16 (wmfAPI* API,wmfRecord* record,unsigned long index)
{	U16 sp1;
	U16 sp2;

	if (index >= record->size)
	{	WMF_ERROR (API,"Bad record - unexpectedly short!");
		API->err = wmf_E_BadFormat;
		return (0);
	}

	index <<= 1;
	sp1 = record->parameter[index];
	index++;
	sp2 = record->parameter[index];

	return ((sp2 << 8) | sp1);
}

static int PutParU16 (wmfAPI* API,wmfRecord* record,unsigned long index,U16 sp)
{	int changed = 0;

	unsigned char c1 = (unsigned char) ( sp       & 0xff);
	unsigned char c2 = (unsigned char) ((sp >> 8) & 0xff);

	if (index >= record->size)
	{	WMF_ERROR (API,"Bad record - unexpectedly short!");
		API->err = wmf_E_BadFormat;
		return (0);
	}

	index <<= 1;
	if (record->parameter[index] != c1)
	{	record->parameter[index] = c1;
		changed = 1;
	}
	index++;
	if (record->parameter[index] != c2)
	{	record->parameter[index] = c2;
		changed = 1;
	}

	return (changed);
}

static S16 ParS16 (wmfAPI* API,wmfRecord* record,unsigned long index)
{	U16 up;

	up = ParU16 (API,record,index);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (up);
	}

	return (U16_2_S16 (up));
}

static S32 ParS32 (wmfAPI* API,wmfRecord* record,unsigned long index)
{	U16 up;

	up = ParU16 (API,record,index);

	if (ERR (API))
	{	WMF_DEBUG (API,"bailing...");
		return (up);
	}

	return (U16_2_S32 (up));
}

static wmfRecord OffsetRecord (wmfAPI* API,wmfRecord* record,unsigned long index)
{	wmfRecord new_record;

	new_record.function = record->function;
	new_record.size = 0;
	new_record.parameter = 0;
	new_record.position = 0;

	if (index > record->size)
	{	WMF_ERROR (API,"Bad record - unexpectedly short!");
		API->err = wmf_E_BadFormat;
		return (new_record);
	}

	new_record.size = record->size - index;
	index <<= 1;
	new_record.parameter = record->parameter + index;

	new_record.position = record->position + index;

	return (new_record);
}

#endif /* ! WMFPLAYER_RECORD_H */
