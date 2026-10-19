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

#ifndef PANEL_GIT_HISTORY_H
#define PANEL_GIT_HISTORY_H

#include <map>
#include <set>
#include <string>
#include <atomic>
#include <thread>
#include <vector>

#include <wx/bitmap.h>
#include <wx/timer.h>

#include <dialogs/git/panel_git_history_base.h>
#include <dialogs/git/git_transfer.h>
#include <widgets/wx_debounced_action.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class GIT_FILE_DIFF_AREA;
struct KIGIT_BRANCH_INFO;
struct KIGIT_COMMIT_DETAIL;
struct KIGIT_COMMIT_INFO;
struct KIGIT_HISTORY_FILTER;
namespace KIGIT
{
struct GIT_IDENTITY;
}


/**
 * The advanced git window's History page: filters above a list of commits beside the selected
 * commit's detail, with what the commit did to a chosen file under the detail's file list.
 */
class PANEL_GIT_HISTORY : public PANEL_GIT_HISTORY_BASE
{
public:
    PANEL_GIT_HISTORY( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );
    ~PANEL_GIT_HISTORY() override;

    /// Load the repository's recent commits in the background, up to the current window.
    void Populate();

    /// Offer every branch in the branch filter, keeping the current choice.
    void SetBranches( const std::vector<KIGIT_BRANCH_INFO>& aBranches );

    /// Narrow the list to the commits that changed aPath, a file or folder relative to the
    /// repository root. Empty shows every commit again.
    void ShowHistoryForFile( const wxString& aPath );

private:
    /// Restart the history at the first window after the search or branch filter changed.
    void reloadHistory();

    /// Apply a loaded history window, keeping the currently selected commit when possible.
    void fillHistory( const std::vector<KIGIT_COMMIT_INFO>& aCommits, size_t aLimit );

    /// The search text and branch the page is narrowed to.
    KIGIT_HISTORY_FILTER historyFilter() const;

    /// Fill the detail pane with aHash, or clear it when the hash is empty or gone.
    void showCommitDetail( const wxString& aHash );
    void fillCommitDetail( const KIGIT_COMMIT_DETAIL& aDetail );
    void clearCommitDetail();

    /// Show what the selected commit did to aPath under the file list, or hide that area.
    void showFileDiff( const wxString& aPath );
    void hideFileDiff();

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

    /// Restore the project as it was at the given commit, recorded as a new commit on top of
    /// the current head.
    void onRestoreVersion( const wxString& aCommitHash );
    void restoreVersion( const wxString& aCommitHash, const KIGIT::GIT_IDENTITY& aIdentity );

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    wxDataViewColumn*   m_historyRefsColumn;
    GIT_FILE_DIFF_AREA* m_fileDiff;

    /// Set when the page goes, so a fetch still waiting on the network gives up rather than
    /// holding the close for its timeout.
    std::atomic<bool> m_avatarCancel{ false };

    /// The avatar fetches this page started, joined before it goes.
    std::vector<std::thread> m_avatarWorkers;

    /// Author email for each history row, so a fetched avatar can find its rows.
    std::vector<wxString> m_historyEmails;

    /// Whether each row's commit is the one HEAD points at. With every branch listed, the top
    /// row can belong to another branch, and only HEAD's commit can be amended.
    std::vector<bool> m_historyAtHead;

    /// Fetched web avatars, keyed by author email.
    std::map<wxString, wxBitmap> m_avatarCache;

    /// Emails whose avatar fetch was already started, so each is fetched at most once.
    std::set<wxString> m_avatarRequested;

    /// Listing and selection have separate requests so choosing a commit does not cancel paging.
    KIGIT_QUERY m_historyQuery;
    KIGIT_QUERY m_detailQuery;

    /// Read action preconditions without delaying history selection or scrolling.
    KIGIT_QUERY m_actionQuery;

    /// Waits for the user to pause typing in the search box before walking the history again.
    WX_DEBOUNCED_ACTION m_historySearchDebounce;

    /// Polls whether the user scrolled near the end of the history, which is the one portable
    /// way to notice scrolling on every wxDataViewCtrl port, native ones included.
    wxTimer m_historyScrollTimer;

    /// How many commits the page currently shows at most. Grows as the user scrolls.
    size_t m_historyLimit = 50;

    /// The branch the list is narrowed to, or empty for every branch.
    wxString m_historyBranchName;

    /// The file or folder the history is narrowed to, or empty for all of it.
    wxString m_historyPath;

    /// The commit the detail pane shows, so a refresh can put the selection back on it.
    wxString m_detailHash;

    /// Its author's email, so a web avatar that arrives later lands on the pane too.
    wxString m_detailEmail;

    /// The first parent of the shown commit, which the diff area's before side names.
    wxString m_detailParent;

    /// Where the user left the sash, so the pane comes back at the same width.
    int m_historySashPos = 0;

    /// Where the user left the sash between the file list and the diff area.
    int m_detailSashPos = 0;

    /// True once a load returned fewer commits than asked for, meaning everything is shown.
    bool m_historyComplete = false;

    /// True while the list is being refilled, when its selection events are its own.
    bool m_fillingHistory = false;
};

#endif // PANEL_GIT_HISTORY_H
