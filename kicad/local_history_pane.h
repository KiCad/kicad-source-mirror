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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file local_history_pane.h
 */

#ifndef LOCAL_HISTORY_PANE_H
#define LOCAL_HISTORY_PANE_H

#include <wx/datetime.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <vector>

wxDECLARE_EVENT( EVT_LOCAL_HISTORY_REFRESH, wxCommandEvent );

class KICAD_MANAGER_FRAME;

struct LOCAL_COMMIT_INFO
{
    wxString   hash;
    wxString   summary;
    wxString   message;
    wxDateTime date;
};

class LOCAL_HISTORY_PANE : public wxPanel
{
public:
    LOCAL_HISTORY_PANE( KICAD_MANAGER_FRAME* aParent );
    ~LOCAL_HISTORY_PANE();

    void RefreshHistory( const wxString& aProjectPath );

private:
    void OnMotion( wxMouseEvent& aEvent );
    void OnRightClick( wxListEvent& aEvent );
    void OnRefreshEvent( wxCommandEvent& aEvent );
    void OnRefreshTimer( wxTimerEvent& aEvent );

    KICAD_MANAGER_FRAME*           m_frame;
    wxListCtrl*                    m_list;
    std::vector<LOCAL_COMMIT_INFO> m_commits;
    wxTimer                        m_refreshTimer;
};

#endif // LOCAL_HISTORY_PANE_H
