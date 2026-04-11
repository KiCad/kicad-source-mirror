/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee.org>
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

#include <3d_canvas/board_adapter.h>
#include <3d_viewer/tools/eda_3d_conditions.h>

#include <functional>

using namespace std::placeholders;


SELECTION_CONDITION EDA_3D_CONDITIONS::GridSize( GRID3D_TYPE aGridSize )
{
    return std::bind( &EDA_3D_CONDITIONS::gridSizeFunction, _1, m_adapter, aGridSize );
}


bool EDA_3D_CONDITIONS::gridSizeFunction( const SELECTION& aSelection,
                                          BOARD_ADAPTER* aAdapter,
                                          GRID3D_TYPE aGridSize )
{
    return aAdapter->m_Cfg->m_Render.grid_type == aGridSize;
}
