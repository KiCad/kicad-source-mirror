/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include "../raytracing/shapes3D/bbox_3d.h"
#include "../raytracing/shapes2D/round_segment_2d.h"

/**
 * Draw a round arrow.
 *
 * @param aPosition is the start position of the arrow.
 * @param aTargetPos is the end position of the arrow.
 * @param aSize is the diameter of the arrow.
 */
void DrawRoundArrow( SFVEC3F aPosition, SFVEC3F aTargetPos, float aSize );


/**
 * Draw the bounding box lines.
 *
 * @param aBBox is the box to draw.
 */
void DrawBoundingBox( const BBOX_3D& aBBox );


/**
 * Draw a half open cylinder with diameter 1.0f and height 1.0f.
 *
 * The bottom center is at (0,0,0) and top center is at (0,0,1).
 *
 * @param aNrSidesPerCircle is the number of segments to approximate a circle.
 */
void DrawHalfOpenCylinder( unsigned int aNrSidesPerCircle );


/**
 * Draw a thick line segment with rounded ends.
 *
 * @param aSegment is the thick segment to draw
 * @param aNrSidesPerCircle is the number of segments to approximate the circle used to draw
 *                          the rounded ends of the segment.
 */
void DrawSegment( const ROUND_SEGMENT_2D& aSegment, unsigned int aNrSidesPerCircle );

#endif // OPENGL_UTILS_H
