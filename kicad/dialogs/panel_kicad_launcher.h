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

#ifndef KICAD_PANEL_KICAD_LAUNCHER_H
#define KICAD_PANEL_KICAD_LAUNCHER_H

#include "panel_kicad_launcher_base.h"

class TOOL_MANAGER;
class KICAD_MANAGER_FRAME;

class PANEL_KICAD_LAUNCHER : public PANEL_KICAD_LAUNCHER_BASE
{
public:
    PANEL_KICAD_LAUNCHER( wxWindow* aParent );

    ~PANEL_KICAD_LAUNCHER();

    void CreateLaunchers();

private:
    void onThemeChanged( wxSysColourChangedEvent& aEvent );
    void onLauncherButtonClick( wxCommandEvent& aEvent );

    KICAD_MANAGER_FRAME* m_frame;
};


#endif // KICAD_PANEL_KICAD_LAUNCHER_H
