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

#include <eda_item.h>
#include <eda_units.h>
#include <math/vector2d.h>

class EDA_SHAPE;


/**
 * Abstract interface for interactive shape-drawing behaviours.
 *
 * Concrete implementations wrap a geometry manager (for the construction
 * state machine) and an assistant (for the visual overlay).
 *
 * Callers drive the behaviour through this class and finally must use
 * ApplyToShape() to transfer the geometry to an EDA_SHAPE.
 *
 * The visual assistant (returned by @ref GetAssistant()) must be added to a
 * KIGFX::VIEW by the caller.
 */
class SHAPE_DRAW_BEHAVIOR
{
public:
    virtual ~SHAPE_DRAW_BEHAVIOR() = default;

    //
    // Non-geometric state interfaces
    //

    /// True when all points have been locked in.
    virtual bool IsComplete() const = 0;

    /// True if the geometry changed since the last call to @ref ClearGeometryChanged().
    virtual bool HasGeometryChanged() const = 0;

    /// Reset the geometry-changed flag (call after updating the preview).
    virtual void ClearGeometryChanged() = 0;

    /// Return the current construction step (0-based, shape-specific meaning).
    virtual int GetStep() const = 0;

    /// Reset the behaviour to its initial state for chained object creation loops.
    virtual void Reset() = 0;

    //
    // Interactive operations called from a TOOL framework event loop
    //

    /// Lock in a point and advance the construction state.
    virtual void AddPoint( const VECTOR2I& aPosition ) = 0;

    /// Preview the cursor position without advancing state.
    virtual void SetCursorPosition( const VECTOR2I& aPosition ) = 0;

    /// Undo the last locked-in point.
    virtual void RemoveLastPoint() = 0;

    /// Flip arc direction (applies only when the shape has such a
    /// concept of directionality, e.g. circular arcs).
    virtual void ToggleClockwise() {}

    /// Enable or disable angle snapping (circular arcs only; no-op for others).
    virtual void SetAngleSnap( bool aSnap ) {}

    /**
     * Called when the user invokes the properties action mid-draw.
     *
     * @return true if the behavior handled the request (the event loop
     *         should continue); false if the event should be passed on.
     */
    virtual bool OnProperties( EDA_SHAPE& aShape ) { return false; }

    //
    // Geometry access
    //

    /// Transfer the current geometry to an EDA_SHAPE.
    virtual void ApplyToShape( EDA_SHAPE& aShape ) const = 0;

    /// Return the visual assistant overlay item.
    virtual EDA_ITEM& GetAssistant() = 0;

    /// Forward a units change to the assistant overlay.
    virtual void SetUnits( EDA_UNITS aUnits ) = 0;
};
