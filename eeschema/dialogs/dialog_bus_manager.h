/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_BUS_MANAGER_H_
#define _DIALOG_BUS_MANAGER_H_

#include "dialog_shim.h"

#include <sch_edit_frame.h>
#include <wx/listctrl.h>

#include <bus_alias.h>

class DIALOG_BUS_MANAGER : public DIALOG_SHIM
{
public:
    DIALOG_BUS_MANAGER( SCH_EDIT_FRAME* aParent );

    ~DIALOG_BUS_MANAGER() {}

    bool TransferDataFromWindow() override;

    bool TransferDataToWindow() override;

    void OnAddBus( wxCommandEvent& aEvent );
    void OnRenameBus( wxCommandEvent& aEvent );
    void OnRemoveBus( wxCommandEvent& aEvent );

    void OnAddSignal( wxCommandEvent& aEvent );
    void OnRenameSignal( wxCommandEvent& aEvent );
    void OnRemoveSignal( wxCommandEvent& aEvent );

protected:
    void OnInitDialog( wxInitDialogEvent& aEvent );
    void OnSelectBus( wxListEvent& event );
    void OnSelectSignal( wxListEvent& event );

private:
    virtual void OnOkClick( wxCommandEvent& aEvent );
    virtual void OnCancelClick( wxCommandEvent& aEvent );
    wxString getAliasDisplayText( std::shared_ptr< BUS_ALIAS > aAlias );

protected:
    SCH_EDIT_FRAME* m_parent;

    wxListView* m_bus_list_view;
    wxListView* m_signal_list_view;
    wxTextCtrl* m_bus_edit;
    wxTextCtrl* m_signal_edit;

    wxButton* m_btn_add_bus;
    wxButton* m_btn_rename_bus;
    wxButton* m_btn_remove_bus;

    wxButton* m_btn_add_signal;
    wxButton* m_btn_rename_signal;
    wxButton* m_btn_remove_signal;

private:
    std::vector< std::shared_ptr< BUS_ALIAS > > m_aliases;
    std::unordered_set<SCH_SCREEN*> m_screens;
    std::shared_ptr< BUS_ALIAS > m_active_alias;

    DECLARE_EVENT_TABLE()
};


#endif

// _DIALOG_BUS_MANAGER_H_
