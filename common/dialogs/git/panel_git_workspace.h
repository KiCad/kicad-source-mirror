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

#ifndef PANEL_GIT_WORKSPACE_H
#define PANEL_GIT_WORKSPACE_H

#include <dialogs/git/panel_git_workspace_base.h>
#include <mdw/client/mdw_item.h>
#include <dialogs/git/workspace_detail.h>
#include <dialogs/git/workspace_search_query.h>
#include <dialogs/git/workspace_tree_model.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>

#include <wx/datetime.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class MDW_SESSION;
class wxButton;
class wxDataViewEvent;


/**
 * Every file the project's design workspace manages, who is editing it and its versions. The
 * advanced git window shows it as a page while the design workspace preference is on, and a
 * project without git shows it in its own window.
 */
class PANEL_GIT_WORKSPACE : public PANEL_GIT_WORKSPACE_BASE
{
public:
    /// aDialog is null outside the advanced git window, which hides the git-only actions.
    PANEL_GIT_WORKSPACE( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );
    ~PANEL_GIT_WORKSPACE();

    /// Read the workspace's folders, files and claims on the session's worker, then fill the tree.
    void Populate();

    /// Select and reveal aItemId once the tree holds it, now or after the next Populate().
    void SelectItem( const wxString& aItemId );

private:
    std::shared_ptr<MDW_SESSION> session() const;

    /// The folder path shown for aItem.
    wxString folderText( const MDW_ITEM_INFO& aItem ) const;

    /**
     * Rebuild the tree from m_folders and m_items, keeping the rows whose name or folder matches the search, the
     * expanded folders and the selection. aRefreshDetail false leaves the detail of a still selected item alone.
     */
    void fillTree( bool aRefreshDetail = true );

    /// The text of aNode's cell in aColumn, a WORKSPACE_TREE_MODEL::COLUMN.
    wxString cellText( const WORKSPACE_TREE_NODE& aNode, unsigned aColumn ) const;

    /// Show the State and Editing columns only when the bound backend offers lifecycle and locking.
    void showColumns();

    void rememberExpanded( const WORKSPACE_TREE_NODE& aNode );
    void restoreExpanded( const WORKSPACE_TREE_NODE& aNode );

    wxString bindingKey() const;

    /// Start over from an empty list when the project was bound elsewhere. True when it was.
    bool followBinding();

    void forgetListing();

    void setItemCount( const wxString& aText );

    /// Ask the source's search, when it offers one, or filter the listed rows.
    void onSearch();

    /// Show the provider's last status, its links and its stale greying over the shown rows.
    void applyQueryStatus();

    /// True when a reconnect has put a new source in place of the one m_query was made for.
    bool querySourceChanged() const;

    /// True while the project's session is open but cannot reach its backend.
    bool offline() const;

    /// Grey the shown rows and note when they were last good, for a session that went offline.
    void showOffline();

    /// Name the bound source in the empty search box when it searches on the server.
    void updateSearchHint();

    void showDetail( const MDW_ITEM_INFO& aItem );
    void hideDetail();

    /// Fill the detail rows and the action buttons from the selected item, its lock and its local changes.
    void showState();

    /// The selected item's actions, with a check out or check in still running also disabled.
    std::vector<WORKSPACE_ACTION> itemActions() const;

    /// Make the buttons match aActions, keeping the ones already shown when only their state changed.
    void showActions( const std::vector<WORKSPACE_ACTION>& aActions );

    /// Offer the actions of the item under the pointer, which becomes the selected one.
    void onContextMenu( wxDataViewEvent& aEvent );

    void runAction( WORKSPACE_ACTION::ID aId );

    /// Read the lifecycle states the selected item can move to, when the source has lifecycles.
    void readTransitions();

    /// The file the selected item is edited in, as recorded at check out or else as the source suggests.
    wxString selectedPath() const;

    /// Draw every cell again after a change the model was not told about, the cheapest way the platform allows.
    void refreshCells();

    /// Refresh every row's Editing cell and the detail after a lock changed.
    void onLockChanged();

    void onCapabilitiesChanged();

    /// Which of locking and versions the bound backend offers now, as bits.
    unsigned detailGates() const;

    wxString editingText( const wxString& aItemId ) const;

    void onEditing();

    /// Remove another user's claim on the selected item, after asking for the reason they are told.
    void onReleaseClaim();

    void onChangeState();
    void onOpen();

    /// The project manager window, which opens files as its tree does, or null when KiCad runs without one.
    EDA_BASE_FRAME* projectManager() const;

