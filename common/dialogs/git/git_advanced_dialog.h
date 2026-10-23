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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GIT_ADVANCED_DIALOG_H
#define GIT_ADVANCED_DIALOG_H

#include <atomic>
#include <functional>
#include <memory>

#include <dialogs/git/dialog_git_advanced_base.h>
#include <dialogs/git/git_transfer.h>

class EDA_BASE_FRAME;
#include <git/git_editor_snapshot.h>

class GIT_COMMIT_PANE;
class KIGIT_COMMON;
class GIT_FILE_DIFF_AREA;
class PANEL_GIT_BRANCHES;
class PANEL_GIT_HISTORY;
class PANEL_GIT_REMOTES;
class PANEL_GIT_STASHES;
class PANEL_GIT_REPOSITORY;
class PANEL_GIT_TAGS;
class wxSplitterWindow;
struct GIT_WORKTREE_STATUS;


/**
 * The advanced Git window: a non-modal dialog with a top bar naming the repository and branch
 * and a sidebar of pages (History, Branches, Remotes, Stashes, Tags, Repository) over the live
 * repository. The Changes page carries the same commit controls as the docked Version Control
 * panel, and branch, tag, remote, stash and identity management happen here.
 */
class GIT_ADVANCED_DIALOG : public DIALOG_GIT_ADVANCED_BASE
{
public:
    GIT_ADVANCED_DIALOG( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane );
    ~GIT_ADVANCED_DIALOG() override;

    /// Raise the one window for this session, creating it on first use. The manager and both
    /// editors share it, so a working area never has two windows disagreeing about the repository.
    /// The pane that opens it runs the window's Pull and Push.
    static void ShowForFrame( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane );

    /// The live window, or null when none is open.
    static GIT_ADVANCED_DIALOG* Active();

    /// Open the window on its History page, narrowed to the commits that changed aPath.
    static void ShowHistoryForFile( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane, const wxString& aPath );

    /// Turn to the History page narrowed to aPath, a file or folder relative to the repository
    /// root. Empty shows every commit again.
    void ShowHistoryForFile( const wxString& aPath );

    /// Open the Changes page to amend the selected history commit.
    void ShowAmend( const wxString& aCommitHash );

    /// Close the live window, for when the project it was filled from goes away.
    static void CloseActive();

    /// Close the live window if the given pane opened it, for when that pane goes away.
    static void CloseForPane( GIT_COMMIT_PANE* aPane );

    void ReloadPages();

    /// Put the hosted commit panel into, or out of, its transfer state, for a transfer this
    /// window or one of its pages started.
    void SetPaneTransferBusy( bool aBusy, const wxString& aStatus = wxEmptyString );

    /// Show a line of transfer progress on the hosted commit panel's status line.
    void ShowTransferProgress( const wxString& aText );

    /// Run a repository write under the action lock, then refresh every surface on success.
    /// Shows a busy message if the lock is held, or aError if the action returns false. Set
    /// aRewritesWorkingTree for operations that change the files the editors have open, so they
    /// are reloaded from disk too.
    void RunGitAction( const std::function<bool( KIGIT_COMMON& )>& aFn, const wxString& aError,
                       bool aRewritesWorkingTree = false );

    /// As above, for an action with a message of its own: what it writes to aDetail is shown
    /// in place of libgit2's text.
    void RunGitAction( const std::function<bool( KIGIT_COMMON&, wxString& aDetail )>& aFn, const wxString& aError,
                       bool aRewritesWorkingTree = false );

    /// True while the frame's git session is still the one this window's lists were filled
    /// from. On a mismatch the window is stale: it closes itself and the action must abort.
    bool SessionStillCurrent();

    /// After the asynchronous LFS check, save the open editors and snapshot their documents
    /// before an action rewrites files on disk. Returns false to abort the action.
    bool ConfirmWorkingTreeChange();

    /// Recheck unsaved editors against the clean-tree snapshot after a confirmation, then
    /// snapshot their documents for reload. Does not save editors or create dirty files.
    bool ConfirmWorkingTreeChange( const GIT_WORKTREE_STATUS& aStatus );

    /// Check the working tree and LFS on a worker, then continue on the UI thread only when
    /// nothing is uncommitted or unsaved. Saving first would create changes that block it.
    void RefuseOverUncommittedWork( std::function<void( const GIT_WORKTREE_STATUS& )> aReady );

    /// Switch the working tree to the named branch, after confirming, then refresh.
    void SwitchToBranch( const wxString& aBranch );

private:
    /// The Changes page: the docked panel's commit controls with the selected file's diff below.
    wxWindow* buildChangesPage( wxWindow* aBook );

    /// Show the working copy of aPath against its committed version, or hide that area.
    void showWorkingDiff( const wxString& aPath );
    void hideWorkingDiff();

    /// Fill the top bar's repository name and branch list.
    void populateTopBar();

    /// Keep every text box in aWindow out of the dialog's saved control state.
    void optOutOfTextPersistence( wxWindow* aWindow );

    /// Fetch from the remote on a worker, then refresh every surface. aRetriedAfterTrust marks a fetch
    /// already rerun after a trusted host key.
    void onFetch( bool aRetriedAfterTrust = false );
    void startFetch( std::shared_ptr<KIGIT_COMMON> aCommon, bool aRetriedAfterTrust );

    /// On-disk identity of the open documents, taken by ConfirmWorkingTreeChange() so only the
    /// documents an operation actually rewrote are reloaded.
    EDITOR_SNAPSHOT m_docsBeforeChange;


    /// The fetch the top bar started.
    KIGIT_TRANSFER m_fetch;

    EDA_BASE_FRAME*  m_parent;
    GIT_COMMIT_PANE* m_pane;

    PANEL_GIT_HISTORY*    m_historyPage;
    PANEL_GIT_BRANCHES*   m_branchesPage;
    PANEL_GIT_REMOTES*    m_remotesPage;
    PANEL_GIT_STASHES*    m_stashesPage;
    PANEL_GIT_TAGS*       m_tagsPage;
    PANEL_GIT_REPOSITORY* m_repositoryPage;

    // Changes page
    wxSplitterWindow*   m_changesSplitter;
    GIT_COMMIT_PANE*    m_changesPane;
    GIT_FILE_DIFF_AREA* m_workingDiff;

    /// Attach once to the project's session. A replacement closes this window so old rows and
    /// pending query results cannot be applied to a different project.
    void syncListener();

    std::weak_ptr<KIGIT_COMMON> m_listenerSession;
    bool                        m_listenerAttached = false;

    /// Reads the top bar and history branch choices without holding up the UI.
    KIGIT_QUERY m_reloadQuery;

    /// Preflight reads for actions that may replace the working tree.
    KIGIT_QUERY m_actionQuery;
    KIGIT_QUERY m_fetchQuery;

    /// Where the user left the sash between the Changes page's list and its diff area.
    int m_changesSashPos = 0;
};

#endif // GIT_ADVANCED_DIALOG_H
