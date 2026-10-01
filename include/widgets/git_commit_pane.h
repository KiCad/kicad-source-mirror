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

#ifndef GIT_COMMIT_PANE_H
#define GIT_COMMIT_PANE_H

#include <atomic>
#include <memory>
#include <thread>
#include <set>

#include <wx/timer.h>

#include <dialogs/git/git_commit_pane_base.h>
#include <git/git_editor_snapshot.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class KIGIT_COMMON;
class wxCommandEvent;
class wxDataViewEvent;


/**
 * Docked panel that lists the current project's changed files with a checkbox each, plus a
 * message field, a Commit and Push button, and a button that opens the advanced Git window.
 */
class GIT_COMMIT_PANE : public GIT_COMMIT_PANE_BASE
{
public:
    GIT_COMMIT_PANE( EDA_BASE_FRAME* aParent );
    ~GIT_COMMIT_PANE() override;

    /// Read the project's git status and repopulate the list.
    void RefreshFileList();

    /// Reload the advanced window when it is open, for a preferences change it also shows.
    void ReloadAdvancedWindow();

private:
    void onCommit( wxCommandEvent& aEvent );
    void onCommitOnly( wxCommandEvent& aEvent );
    void onPull( wxCommandEvent& aEvent );

    /// Drop an in-progress merge or rebase and return the project to its last commit.
    void abandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon );
    void onAmendToggled( wxCommandEvent& aEvent );
    void onFileActivated( wxDataViewEvent& aEvent );
    void onChecklistContextMenu( wxDataViewEvent& aEvent );
    void discardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath );

    /// Commit the checked files (or amend the last commit when the amend box is ticked), then
    /// push when aAndPush is set. Both commit buttons land here.
    void doCommit( bool aAndPush );
    void onChecklistChanged( wxDataViewEvent& aEvent );
    void onMessageChanged( wxCommandEvent& aEvent );
    void onIdle( wxIdleEvent& aEvent );
    void onSize( wxSizeEvent& aEvent );
    void onOpenAdvanced( wxCommandEvent& aEvent );

    /// Set the status line text and wrap it to the current panel width.
    void setStatusText( const wxString& aText );

    /// Clear the list and disable every control while version control is off.
    void showDisabledState();

    /// Enable or disable the controls while a network operation runs, showing a status.
    void setBusy( bool aBusy, const wxString& aStatus );

    // Only these submit methods start a worker thread. The on*Done continuations run back on
    // the UI thread, so credential prompts and confirmations stay there.
    void submitPush( std::shared_ptr<KIGIT_COMMON> aCommon );
    void onPushDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError, bool aAuthFailure,
                     bool aCancelled );
    /// aThenPush pushes after a successful pull, which belongs only to the rejected-push
    /// recovery flow. A plain Get Changes leaves local commits unpublished.
    void submitPull( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush );

    /// Resolve a conflicted pull file by file, using the visual merge tool for design files,
    /// then create the merge commit. Cancelling leaves the repository exactly as it was.
    void resolvePullConflicts( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush );
    void onPullDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError, bool aAuthFailure,
                     bool aThenPush, bool aCancelled );

    /// Show the message field whenever a commit is possible, including a message-only amend.
    void updateMessageVisibility( bool aHasChanges );

    /// Set what the commit buttons say, and whether they are enabled, for the current state.
    void updateCommitButton();

    /// True when the schematic or board editor holds unsaved changes. Those are committable
    /// even with a clean working tree, because the commit flow saves the editors first.
    bool anyEditorDirty() const;

    /// Register a refresh listener with the project's current git session, re-registering
    /// if the session changed since last time (it is recreated when a project is loaded).
    void syncListener();

    EDA_BASE_FRAME* m_frame;

    bool m_hasUnpushedCommits;

    bool m_hasRemote;

    /// True when HEAD is a commit rather than a branch, so nothing can be committed or pushed.
    bool m_headDetached;

    /// True when the upstream has commits this project does not, so Get Changes has work to do.
    bool m_remoteAhead;

    /// True while a merge or rebase is in progress. The network button then reads Abandon.
    bool m_operationInProgress;

    /// True when an open editor held unsaved changes at the last refresh.
    bool m_editorsDirty;

    /// True while a push or pull worker is running. The Get Changes button then reads Cancel
    /// and requests cancellation of the running transfer.
    bool m_networkBusy;

    /// The token of the push or pull this panel submitted last. Held so Cancel reaches it even
    /// while the task is still queued and has no handler yet.
    std::shared_ptr<std::atomic<bool>> m_networkCancel;

    /// The push or pull running for this pane, joined before the pane goes.
    std::thread m_networkWorker;

    /// Unwrapped status text, kept so it can be re-wrapped on resize.
    wxString m_statusText;

    /// True once this panel has attached its refresh listener to a session at least once. A
    /// first attachment is not a project change, so it must not close the Advanced window.
    bool m_listenerAttached = false;

    /// The git session this panel registered a refresh listener with, weak so it does not
    /// keep the session alive. Used to unregister on destruction.
    std::weak_ptr<KIGIT_COMMON> m_listenerSession;

    /// Paths shown in the list at the last refresh, so a refresh can tell which files the user
    /// deliberately unticked from files that are simply new, and preserve those unticks.
    std::set<wxString> m_shownPaths;

    /// On-disk identity of the open documents, taken before a pull so only the ones the merge
    /// rewrote are reloaded afterwards.
    EDITOR_SNAPSHOT m_docsBeforePull;

    /// Re-runs RefreshFileList once when it had to skip because another git operation held the
    /// action lock. The background fetch can hold it for a whole network round trip and does
    /// not broadcast a refresh, so without a retry the list would stay stale.
    wxTimer m_refreshRetryTimer;
};

#endif // GIT_COMMIT_PANE_H
