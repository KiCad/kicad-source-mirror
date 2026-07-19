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

#include <geometry/eda_angle.h>
#include <math/vector2d.h>
#include <eda_units.h>
#include <preview_items/ellipse_assistant.h>
#include <tool/managed_draw_behavior.h>

struct EDA_IU_SCALE;
class EDA_SHAPE;


/**
 * Interactive elliptical-arc drawing behaviour:
 * bbox corner 1 -> bbox corner 2 -> start angle -> end angle.
 */
class ELLIPSE_ARC_DRAW_BEHAVIOR
        : public MANAGED_DRAW_BEHAVIOR<KIGFX::PREVIEW::ELLIPSE_GEOM_MANAGER, KIGFX::PREVIEW::ELLIPSE_ASSISTANT>
{
public:
    using MANAGED_DRAW_BEHAVIOR::MANAGED_DRAW_BEHAVIOR;

    ELLIPSE_ARC_DRAW_BEHAVIOR( const ELLIPSE_ARC_DRAW_BEHAVIOR& ) = delete;
    ELLIPSE_ARC_DRAW_BEHAVIOR& operator=( const ELLIPSE_ARC_DRAW_BEHAVIOR& ) = delete;

    void ApplyToShape( EDA_SHAPE& aShape ) const override
    {
        const ELLIPSE<int> ellipse = m_manager.GetEllipse();

        aShape.SetCenter( ellipse.Center );
        aShape.SetEllipseMajorRadius( ellipse.MajorRadius );
        aShape.SetEllipseMinorRadius( ellipse.MinorRadius );
        aShape.SetEllipseRotation( ellipse.Rotation );
        aShape.SetEllipseStartAngle( ellipse.StartAngle );
        aShape.SetEllipseEndAngle( ellipse.EndAngle );
    }
};
