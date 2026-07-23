/* libwmf ("metadefs.h"): library for wmf conversion
   Copyright (C) 2000-2002 - various; see CREDITS, ChangeLog, and sources

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


#ifndef WMFMETADEFS_H
#define WMFMETADEFS_H

/* Modes for WmfSetMapMode
 */
#define MM_TEXT        1
#define MM_LOMETRIC    2
#define MM_HIMETRIC    3
#define MM_LOENGLISH   4
#define MM_HIENGLISH   5
#define MM_TWIPS       6
#define MM_ISOTROPIC   7
#define MM_ANISOTROPIC 8
#define MM_DPI         9

/* meta file api defs
 */
#define META_SAVEDC			0x001E	/*i dont think ive saved everything ?*/
#define META_REALIZEPALETTE		0x0035	/*safely ignored methinks*/
#define META_SETPALENTRIES		0x0037	/*safely ignored methinks*/
#define META_CREATEPALETTE		0x00f7	/*not implemented yet*/
#define META_SETBKMODE			0x0102	/*working on it*/
#define META_SETMAPMODE			0x0103	/*more or less implemented*/
#define META_SETROP2			0x0104	/*maybe it works maybe it doesnt*/
#define META_SETPOLYFILLMODE		0x0106
#define META_SETSTRETCHBLTMODE		0x0107	/*dont know yet*/
#define META_SETTEXTCHAREXTRA		0x0108	/*extra space after each character*/
#define META_RESTOREDC			0x0127	/*i dont think ive restored everything ?*/
#define META_INVERTREGION		0x012A	/*invert the select region*/
#define META_PAINTREGION		0x012B	/*paint the selected region*/
#define META_SELECTCLIPREGION		0x012C	/*set a region to be the clipping region*/
#define META_SELECTOBJECT		0x012D	/*select what object to draw with*/
#define META_SETTEXTALIGN		0x012E	/*working on it, vertial done for X*/
#define META_DIBCREATEPATTERNBRUSH	0x0142	/*implemented fully i think*/
#define META_DELETEOBJECT		0x01f0	/*delete an object, deleting selected object makes 
						  you fall back to the previous selected object ?*/
#define META_SETBKCOLOR			0x0201								
#define META_SETTEXTCOLOR		0x0209	/*text color, stored*/
#define META_SETTEXTJUSTIFICATION	0x020A	/*amount of extra space to add to each space i believe*/
#define META_SETWINDOWORG 		0x020B	/*defines the origin of the coordinate system that the wmf 
						  functions are against*/
#define META_SETWINDOWEXT		0x020C	/*defines the width and height of the coord-system*/
#define META_SETVIEWPORTORG 		0x020D
#define META_SETVIEWPORTEXT		0x020E	/*only makes a difference in those MM_ISOTROPIC and MM_ANISOTROPIC modes*/
#define META_OFFSETWINDOWORG		0x020F	/*changes the origin of the logical window*/
#define META_OFFSETVIEWPORTORG		0x0211	/*changes the origin of the device window*/
#define META_LINETO			0x0213	/*fairly obvious, once clipping is engaged*/
#define META_MOVETO			0x0214	/*obvious
						  see code for more*/
#define META_OFFSETCLIPRGN		0x0220	/*move the  clipping reg*/
#define META_FILLREGION			0x0228	/*fill the selected region with the selected brush*/
#define META_SETMAPPERFLAGS		0x0231	/*font mapping flag, ignorable methinks*/
#define META_SELECTPALETTE		0x0234	/*safely ignored methinks*/
#define META_CREATEFONTINDIRECT		0x02FB	/*gd mapping needs to be done, X one needs to be made robust*/
#define META_CREATEPENINDIRECT		0x02FA	/*uncertain*/
#define META_CREATEBRUSHINDIRECT	0x02FC	/*undertain*/
#define META_POLYGON			0x0324	/*obvious*/
#define META_POLYLINE			0x0325	/*obvious*/
#define META_SCALEWINDOWEXT		0x0410	/*modify the extent of the window*/
#define META_SCALEVIEWPORTEXT		0x0412	/*modify the extent of the viewport*/
#define META_EXCLUDECLIPRECT		0x0415	/*remove part of a clipping rect*/
#define META_INTERSECTCLIPRECT		0x0416	/*a clipping rectangle, often used for lines*/
#define META_ELLIPSE	 		0x0418	/*obvious*/
#define META_FLOODFILL 			0x0419	/*reasonably obvious*/
#define META_RECTANGLE			0x041B	/*obvious*/
#define META_SETPIXEL			0x041F	/*obvious*/
#define META_FRAMEREGION		0x0429	/*reasonably obvious*/

#define META_TEXTOUT			0x0521
#define META_POLYPOLYGON		0x0538	/*working*/
#define META_EXTFLOODFILL		0x0548	/*what are filltypes ?*/
#define META_ROUNDRECT			0x061C	/*reasonably obvious*/
#define META_PATBLT			0x061D	/*more ROP stuff*/
#define META_ESCAPE			0x0626	/*i think i can ignore this one*/
#define META_CREATEREGION		0x06FF	/*awkward*/
#define META_ARC		  	0x0817	/*obvious*/
#define META_PIE			0x081A	/*a pie*/
#define META_CHORD			0x0830	/*a chord*/
#define META_DIBBITBLT			0x0940	/*not so sure yet, some common ones working so far*/
#define META_EXTTEXTOUT			0x0a32 	/*working on it, basically working for X*/
#define META_DIBSTRETCHBLT		0x0b41	/*working on it*/
#define META_SETDIBTODEV		0x0d33	/*a reasonably complicated mechanism of copying a graphic to the screen*/
#define META_STRETCHDIB			0x0f43	/*working on it*/

/* Regions */

#define ERROR         0
#define NULLREGION    1
#define SIMPLEREGION  2
#define COMPLEXREGION 3

#define RGN_AND  1
#define RGN_OR   2
#define RGN_XOR  3
#define RGN_DIFF 4
#define RGN_COPY 5

#define CLIP_INTERSECT 0x0001
#define CLIP_EXCLUDE   0x0002
#define CLIP_KEEPRGN   0x0004

#endif /* ! WMFMETADEFS_H */
