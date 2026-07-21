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

#include <eda_units.h>
#include <preview_items/bezier_assistant.h>
#include <tool/managed_draw_behavior.h>

struct EDA_IU_SCALE;
class EDA_SHAPE;


/**
 * Interactive bezier drawing behaviour:
 * start -> control1 -> end -> control2.
 */
class BEZIER_DRAW_BEHAVIOR
        : public MANAGED_DRAW_BEHAVIOR<KIGFX::PREVIEW::BEZIER_GEOM_MANAGER, KIGFX::PREVIEW::BEZIER_ASSISTANT>
{
public:
    using MANAGED_DRAW_BEHAVIOR::MANAGED_DRAW_BEHAVIOR;

    BEZIER_DRAW_BEHAVIOR( const BEZIER_DRAW_BEHAVIOR& ) = delete;
    BEZIER_DRAW_BEHAVIOR& operator=( const BEZIER_DRAW_BEHAVIOR& ) = delete;

    void ApplyToShape( EDA_SHAPE& aShape ) const override
    {
        aShape.SetStart( m_manager.GetStart() );
        aShape.SetBezierC1( m_manager.GetControlC1() );
        aShape.SetEnd( m_manager.GetEnd() );
        aShape.SetBezierC2( m_manager.GetControlC2() );
        aShape.RebuildBezierToSegmentsPointsList();
    }
};
