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
#include <math/vector2d.h>
#include <tool/shape_draw_behavior.h>

struct EDA_IU_SCALE;


/**
 * Template base for SHAPE_DRAW_BEHAVIOR implementations that wrap a
 * MULTISTEP_GEOM_MANAGER and an assistant.
 *
 * Provides the standard pass-through methods (IsStarted, AddPoint, etc.)
 * so that concrete classes only need to supply ApplyToShape() and any
 * shape-specific geometry accessors.
 *
 * @tparam ManagerT   the geometry-manager type (e.g. ARC_GEOM_MANAGER).
 * @tparam AssistantT the visual-assistant type (e.g. ARC_ASSISTANT).
 */
template <typename ManagerT, typename AssistantT>
class MANAGED_DRAW_BEHAVIOR : public SHAPE_DRAW_BEHAVIOR
{
    static_assert( std::is_base_of_v<EDA_ITEM, AssistantT>, "AssistantT must derive from EDA_ITEM" );

public:
    MANAGED_DRAW_BEHAVIOR( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
            m_manager(),
            m_assistant( m_manager, aIuScale, aUnits )
    {
    }

    //
    // Non-geometry state interfaces
    //

    bool IsComplete() const override       { return m_manager.IsComplete(); }
    bool HasGeometryChanged() const override { return m_manager.HasGeometryChanged(); }
    void ClearGeometryChanged() override   { m_manager.ClearGeometryChanged(); }
    void Reset() override                  { m_manager.Reset(); }

    int GetStep() const override           { return static_cast<int>( m_manager.GetStep() ); }

    //
    // Interactive geometry operations
    //

    void AddPoint( const VECTOR2I& aP ) override     { m_manager.AddPoint( aP, true ); }
    void SetCursorPosition( const VECTOR2I& aP ) override { m_manager.AddPoint( aP, false ); }
    void RemoveLastPoint() override                  { m_manager.RemoveLastPoint(); }

    // Assistant stuff

    AssistantT& GetAssistant() override { return m_assistant; }
    void SetUnits( EDA_UNITS aUnits ) override { m_assistant.SetUnits( aUnits ); }

protected:
    ManagerT            m_manager;
    AssistantT          m_assistant;
};
