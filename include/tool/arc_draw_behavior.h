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
#include <preview_items/arc_assistant.h>
#include <tool/managed_draw_behavior.h>

struct EDA_IU_SCALE;
class EDA_SHAPE;


/**
 * Interactive arc drawing behaviour: center -> start -> end angle.
 */
class ARC_DRAW_BEHAVIOR : public MANAGED_DRAW_BEHAVIOR<KIGFX::PREVIEW::ARC_GEOM_MANAGER, KIGFX::PREVIEW::ARC_ASSISTANT>
{
public:
    using MANAGED_DRAW_BEHAVIOR::MANAGED_DRAW_BEHAVIOR;

    ARC_DRAW_BEHAVIOR( const ARC_DRAW_BEHAVIOR& ) = delete;
    ARC_DRAW_BEHAVIOR& operator=( const ARC_DRAW_BEHAVIOR& ) = delete;

    void SetAngleSnap( bool aSnap ) override { m_manager.SetAngleSnap( aSnap ); }
    void ToggleClockwise() override  { m_manager.ToggleClockwise(); }

    bool OnProperties( EDA_SHAPE& aShape ) override
    {
        if( m_manager.GetStep() == KIGFX::PREVIEW::ARC_GEOM_MANAGER::SET_START )
        {
            aShape.SetArcAngleAndEnd( ANGLE_90 );
            return true;
        }

        if( m_manager.GetStep() == KIGFX::PREVIEW::ARC_GEOM_MANAGER::SET_ANGLE
            && m_manager.GetStartRadiusEnd() != m_manager.GetEndRadiusEnd() )
        {
            return true;
        }

        return false;
    }

    void ApplyToShape( EDA_SHAPE& aShape ) const override
    {
        const VECTOR2I center = m_manager.GetOrigin();
        aShape.SetCenter( center );

        // Swap start/end when clockwise so EDA_SHAPE stores them in geometric order.
        if( m_manager.GetSubtended() < ANGLE_0 )
        {
            aShape.SetStart( m_manager.GetStartRadiusEnd() );
            aShape.SetEnd( m_manager.GetEndRadiusEnd() );
        }
        else
        {
            aShape.SetStart( m_manager.GetEndRadiusEnd() );
            aShape.SetEnd( m_manager.GetStartRadiusEnd() );
        }
    }
};
