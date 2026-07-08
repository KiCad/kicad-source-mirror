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

#include <memory>

#include <geometry/eda_angle.h>
#include <math/vector2d.h>
#include <eda_units.h>
#include <preview_items/arc_assistant.h>

struct EDA_IU_SCALE;
class EDA_SHAPE;


/**
 * Encapsulate the interactive arc drawing UX: center -> start -> end angle.
 *
 * This is a reusable behaviour for any canvas-based tool that draws circular
 * arcs through a three-click construction sequence.  It wraps
 * KIGFX::PREVIEW::ARC_GEOM_MANAGER for the geometry state machine and
 * KIGFX::PREVIEW::ARC_ASSISTANT for the visual overlay, leaving the caller
 * responsible for the event loop, grid-snapping, and commit logic, since
 * we don't have a common interface for those things.
 *
 * This is a pretty thin wrapper around the geometry manager, but the thing it's
 * really doing is owning and binding the mananger and the assistant (view item)
 * together.
 *
 * The visual assistant must still be added to a KIGFX::VIEW by the caller.
 */
class ARC_DRAW_BEHAVIOR
{
public:
    ARC_DRAW_BEHAVIOR( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits );

    // Non-copyable, non-movable: ARC_ASSISTANT holds a const ref of
    // the internal ARC_GEOM_MANAGER.
    ARC_DRAW_BEHAVIOR( const ARC_DRAW_BEHAVIOR& ) = delete;
    ARC_DRAW_BEHAVIOR& operator=( const ARC_DRAW_BEHAVIOR& ) = delete;

    //
    // State queries
    //

    /// True after at least one point has been locked in.
    bool IsStarted() const;

    /// True when all three points have been locked in.
    bool IsComplete() const;

    /// True if the geometry changed since the last call to ClearGeometryChanged().
    bool HasGeometryChanged() const;

    /// Reset the geometry-changed flag (call after updating the preview).
    void ClearGeometryChanged();

    //
    // Interactive operations called from a TOOL framework event loop
    //

    /// Lock in a point and advance the construction state.
    void AddPoint( const VECTOR2I& aPosition );

    /// Preview the cursor position without advancing state.
    void SetCursorPosition( const VECTOR2I& aPosition );

    /// Undo the last locked-in point.
    void RemoveLastPoint();

    /// Flip arc direction between clockwise and counter-clockwise.
    void ToggleClockwise();

    /// Enable or disable angle snapping for the next point.
    void SetAngleSnap( bool aSnap );

    //
    // Geometry access (for updating the preview shape)
    //

    VECTOR2I  GetCenter() const;
    VECTOR2I  GetStartPoint() const;
    VECTOR2I  GetEndPoint() const;
    double    GetRadius() const;
    EDA_ANGLE GetStartAngle() const;
    EDA_ANGLE GetSubtendedAngle() const;

    /**
     * Transfer the current arc geometry to an EDA_SHAPE.
     *
     * Works for both PCB_SHAPE and SCH_SHAPE since both inherit the arc
     * storage from EDA_SHAPE.
     */
    void ApplyToShape( EDA_SHAPE& aShape ) const;

    const KIGFX::PREVIEW::ARC_GEOM_MANAGER& GetManager() const { return *m_manager; }

    KIGFX::PREVIEW::ARC_ASSISTANT& GetAssistant() { return *m_assistant; }

    /// Forward a units change to the assistant overlay.
    void SetUnits( EDA_UNITS aUnits ) { m_assistant->SetUnits( aUnits ); }

private:
    std::unique_ptr<KIGFX::PREVIEW::ARC_GEOM_MANAGER> m_manager;
    std::unique_ptr<KIGFX::PREVIEW::ARC_ASSISTANT>    m_assistant;
};
