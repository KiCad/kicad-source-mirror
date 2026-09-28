/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <math/vector2d.h>

/**
 * Construction helpers for the interactive arc drawing modes.
 *
 * Every solver returns the arc as start/mid/end, the same form the file formats store. The midpoint fixes
 * the sweep direction, so no separate clockwise flag is needed. Start and end are always returned exactly
 * as given.
 */
namespace KIGEOM
{

struct ARC_SOLUTION
{
    bool     valid = false;   ///< False for degenerate input (collinear points, zero chord, etc)
    VECTOR2I start;
    VECTOR2I mid;             ///< The point on the arc halfway along the sweep
    VECTOR2I end;
    VECTOR2D center;
    double   radius = 0.0;
};


/// Project @a aCursor onto the perpendicular bisector of the chord @a aStart - @a aEnd.
VECTOR2I ProjectToChordBisector( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor );


/// Arc from @a aStart to @a aEnd that passes through @a aOnArc.
ARC_SOLUTION ArcThroughPoints( const VECTOR2I& aStart, const VECTOR2I& aOnArc, const VECTOR2I& aEnd );


/**
 * Arc from @a aStart to @a aEnd whose midpoint is @a aCursor projected onto the chord bisector.
 *
 * Invalid when the projection lands on the chord.
 */
ARC_SOLUTION ArcFromStartEndMidDrag( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor );


/**
 * Arc from @a aStart to @a aEnd centered on @a aCursor projected onto the chord bisector.
 *
 * @param aMajor false for the minor arc (sweep up to 180 degrees), true for the complementary major arc.
 *               A center on the chord gives a semicircle in either case, and the flag then picks the side.
 */
ARC_SOLUTION ArcFromStartEndCenterDrag( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor,
                                        bool aMajor );


/**
 * Arc leaving @a aStart along @a aTangent and ending at @a aEnd.
 *
 * Invalid when @a aEnd lies on the tangent line or @a aTangent is zero.
 */
ARC_SOLUTION ArcFromStartTangentEnd( const VECTOR2I& aStart, const VECTOR2D& aTangent, const VECTOR2I& aEnd );

} // namespace KIGEOM
