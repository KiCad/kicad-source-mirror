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

#ifndef PREVIEW_ITEMS_ELLIPSE_GEOMETRY_MANAGER_H
#define PREVIEW_ITEMS_ELLIPSE_GEOMETRY_MANAGER_H

#include <preview_items/multistep_geom_manager.h>
#include <geometry/eda_angle.h>
#include <geometry/ellipse.h>

namespace KIGFX
{
namespace PREVIEW
{

/**
 * Manage the construction of an elliptical arc through sequential setting of
 * critical points: two bounding-box corners (to define the ellipse), then
 * start and end angles on the ellipse.
 *
 * The manager is driven by setting cursor points, which update the geometry
 * and optionally advance the manager state.
 */
class ELLIPSE_GEOM_MANAGER : public MULTISTEP_GEOM_MANAGER
{
public:
    ELLIPSE_GEOM_MANAGER() {}

    enum ELLIPSE_STEPS
    {
        SET_BBOX_C1 = 0, ///< Waiting to lock in first bbox corner
        SET_BBOX_C2,     ///< Waiting to lock in second bbox corner
        SET_START_ANGLE, ///< Waiting to lock in the start angle
        SET_END_ANGLE,   ///< Waiting to lock in the end angle
        COMPLETE
    };

    int getMaxStep() const override { return COMPLETE; }

    ELLIPSE_STEPS GetStep() const { return static_cast<ELLIPSE_STEPS>( getStep() ); }

    /*
    * Geometry query interface - used by clients of the manager
    */

    /**
     * Get the ellipse defined by the current geometry (valid after SET_BBOX_C2).
     *
     * Until the angles are set, the ellipse is considered to be a full ellipse.  After the angles
     * are set, the ellipse is considered to be an arc segment.
     */
    ELLIPSE<int> GetEllipse() const;

    ///< Get the start angle (valid after SET_START_ANGLE).
    EDA_ANGLE GetStartAngle() const { return m_startAngle; }

    ///< Get the end angle (valid after SET_END_ANGLE).  Always > startAngle.
    EDA_ANGLE GetEndAngle() const { return m_endAngle; }

    ///< Get the first bbox corner (valid after SET_BBOX_C1).
    VECTOR2I GetBboxCorner1() const { return m_bboxC1; }

    ///< Get the second bbox corner (valid after SET_BBOX_C2).
    VECTOR2I GetBboxCorner2() const { return m_bboxC2; }

protected:
    bool acceptPoint( const VECTOR2I& aPt ) override;

private:
    bool setBboxCorner1( const VECTOR2I& aPt );
    bool setBboxCorner2( const VECTOR2I& aPt );
    bool setStartAngle( const VECTOR2I& aPt );
    bool setEndAngle( const VECTOR2I& aPt );

    VECTOR2I  m_bboxC1;
    VECTOR2I  m_bboxC2;
    VECTOR2I  m_center;
    double    m_majorRadius = 1.0;
    double    m_minorRadius = 1.0;
    EDA_ANGLE m_rotation = ANGLE_0;
    EDA_ANGLE m_startAngle = ANGLE_0;
    EDA_ANGLE m_endAngle = ANGLE_360;
};

} // namespace PREVIEW
} // namespace KIGFX

#endif // PREVIEW_ITEMS_ELLIPSE_GEOMETRY_MANAGER_H
