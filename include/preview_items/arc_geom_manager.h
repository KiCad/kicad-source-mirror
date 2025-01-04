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

#ifndef PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H
#define PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H

#include <preview_items/multistep_geom_manager.h>
#include <geometry/eda_angle.h>

namespace KIGFX {
namespace PREVIEW {

/**
 * Manage the construction of a circular arc though sequential setting of critical points:
 * center, arc start and arc end. The manager is driven by setting cursor points, which
 * update the geometry, and optionally advance the manager state.
 *
 * Interfaces are provided to return both arc geometry (can be used to set up real arcs on
 * PCBs, for example) as well as important control points for informational overlays.
 */
class ARC_GEOM_MANAGER: public MULTISTEP_GEOM_MANAGER
{
public:
    ARC_GEOM_MANAGER()
    {}

    enum ARC_STEPS
    {
        SET_ORIGIN = 0,     ///< Waiting to lock in origin point
        SET_START,          ///< Waiting to lock in the arc start point
        SET_ANGLE,          ///< Waiting to lock in the arc end point
        COMPLETE
    };

    int getMaxStep() const override
    {
        return COMPLETE;
    }

    /**
     * Get the current step the manager is on (useful when drawing
     * something depends on the current state)
     */
    ARC_STEPS GetStep() const
    {
        return static_cast<ARC_STEPS>( getStep() );
    }

    bool acceptPoint( const VECTOR2I& aPt ) override;

    ///< The arc to be clockwise from start
    void SetClockwise( bool aCw );

    ///< Reverse the current are direction
    void ToggleClockwise();

    ///< Set angle snapping (for the next point)
    void SetAngleSnap( bool aSnap )
    {
        m_angleSnap = aSnap;
    }

    /*
     * Geometry query interface - used by clients of the manager
     */

    ///< Get the center point of the arc (valid when state > SET_ORIGIN)
    VECTOR2I GetOrigin() const;

    ///< Get the coordinates of the arc start
    VECTOR2I GetStartRadiusEnd() const;

    ///< Get the coordinates of the arc end point
    VECTOR2I GetEndRadiusEnd() const;

    ///< Get the radius of the arc (valid if step >= SET_START)
    double GetRadius() const;

    ///< Get the angle of the vector leading to the start point (valid if step >= SET_START)
    EDA_ANGLE GetStartAngle() const;

    ///< Get the angle of the vector leading to the end point (valid if step >= SET_ANGLE)
    EDA_ANGLE GetSubtended() const;

private:

    /*
     * Point acceptor functions
     */

    ///< Set the center point of the arc
    bool setOrigin( const VECTOR2I& aOrigin );

    ///< Set the end of the first radius line (arc start)
    bool setStart( const VECTOR2I& aEnd );

    ///< Set a point of the second radius line (collinear with arc end)
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
};

}       // PREVIEW
}       // KIGFX

#endif  // PREVIEW_ITEMS_ARC_GEOMETRY_MANAGER_H
