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
#include <functional>
#include <memory>
#include <optional>
#include <set>

#include <dialogs/git/git_commit_pane_base.h>
#include <dialogs/git/git_transfer.h>
#include <git/git_editor_snapshot.h>
#include <git/kigit_known_hosts.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class KIGIT_COMMON;
enum class GitOperation;
class wxCommandEvent;
class wxMenuItem;
class wxDataViewEvent;
class wxHyperlinkCtrl;
class wxStaticText;


/**
 * Docked panel for committing: the branch to commit to, a summary and description, the commit
 * options, the changed files with a checkbox each, the commit and Get Changes buttons, and a
 * gear holding the commit settings and the advanced Git window.
 */
class GIT_COMMIT_PANE : public GIT_COMMIT_PANE_BASE
{
public:
    GIT_COMMIT_PANE( EDA_BASE_FRAME* aParent );

    /// The same panel inside another window, such as the advanced window's Changes page. The
    /// gear's entry for opening that window is left out there.
    GIT_COMMIT_PANE( wxWindow* aParent, EDA_BASE_FRAME* aFrame );
    ~GIT_COMMIT_PANE() override;

    /// Lay out the pane, then re-wrap the status line when the width its column got has changed.
    bool Layout() override;

    /// Called with the path of the file the user selects in the list, or empty when none is.
    void SetFileSelectedHandler( std::function<void( const wxString& )> aHandler );

    /// The message as typed. A second form on the same project shows the same text.
    wxString GetCommitMessage() const;
    void     SetCommitMessage( const wxString& aMessage );

    /// Called when the user edits the message, so another form can follow along.
    void SetMessageChangedHandler( std::function<void()> aHandler ) { m_messageChangedHandler = aHandler; }

    /// Read the project's git status and repopulate the list.
    void RefreshFileList();

    /// Put the panel into the state it uses while a transfer runs, for one started elsewhere.
    /// The advanced window hosts this panel, so a fetch or a force push begun from there must
    /// disable the same controls. A transfer this panel started keeps the state until it ends.
    void SetTransferBusy( bool aBusy, const wxString& aStatus = wxEmptyString );

    /// Show a line of transfer progress. Called from the UI thread by a KIGIT_STATUS_REPORTER.
    void ShowTransferProgress( const wxString& aText );

    /// Reload the advanced window when it is open, for a preferences change it also shows.
    void ReloadAdvancedWindow();

    /// Open the advanced window on its History page.
    void ShowHistory();

    /// Get changes from the remote, as the Get Changes button does. The advanced window's
    /// Pull button lands here, so one place owns the merge and its conflict handling.
    void Pull();

    /// Push the local commits, as the Push button does. The advanced window's Push button
    /// lands here too.
    void Push();

    /// Prepare the amend form. A history entry supplies its hash and starts with no files selected.
    void BeginAmend( const wxString& aExpectedCommit = wxEmptyString );

    /// Called when a transfer starts and when it ends, so an owning window can follow along.
    void SetBusyChangedHandler( std::function<void( bool )> aHandler ) { m_busyChangedHandler = aHandler; }

    /// The width the panel needs so its widest row fits, for the dock that holds it.
    int GetMinDockWidth() const { return m_minDockWidth; }

private:
    /// The split button's main half. It commits, or pushes when that is all there is to do.
    void onCommit( wxCommandEvent& aEvent );
    void onCommitAndPush( wxCommandEvent& aEvent );
    void onPull( wxCommandEvent& aEvent );

