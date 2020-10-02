/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file gbr_plotter_aperture_macros.h
 * @brief specialized plotter for GERBER files format
 */

#pragma once

// A aperture macro to define a rounded rect pad shape
#define APER_MACRO_ROUNDRECT_NAME "RoundRect"

#define APER_MACRO_ROUNDRECT_HEADER \
"%AMRoundRect*\n\
0 Rectangle with rounded corners, with rotation*\n\
0 The origin of the aperture is its center*\n\
0 $1 X-size*\n\
0 $2 Y-size*\n\
0 $3 Rounding radius*\n\
0 $4 Rotation angle, in degrees counterclockwise*\n\
0 Add two overlapping rectangle primitives as box body*\n\
21,1,$1,$2-$3-$3,0,0,$4*\n\
21,1,$1-$3-$3,$2,0,0,$4*\n\
0 Add four circle primitives for the rounded corners*\n\
$5=$1/2*\n\
$6=$2/2*\n\
$7=2x$3*\n\
1,1,$7,$5-$3,$6-$3,$4*\n\
1,1,$7,-$5+$3,$6-$3,$4*\n\
1,1,$7,-$5+$3,-$6+$3,$4*\n\
1,1,$7,$5-$3,-$6+$3,$4*%\n"

// A aperture macro to define a rotated rect pad shape
#define APER_MACRO_ROT_RECT_NAME "RotRect"

#define APER_MACRO_ROT_RECT_HEADER \
"%AMRotRect*\n\
0 Rectangle, with rotation*\n\
0 The origin of the aperture is its center*\n\
0 $1 length*\n\
0 $2 width*\n\
0 $3 Rotation angle, in degrees counterclockwise*\n\
0 Add horizontal line*\n\
21,1,$1,$2,0,0,$3*%\n"


// A aperture macro to define a oval pad shape
#define APER_MACRO_HORIZ_OVAL_NAME "HorizOval"

#define APER_MACRO_HORIZ_OVAL_HEADER \
"%AMHorizOval*\n\
0 Thick line with rounded ends, with rotation*\n\
0 The origin of the aperture is its center*\n\
0 $1 length (X dim)*\n\
0 $2 width (Y dim)*\n\
0 $3 Rotation angle, in degrees counterclockwise*\n\
0 Add horizontal line*\n\
21,1,$1-$2,$2,0,0,$3*\n\
0 Add two circle primitives for the rounded ends*\n\
$4=($1-$2)/2*\n\
1,1,$2,$4,0,$3*\n\
1,1,$2,-$4,0,$3*%\n"

// A aperture macro to define a trapezoid (polygon) by 4 corners
#define APER_MACRO_OUTLINE4P_NAME "Outline4P"

#define APER_MACRO_OUTLINE4P_HEADER \
"%AMOutline4P*\n\
0 Free polygon, 4 corners , with rotation*\n\
0 The origin of the aperture is its center*\n\
0 number of corners: always 4*\n\
0 $1 to $8 corner X, Y*\n\
0 $9 Rotation angle, in degrees counterclockwise*\n\
0 create outline with 4 corners*\n\
4,1,4,$1,$2,$3,$4,$5,$6,$7,$8,$1,$2,$9*%\n"

