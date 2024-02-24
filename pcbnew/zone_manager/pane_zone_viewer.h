/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PANE_ZONE_VIEWER_H
#define PANE_ZONE_VIEWER_H


#include "zone_selection_change_notifier.h"
#include <unordered_map>
#include <wx/window.h>
#include <wx/panel.h>
#include <wx/notebook.h>
class wxDataViewCtrl;
class PANEL_ZONE_GAL;
class PCB_BASE_FRAME;
class PANEL_ZONE_GAL_CONTAINER;
class ROW_ICON_PROVIDER;
class PANE_ZONE_VIEWER : public wxNotebook, public ZONE_SELECTION_CHANGE_NOTIFIER
{
public:
    PANE_ZONE_VIEWER( wxWindow* aParent, PCB_BASE_FRAME* aPcbFrame );
    ~PANE_ZONE_VIEWER() override;

    void ActivateSelectedZone( ZONE* new_zone ) override;

    void OnNotebook( wxNotebookEvent& aEvent );

    PANEL_ZONE_GAL* GetZoneGAL() const { return m_zoneGAL; }


private:
    PCB_BASE_FRAME*                                    m_pcbFrame;
    std::unordered_map<int, PANEL_ZONE_GAL_CONTAINER*> m_zoneContainers;
    PANEL_ZONE_GAL*                                    m_zoneGAL{};
};

#endif