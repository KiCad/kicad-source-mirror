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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PANEL_KICAD_LAUNCHER_H
#define KICAD_PANEL_KICAD_LAUNCHER_H

#include "panel_kicad_launcher_base.h"

#include <unordered_map>

class TOOL_MANAGER;
class TOOL_ACTION;
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

    // Action lookup keyed by button id rather than stashed as button client data, so the
    // handler for the button's asynchronous click event never has to dereference the event
    // object, which CreateLaunchers() may have already destroyed by the time the event arrives.
    std::unordered_map<int, const TOOL_ACTION*> m_actionForId;

    // Source of never-reused button ids so a stale click cannot alias a rebuilt button's id.
    int                                         m_nextLauncherId = wxID_HIGHEST + 1;
};


#endif // KICAD_PANEL_KICAD_LAUNCHER_H
