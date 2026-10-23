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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_GIT_BRANCHES_H
#define PANEL_GIT_BRANCHES_H

#include <vector>

#include <dialogs/git/git_transfer.h>
#include <dialogs/git/panel_git_branches_base.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
struct KIGIT_BRANCH_DETAIL;


/**
 * The advanced git window's Branches page: a searchable list beside the selected branch's
 * detail, with the actions that create, delete and force push a branch.
 */
class PANEL_GIT_BRANCHES : public PANEL_GIT_BRANCHES_BASE
{
public:
    PANEL_GIT_BRANCHES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );
    ~PANEL_GIT_BRANCHES() override;

    /// Fill the list from the repository's branches. The repository is read on a worker.
    void Populate();

    /// Ask a force push still running to stop. It finishes on its own.
    void StopTransfer();

private:
    /// Refill the list after the search text changed.
    void reloadBranches();

    /// Fill the detail pane with aBranch, or hide it when the name is empty or gone.
    void showBranchDetail( const wxString& aBranch );
    void fillBranchDetail( const KIGIT_BRANCH_DETAIL& aDetail );
    void hideBranchDetail();

    void onNewBranch();
    void onDeleteBranch();

    /// Force push the current branch, after an explicit confirmation. aRetriedAfterTrust marks a push
    /// already rerun after a trusted host key.
    void onForcePush( bool aRetriedAfterTrust = false );
    void startForcePush( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aExpectedBranch,
                         const wxString& aExpectedHeadOid, bool aRetriedAfterTrust );

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    /// Whether each listed row is a remote-tracking branch. Those are shown but cannot be
    /// switched to, so the row has to say which it is.
    std::vector<bool> m_branchIsRemote;

    /// The force push this page started.
    KIGIT_TRANSFER m_push;

    /// List and detail reads share the session lock without waiting on the UI thread.
    KIGIT_QUERY m_listQuery;
    KIGIT_QUERY m_detailQuery;

    /// Reads needed before an action's confirmation, separate from list refreshes.
    KIGIT_QUERY m_actionQuery;

    /// Where the user left the sash between the list and its detail pane.
    int m_branchSashPos = 0;

    /// True while the list is being refilled, when its selection events are its own.
    bool m_fillingBranches = false;
};

#endif // PANEL_GIT_BRANCHES_H
