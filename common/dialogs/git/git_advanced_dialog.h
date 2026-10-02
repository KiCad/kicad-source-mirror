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
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <thread>
#include <vector>

#include <wx/bitmap.h>
#include <wx/timer.h>

#include <dialog_shim.h>

class EDA_BASE_FRAME;
#include <git/git_editor_snapshot.h>

enum class BITMAPS : unsigned int;
class GIT_COMMIT_PANE;
class KIGIT_COMMON;
struct KIGIT_COMMIT_DETAIL;
struct KIGIT_COMMIT_INFO;
struct KIGIT_HISTORY_FILTER;
class WX_TREEBOOK;
class wxDataViewColumn;
class wxDataViewListCtrl;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxListCtrl;
class wxPanel;
class wxSearchCtrl;
class wxSplitterWindow;
class wxStaticBitmap;
class wxStaticText;
class wxTextCtrl;


/**
 * The advanced Git window: a non-modal dialog with a top bar naming the repository and branch
 * and a sidebar of pages (History, Branches, Remotes, Stashes, Tags, Settings) over the live
 * repository. Committing, pushing and pulling stay in the docked Version Control panel. Branch,
 * tag, remote, stash and identity management happen here.
 */
class GIT_ADVANCED_DIALOG : public DIALOG_SHIM
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

    /// Close the live window, for when the project it was filled from goes away.
    static void CloseActive();

    /// Close the live window if the given pane opened it, for when that pane goes away.
    static void CloseForPane( GIT_COMMIT_PANE* aPane );

    void ReloadPages();

