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

#ifndef PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H
#define PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H

#include <array>

#include <preview_items/multistep_geom_manager.h>
#include <geometry/arc_construction.h>
#include <geometry/eda_angle.h>
#include <tool/arc_draw_mode.h>

namespace KIGFX {
namespace PREVIEW {

/**
 * Manage the construction of a circular arc though sequential setting of critical points.
 * Which points are set, and in what order, depends on the ARC_DRAW_MODE. The manager is driven
 * by setting cursor points, which update the geometry, and optionally advance the manager state.
 *
 * Interfaces are provided to return both arc geometry (can be used to set up real arcs on
 * PCBs, for example) as well as important control points for informational overlays.
 *
 * Every mode except ARC_DRAW_MODE::CENTER_START_END keeps the clicked endpoints exactly and
 * describes the arc through GetSolution(), because storing them as center, radius and angles
 * moves them by up to one internal unit.
 */
class ARC_GEOM_MANAGER: public MULTISTEP_GEOM_MANAGER
{
public:
    ARC_GEOM_MANAGER()
    {}

    /// The point each step waits for depends on the mode, for example the origin, the start, then the end
    enum ARC_STEPS
    {
        FIRST_POINT = 0,    ///< Waiting to lock in the first point
        SECOND_POINT,       ///< Waiting to lock in the second point
        THIRD_POINT,        ///< Waiting to lock in the third point
        COMPLETE
    };

    int getMaxStep() const override
    {
        return HasTangentSeed() ? SECOND_POINT : COMPLETE;
    }

    /**
     * Get the current step the manager is on (useful when drawing
     * something depends on the current state)
     */
    ARC_STEPS GetStep() const
    {
        return IsComplete() ? COMPLETE : static_cast<ARC_STEPS>( getStep() );
    }

    /**
     * Reset to the initial state, ready for the next arc.
     *
     * The draw mode is kept. The tangent seed is cleared and must be set again.
     */
    void Reset() override;

    /**
     * Select the construction sequence.
     *
     * Switching mid-draw keeps the first locked point when it is an arc start in both modes, which is
     * every mode except CENTER_START_END. Otherwise construction restarts.
     */
    void SetMode( ARC_DRAW_MODE aMode );

    ARC_DRAW_MODE GetMode() const { return m_mode; }

    /**
     * Preload the start point and departure direction for ARC_DRAW_MODE::TANGENT.
     *
     * Only takes effect before the first point is locked in. Without a seed TANGENT takes its start
     * and direction from clicks like START_DIR_END, and with one a single click sets the end.
     *
     * With @a aDirectionIsAxis only the tangent line is known, so the sign giving the smaller sweep to the
     * cursor is used and ToggleClockwise() flips it.
     */
    void SetTangentSeed( const VECTOR2I& aStart, const VECTOR2D& aDirection, bool aDirectionIsAxis = false );

    /// True when the TANGENT mode is running from a seed, so its first click already sets the end
    bool HasTangentSeed() const { return m_mode == ARC_DRAW_MODE::TANGENT && m_hasSeed; }

    /// True when there is something to preview, which a seeded tangent has before any click
    bool HasPreview() const { return !IsReset() || HasTangentSeed(); }


    /**
     * Reverse the current arc direction in center mode, or switch between the minor and major arc
     * in START_END_CENTER mode, or the departure sign of an axis tangent seed. Has no effect in the other
     * modes, where the points fix the direction.
     */
    void ToggleClockwise();

    /// Set angle snapping (for the next point)
    void SetAngleSnap( bool aSnap )
    {
        m_angleSnap = aSnap;
    }

    /*
     * Geometry query interface - used by clients of the manager
     */

    /// Get the center point of the arc (valid when state > FIRST_POINT in center mode)
    VECTOR2I GetOrigin() const;

    /// Get the coordinates of the arc start
    VECTOR2I GetStartRadiusEnd() const;

    /// Get the coordinates of the arc end point
    VECTOR2I GetEndRadiusEnd() const;

    /// Get the radius of the arc (valid if step >= SECOND_POINT in center mode)
    double GetRadius() const;

    /// Get the angle of the vector leading to the start point (valid if step >= SECOND_POINT in center mode)
    EDA_ANGLE GetStartAngle() const;

    /// Get the angle of the vector leading to the end point (valid if step >= THIRD_POINT in center mode)
    EDA_ANGLE GetSubtended() const;

    /// True when the committed endpoints are the clicked points, described by GetSolution()
    bool UsesExactEndpoints() const { return m_mode != ARC_DRAW_MODE::CENTER_START_END; }

    /**
     * The arc as start, mid and end (never used in center mode).
     *
     * It is invalid until the arc is fully determined, and keeps the last valid arc while the
     * cursor is on a degenerate position.
     */
    const KIGEOM::ARC_SOLUTION& GetSolution() const { return m_solution; }

    /// Index of the point the next click sets, counting a tangent seed as point 0
    int GetPointIndex() const { return getStep() + ( HasTangentSeed() ? 1 : 0 ); }

    /// A point of the construction as set so far (index 0 to 2), including the one following the cursor
    const VECTOR2I& GetPoint( int aIndex ) const { return m_pts[aIndex]; }

    /// The departure direction of the arc (valid once the direction is set, or seeded)
    const VECTOR2D& GetTangentDirection() const { return m_dir; }

    /// True when a direction is part of the construction, that is START_DIR_END or an unseeded TANGENT
    bool UsesDirectionClick() const
    {
        return m_mode == ARC_DRAW_MODE::START_DIR_END || ( m_mode == ARC_DRAW_MODE::TANGENT && !m_hasSeed );
    }

protected:
    bool acceptPoint( const VECTOR2I& aPt ) override;

private:
    /// Only the center mode returns to the previous step, so a refused click in the others is ignored
    bool rejectRegresses() const override { return m_mode == ARC_DRAW_MODE::CENTER_START_END; }

    /// Point acceptor for every mode that keeps the clicked endpoints
    bool acceptExactPoint( const VECTOR2I& aPt );

    /// Take over a valid solution as the canonical arc state
    void applySolution( const KIGEOM::ARC_SOLUTION& aSolution );

    /// Forget the solution, leaving nothing drawable
    void clearSolution();

    /// Set the departure direction from the second click, snapped if requested
    bool setDirection( const VECTOR2I& aPt );

    /*
     * Point acceptor functions
     */

    /// Set the center point of the arc
    bool setOrigin( const VECTOR2I& aOrigin );

    /// Set the end of the first radius line (arc start)
    bool setStart( const VECTOR2I& aEnd );

    /// Set a point of the second radius line (collinear with arc end)
    bool setEnd( const VECTOR2I& aCursor );

    /*
     * Arc geometry
     */
    bool      m_clockwise  = true;
    VECTOR2I  m_origin;
    double    m_radius = 0.0;
    EDA_ANGLE m_startAngle;
    EDA_ANGLE m_endAngle;

    /*
     * construction parameters
     */
    bool m_angleSnap = false;
    bool m_directionLocked = false;

    ARC_DRAW_MODE m_mode = ARC_DRAW_MODE::CENTER_START_END;

    /// The points set so far for the modes that keep the clicked endpoints, the last following the cursor
    std::array<VECTOR2I, 3> m_pts;
    VECTOR2D                m_dir;
    bool                    m_hasSeed = false;
    bool                    m_seedIsAxis = false;
    bool                    m_axisFlipped = false;
    VECTOR2D                m_axis;
    bool                    m_major = false;
    KIGEOM::ARC_SOLUTION    m_solution;
};

}       // PREVIEW
}       // KIGFX

#endif  // PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H
