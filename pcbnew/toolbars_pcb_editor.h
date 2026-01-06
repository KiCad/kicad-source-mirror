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

#ifndef TOOLBARS_PCB_EDITOR_H_
#define TOOLBARS_PCB_EDITOR_H_

#include <tool/action_toolbar.h>
#include <tool/ui/toolbar_configuration.h>

class PCB_ACTION_TOOLBAR_CONTROLS : public ACTION_TOOLBAR_CONTROLS
{
public:
    static ACTION_TOOLBAR_CONTROL trackWidth;
    static ACTION_TOOLBAR_CONTROL viaDiameter;
    static ACTION_TOOLBAR_CONTROL currentVariant;
};

/**
 * Toolbar configuration for the PCB editor frame.
 */
class PCB_EDIT_TOOLBAR_SETTINGS : public TOOLBAR_SETTINGS
{
public:
    PCB_EDIT_TOOLBAR_SETTINGS() :
            TOOLBAR_SETTINGS( "pcbnew-toolbars" )
    {}

    ~PCB_EDIT_TOOLBAR_SETTINGS() {}

    std::optional<TOOLBAR_CONFIGURATION> DefaultToolbarConfig( TOOLBAR_LOC aToolbar ) override;
};

#endif /* TOOLBARS_PCB_EDITOR_H_ */
