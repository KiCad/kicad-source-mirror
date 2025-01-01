/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
// In many gerber readers, the rotation of the full shape is broken
// so we are using primitives that does not need a rotation around aperture origin.
// Note also the primitive 1 (circle) can use 4 or 5 parameters
// the 5th parameter is the rotation (not used by Kicad ) around aperture origin and is
// a recent optional parameter and can create compatibility issues with old
// Gerber viewer, so it is not output (default = 0).

#define APER_MACRO_ROUNDRECT_NAME "RoundRect"

#define APER_MACRO_ROUNDRECT_HEADER \
"%AMRoundRect*\n\
0 Rectangle with rounded corners*\n\
0 $1 Rounding radius*\n\
0 $2 $3 $4 $5 $6 $7 $8 $9 X,Y pos of 4 corners*\n\
0 Add a 4 corners polygon primitive as box body*\n\
4,1,4,$2,$3,$4,$5,$6,$7,$8,$9,$2,$3,0*\n\
0 Add four circle primitives for the rounded corners*\n\
1,1,$1+$1,$2,$3*\n\
1,1,$1+$1,$4,$5*\n\
1,1,$1+$1,$6,$7*\n\
1,1,$1+$1,$8,$9*\n\
0 Add four rect primitives between the rounded corners*\n\
20,1,$1+$1,$2,$3,$4,$5,0*\n\
20,1,$1+$1,$4,$5,$6,$7,0*\n\
20,1,$1+$1,$6,$7,$8,$9,0*\n\
20,1,$1+$1,$8,$9,$2,$3,0*\
%\n"

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
// In many gerber readers, the rotation of the full shape is broken
// so we are using a primitive that does not need a rotation to be
// plotted
#define APER_MACRO_SHAPE_OVAL_NAME "HorizOval"

#define APER_MACRO_SHAPE_OVAL_HEADER \
"%AMHorizOval*\n\
0 Thick line with rounded ends*\n\
0 $1 width*\n\
0 $2 $3 position (X,Y) of the first rounded end (center of the circle)*\n\
0 $4 $5 position (X,Y) of the second rounded end (center of the circle)*\n\
0 Add line between two ends*\n\
20,1,$1,$2,$3,$4,$5,0*\n\
0 Add two circle primitives to create the rounded ends*\n\
1,1,$1,$2,$3*\n\
1,1,$1,$4,$5*%\n"

// A aperture macro to define a trapezoid (polygon) by 4 corners
// and a rotation angle
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

// A aperture macro to define a polygon by 5 corners
// and a rotation angle (useful for chamfered rect pads)
#define APER_MACRO_OUTLINE5P_NAME "Outline5P"

#define APER_MACRO_OUTLINE5P_HEADER \
"%AMOutline5P*\n\
0 Free polygon, 5 corners , with rotation*\n\
0 The origin of the aperture is its center*\n\
0 number of corners: always 5*\n\
0 $1 to $10 corner X, Y*\n\
0 $11 Rotation angle, in degrees counterclockwise*\n\
0 create outline with 5 corners*\n\
4,1,5,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$1,$2,$11*%\n"

// A aperture macro to define a polygon by 6 corners
// and a rotation angle (useful for chamfered rect pads)
#define APER_MACRO_OUTLINE6P_NAME "Outline6P"

#define APER_MACRO_OUTLINE6P_HEADER \
"%AMOutline6P*\n\
0 Free polygon, 6 corners , with rotation*\n\
0 The origin of the aperture is its center*\n\
0 number of corners: always 6*\n\
0 $1 to $12 corner X, Y*\n\
0 $13 Rotation angle, in degrees counterclockwise*\n\
0 create outline with 6 corners*\n\
4,1,6,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$1,$2,$13*%\n"

// A aperture macro to define a polygon by 7 corners
// and a rotation angle (useful for chamfered rect pads)
#define APER_MACRO_OUTLINE7P_NAME "Outline7P"

#define APER_MACRO_OUTLINE7P_HEADER \
"%AMOutline7P*\n\
0 Free polygon, 7 corners , with rotation*\n\
0 The origin of the aperture is its center*\n\
0 number of corners: always 7*\n\
0 $1 to $14 corner X, Y*\n\
0 $15 Rotation angle, in degrees counterclockwise*\n\
0 create outline with 7 corners*\n\
4,1,7,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$1,$2,$15*%\n"

// A aperture macro to define a polygon by 8 corners
// and a rotation angle (useful for chamfered rect pads)
#define APER_MACRO_OUTLINE8P_NAME "Outline8P"

#define APER_MACRO_OUTLINE8P_HEADER \
"%AMOutline8P*\n\
0 Free polygon, 8 corners , with rotation*\n\
0 The origin of the aperture is its center*\n\
0 number of corners: always 8*\n\
0 $1 to $16 corner X, Y*\n\
0 $17 Rotation angle, in degrees counterclockwise*\n\
0 create outline with 8 corners*\n\
4,1,8,$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$1,$2,$17*%\n"
