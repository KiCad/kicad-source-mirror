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

#include <tools/kicad_manager_actions.h>
#include <toolbars_kicad_manager.h>


std::optional<TOOLBAR_CONFIGURATION> KICAD_MANAGER_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    // No other toolbars
    case TOOLBAR_LOC::RIGHT:
    case TOOLBAR_LOC::TOP_AUX:
    case TOOLBAR_LOC::TOP_MAIN:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( KICAD_MANAGER_ACTIONS::newProject )
              .AppendAction( KICAD_MANAGER_ACTIONS::openProject );

        config.AppendSeparator()
              .AppendAction( KICAD_MANAGER_ACTIONS::archiveProject )
              .AppendAction( KICAD_MANAGER_ACTIONS::unarchiveProject );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw );

        config.AppendSeparator()
              .AppendAction( KICAD_MANAGER_ACTIONS::openProjectDirectory );

        break;
    }

    // clang-format on
    return config;
}