    /// Drop an in-progress merge or rebase and return the project to its last commit.
    void abandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon );
    void finishAbandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon, const std::vector<wxString>& aOutside );
    void onAmendToggled( wxCommandEvent& aEvent );

    /// Take back the last commit while no remote has it, keeping its changes staged and its message.
    void onUndoCommit( wxCommandEvent& aEvent );

    void onNewBranchToggled( wxCommandEvent& aEvent );
    void onRecentMessage( wxCommandEvent& aEvent );
    void onShowDiff( wxCommandEvent& aEvent );
    void onFileActivated( wxDataViewEvent& aEvent );
    void onChecklistContextMenu( wxDataViewEvent& aEvent );
    void discardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath );
    void confirmDiscardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath, bool aTracked,
                             bool aConflicted, wxString aRoot );

    /// Commit the checked files (or amend the last commit when the amend box is ticked), then
    /// push when aAndPush is set. Both halves of the commit button land here.
    void doCommit( bool aAndPush );

    struct COMMIT_REQUEST;
    void saveForCommit( std::shared_ptr<KIGIT_COMMON> aCommon, std::shared_ptr<COMMIT_REQUEST> aRequest );
    void finishCommit( std::shared_ptr<KIGIT_COMMON> aCommon, const COMMIT_REQUEST& aRequest );
    void onChecklistChanged( wxDataViewEvent& aEvent );
    void onMessageChanged( wxCommandEvent& aEvent );
    void onIdle( wxIdleEvent& aEvent );

    /// The gear's menu: the advanced window and the author.
    void onSettings( wxCommandEvent& aEvent );
    void setAuthor();

    /// Set the status line text and wrap it to the width of the label's column.
    void setStatusText( const wxString& aText );

    /// Lay out the controls, then rewrap the status line to its column's new width.
    bool layoutAndWrapStatus();

    /// Give the AUI pane the controls' minimum height, so a dock never squeezes them.
    void updateDockMinHeight();

    /// Show a command to copy under the status line, or hide the row when aCommand is empty.
    void showTrustCommand( const wxString& aCommand );

    /// False, with a message, while a transfer is running or a merge waits to be finished.
    bool transferCanStart();

    /// Put the file list beside the commit controls instead of above them, for a wide host,
    /// with a heading over the message and a summary of the working tree under it.
    void layoutWide();

    /// Refresh the staged count in the changes heading, and the summary box where the wide
    /// layout has one.
    void updateSummary();

    struct STATUS;

    /// Apply a completed repository snapshot on the UI thread.
    void applyStatus( const STATUS& aStatus );

    /// Fill the branch dropdown from the completed snapshot.
    void populateBranches( const STATUS& aStatus );

    /// Fill the Recent messages dropdown from the completed snapshot.
    void populateRecentMessages( const std::vector<wxString>& aMessages );

    /// Where the platform has no hint for a multi line box, a label lies over the description
    /// box while it is empty.
    void updateDescriptionHint();
    void setDescription( const wxString& aText );

    /// Clear the list and disable every control while version control is off.
    void showDisabledState();

    /// Enable or disable the controls while a network operation runs, showing a status.
    void setBusy( bool aBusy, const wxString& aStatus );

    /// Read an action's prerequisites off the UI thread, then resume only in the same project.
    void runActionQuery( std::shared_ptr<KIGIT_COMMON> aCommon, std::function<void( KIGIT_COMMON& )> aWork,
                         std::function<void()> aDone, const wxString& aStatus );

    // The on*Done continuations run back on the UI thread, so credential prompts and
    // confirmations stay there. aRetriedAfterTrust marks a transfer already rerun after a trusted host key.
    void submitPush( std::shared_ptr<KIGIT_COMMON> aCommon, bool aRetriedAfterTrust = false );
    void startPush( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aBranch, const wxString& aHead,
                    bool aRetriedAfterTrust );
    void onPushDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError,
                     const wxString& aDetails, bool aAuthFailure,
                     const std::optional<KIGIT::HOST_KEY_FAILURE>& aHostKey, bool aRetriedAfterTrust, bool aCancelled );
    /// aThenPush pushes after a successful pull, which belongs only to the rejected-push
    /// recovery flow. A plain Get Changes leaves local commits unpublished.
    void submitPull( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush, bool aRetriedAfterTrust = false );
    void startPull( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush, bool aRetriedAfterTrust );

    /// Resolve a conflicted pull file by file, using the visual merge tool for design files,
    /// then create the merge commit. Cancelling leaves the repository exactly as it was.
    void resolvePullConflicts( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush );
    struct MERGE_PREPARATION;
    void finishPullConflictPreparation( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush,
                                        std::shared_ptr<MERGE_PREPARATION> aPreparation );
    void onPullDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError,
                     const wxString& aDetails, bool aAuthFailure,
                     const std::optional<KIGIT::HOST_KEY_FAILURE>& aHostKey, bool aRetriedAfterTrust, bool aThenPush,
                     bool aCancelled );

    /// Show the message field whenever a commit is possible, including a message-only amend.
    void updateMessageVisibility( bool aHasChanges );

    /// Set what the commit buttons say, and whether they are enabled, for the current state.
    void updateCommitButton();

    /// Show how many of the summary's 72 columns are left, once that is 10 or fewer.
    void updateSummaryCount();

    /// The noun for the operation in progress, for messages.
    wxString operationNoun() const;

    /// The label the network button carries while an operation is in progress.
    wxString abandonLabel() const;

    /// Register a refresh listener with the project's current git session, re-registering
    /// if the session changed since last time (it is recreated when a project is loaded).
    void syncListener();

    EDA_BASE_FRAME* m_frame;

    /// The widest row's minimum width, measured before anything is hidden.
    int m_minDockWidth = 0;

    /// True on the advanced window's Changes page, false in the docked panel.
    bool m_inAdvancedWindow = false;

    bool m_hasUnpushedCommits;

    bool m_hasRemote;

    /// Whether the last completed status snapshot had a repository.
    bool m_hasRepo = false;

    /// True when HEAD is a commit rather than a branch, so nothing can be committed or pushed.
    bool m_headDetached;

    /// True when the upstream has commits this project does not, so Get Changes has work to do.
    bool m_upstreamAhead;

    /// True while a merge or rebase is in progress. The network button then reads Abandon.
    bool m_operationInProgress;

    /// Which operation, so the button and the explanation can name it.
    GitOperation m_operation;

    /// True when an open editor held unsaved changes at the last refresh.
    bool m_editorsDirty;

    /// Whether the current branch allows an amend. A refresh must not change an active amend into a commit.
    bool m_canAmend = false;

    /// The branch and commit chosen when amend started. Keep them fixed while editing.
    std::pair<wxString, wxString> m_amendHead;

    /// The branch and commit the last refresh showed, so Undo takes back only the commit the user saw.
    std::pair<wxString, wxString> m_undoHead;

    /// The description box's hint, a label laid over it while it is empty. Null where the box
    /// shows its own.
    wxStaticText* m_descriptionHint = nullptr;

    /// Opens the History page from the docked panel. Never shown inside the advanced window.
    wxHyperlinkCtrl* m_historyLink = nullptr;

    /// The full messages behind the Recent messages dropdown, in its order after the caption.
    std::vector<wxString> m_recentMessageTexts;

    /// The arrow menu's entry, relabelled with the main button.
    wxMenuItem* m_commitAndPushItem = nullptr;

    /// True while the main button reads Push, with nothing left to commit.
    bool m_commitButtonPushes = false;


    /// The push or pull this panel submitted last.
    KIGIT_TRANSFER m_transfer;


    /// Told when a transfer starts and ends. Empty unless an owning window asked.
    std::function<void( bool )> m_busyChangedHandler;

    /// Unwrapped status text, kept so it can be re-wrapped on resize.
    wxString m_statusText;

    /// The width the status text was last wrapped to, so a layout re-wraps it only when that changes.
    int m_statusWrapWidth = -1;

    /// The controls' minimum height from the last layout, applied to the AUI pane at idle time.
    int       m_dockMinHeight = -1;
    wxWindow* m_dockParent = nullptr;
    bool      m_dockUpdatePending = false;

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

    /// Told when the message is edited. Empty unless an owning window asked.
    std::function<void()> m_messageChangedHandler;

    /// The branch a commit will land on, empty when detached or before the first commit.
    wxString m_currentBranch;

    std::function<void( const wxString& )> m_fileSelectedHandler;

    // The summary box of the wide layout. Null in the docked panel.
    wxStaticText* m_summaryFiles = nullptr;
    wxStaticText* m_summaryAdded = nullptr;
    wxStaticText* m_summaryRemoved = nullptr;

    /// Lines the working tree adds and removes against HEAD, read at the last refresh.
    std::pair<int, int> m_lineStats;

    /// Coalesce refreshes while a repository read is running, and discard stale results.
    KIGIT_QUERY m_refresh;
    KIGIT_QUERY m_actionQuery;
    bool        m_actionPending = false;
};

#endif // GIT_COMMIT_PANE_H