private:
    /// The History page: filters above a list of commits beside the selected commit's detail.
    wxWindow* buildHistoryPage( wxWindow* aBook );

    /// Fill the History list from the repository's recent commits, up to the current window.
    void populateHistory();

    /// Restart the history at the first window after the search or branch filter changed.
    void reloadHistory();

    /// Offer every branch in the History page's branch filter, keeping the current choice.
    void populateHistoryBranches( const std::vector<std::pair<wxString, bool>>& aBranches );

    /// The search text and branch the History page is narrowed to.
    KIGIT_HISTORY_FILTER historyFilter() const;

    /// Fill the detail pane with aHash, or clear it when the hash is empty or gone.
    void showCommitDetail( const wxString& aHash );
    void fillCommitDetail( const KIGIT_COMMIT_DETAIL& aDetail );
    void clearCommitDetail();

    /// Append rows for aCommits starting at aFrom, without clearing, so growing the history
    /// window keeps the scroll position.
    void appendHistoryRows( const std::vector<KIGIT_COMMIT_INFO>& aCommits, size_t aFrom );

    /// Grow the history window when the user has scrolled near its end.
    void onHistoryScrollTick();

    /// Start a background fetch of the contributor's web avatar for aEmail, if opted in and not
    /// already cached or requested.
    void requestAvatar( const wxString& aEmail, int aDiameter );

    /// On the UI thread, decode the fetched avatar bytes and place the photo on every history
    /// row for aEmail. Does nothing when the bytes are empty (no web avatar).
    void applyAvatar( const wxString& aEmail, int aDiameter, const std::string& aBytes );

    void populateBranches();

    void populateRemotes();

    void populateStashes();

    void populateTags();

    /// Fill the Repository page from the repository's git configuration.
    void populateRepository();

    /// Switch the working tree to the named branch, after confirming, then refresh.
    void switchToBranch( const wxString& aBranch );

    /// Amend the newest, not yet pushed commit's message from the History page.
    void onAmendLastCommit();

    /// Force push the current branch from the Branches page, after an explicit confirmation.
    void onForcePush();

    /// Fill the top bar's repository name and branch list.
    void populateTopBar();

    /// Fetch from the remote on a worker, then refresh every surface.
    void onFetch();

    /// Make a top bar button with its icon.
    wxButton* makeBarButton( BITMAPS aIcon, const wxString& aLabel, const std::function<void()>& aHandler );

    /// Restore the project as it was at the given commit, recorded as a new commit on top of
    /// the current head. Offered from the History page's context menu.
    void onRestoreVersion( const wxString& aCommitHash );

    /// True while the frame's git session is still the one this window's lists were filled
    /// from. On a mismatch the window is stale: it closes itself and the action must abort.
    bool sessionStillCurrent();

    /// Before an action that rewrites files on disk, save the open editors and snapshot their
    /// documents. Returns false to abort the action.
    bool confirmWorkingTreeChange();

    /// Before an action that needs a clean tree, refuse while anything is uncommitted or unsaved,
    /// since saving the editors would only create the changes that block it. False to abort.
    bool refuseOverUncommittedWork();

    /// Reload the editors from disk and refresh the panels, after an operation rewrote files.
    void finishWorkingTreeChange();

    /// On-disk identity of the open documents, taken by confirmWorkingTreeChange() so only the
    /// documents an operation actually rewrote are reloaded.
    EDITOR_SNAPSHOT m_docsBeforeChange;

    void addButtonRow( wxWindow* aPage, const std::vector<std::pair<wxString, std::function<void()>>>& aButtons );

    /// Run a repository write under the action lock, then refresh every surface on success.
    /// Shows a busy message if the lock is held, or aError if the action returns false. Set
    /// aRewritesWorkingTree for operations that change the files the editors have open, so they
    /// are reloaded from disk too.
    void runGitAction( const std::function<bool( KIGIT_COMMON& )>& aFn, const wxString& aError,
                       bool aRewritesWorkingTree = false );

    /// The text of the selected row in a list, or empty when nothing is selected.
    wxString selectedText( wxListCtrl* aList, int aColumn ) const;

    void onNewBranch();
    void onDeleteBranch();
    void onStashSave();
    void runRemoteAction( const std::function<bool( KIGIT_COMMON& )>& aMutation, const wxString& aError );
    void restoreSelectedStash( bool aRemoveAfterApply );

    /// The stash shown on aRow, by its commit, or empty when the row is gone.
    wxString stashIdForRow( long aRow ) const;
    void onStashApply();
    void onStashPop();
    void onStashDrop();
    void onNewTag();
    void onCheckoutTag();
    void onDeleteTag();
    void onAddRemote();
    void onEditRemote();
    void onRemoveRemote();
    void onSaveRepositoryConfig();

    /// Set when the window goes, so a fetch still waiting on the network gives up rather
    /// than holding the close for its timeout.
    std::atomic<bool> m_avatarCancel{ false };

    /// The avatar fetches the History page started, joined before this window goes.
    std::vector<std::thread> m_avatarWorkers;

    /// The push the Branches page started, and its token, so closing can stop it and wait.
    std::thread                        m_pushWorker;
    std::shared_ptr<std::atomic<bool>> m_pushCancel;

    /// The fetch the top bar started, and its token, so closing can stop it and wait.
    std::thread                        m_fetchWorker;
    std::shared_ptr<std::atomic<bool>> m_fetchCancel;

    EDA_BASE_FRAME*     m_parent;
    GIT_COMMIT_PANE*    m_pane;
    WX_TREEBOOK*        m_book;
    wxDataViewListCtrl* m_historyList;
    wxDataViewColumn*   m_historyRefsColumn;
    wxSplitterWindow*   m_historySplitter;
    wxPanel*            m_commitDetail;
    wxSearchCtrl*       m_historySearch;
    wxChoice*           m_historyBranch;
    wxCheckBox*         m_historyShowRefs;
    wxStaticText*       m_commitTitle;
    wxStaticBitmap*     m_commitAvatar;
    wxStaticText*       m_commitAuthor;
    wxTextCtrl*         m_commitMessage;
    wxStaticText*       m_commitStats;
    wxListCtrl*         m_commitFiles;
    wxListCtrl*         m_branchesList;
    wxListCtrl*         m_remotesList;
    wxListCtrl*         m_stashesList;
    wxListCtrl*         m_tagsList;
    wxTextCtrl*         m_repoUserName;
    wxTextCtrl*         m_repoUserEmail;
    wxStaticText*       m_repoRemoteUrl;
    wxStaticText*       m_repoName;
    wxChoice*           m_branchChoice;
    wxButton*           m_fetchButton;
    wxButton*           m_pullButton;
    wxButton*           m_pushButton;

    /// Move the refresh listener onto the project's current git session, in case a project
    /// reload has replaced it since the window opened.
    void syncListener();

    /// The git session this window is currently listening to for refreshes. Weak so a project
    /// reload can replace the session and this window follows it instead of pinning the old one.
    std::weak_ptr<KIGIT_COMMON> m_listenerSession;

    /// The stash commit behind each row of the Stashes page, since a row's index is only valid
    /// for the listing it came from.
    std::vector<wxString> m_stashIds;

    /// Author email for each history row, so a fetched avatar can find its rows.
    std::vector<wxString> m_historyEmails;

    /// Fetched web avatars, keyed by author email.
    std::map<wxString, wxBitmap> m_avatarCache;

    /// Emails whose avatar fetch was already started, so each is fetched at most once.
    std::set<wxString> m_avatarRequested;

    /// Re-runs ReloadPages once when it had to skip because another git operation held the
    /// action lock, so a window opened during a background fetch does not stay empty.
    wxTimer m_reloadRetryTimer;

    /// How many commits the History page currently shows at most. Grows as the user scrolls.
    size_t m_historyLimit = 50;

    /// The branch the History page is narrowed to, or empty for every branch.
    wxString m_historyBranchName;

    /// The commit the detail pane shows, so a refresh can put the selection back on it.
    wxString m_detailHash;

    /// Its author's email, so a web avatar that arrives later lands on the pane too.
    wxString m_detailEmail;

    /// Where the user left the sash, so the pane comes back at the same width.
    int m_historySashPos;

    /// Waits for the user to pause typing in the search box before walking the history again.
    wxTimer m_historySearchTimer;

    /// True once a load returned fewer commits than asked for, meaning everything is shown.
    bool m_historyComplete = false;

    /// True while the history list is being refilled, when its selection events are its own.
    bool m_fillingHistory = false;

    /// Polls whether the user scrolled near the end of the history, which is the one portable
    /// way to notice scrolling on every wxDataViewCtrl port, native ones included.
    wxTimer m_historyScrollTimer;
};

#endif // GIT_ADVANCED_DIALOG_H
