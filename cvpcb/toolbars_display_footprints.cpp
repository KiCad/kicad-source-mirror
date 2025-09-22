/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <toolbars_display_footprints.h>

#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/ui/toolbar_configuration.h>
#include <tools/pcb_actions.h>

std::optional<TOOLBAR_CONFIGURATION> DISPLAY_FOOTPRINTS_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    // Currently, no top aux or right toolbars
    case TOOLBAR_LOC::RIGHT:
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::selectionTool )
              .AppendAction( ACTIONS::measureTool );

        config.AppendSeparator()
              .AppendAction( ACTIONS::toggleGrid )
              .AppendAction( ACTIONS::togglePolarCoords )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::millimetersUnits )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits ) )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Crosshair modes" ) )
                            .AddAction( ACTIONS::cursorSmallCrosshairs )
                            .AddAction( ACTIONS::cursorFullCrosshairs )
                            .AddAction( ACTIONS::cursor45Crosshairs ) );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::showPadNumbers )
              .AppendAction( PCB_ACTIONS::padDisplayMode )
              .AppendAction( PCB_ACTIONS::textOutlines )
              .AppendAction( PCB_ACTIONS::graphicsOutlines );
        break;

    case TOOLBAR_LOC::TOP_MAIN:
        config.AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendAction( ACTIONS::show3DViewer );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::gridSelect );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::zoomSelect );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::fpAutoZoom );
        break;
    }

    // clang-format on
    return config;
}
