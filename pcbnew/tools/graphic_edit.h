/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRAPHIC_EDIT_H
#define GRAPHIC_EDIT_H

#include <eda_shape.h>
#include <geometry/shape_arc.h>
#include <math/vector2d.h>

#include <limits>
#include <vector>

class BOARD_ITEM;
class EDA_ITEM;
class PCB_SHAPE;

/// Larger arcs overflow the boundary search box.
constexpr int MAX_GRAPHIC_EDIT_ARC_RADIUS = std::numeric_limits<int>::max() / 2;

/// Sweeps this near zero or a full turn are ambiguous.
constexpr double GRAPHIC_EDIT_ANGLE_EPSILON = 1e-6;

/// Why a planner refused.  Each one gets its own message.
enum class GRAPHIC_EDIT_REFUSAL
{
    NONE,
    UNSUPPORTED_SOURCE,
    LOCKED_SOURCE,
    NO_INTERSECTION,
    AMBIGUOUS,
    DEGENERATE
};

struct GRAPHIC_EDIT_GEOMETRY
{
    /// A result need not be the same kind as the source.  A circle carries its centre in
    /// m_Start and a point on the rim in m_End.
    SHAPE_T  m_Shape = SHAPE_T::SEGMENT;

    VECTOR2I m_Start;

    /// Arcs only.  Everything else leaves it default.
    VECTOR2I m_Mid;
    VECTOR2I m_End;
};

struct GRAPHIC_EDIT_RESULT
{
    explicit operator bool() const { return m_Refusal == GRAPHIC_EDIT_REFUSAL::NONE; }

    GRAPHIC_EDIT_REFUSAL m_Refusal = GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION;

    /// What the source becomes.  Empty means the whole source goes.
    std::vector<GRAPHIC_EDIT_GEOMETRY> m_Geometry;

    /// What the preview draws.  Trim shows the span it would remove, so it is not m_Geometry.
    /// Left empty, the tool falls back to m_Geometry.
    std::vector<GRAPHIC_EDIT_GEOMETRY> m_Preview;

    std::vector<const BOARD_ITEM*>     m_Boundaries;
};

/// Any graphical shape a planner or a boundary search might use.  Anything else gives null.
const PCB_SHAPE* GraphicEditShape( const EDA_ITEM* aItem );

/// Extend needs two ends to work with.
bool IsGraphicExtendSource( const PCB_SHAPE& aShape );

/// Trim also takes the closed shapes, which it opens up.
bool IsGraphicTrimSource( const PCB_SHAPE& aShape );

/// The shape to edit, or null with the reason in aResult.  aAccepts decides the kinds.
const PCB_SHAPE* GraphicEditSource( const BOARD_ITEM& aSource, bool ( *aAccepts )( const PCB_SHAPE& ),
                                    GRAPHIC_EDIT_RESULT& aResult );

/// A boundary usable against aSource.  Null if it is the source, off-layer or unusable.
const PCB_SHAPE* GraphicEditBoundary( const BOARD_ITEM* aBoundary, const PCB_SHAPE& aSource );

SHAPE_ARC GraphicEditArc( const PCB_SHAPE& aShape );

/// False for radii and sweeps the planners refuse.  Check before planning an arc.
bool IsGraphicEditArcUsable( const SHAPE_ARC& aArc );

/// Points built by different routes land a rounding step apart.
bool GraphicEditCoincident( const VECTOR2I& aA, const VECTOR2I& aB );

#endif
