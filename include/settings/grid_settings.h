/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GRID_SETTINGS_H
#define _GRID_SETTINGS_H

#include <eda_units.h>
#include <wx/string.h>
#include <nlohmann/json_fwd.hpp>

class UNITS_PROVIDER;

/**
 * Common grid settings, available to every frame
 */
struct KICOMMON_API GRID
{
    bool operator==( const GRID& aOther ) const;

    /**
     * Returns a string representation of the grid in specified units.
     * Will reduce to a single dimension if the grid is square.
     */
    wxString MessageText( EDA_IU_SCALE aScale, EDA_UNITS aUnits, bool aDisplayUnits = true ) const;

    /**
     * Returns a string representation of the grid in the user's units.
     * Will reduce to a single dimension if the grid is square.
     */
    wxString UserUnitsMessageText( UNITS_PROVIDER* aProvider, bool aDisplayUnits = true ) const;

    VECTOR2D ToDouble( EDA_IU_SCALE aScale ) const;

    wxString name;
    wxString x;
    wxString y;
};

KICOMMON_API bool operator!=( const GRID& lhs, const GRID& rhs );
KICOMMON_API bool  operator<( const GRID& lhs, const GRID& rhs );

KICOMMON_API void to_json( nlohmann::json& j, const GRID& g );
KICOMMON_API void  from_json( const nlohmann::json& j, GRID& g );


struct GRID_SETTINGS
{
    bool              axes_enabled;
    std::vector<GRID> grids;
    wxString          user_grid_x;
    wxString          user_grid_y;
    int               last_size_idx;
    int               fast_grid_1;
    int               fast_grid_2;
    double            line_width;
    double            min_spacing;
    bool              show;
    int               style;
    int               snap;
    bool              force_component_snap;
    bool              overrides_enabled;
    bool              override_connected;
    int               override_connected_idx;
    bool              override_wires;
    int               override_wires_idx;
    bool              override_vias;
    int               override_vias_idx;
    bool              override_text;
    int               override_text_idx;
    bool              override_graphics;
    int               override_graphics_idx;
};

#endif
