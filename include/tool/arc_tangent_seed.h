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

#include <optional>

#include <geometry/seg.h>
#include <math/vector2d.h>

class EDA_SHAPE;

/// Screen distance in pixels within which a click picks a graphic to start a tangent arc from.
constexpr int ARC_SEED_PICK_PIXELS = 5;

/// Start point and departure direction that ARC_DRAW_MODE::TANGENT needs to begin an arc.
struct ARC_TANGENT_SEED
{
    VECTOR2I m_start;
    VECTOR2D m_direction;
    bool     m_directionIsAxis = false;   ///< True when only the tangent line is known, not its sign
};


/**
 * Seed the next tangent arc from whichever endpoint of @a aShape lies nearest @a aNear.
 *
 * Used to chain arcs, where @a aNear is the last point the user clicked on the previous arc.
 */
std::optional<ARC_TANGENT_SEED> ArcTangentSeedNear( const EDA_SHAPE& aShape, const VECTOR2I& aNear );


/**
 * Tangent seed for a point on or near a segment, arc or Bezier shape.
 *
 * A point within @a aTolerance of an endpoint gives the outward direction at that endpoint, and the seed starts at
 * the endpoint itself.  Any other point within @a aTolerance of the shape gives the tangent axis at the nearest
 * point of the shape, and the seed starts there.
 *
 * @param aTolerance is the largest distance in IU between @a aPoint and the shape.
 * @return std::nullopt for other shape types or when the point is not close enough.
 */
std::optional<ARC_TANGENT_SEED> ArcTangentSeedAt( const EDA_SHAPE& aShape, const VECTOR2I& aPoint, int aTolerance );

/// @copydoc ArcTangentSeedAt(const EDA_SHAPE&,const VECTOR2I&,int)
std::optional<ARC_TANGENT_SEED> ArcTangentSeedAt( const SEG& aSegment, const VECTOR2I& aPoint, int aTolerance );
