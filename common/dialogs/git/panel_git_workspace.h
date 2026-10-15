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
#include <mdw/client/mdw_baseline_design.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>
#include <widgets/mdw_member_index.h>

#include <wx/datetime.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class MDW_SESSION;
struct WORKSPACE_WORKING_FILES;
class wxButton;
class wxDataViewEvent;
class wxStaticText;


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
    /// A team library of the project's library tables
    struct TEAM_LIBRARY
    {
        wxString         key; ///< "sym:<nickname>" or "fp:<nickname>", as the tree knows it
        MDW_LIBRARY_TYPE type = MDW_LIBRARY_TYPE::UNKNOWN;
        wxString         nickname;
        MDW_TEAM_LIBRARY binding;
        wxString         path;   ///< The row's expanded URI
        wxString         marker; ///< The row's library copy marker
        MDW_CAPABILITIES caps;   ///< The source's, as the rows show
    };

    /// A member the tree shows, as its library last listed it
    struct SHOWN_MEMBER
    {
        wxString         library; ///< The TEAM_LIBRARY::key of its library
        MDW_MEMBER_ENTRY entry;
    };

    /// The source and listing a member's detail and actions were built from
    struct MEMBER_TARGET
    {
        std::weak_ptr<MDW_SOURCE>      source;
        wxString                       libraryId;
        wxString                       snapshot;
        wxString                       localRoot; ///< The copy the member is read from
        wxString                       file;      ///< The member's file in that copy
        wxString                       memberId;
        wxString                       revisionId;
        std::set<MDW_CAPABILITY_GROUP> caps;

        bool operator==( const MEMBER_TARGET& aOther ) const
        {
            // Owner equality, so a source freed and another made at its address still differ
            return !source.owner_before( aOther.source ) && !aOther.source.owner_before( source )
                   && libraryId == aOther.libraryId && snapshot == aOther.snapshot && localRoot == aOther.localRoot
                   && file == aOther.file && memberId == aOther.memberId && revisionId == aOther.revisionId
                   && caps == aOther.caps;
        }
    };

    /// An action as it was offered and the selection it was offered for, checked again before it runs
    struct ACTION_REQUEST
    {
        std::weak_ptr<MDW_SOURCE>    source;
        wxString                     selected;
        WORKSPACE_ACTION::ID         id = WORKSPACE_ACTION::ID::OPEN;
        MDW_CLAIM_ACTION::KIND       claim = MDW_CLAIM_ACTION::KIND::START;
        std::optional<MEMBER_TARGET> member;     ///< The shown member's binding, as it resolves now
        wxString                     itemId;     ///< What the source names the selection
        wxString                     revisionId; ///< The revision the action names
    };

    /// The parts the saved design placed when Find last read it, shown until Find or a save of its root files
    struct DESIGN_PARTS
    {
        wxString                                     project; ///< The project's full name
        std::vector<std::filesystem::file_time_type> savedAt; ///< MdwDesignWriteTimes() before the read
        std::vector<MDW_PLACED_INPUT>                inputs;
        wxString                                     unread; ///< Why the design could not be read, or empty
    };

    std::shared_ptr<MDW_SESSION> session() const;

    /// Find the project's team libraries again, as after a source connects or the tables change.
    void refreshTeamLibraries();

    /// Append the team libraries of the project's library tables to m_teamLibraries.
    void findTeamLibraries();

    /// Start listing the library under aKey once it is expanded; the tree fills when the listing lands.
    void listLibrary( const wxString& aKey );

    void onMembersChanged( const MDW_MEMBER_CHANGES& aChanges );

    /// The team libraries as the tree shows them, each expanded one with its members, which go into m_shownMembers.
    std::vector<WORKSPACE_TREE_LIBRARY> shownLibraries();

    /// Rebuild the tree once, after the event in progress, for a listing that was already there.
    void queueFill();

    void showMemberDetail( const WORKSPACE_TREE_NODE& aNode );

    /// The source and reference of the selected item or member, which its versions and states come from.
    std::shared_ptr<MDW_SOURCE> selectedSource() const;
    MDW_ITEM_REF                selectedRef() const;

    /// The revision the detail shows, whose states are offered and which a state change names.
    wxString selectedRevision() const;

    /// The team library under aKey, or null once it is no longer one.
    const TEAM_LIBRARY* teamLibrary( const wxString& aKey ) const;

    /// The library of the member in the detail pane, or null for none.
    const TEAM_LIBRARY* memberLibrary() const;

    /// aMember as its library binds it now, or nothing once the library is gone.
    std::optional<MEMBER_TARGET> memberTarget( const SHOWN_MEMBER& aMember ) const;

    /// The shown member's target as its library resolves now, or nothing for a design item.
    std::optional<MEMBER_TARGET> shownMemberTarget() const;

    /// The file the selected member is saved in, as its editor resolves it.
    wxString memberFile() const;

    /// True when a project with a schematic or board is open and the host can load them through the kifaces.
    bool canReadDesign() const;

    /// True when m_designParts was read from the project's design files as they are now.
    bool designPartsCurrent() const;

    /// The Where used value for the selected member, empty until the design's current files are read.
    wxString whereUsedText() const;

    /// Read which parts the saved design places, every sheet of it again.
    void findWhereUsed();

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

    /// Open every row below aNode that shows search results, leaving the libraries not listed yet closed.
    void expandFiltered( const WORKSPACE_TREE_NODE& aNode );

    void rememberExpanded( const WORKSPACE_TREE_NODE& aNode );
    void restoreExpanded( const WORKSPACE_TREE_NODE& aNode );

    wxString bindingKey() const;

    /// Start over from an empty list when the project was bound elsewhere. True when it was.
    bool followBinding();

    void forgetListing();

    /// Drop the listing read still out, stopping its walk, and answer the generation of the next one
    std::uint64_t nextListFetch();

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

    /// Wrap m_actionRefusals to the detail pane's width below the action buttons.
    void wrapActionRefusals();

    /// Offer the actions of the item under the pointer, which becomes the selected one.
    void onContextMenu( wxDataViewEvent& aEvent );

    /// aAction as offered for the selection now, to run after the click or menu that chose it.
    ACTION_REQUEST actionRequest( const WORKSPACE_ACTION& aAction ) const;

    /// Run aRequest unless the selection, its source or the command offered for it has changed since.
    void runAction( const ACTION_REQUEST& aRequest );

    /// Read the lifecycle states the selected item can move to, and its permission to move, when the source has
    /// lifecycles.
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

    /// What the Editing column says of aLock.
    static wxString lockText( const MDW_LOCK_INFO& aLock );

    /// Take the claim on the selected item with aStart, or give up our own.
    void onEditing( bool aStart );

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

    /// The files the user edits for aItem, which Save Copy must never replace.
    WORKSPACE_WORKING_FILES workingFiles( const MDW_ITEM_INFO& aItem );

    /**
     * The working tree of the repository holding the project, or empty when none holds it. It is also empty until
     * the first call's search off the UI thread answers, which then updates the shown item's working files.
     */
    wxString projectWorktree();

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

    std::vector<TEAM_LIBRARY>                   m_teamLibraries;
    std::set<wxString>                          m_librariesListed; ///< Keys of libraries expanded, so listed
    std::map<wxString, SHOWN_MEMBER>            m_shownMembers;    ///< By tree key
    std::optional<SHOWN_MEMBER>                 m_member;          ///< The member in the detail pane
    std::optional<MEMBER_TARGET>                m_memberTarget;    ///< What m_member's detail was built from
    std::shared_ptr<MDW_MEMBER_INDEX::LISTENER> m_memberListener;
    bool                                        m_fillQueued = false;
    std::optional<DESIGN_PARTS>                 m_designParts;
    bool                                        m_readingDesign = false; ///< findWhereUsed() runs

    /// Set once the listing read still out is no longer wanted, which stops its walk
    std::shared_ptr<std::atomic<bool>> m_listCancel;

    /// Held by the request's completion, so a request the worker discards also stops being pending
    std::weak_ptr<int>         m_checkOutPending;
    std::weak_ptr<int>         m_checkInPending;
    std::unique_ptr<WORKSPACE_SEARCH_QUERY> m_query;       ///< The source's search while the box holds text
    WORKSPACE_SEARCH_QUERY::STATUS          m_queryStatus; ///< Last status from m_query, kept across list rebuilds
    bool                       m_queryHasMore = false;
    std::weak_ptr<MDW_SOURCE>  m_querySource;
    int                        m_sashPos = 0; ///< In DIPs, so the tree keeps its width across a DPI change
    std::optional<wxString>    m_worktree;    ///< From projectWorktree(), empty for no repository, unset until found
    wxString                   m_worktreeFor; ///< The project folder m_worktree is found for
    std::weak_ptr<MDW_SESSION> m_worktreeSession; ///< Runs the lookup for m_worktreeFor until it answers

    wxObjectDataPtr<WORKSPACE_TREE_MODEL> m_model;
    wxDataViewColumn*                     m_nameColumn = nullptr;
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
    kiapi::workspace::Permission                       m_transitionPermission; ///< The selection's, unknown until read
    MDW_OP                                             m_permissionsOp;
    bool                                               m_permissionsReading = false; ///< m_permissionsOp runs

    std::vector<std::pair<WORKSPACE_ACTION, wxButton*>> m_actionButtons; ///< Each button and the action it shows
    wxStaticText* m_actionRefusalText = nullptr;
    wxString      m_actionRefusals; ///< Each disabled action with its reason, one per line, before wrapping

    MDW_SUBSCRIPTION           m_connections;   ///< Greys the rows offline and reads them again once online
    MDW_SUBSCRIPTION           m_events;        ///< Follows lock and capability changes after the listing

    /// Non-owning; a weak copy, safe to pass between threads, tells a late completion whether the panel is gone
    std::shared_ptr<PANEL_GIT_WORKSPACE> m_lifetime{ this, []( PANEL_GIT_WORKSPACE* ) {} };
};

#endif // PANEL_GIT_WORKSPACE_H
