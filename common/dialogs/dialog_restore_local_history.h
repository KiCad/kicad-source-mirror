/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <dialog_shim.h>
#include <local_history.h>

class wxButton;
class wxCommandEvent;
class wxListCtrl;
class wxListEvent;
class wxTextCtrl;

class DIALOG_RESTORE_LOCAL_HISTORY : public DIALOG_SHIM
{
public:
    DIALOG_RESTORE_LOCAL_HISTORY( wxWindow* aParent, const std::vector<LOCAL_HISTORY_SNAPSHOT_INFO>& aSnapshots );

    wxString GetSelectedHash() const;

private:
    void BuildUi();
    void Populate();
    void UpdateDetails( long aIndex );
    void OnSelectionChanged( wxListEvent& aEvent );
    void OnItemActivated( wxListEvent& aEvent );
    void OnRestoreClicked( wxCommandEvent& aEvent );

    const std::vector<LOCAL_HISTORY_SNAPSHOT_INFO>& m_snapshots;
    wxListCtrl*                                     m_list = nullptr;
    wxTextCtrl*                                     m_details = nullptr;
    wxButton*                                       m_restoreButton = nullptr;
    long                                            m_selectedIndex = wxNOT_FOUND;
    wxString                                        m_selectedHash;
};
