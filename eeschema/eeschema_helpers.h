/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef EESCHEMA_HELPERS_H_
#define EESCHEMA_HELPERS_H_

#include <wx/string.h>
#include <sch_io/sch_io_mgr.h>

class SCHEMATIC;
class SCH_EDIT_FRAME;
class SETTINGS_MANAGER;
class PROJECT;

/**
 * Helper functions to do things like load schematics behind the scenes for special functions
 *
 * Similar to the Pcbnew scripting helpers, this is just to have one spot
 * for these types of "hacky" solutions
 */
class EESCHEMA_HELPERS
{
public:
    static SETTINGS_MANAGER* GetSettingsManager();
    static void              SetSchEditFrame( SCH_EDIT_FRAME* aSchEditFrame );
    static PROJECT*          GetDefaultProject( bool aSetActive );
    static SCHEMATIC*        LoadSchematic( const wxString& aFileName, bool aSetActive,
                                            bool aForceDefaultProject,
                                            PROJECT* aProject = nullptr,
                                            bool aCalculateConnectivity = true );
    static SCHEMATIC*        LoadSchematic( const wxString& aFileName,
                                            SCH_IO_MGR::SCH_FILE_T aFormat,
                                            bool aSetActive, bool aForceDefaultProject,
                                            PROJECT* aProject = nullptr,
                                            bool aCalculateConnectivity = true );

private:
    static SCH_EDIT_FRAME*   s_SchEditFrame;
    static SETTINGS_MANAGER* s_SettingsManager;
};

#endif
