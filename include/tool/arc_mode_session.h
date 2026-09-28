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

#include <functional>
#include <optional>

#include <math/vector2d.h>
#include <tool/arc_draw_mode.h>
#include <tool/arc_tangent_seed.h>

class APP_SETTINGS_BASE;
class SHAPE_DRAW_BEHAVIOR;
class TOOL_EVENT;


/**
 * Pick the mode a Draw Arc action starts in and apply it to @a aBehavior.
 *
 * The action's own mode wins over the one saved in @a aSettings, and the chosen mode is saved back.
 */
ARC_DRAW_MODE StartArcDrawMode( const TOOL_EVENT& aEvent, APP_SETTINGS_BASE* aSettings,
                                SHAPE_DRAW_BEHAVIOR& aBehavior );


/**
 * Arc drawing mode and tangent seed bookkeeping for one pass of an editor's managed-shape draw loop.
 *
 * The editor supplies how to find a graphic to seed a tangent arc from and what to refresh when the mode changes.
 * For shapes other than arcs the session only forwards points to the behavior.
 */
class ARC_MODE_SESSION
{
public:
    /// Tangent seed from a graphic under the cursor, given the raw and the snapped cursor positions.
    using SEED_FINDER = std::function<std::optional<ARC_TANGENT_SEED>( const VECTOR2I& aMouse,
                                                                       const VECTOR2I& aPos )>;

    /**
     * @param aChainSeed seeds the arc from the previous one in a chain.  It is applied here, so construct the
     *                   session after the behavior's Reset().
     */
    ARC_MODE_SESSION( SHAPE_DRAW_BEHAVIOR& aBehavior, APP_SETTINGS_BASE* aSettings, bool aDrawingArc,
                      const std::optional<ARC_TANGENT_SEED>& aChainSeed, SEED_FINDER aFindSeed,
                      std::function<void( ARC_DRAW_MODE )> aOnModeChanged );

    ARC_DRAW_MODE Mode() const;

    /// Switch mode mid-draw, which drops any seed.  Does nothing unless an arc is being drawn.
    void SetMode( ARC_DRAW_MODE aMode );

    /// Handle a mode or cycle action; return true if @a aEvent was one.
    bool HandleModeEvent( const TOOL_EVENT& aEvent );

    /// Lock in a point, or seed a tangent arc from the graphic under it when it would be the arc's first point.
    void PlacePoint( const VECTOR2I& aMouse, const VECTOR2I& aPos );

    /// Undo the last point.  A seed taken from a click is undone too, one from a chain is kept.
    void RemoveLastPoint();

private:
    SHAPE_DRAW_BEHAVIOR&                 m_behavior;
    APP_SETTINGS_BASE*                   m_settings;
    bool                                 m_drawingArc;
    bool                                 m_chainSeeded;
    bool                                 m_clickSeeded = false;
    SEED_FINDER                          m_findSeed;
    std::function<void( ARC_DRAW_MODE )> m_onModeChanged;
};
