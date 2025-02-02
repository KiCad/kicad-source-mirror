/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/shape_simple.h>

namespace PNS {

constexpr int HULL_MARGIN = 10;

class ITEM;
class LINE;
class DEBUG_DECORATOR;
class NODE;
/** Various utility functions */

const SHAPE_LINE_CHAIN ArcHull( const SHAPE_ARC& aSeg, int aClearance, int aWalkaroundThickness );

const SHAPE_LINE_CHAIN OctagonalHull( const VECTOR2I& aP0, const VECTOR2I& aSize,
                                      int aClearance, int aChamfer );

const SHAPE_LINE_CHAIN SegmentHull( const SHAPE_SEGMENT& aSeg, int aClearance,
                                    int aWalkaroundThickness );

/**
 * Function ConvexHull()
 *
 * Creates an octagonal hull around a convex polygon.
 * @param aConvex The convex polygon.
 * @param aClearance The minimum distance between polygon and hull.
 * @return A closed line chain describing the octagon.
 */
const SHAPE_LINE_CHAIN ConvexHull( const SHAPE_SIMPLE& aConvex, int aClearance );

SHAPE_RECT ApproximateSegmentAsRect( const SHAPE_SEGMENT& aSeg );

OPT_BOX2I ChangedArea( const ITEM* aItemA, const ITEM* aItemB );
OPT_BOX2I ChangedArea( const LINE& aLineA, const LINE& aLineB );

void HullIntersection( const SHAPE_LINE_CHAIN& hull, const SHAPE_LINE_CHAIN& line,
                       SHAPE_LINE_CHAIN::INTERSECTIONS& ips );

const SHAPE_LINE_CHAIN BuildHullForPrimitiveShape( const SHAPE* aShape, int aClearance,
                                                          int aWalkaroundThickness );

void NodeStats( DEBUG_DECORATOR* aDbg, wxString aLabel, NODE *aNode );

}


#endif    // __PNS_UTILS_H
