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


#ifndef DIALOG_GIT_SWITCH_H
#define DIALOG_GIT_SWITCH_H

#include <dialog_shim.h>
#include <git/kicad_git_common.h>
#include <widgets/wx_debounced_action.h>
#include <vector>

class wxButton;
class wxListView;
class wxTextCtrl;
class wxListEvent;

class DIALOG_GIT_SWITCH : public DIALOG_SHIM
{
public:
    DIALOG_GIT_SWITCH( wxWindow* aParent, std::vector<KIGIT_BRANCH_INFO> aBranches );
    virtual ~DIALOG_GIT_SWITCH();

    wxString GetBranchName() const;

private:
    void PopulateBranchList();
    void OnBranchListSelection(wxListEvent& event);
    void OnBranchListDClick(wxListEvent& event);
    void OnSwitchButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void updateSwitchButton();
    bool branchExists( const wxString& aName ) const;

    wxListView* m_branchList;
    wxTextCtrl* m_branchNameText;
    wxButton* m_switchButton;
    WX_DEBOUNCED_ACTION m_branchNameDebounce;

    wxString m_currentBranch;

    wxString m_lastEnteredText;

    std::vector<KIGIT_BRANCH_INFO> m_branches;
};

#endif // DIALOG_GIT_SWITCH_H
