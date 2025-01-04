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

#pragma once

#include <preview_items/multistep_geom_manager.h>
#include <geometry/eda_angle.h>
#include <geometry/seg.h>

namespace KIGFX
{
namespace PREVIEW
{

/**
 * Manage the construction of a bezier through a series of steps.
 *
 * See also @ref KIGFX::PREVIEW::ARC_GEOM_MANAGER.
 *
 * Interfaces are provided to return both arc geometry (can be used to set up real beziers on
 * PCBs, for example) as well as important control points for informational overlays.
 */
class BEZIER_GEOM_MANAGER : public MULTISTEP_GEOM_MANAGER
{
public:
    BEZIER_GEOM_MANAGER() {}

    enum BEZIER_STEPS
    {
        SET_START = 0, ///< Waiting to lock in the start point
        SET_CONTROL1,  ///< Waiting to lock in the first control point
        SET_END,       ///< Waiting to lock in the end point
        SET_CONTROL2,  ///< Waiting to lock in the second control point
        COMPLETE
    };

    int getMaxStep() const override { return COMPLETE; }

    /**
     * Get the current step the manager is on (useful when drawing
     * something depends on the current state)
     */
    BEZIER_STEPS GetStep() const { return static_cast<BEZIER_STEPS>( getStep() ); }

    bool acceptPoint( const VECTOR2I& aPt ) override;

    /*
     * Geometry query interface - used by clients of the manager
     */

    ///< Get the center point of the arc (valid when state > SET_ORIGIN)
    VECTOR2I GetStart() const;

    ///< Get the coordinates of the arc start
    VECTOR2I GetControlC1() const;
    VECTOR2I GetControlC2() const;

    ///< Get the coordinates of the arc end point
    VECTOR2I GetEnd() const;

private:
    /*
     * Point acceptor functions
     */

    ///< Set the center point of the arc
    bool setStart( const VECTOR2I& aOrigin );
    bool setControlC1( const VECTOR2I& aControl );
    bool setEnd( const VECTOR2I& aCursor );
    bool setControlC2( const VECTOR2I& aControl );

    /*
     * Bezier geometry
     */
    VECTOR2I m_start;
    VECTOR2I m_controlC1;
    VECTOR2I m_end;
    VECTOR2I m_controlC2;
};

} // namespace PREVIEW
} // namespace KIGFX