    void onShowInFolder();

    /// Check-out and check-in for workspaces whose files are not committed through git.
    void onCheckOut();
    void onCheckIn();

    /// Check out aItemId over aDestination, which the caller has confirmed may be replaced.
    void checkOut( const wxString& aItemId, const wxString& aDestination );

    /// Upload aRequest as a new revision of aItem, asking how to resolve a revision checked in since its base.
    void checkIn( const MDW_CHECKIN_REQUEST& aRequest, const MDW_ITEM_INFO& aItem );

    /// Offer Keep Both, Check In Anyway or Cancel for aRequest, refused because of aSince, newest first.
    /// Check In Anyway names aLatest, the item's latest revision, as its base.
    void resolveStaleCheckin( const MDW_CHECKIN_REQUEST& aRequest, const MDW_ITEM_INFO& aItem,
                              const std::vector<MDW_REVISION>& aSince, const wxString& aLatest );

    /// True when the source's files are edited in place and committed rather than checked out and in.
    bool editsInPlace() const;

    GIT_ADVANCED_DIALOG*         m_dialog;
    EDA_BASE_FRAME*              m_frame;
    std::vector<MDW_ITEM_INFO>   m_items;
    std::vector<MDW_FOLDER>      m_folders;           ///< Every folder of the last listing
    wxString                     m_selected;          ///< Item id in the detail pane
    MDW_ITEM_INFO                m_selectedItem;      ///< The item in the detail pane, as last listed
    wxString                     m_pendingSelect;     ///< SelectItem() target not yet listed
    std::uint64_t                m_listFetch = 0;     ///< Drops a reply superseded by a later Populate()
    bool                         m_listStale = false; ///< The listing failed or went offline; reread once online
    std::uint64_t                m_listReading = 0;   ///< m_listFetch of the read still out, 0 for none
    wxDateTime                   m_lastGood;          ///< When the shown rows last came from the server
    std::map<wxString, wxString> m_folderNames;       ///< Each folder's display path, by folder id
    std::vector<MDW_ITEM_INFO>   m_listing;           ///< The last listing read, kept while a search replaces it
    wxDateTime                   m_listingTime;
    std::weak_ptr<MDW_SESSION>   m_listingSession;
    wxString                     m_shownBinding; ///< bindingKey() the rows were read under
    unsigned                     m_detailGates = 0; ///< detailGates() when the detail was last built

    /// Held by the request's completion, so a request the worker discards also stops being pending
    std::weak_ptr<int>         m_checkOutPending;
    std::weak_ptr<int>         m_checkInPending;
    std::unique_ptr<WORKSPACE_SEARCH_QUERY> m_query;       ///< The source's search while the box holds text
    WORKSPACE_SEARCH_QUERY::STATUS          m_queryStatus; ///< Last status from m_query, kept across list rebuilds
    bool                       m_queryHasMore = false;
    std::weak_ptr<MDW_SOURCE>  m_querySource;
    int                        m_sashPos = 0;

    wxObjectDataPtr<WORKSPACE_TREE_MODEL> m_model;
    wxDataViewColumn*                     m_stateColumn = nullptr;
    wxDataViewColumn*                     m_editingColumn = nullptr;
    MDW_CAPABILITIES                      m_caps;                 ///< The bound backend's, as the columns show
    std::optional<std::set<WORKSPACE_TREE_KEY>> m_expanded;       ///< Folders open in the full tree, once seen
    bool                                  m_treeFiltered = false; ///< The tree shows search results

    std::vector<kiapi::workspace::LifecycleTransition> m_transitions; ///< The selected item's, empty until read
    MDW_OP                                             m_transitionsOp;
    bool                                               m_transitionsReading = false; ///< m_transitionsOp runs
    MDW_OP                                             m_transitionOp;
    bool                                               m_transitionPending = false; ///< m_transitionOp runs

    std::vector<std::pair<WORKSPACE_ACTION::ID, wxButton*>> m_actionButtons;

    MDW_SUBSCRIPTION           m_connections;   ///< Greys the rows offline and reads them again once online
    MDW_SUBSCRIPTION           m_events;        ///< Follows lock and capability changes after the listing

    /// Non-owning; a weak copy, safe to pass between threads, tells a late completion whether the panel is gone
    std::shared_ptr<PANEL_GIT_WORKSPACE> m_lifetime{ this, []( PANEL_GIT_WORKSPACE* ) {} };
};

#endif // PANEL_GIT_WORKSPACE_H
