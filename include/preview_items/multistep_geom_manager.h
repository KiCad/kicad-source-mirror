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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PREVIEW_ITEMS_MULTISTEP_GEOMETRY_MANAGER_H
#define PREVIEW_ITEMS_MULTISTEP_GEOMETRY_MANAGER_H

#include <math/vector2d.h>
#include <algorithm>
#include <core/kicad_algo.h>

namespace KIGFX {
namespace PREVIEW {

/**
 * A geometry manager that works by accepting a sequence of points
 * and advancing though stages of geometric construction with each
 * point "locked in".
 *
 * For example, constructing an arc might go:
 *    - Set origin
 *    - Set start point
 *    - Set end point
 *
 * The exact steps and how the points translate to geometry depends on
 * the subclass.
 */
class MULTISTEP_GEOM_MANAGER
{
public:
    virtual ~MULTISTEP_GEOM_MANAGER()
    {}

    /**
     * Add a point to the construction manager
     *
     * @param aPt the new point
     * @param aLockIn whether to "lock in" the point, and move the geometry manager to the
     *                next (or previous) step. False to update geometry and not affect
     *                manager state.
     */
    void AddPoint( const VECTOR2I& aPt, bool aLockIn )
    {
        // hold onto the raw point separately to the managed geometry
        m_lastPoint = aPt;

        // update the geometry
        bool accepted = acceptPoint( aPt );

        // advance or regress the manager
        if( aLockIn )
            performStep( accepted );

        setGeometryChanged();
    }

    /**
     * Undo the last point, and move the manager back to the previous step.
     */
    void RemoveLastPoint()
    {
        performStep( false );

        // process the last point again, but in the previous step mode it doesn't matter if
        // accepted or not, as long as the geometry is regenerated if needed.
        acceptPoint( GetLastPoint() );
        setGeometryChanged();
    }

    /**
     * @return true if the manager is in the initial state
     */
    bool IsReset() const
    {
        return m_step == 0;
    }

    /**
     * Reset the manager to the initial state.
     */
    void Reset()
    {
        m_step = 0;
        setGeometryChanged();
    }

    /**
     * @return true if the manager reached the final state.
     */
    bool IsComplete() const
    {
        return m_step == getMaxStep();
    }

    /**
     * Get the last point added (locked in or not).
     *
     * This can* be useful when drawing previews, as the point given isn't always what gets
     * locked into the geometry, if that step doesn't have full degrees of freedom.
     */
    VECTOR2I GetLastPoint() const
    {
        return m_lastPoint;
    }

    /**
     * @return true if the geometry has changed, eg such that a client should redraw.
     */
    bool HasGeometryChanged() const
    {
        return m_changed;
    }

    /**
     * Clear the geometry changed flag, call after the client code has updated everything
     * as needed.
     */
    void ClearGeometryChanged()
    {
        m_changed = false;
    }

protected:

    ///< Mark the geometry as changed for clients to notice
    void setGeometryChanged()
    {
        m_changed = true;
    }

    ///< Get the current stage of the manager
    int getStep() const
    {
        return m_step;
    }

private:

    ///< Function that accepts a point for a stage, or rejects it
    ///< to return to the previous stage
    virtual bool acceptPoint( const VECTOR2I& aPt ) = 0;

    /**
     * The highest step this manager has - used to recognize completion
     * and to clamp the step as it advances.
     */
    virtual int getMaxStep() const = 0;

    ///< Moves the manager forward or backward through the stages
    void performStep( bool aForward )
    {
        m_step += aForward ? 1 : -1;

        // clamp to allowed values
        m_step = std::clamp( m_step, 0, getMaxStep() );
    }

    ///< Has the geometry changed such that a client should redraw?
    bool m_changed = false;

    ///< The last (raw) point added, which is usually the cursor position
    VECTOR2I m_lastPoint;

    /**
     * The current manager step, from 0 to some highest number that depends on the manager.
     */
    int m_step = 0;
};

}       // PREVIEW
}       // KIGFX

#endif  // PREVIEW_ITEMS_MULTIPOINT_GEOMETRY_MANAGER_H
