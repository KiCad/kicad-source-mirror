/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef PANEL_SETUP_BUSES_H
#define PANEL_SETUP_BUSES_H

#include <map>
#include <dialogs/panel_setup_buses_base.h>
#include <bus_alias.h>

class SCH_EDIT_FRAME;
class BUS_ALIAS;

class PANEL_SETUP_BUSES : public PANEL_SETUP_BUSES_BASE
{
public:
    PANEL_SETUP_BUSES( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame );

    ~PANEL_SETUP_BUSES();

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void ImportSettingsFrom( const std::map<wxString, std::vector<wxString>>& aAliases );

protected:
    void OnAddAlias( wxCommandEvent& aEvent ) override;
    void OnDeleteAlias( wxCommandEvent& aEvent ) override;
    void OnAddMember( wxCommandEvent& aEvent ) override;
    void OnRemoveMember( wxCommandEvent& aEvent ) override;
    void OnAliasesGridCellChanging( wxGridEvent& event );
    void OnMemberGridCellChanging( wxGridEvent& event );
    void OnMemberGridCellChanged( wxGridEvent& event );
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void loadAliases();

    void reloadMembersGridOnIdle( wxIdleEvent& aEvent );

    void doReloadMembersGrid();

    /**
     * Keep the BUS_ALIAS member list synchronized with the values displayed in the grid.
     *
     * This is primarily needed when rows are added programmatically (for example via
     * clipboard paste) because the BUS_ALIAS objects are not owned by the grid model.
     */
    void updateAliasMembers( int aAliasIndex );

private:
    SCH_EDIT_FRAME* m_frame;
    wxString        m_membersLabelTemplate;

    std::vector< std::shared_ptr<BUS_ALIAS> > m_aliases;
    int                                       m_lastAlias;
    wxString                                  m_lastAliasName;
    bool                                      m_membersGridDirty;

    wxString        m_errorMsg;
    WX_GRID*        m_errorGrid;
    int             m_errorRow;
};


#endif      // PANEL_SETUP_BUSES_H
