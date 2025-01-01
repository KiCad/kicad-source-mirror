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


#ifndef DIALOG_GIT_SWITCH_H
#define DIALOG_GIT_SWITCH_H

#include <dialog_shim.h>
#include <wx/timer.h>
#include <git2.h>

class wxButton;
class wxListView;
class wxTextCtrl;
class wxListEvent;

struct BranchData
{
    wxString commitString;
    time_t   lastUpdated;
    bool     isRemote;
};

class DIALOG_GIT_SWITCH : public DIALOG_SHIM
{
public:
    DIALOG_GIT_SWITCH(wxWindow* aParent, git_repository* aRepository);
    virtual ~DIALOG_GIT_SWITCH();

    wxString GetBranchName() const;

private:
    void PopulateBranchList();
    void OnBranchListSelection(wxListEvent& event);
    void OnBranchListDClick(wxListEvent& event);
    void OnSwitchButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void GetBranches();

    wxListView* m_branchList;
    wxTextCtrl* m_branchNameText;
    wxButton* m_switchButton;
    wxTimer m_timer;

    wxString m_currentBranch;

    git_repository* m_repository;
    wxString m_lastEnteredText;
    bool m_existingBranch;

    std::map<wxString, BranchData> m_branches;

    void StartTimer();
    void StopTimer();
};

#endif // DIALOG_GIT_SWITCH_H
