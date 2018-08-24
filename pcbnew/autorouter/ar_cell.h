/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
 *
 * First copyright (C) Randy Nevin, 1989 (see PCBCA package)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef _AR_CELL_H_
#define _AR_CELL_H_

/* Bits characterizing cell */
#define CELL_IS_EMPTY 0x00
#define CELL_IS_HOLE 0x01   /* a conducting hole or obstacle */
#define CELL_IS_MODULE 0x02 /* auto placement occupied by a module */
#define CELL_IS_EDGE 0x20   /* Area and auto-placement: limiting cell contour (Board, Zone) */
#define CELL_IS_FRIEND 0x40 /* Area and auto-placement: cell part of the net */
#define CELL_IS_ZONE 0x80   /* Area and auto-placement: cell available */

/* Bit masks for presence of obstacles to autorouting */
#define OCCUPE 1         /* Autorouting: obstacle tracks and vias. */
#define VIA_IMPOSSIBLE 2 /* Autorouting: obstacle for vias. */
#define CURRENT_PAD 4


/* traces radiating outward from a hole to a side or corner */
#define HOLE_NORTH 0x00000002L     /* upward       */
#define HOLE_NORTHEAST 0x00000004L /* upward and right */
#define HOLE_EAST 0x00000008L      /* to the right     */
#define HOLE_SOUTHEAST 0x00000010L /* downward and right   */
#define HOLE_SOUTH 0x00000020L     /* downward     */
#define HOLE_SOUTHWEST 0x00000040L /* downward and left    */
#define HOLE_WEST 0x00000080L      /* to the left      */
#define HOLE_NORTHWEST 0x00000100L /* upward and left  */

/* straight lines through the center */
#define LINE_HORIZONTAL 0x00000002L /* left-to-right line   */
#define LINE_VERTICAL 0x00000004L   /* top-to-bottom line   */

/* lines cutting across a corner, connecting adjacent sides */
#define CORNER_NORTHEAST 0x00000008L /* upper right corner   */
#define CORNER_SOUTHEAST 0x00000010L /* lower right corner   */
#define CORNER_SOUTHWEST 0x00000020L /* lower left corner    */
#define CORNER_NORTHWEST 0x00000040L /* upper left corner    */

/* diagonal lines through the center */
#define DIAG_NEtoSW 0x00000080L /* northeast to southwest */
#define DIAG_SEtoNW 0x00000100L /* southeast to northwest */

/* 135 degree angle side-to-far-corner lines */
#define BENT_NtoSE 0x00000200L /* north to southeast   */
#define BENT_NtoSW 0x00000400L /* north to southwest   */
#define BENT_EtoSW 0x00000800L /* east to southwest    */
#define BENT_EtoNW 0x00001000L /* east to northwest    */
#define BENT_StoNW 0x00002000L /* south to northwest   */
#define BENT_StoNE 0x00004000L /* south to northeast   */
#define BENT_WtoNE 0x00008000L /* west to northeast    */
#define BENT_WtoSE 0x00010000L /* west to southeast    */

/* 90 degree corner-to-adjacent-corner lines */
#define ANGLE_NEtoSE 0x00020000L /* northeast to southeast */
#define ANGLE_SEtoSW 0x00040000L /* southeast to southwest */
#define ANGLE_SWtoNW 0x00080000L /* southwest to northwest */
#define ANGLE_NWtoNE 0x00100000L /* northwest to northeast */

/* 45 degree angle side-to-near-corner lines */
#define SHARP_NtoNE 0x00200000L /* north to northeast   */
#define SHARP_EtoNE 0x00400000L /* east to northeast    */
#define SHARP_EtoSE 0x00800000L /* east to southeast    */
#define SHARP_StoSE 0x01000000L /* south to southeast   */
#define SHARP_StoSW 0x02000000L /* south to southwest   */
#define SHARP_WtoSW 0x04000000L /* west to southwest    */
#define SHARP_WtoNW 0x08000000L /* west to northwest    */
#define SHARP_NtoNW 0x10000000L /* north to northwest   */

/* directions the cell can be reached from (point to previous cell) */
#define FROM_NOWHERE 0
#define FROM_NORTH 1
#define FROM_NORTHEAST 2
#define FROM_EAST 3
#define FROM_SOUTHEAST 4
#define FROM_SOUTH 5
#define FROM_SOUTHWEST 6
#define FROM_WEST 7
#define FROM_NORTHWEST 8
#define FROM_OTHERSIDE 9


#endif // __AR_CELL_H
