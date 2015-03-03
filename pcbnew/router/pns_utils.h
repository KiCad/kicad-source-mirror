/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_UTILS_H
#define __PNS_UTILS_H

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>

#define HULL_MARGIN 10

class PNS_ITEM;

/** Various utility functions */

const SHAPE_LINE_CHAIN OctagonalHull( const VECTOR2I& aP0, const VECTOR2I& aSize,
                                      int aClearance, int aChamfer );

const SHAPE_LINE_CHAIN SegmentHull ( const SHAPE_SEGMENT& aSeg, int aClearance,
                                     int aWalkaroundThickness );

SHAPE_RECT ApproximateSegmentAsRect( const SHAPE_SEGMENT& aSeg );

void DrawDebugPoint( VECTOR2I aP, int aColor );
void DrawDebugBox( BOX2I aB, int aColor );
void DrawDebugSeg( SEG aS, int aColor );
void DrawDebugDirs( VECTOR2D aP, int aMask, int aColor );

OPT_BOX2I ChangedArea( const PNS_ITEM* aItemA, const PNS_ITEM* aItemB );


#endif    // __PNS_UTILS_H
