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

 #ifndef TOOLBARS_DISPLAY_FOOTPRINTS_H_
 #define TOOLBARS_DISPLAY_FOOTPRINTS_H_

#include <tool/ui/toolbar_configuration.h>

/**
 * Toolbar configuration for the Display Footprints frame.
 */
class DISPLAY_FOOTPRINTS_TOOLBAR_SETTINGS : public TOOLBAR_SETTINGS
{
public:
    DISPLAY_FOOTPRINTS_TOOLBAR_SETTINGS() :
            TOOLBAR_SETTINGS( "display_footprints-toolbars" )
    {}

    ~DISPLAY_FOOTPRINTS_TOOLBAR_SETTINGS() {}

    std::optional<TOOLBAR_CONFIGURATION> DefaultToolbarConfig( TOOLBAR_LOC aToolbar ) override;
};

#endif /* TOOLBARS_DISPLAY_FOOTPRINTS_H_ */
