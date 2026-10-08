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

#include <dialogs/git/panel_git_workspace.h>

#include <dialogs/dialog_library_source_setup.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_ui_utils.h>
#include <dialogs/git/panel_mdw_versions.h>
#include <dialogs/git/workspace_detail.h>
#include <dialogs/git/workspace_item_files.h>
#include <dialogs/git/workspace_search_query.h>
#include <dialogs/git/workspace_tree.h>
#include <confirm.h>
#include <kidialog.h>
#include <kiway.h>
#include <eda_base_frame.h>
#include <launch_ext.h>
#include <mdw/mdw_claim_text.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_lock_tracker.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_part.h>
#include <mdw/client/mdw_project_file.h>
#include <mdw/client/mdw_revision.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_session.h>
#include <mdw/mdw_workspace_service.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <widgets/mdw_editing.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/wx_dataviewctrl.h>
#include <wildcards_and_files_ext.h>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/textdlg.h>
#include <wx/weakref.h>
#include <wx/datetime.h>
#include <wx/intl.h>

#include <algorithm>
#include <deque>
#include <tuple>
#include <utility>


namespace
{
struct LISTING
{
    MDW_RESULT                   status = MDW_RESULT::Ok();
    std::vector<MDW_ITEM_INFO>   items;
    std::vector<MDW_FOLDER>      folders;
    std::map<wxString, wxString> folderNames;
};


size_t countItems( const WORKSPACE_TREE_NODE& aNode )
{
    size_t count = aNode.kind == WORKSPACE_TREE_NODE::KIND::ITEM ? 1 : 0;

    for( const WORKSPACE_TREE_NODE& child : aNode.children )
        count += countItems( child );

    return count;
}


struct CHECKOUT
{
    MDW_RESULT          status = MDW_RESULT::Ok();
    MDW_CHECKOUT_RESULT result;
};


struct CHECKIN
{
    MDW_RESULT                status = MDW_RESULT::Ok();
    MDW_REVISION              revision;
    std::vector<MDW_REVISION> since;  ///< On a stale base, the revisions checked in after it, newest first
    wxString                  latest; ///< On a stale base, the item's latest revision
};
}


PANEL_GIT_WORKSPACE::PANEL_GIT_WORKSPACE( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_WORKSPACE_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_shownBinding = bindingKey();
    updateSearchHint();

    m_model = new WORKSPACE_TREE_MODEL();
    m_model->SetCellText(
            [this]( const WORKSPACE_TREE_NODE& aNode, unsigned aColumn )
            {
                return cellText( aNode, aColumn );
            } );
    m_itemsTree->AssociateModel( m_model.get() );

    m_itemsTree->AppendTextColumn( _( "Name" ), WORKSPACE_TREE_MODEL::COL_NAME, wxDATAVIEW_CELL_INERT, FromDIP( 200 ),
                                   wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE );
    m_stateColumn = m_itemsTree->AppendTextColumn( _( "State" ), WORKSPACE_TREE_MODEL::COL_STATE, wxDATAVIEW_CELL_INERT,
                                                   FromDIP( 130 ), wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE );
    m_editingColumn =
            m_itemsTree->AppendTextColumn( _( "Editing" ), WORKSPACE_TREE_MODEL::COL_EDITING, wxDATAVIEW_CELL_INERT,
                                           FromDIP( 100 ), wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE );
    m_itemsTree->AppendTextColumn( _( "Last Change" ), WORKSPACE_TREE_MODEL::COL_LAST_CHANGE, wxDATAVIEW_CELL_INERT,
                                   FromDIP( 110 ), wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE );
    showColumns();

    m_versions->EnablePreview( m_frame );
    m_versions->SetModifiedChanged(
            [this]()
            {
                showState();
            } );

    KIGIT::KeepDetailWidth( m_workspaceSplitter );

    // The detail pane appears once a file is selected
    m_sashPos = FromDIP( 560 );
    m_workspaceSplitter->Unsplit( m_itemDetail );

    m_search->Bind( wxEVT_TEXT,
                    [this]( wxCommandEvent& )
                    {
                        onSearch();
                    } );

    m_moreLink->Bind( wxEVT_HYPERLINK,
                      [this]( wxHyperlinkEvent& )
                      {
                          if( querySourceChanged() )
                              onSearch();
                          else if( m_query && !m_queryStatus.searching )
                              m_query->More();
                      } );

    m_connections = MDW_CLIENT::Get().SubscribeConnections(
            this,
            [this]( const wxString& aProjectPath )
            {
                PROJECT& project = m_frame->Prj();

                if( aProjectPath != project.GetProjectFullName() )
                    return;

                if( followBinding() )
                    return;

                MDW_CONNECTION state = MDW_CLIENT::Get().ProjectConnection( project ).status.phase;

                // A reconnect may bring a source that searches differently
                updateSearchHint();

                // Every OFFLINE notice greys again, so a refresh started by a brief reconnect cannot outlive it
                if( state == MDW_CONNECTION::OFFLINE )
                    showOffline();
                else if( m_query && WorkspaceSearchRefreshes( m_queryStatus, state ) )
                    onSearch();
                else if( !m_query && state == MDW_CONNECTION::ONLINE && m_listStale && m_listReading != m_listFetch )
                {
                    // Text typed while the source could not search becomes a search once it can
                    if( !m_search->GetValue().IsEmpty() )
                        onSearch();

                    if( !m_query )
                        Populate();
                }
            } );

    // The tracker applies each event before delivery, so it already holds this lock
    m_events = MDW_CLIENT::Get().Subscribe(
            this,
            [this]( const MDW_EVENT& aEvent )
            {
                // Library sources share this bus, and their items are not listed here
                std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

                if( !source || aEvent.source != source->Info().locator )
                    return;

                if( aEvent.type == MDW_EVENT_TYPE::ITEM_LOCKED || aEvent.type == MDW_EVENT_TYPE::ITEM_UNLOCKED
                    || aEvent.type == MDW_EVENT_TYPE::LOCKS_CHANGED )
                {
                    onLockChanged();
                }
                else if( aEvent.type == MDW_EVENT_TYPE::REVISION_RELEASED )
                {
                    // The State column reads the listing, so a release needs it read again
                    Populate();
                }
                else if( aEvent.type == MDW_EVENT_TYPE::CAPABILITIES_CHANGED )
                {
                    onCapabilitiesChanged();
                }
            } );

    m_retryLink->Bind( wxEVT_HYPERLINK,
                       [this]( wxHyperlinkEvent& )
                       {
                           if( querySourceChanged() )
                               onSearch();
                           else if( m_query )
                               m_query->Retry();
                       } );

    m_itemsTree->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED,
                       [this]( wxDataViewEvent& )
                       {
                           const WORKSPACE_TREE_NODE* node = m_model->NodeOf( m_itemsTree->GetSelection() );

                           if( node && node->kind == WORKSPACE_TREE_NODE::KIND::ITEM && node->item < m_items.size() )
                               showDetail( m_items[node->item] );
                           else
                               hideDetail();
                       } );

    m_itemsTree->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                       [this]( wxDataViewEvent& aEvent )
                       {
                           onContextMenu( aEvent );
                       } );

    m_connectLibraryButton->Bind( wxEVT_BUTTON,
                                  [this]( wxCommandEvent& )
                                  {
                                      SetUpLibrarySource( m_frame, m_frame->Kiway(), &m_frame->Prj() );
                                  } );
}


PANEL_GIT_WORKSPACE::~PANEL_GIT_WORKSPACE()
{
    // The control outlives this panel's members during teardown, and its cells call back into them
    m_itemsTree->AssociateModel( nullptr );
}


wxString PANEL_GIT_WORKSPACE::bindingKey() const
{
    const MDW_PROJECT_BINDING binding = MDW_CLIENT::ProjectBinding( m_frame->Prj() );

    return binding.backend_id + wxS( "\n" ) + binding.workspace_id + wxS( "\n" ) + binding.binding;
}


bool PANEL_GIT_WORKSPACE::followBinding()
{
    wxString binding = bindingKey();

    if( binding == m_shownBinding )
        return false;

    m_shownBinding = binding;

    // Reads, rows and the selection of the previous workspace would reach the new one's session
    ++m_listFetch;
    m_query.reset();
    m_queryStatus = WORKSPACE_SEARCH_QUERY::STATUS();
    m_queryHasMore = false;
    m_items.clear();
    m_pendingSelect.clear();
    m_lastGood = wxDateTime();
    m_expanded.reset();

    // An old tree left in the model would be snapshotted and reselected as if it were the new workspace's
    m_model->Reset( WORKSPACE_TREE_NODE() );

    forgetListing();
    hideDetail();
    fillTree();

    if( m_search->GetValue().IsEmpty() )
        Populate();
    else
        onSearch();

    return true;
}


void PANEL_GIT_WORKSPACE::onSearch()
{
    const wxString text = m_search->GetValue();

    if( text.IsEmpty() )
    {
        // The provider's rows replaced the listing, so the last one read stands in until a fresh read
        bool searched = std::exchange( m_query, nullptr ) != nullptr;

        if( searched )
        {
            // A listing from before a rebind belongs to another workspace
            bool sameSession = !m_listingSession.expired() && m_listingSession.lock() == session();

            m_items = sameSession ? m_listing : std::vector<MDW_ITEM_INFO>();
            m_lastGood = sameSession ? m_listingTime : wxDateTime();

            if( !sameSession )
            {
                m_folders.clear();
                m_folderNames.clear();
            }
        }

        m_queryStatus = WORKSPACE_SEARCH_QUERY::STATUS();
        m_queryHasMore = false;
        applyQueryStatus();

        fillTree();

        if( searched )
            Populate();

        return;
    }

    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    // A reconnect replaces the source, and a query on the old one would only ever report it lost
    bool replaced = false;

    if( m_query && source && source != m_querySource.lock() )
    {
        // Its rows and status describe the old query, so they go with it
        m_query.reset();
        m_items.clear();
        m_queryStatus = WORKSPACE_SEARCH_QUERY::STATUS();
        m_queryHasMore = false;
        fillTree();
        applyQueryStatus();
        replaced = true;
    }

    if( !m_query && source )
    {
        m_query = WORKSPACE_SEARCH_QUERY::Create( source );
        m_querySource = source;

        if( m_query )
            m_lastGood = wxDateTime();
    }

    if( !m_query )
    {
        // The rows went with the dropped query, so a source that declines search needs the full listing
        if( replaced )
            Populate();
        else
            fillTree();

        return;
    }

    ++m_listFetch;

    MDW_SEARCH_QUERY query;
    query.text = text;

    m_query->Begin( query,
                    [this]( const std::vector<MDW_ITEM_INFO>& aRows, bool aFirstPage, bool aHasMore,
                            const WORKSPACE_SEARCH_QUERY::STATUS& aStatus )
                    {
                        // A replacing page clears the list even with an error, so denied rows do not linger
                        if( aFirstPage )
                            m_items.clear();

                        m_items.insert( m_items.end(), aRows.begin(), aRows.end() );
                        m_queryStatus = aStatus;
                        m_queryHasMore = aHasMore;

                        if( aStatus.result.IsOk() && !aStatus.searching )
                            m_lastGood = wxDateTime::Now();

                        if( aFirstPage || !aRows.empty() )
                        {
                            // Reselecting the open row would reload its detail and drop unsaved edits
                            fillTree( aFirstPage );
                        }
                        else
                        {
                            applyQueryStatus();
                        }
                    } );

    // Typing while offline would only send a request known to fail
    if( offline() )
        showOffline();
}


bool PANEL_GIT_WORKSPACE::querySourceChanged() const
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    // Without a replacement the old query stays, and reports itself offline
    return m_query && source && source != m_querySource.lock();
}


void PANEL_GIT_WORKSPACE::applyQueryStatus()
{
    m_moreLink->Show( m_query && m_queryHasMore );
    m_moreLink->Enable( !m_queryStatus.searching );
    m_retryLink->Show( m_query && m_queryStatus.can_retry );

    if( m_query )
        m_itemCount->SetLabel( m_queryStatus.text );

    m_itemListPanel->Layout();

    // An update without rows still has to clear the grey a stale one left
    if( m_query )
        m_model->SetGreyed( m_queryStatus.stale );
}


bool PANEL_GIT_WORKSPACE::offline() const
{
    return MDW_CLIENT::Get().ProjectConnection( m_frame->Prj() ).status.phase == MDW_CONNECTION::OFFLINE;
}


void PANEL_GIT_WORKSPACE::showOffline()
{
    WORKSPACE_SEARCH_QUERY::STATUS offline = WorkspaceOfflineStatus( m_queryStatus, !m_items.empty(), m_lastGood );

    if( m_query )
    {
        // A request or retry still pending would only fail, or show live rows while offline
        m_query->Cancel();
        m_queryStatus = offline;
        m_queryHasMore = false;
        applyQueryStatus();
        return;
    }

    // A read still waiting on the lost backend would only fail later
    ++m_listFetch;
    m_listStale = true;
    setItemCount( offline.text );
    m_model->SetGreyed( true );
}


void PANEL_GIT_WORKSPACE::updateSearchHint()
{
    m_search->SetHint( WORKSPACE_SEARCH_QUERY::Offers( MDW_CLIENT::Get().ProjectSource() )
                               ? WORKSPACE_SEARCH_QUERY::Hint()
                               : _( "Search files" ) );
}


std::shared_ptr<MDW_SESSION> PANEL_GIT_WORKSPACE::session() const
{
    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();

    if( !framework )
        return nullptr;

    return framework->Session( m_frame->Prj().GetProjectFullName() );
}


void PANEL_GIT_WORKSPACE::setItemCount( const wxString& aText )
{
    m_itemCount->SetLabel( aText );
    m_itemCount->SetToolTip( wxString() );

    // The label keeps the width of its previous text until the column lays out again
    m_itemListPanel->Layout();
}


void PANEL_GIT_WORKSPACE::Populate()
{
    updateSearchHint();

    // The full listing would replace the search results, so the search runs again instead
    if( m_query )
    {
        onSearch();
        return;
    }

    std::shared_ptr<MDW_SESSION> current = session();
    std::uint64_t                fetch = ++m_listFetch;

    if( !current )
    {
        // Read again once a session comes online
        m_listStale = true;
        m_lastGood = wxDateTime();
        m_items.clear();
        forgetListing();
        fillTree();
        setItemCount( _( "This project is not connected to a design workspace." ) );
        return;
    }

    if( offline() )
    {
        showOffline();
        return;
    }

    setItemCount( _( "Reading the workspace" ) + wxS( "\u2026" ) );
    m_listReading = fetch;

    current->RunAsync<LISTING>(
            this,
            [source = MDW_CLIENT::Get().ProjectSource(), weak = std::weak_ptr<MDW_SESSION>( current )]()
            {
                LISTING listing;

                if( !source )
                {
                    listing.status = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
                    return listing;
                }

                auto readFolders = [&]( const wxString& aParent, std::vector<MDW_FOLDER>& aOut )
                {
                    auto [result, folders] = MdwSync<MDW_PAGED<MDW_FOLDER>>(
                            [&]( MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone )
                            {
                                return MdwReadAllPages<MDW_FOLDER>(
                                        [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_FOLDER>> aPageDone )
                                        {
                                            return source->Folders( aParent, aPage, std::move( aPageDone ) );
                                        },
                                        0, std::move( aDone ) );
                            } );

                    aOut.insert( aOut.end(), folders.rows.begin(), folders.rows.end() );
                    return result;
                };

                std::vector<MDW_FOLDER> roots;
                listing.status = readFolders( wxEmptyString, roots );

                std::deque<wxString> folders;

                // The walk knows each folder's parent, whether or not the backend filled it in
                for( MDW_FOLDER& folder : roots )
                {
                    folder.parent_folder_id.clear();
                    listing.folderNames[folder.folder_id] = WorkspaceFolderName( wxEmptyString, folder );
                    folders.push_back( folder.folder_id );
                    listing.folders.push_back( std::move( folder ) );
                }

                while( listing.status.IsOk() && !folders.empty() )
                {
                    wxString                folder = folders.front();
                    std::vector<MDW_FOLDER> children;

                    folders.pop_front();

                    auto [status, items] = MdwSync<MDW_PAGED<MDW_ITEM_INFO>>(
                            [&]( MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone )
                            {
                                return source->Items( folder, wxEmptyString, MDW_PAGE(), std::move( aDone ) );
                            } );

                    listing.status = status;

                    if( listing.status.IsOk() )
                        listing.status = readFolders( folder, children );

                    for( MDW_ITEM_INFO& item : items.rows )
                    {
                        if( !item.is_folder )
                            listing.items.push_back( std::move( item ) );
                    }

                    for( MDW_FOLDER& child : children )
                    {
                        child.parent_folder_id = folder;
                        listing.folderNames[child.folder_id] =
                                WorkspaceFolderName( listing.folderNames[folder], child );
                        folders.push_back( child.folder_id );
                        listing.folders.push_back( std::move( child ) );
                    }
                }

                // Claims are shown when the backend has them, but a list without them is still useful
                if( std::shared_ptr<MDW_SESSION> live = weak.lock();
                    live && listing.status.IsOk() && source->Has( MDW_CAPABILITY_GROUP::LOCKING ) )
                {
                    live->ListLocks();
                }

                return listing;
            },
            [this, fetch, weak = std::weak_ptr<MDW_SESSION>( current )]( LISTING aListing )
            {
                if( fetch != m_listFetch )
                    return;

                m_listReading = 0;

                if( WorkspaceSearchOffline( aListing.status ) )
                {
                    // The rows already shown are still the best answer, greyed
                    showOffline();
                    return;
                }

                if( !aListing.status.IsOk() )
                {
                    m_listStale = true;
                    m_items.clear();
                    forgetListing();
                    fillTree();
                    setItemCount( MdwErrorMessage( aListing.status ) );
                    m_itemCount->SetToolTip( MdwErrorText( aListing.status ) );
                    return;
                }

                m_listStale = false;
                m_lastGood = wxDateTime::Now();
                m_listingTime = m_lastGood;
                m_listingSession = weak;
                m_listing = aListing.items;
                m_folders = std::move( aListing.folders );
                m_folderNames = std::move( aListing.folderNames );
                m_items = std::move( aListing.items );
                m_versions->RefreshChanges();
                fillTree();
            } );
}


wxString PANEL_GIT_WORKSPACE::folderText( const MDW_ITEM_INFO& aItem ) const
{
    if( auto it = m_folderNames.find( aItem.folder_id ); it != m_folderNames.end() )
        return it->second;

    // A search hit outside the walk has no folder name, so drop the item's own name from its path hint
    if( aItem.path_hint.IsEmpty() )
        return aItem.folder_id;

    return aItem.path_hint.BeforeLast( '/' ).Trim();
}


void PANEL_GIT_WORKSPACE::forgetListing()
{
    m_listing.clear();
    m_folders.clear();
    m_folderNames.clear();
    m_listingTime = wxDateTime();
    m_listingSession.reset();
}


void PANEL_GIT_WORKSPACE::fillTree( bool aRefreshDetail )
{
    // A provider already matched its rows, possibly on fields the tree does not show
    wxString                          filter = m_query ? wxString() : m_search->GetValue();
    std::optional<WORKSPACE_TREE_KEY> keep;

    if( !m_pendingSelect.IsEmpty() )
        keep.emplace( WORKSPACE_TREE_NODE::KIND::ITEM, m_pendingSelect );
    else if( const WORKSPACE_TREE_NODE* shown = m_model->NodeOf( m_itemsTree->GetSelection() ) )
        keep.emplace( shown->kind, shown->key );

    // Only the full tree records what is open, so a search's expanded matches do not stick
    if( !m_treeFiltered && !m_model->Root().children.empty() )
    {
        m_expanded.emplace();
        rememberExpanded( m_model->Root() );
    }

    showColumns();

    const std::vector<MDW_FOLDER> noFolders;
    const WORKSPACE_TREE_NODE*    selected = nullptr;

    {
        // The rebuild drops and restores the selection, which is no change to the detail
        wxEventBlocker blocker( m_itemsTree, wxEVT_DATAVIEW_SELECTION_CHANGED );

        // Rows the source searched for have no folders of their own, so they stand flat at the root
        m_model->Reset( BuildWorkspaceTree( m_query ? noFolders : m_folders, m_items, {}, filter ) );
        m_treeFiltered = m_query || !filter.IsEmpty();

        if( m_treeFiltered )
        {
            m_itemsTree->ExpandAll();
        }
        else if( m_expanded )
        {
            restoreExpanded( m_model->Root() );
        }
        else
        {
            // Opened for the first time, a project kept wholly in folders would show only their names
            for( const WORKSPACE_TREE_NODE& child : m_model->Root().children )
                m_itemsTree->Expand( m_model->ItemOf( child ) );
        }

        std::vector<const WORKSPACE_TREE_NODE*> path;

        if( keep )
            path = WorkspaceTreePath( m_model->Root(), *keep );

        if( !path.empty() )
        {
            selected = path.back();
            path.pop_back();

            for( const WORKSPACE_TREE_NODE* ancestor : path )
                m_itemsTree->Expand( m_model->ItemOf( *ancestor ) );

            wxDataViewItem item = m_model->ItemOf( *selected );

            m_pendingSelect.clear();
            m_itemsTree->Select( item );
            EnsureVisibleIfEnabled( m_itemsTree, item );
        }
    }

    size_t count = countItems( m_model->Root() );

    // A listing that failed or went offline keeps its line, and its rows stay grey
    if( m_query || !m_listStale )
        setItemCount( wxString::Format( wxPLURAL( "%zu file", "%zu files", count ), count ) );

    if( m_query )
        applyQueryStatus();
    else
        m_model->SetGreyed( m_listStale );

    if( !selected || selected->kind != WORKSPACE_TREE_NODE::KIND::ITEM || selected->item >= m_items.size() )
        hideDetail();
    else if( aRefreshDetail )
        showDetail( m_items[selected->item] );
}


void PANEL_GIT_WORKSPACE::rememberExpanded( const WORKSPACE_TREE_NODE& aNode )
{
    // A folder inside a closed one is not shown open, so the walk stops at the closed one
    for( const WORKSPACE_TREE_NODE& child : aNode.children )
    {
        if( !child.children.empty() && m_itemsTree->IsExpanded( m_model->ItemOf( child ) ) )
        {
            m_expanded->emplace( child.kind, child.key );
            rememberExpanded( child );
        }
    }
}


void PANEL_GIT_WORKSPACE::restoreExpanded( const WORKSPACE_TREE_NODE& aNode )
{
    for( const WORKSPACE_TREE_NODE& child : aNode.children )
    {
        if( !child.children.empty() && m_expanded->count( { child.kind, child.key } ) )
        {
            m_itemsTree->Expand( m_model->ItemOf( child ) );
            restoreExpanded( child );
        }
    }
}


void PANEL_GIT_WORKSPACE::showColumns()
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    m_caps = source ? source->Capabilities() : MDW_CAPABILITIES();
    m_stateColumn->SetHidden( !m_caps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) );
    m_editingColumn->SetHidden( !m_caps.Has( MDW_CAPABILITY_GROUP::LOCKING ) );
}


wxString PANEL_GIT_WORKSPACE::cellText( const WORKSPACE_TREE_NODE& aNode, unsigned aColumn ) const
{
    // m_items can change just ahead of the rebuild that drops this node
    if( aNode.kind != WORKSPACE_TREE_NODE::KIND::ITEM || aNode.item >= m_items.size() )
        return aColumn == WORKSPACE_TREE_MODEL::COL_NAME ? aNode.name : wxString();

    const MDW_ITEM_INFO& item = m_items[aNode.item];

    switch( aColumn )
    {
    case WORKSPACE_TREE_MODEL::COL_NAME:
    {
        // A row outside the folder tree would otherwise look the same as its namesakes elsewhere
        bool     outside = m_query || ( !item.folder_id.IsEmpty() && !m_folderNames.count( item.folder_id ) );
        wxString folder = outside ? folderText( item ) : wxString();

        return folder.IsEmpty() ? aNode.name : aNode.name + wxS( "  (" ) + folder + wxS( ")" );
    }

    case WORKSPACE_TREE_MODEL::COL_STATE: return WorkspaceStateCell( m_caps, item.state, item.revision_label );
    case WORKSPACE_TREE_MODEL::COL_EDITING:
        // A team library's locking shows the column, which says nothing of a design source without it
        return m_caps.Has( MDW_CAPABILITY_GROUP::LOCKING ) ? editingText( item.item_id ) : wxString();
    case WORKSPACE_TREE_MODEL::COL_LAST_CHANGE:
        return item.modified.IsValid() ? item.modified.FormatDate() : wxString();
    default: return wxString();
    }
}


wxString PANEL_GIT_WORKSPACE::editingText( const wxString& aItemId ) const
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current )
        return wxEmptyString;

    MDW_LOCK_INFO lock = current->LockTracker().Get( aItemId );

    if( !lock.is_locked )
        return MdwClaimLabel( lock, wxDateTime::Now() );

    if( lock.locked_by_me )
        return _( "You" );

    return lock.owner_display_name.IsEmpty() ? _( "Someone" ) : lock.owner_display_name;
}


void PANEL_GIT_WORKSPACE::showDetail( const MDW_ITEM_INFO& aItem )
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current )
        return;

    // Another item's states would be offered as this one's until the read lands
    if( aItem.item_id != m_selected )
        m_transitions.clear();

    m_detailGates = detailGates();
    m_selected = aItem.item_id;
    m_selectedItem = aItem;
    m_itemTitle->SetLabel( aItem.display_name );

    readTransitions();

    if( !m_workspaceSplitter->IsSplit() )
        m_workspaceSplitter->SplitVertically( m_itemListPanel, m_itemDetail, m_sashPos );

    std::shared_ptr<MDW_WORKSPACE_SERVICE> service = current->Service();
    bool revisions = service && service->Supports( MDW_CAPABILITY_GROUP::REVISIONS );

    m_versions->Show( revisions );

    if( revisions )
    {
        MDW_ITEM_REF item;
        item.item_id = aItem.item_id;

        m_versions->SetItem( MDW_CLIENT::Get().ProjectSource(), item, m_frame->Prj().GetProjectDirectory(),
                             WorkspaceWorkingFiles( m_frame->Prj().GetLocalSettings().WorkspaceItems(),
                                                    m_frame->Prj().GetProjectPath(), aItem ),
                             m_frame, wxString() );
    }
    else
    {
        m_versions->Clear();
    }

    // After the versions, whose local changes for the previous item would otherwise show
    showState();
}


wxString PANEL_GIT_WORKSPACE::selectedPath() const
{
    PROJECT& project = m_frame->Prj();

    // After a rename on the server the file being edited is still the one recorded at check out
    return WorkspaceCheckinPath( project.GetLocalSettings().WorkspaceItems(), project.GetProjectPath(),
                                 m_selectedItem );
}


void PANEL_GIT_WORKSPACE::readTransitions()
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    if( !source || !source->Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
    {
        m_transitionsOp = MDW_OP();
        m_transitionsReading = false;
        m_transitions.clear();
        return;
    }

    using TRANSITIONS = std::vector<kiapi::workspace::LifecycleTransition>;

    MDW_ITEM_REF item;
    item.item_id = m_selected;

    MDW_PART part( source, item );

    m_transitionsReading = true;
    m_transitionsOp =
            part.Transitions( wxEmptyString,
                              [this, itemId = m_selected]( const MDW_RESULT& aResult, const TRANSITIONS& aTransitions )
                              {
                                  if( itemId != m_selected )
                                      return;

                                  m_transitionsReading = false;

                                  // NOT_SUPPORTED, as the git design source answers, offers no states
                                  m_transitions = aResult.IsOk() ? aTransitions : TRANSITIONS();
                                  showState();
                              } );
}


void PANEL_GIT_WORKSPACE::showState()
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() )
        return;

    MDW_LOCK_INFO lock = current->LockTracker().Get( m_selected );
    wxString      folder = m_selectedItem.folder_id.IsEmpty() ? wxString() : folderText( m_selectedItem );

    m_itemDetail->Freeze();
    m_detailRows->Clear( true );

    std::vector<WORKSPACE_DETAIL_ROW> rows =
            WorkspaceDetailRows( m_caps, m_selectedItem, lock, m_versions->Modified(), folder, wxDateTime::Now() );

    for( const WORKSPACE_DETAIL_ROW& row : rows )
    {
        m_detailRows->Add( new wxStaticText( m_itemDetail, wxID_ANY, row.label ), 0, wxALIGN_TOP );
        m_detailRows->Add( new wxStaticText( m_itemDetail, wxID_ANY, row.value ), 0, wxEXPAND );
    }

    showActions( itemActions() );

    m_itemDetail->Layout();
    m_itemDetail->Thaw();
}


std::vector<WORKSPACE_ACTION> PANEL_GIT_WORKSPACE::itemActions() const
{
    using ID = WORKSPACE_ACTION::ID;

    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() )
        return {};

    const wxString         path = selectedPath();
    WORKSPACE_ACTION_FACTS facts;
    facts.locking = m_caps.Has( MDW_CAPABILITY_GROUP::LOCKING );
    facts.inPlace = editsInPlace();
    facts.localFile = wxFileName::FileExists( path );
    facts.lock = current->LockTracker().Get( m_selected );
    facts.transitions = m_transitions;

    // A re-read of states already shown keeps them, so the command does not blink off and on
    facts.transitionsReading = m_transitionsReading && m_transitions.empty();

    // Opening a project would close the window that owns this page while its handler runs
    wxString ext = wxFileName( path ).GetExt();
    bool     project = ext.IsSameAs( FILEEXT::ProjectFileExtension, false )
                   || ext.IsSameAs( FILEEXT::LegacyProjectFileExtension, false );

    facts.openable = projectManager() && !project;

    std::vector<WORKSPACE_ACTION> actions = WorkspaceItemActions( facts );

    // A second request while one runs would take, upload or move the item twice
    for( WORKSPACE_ACTION& action : actions )
    {
        if( action.id == ID::CHECK_OUT && !m_checkOutPending.expired() )
        {
            action.allowed = false;
            action.reason = _( "Checking out..." );
        }
        else if( action.id == ID::CHECK_IN && !m_checkInPending.expired() )
        {
            action.allowed = false;
            action.reason = _( "Checking in..." );
        }
        else if( action.id == ID::CHANGE_STATE && m_transitionPending )
        {
            action.allowed = false;
            action.reason = _( "Changing state..." );
        }
    }

    return actions;
}


void PANEL_GIT_WORKSPACE::showActions( const std::vector<WORKSPACE_ACTION>& aActions )
{
    bool same = std::ranges::equal( aActions, m_actionButtons,
                                    []( const WORKSPACE_ACTION& aAction, const auto& aButton )
                                    {
                                        return aAction.id == aButton.first;
                                    } );

    if( !same )
    {
        m_actionSizer->Clear( true );
        m_actionButtons.clear();
        m_actionSizer->AddStretchSpacer();

        for( const WORKSPACE_ACTION& action : aActions )
        {
            wxButton* button = new wxButton( m_itemDetail, wxID_ANY, action.label );

            // Deferred, since an action can refresh the detail and delete this button inside its own click
            button->Bind( wxEVT_BUTTON,
                          [this, id = action.id]( wxCommandEvent& )
                          {
                              CallAfter(
                                      [this, id]()
                                      {
                                          runAction( id );
                                      } );
                          } );

            m_actionSizer->Add( button, 0, wxLEFT, FromDIP( 4 ) );
            m_actionButtons.emplace_back( action.id, button );
        }
    }

    for( size_t ii = 0; ii < aActions.size(); ++ii )
    {
        const WORKSPACE_ACTION& action = aActions[ii];
        wxButton*               button = m_actionButtons[ii].second;
        wxString                tip = action.reason;

        if( tip.IsEmpty() && action.id == WORKSPACE_ACTION::ID::EDITING )
            tip = _( "Signals editing intent to teammates; others can still edit." );

        button->SetLabel( action.label );
        button->Enable( action.allowed );
        button->SetToolTip( tip );
    }

    // A teammate's claim lengthens the labels, which must not be cut off
    KIGIT::KeepDetailWidth( m_workspaceSplitter );
}


void PANEL_GIT_WORKSPACE::onContextMenu( wxDataViewEvent& aEvent )
{
    const WORKSPACE_TREE_NODE* node = m_model->NodeOf( aEvent.GetItem() );

    if( !node || node->kind != WORKSPACE_TREE_NODE::KIND::ITEM || node->item >= m_items.size() )
        return;

    // The actions act on the selected item, and a right click does not select on every platform
    if( m_itemsTree->GetSelection() != aEvent.GetItem() )
    {
        m_itemsTree->Select( aEvent.GetItem() );
        showDetail( m_items[node->item] );
    }

    std::vector<WORKSPACE_ACTION> actions = itemActions();

    if( actions.empty() )
        return;

    wxMenu menu;

    for( size_t ii = 0; ii < actions.size(); ++ii )
    {
        const WORKSPACE_ACTION& action = actions[ii];
        wxString                text = action.label;

        // A popup's help string shows nowhere in a dialog, and the editing label already names the holder
        if( !action.allowed && !action.reason.IsEmpty() && action.id != WORKSPACE_ACTION::ID::EDITING )
        {
            wxString reason = action.reason;

            // A menu label reads a single ampersand as a mnemonic marker
            reason.Replace( wxS( "&" ), wxS( "&&" ) );
            text = wxString::Format( _( "%s (%s)" ), action.label, reason );
        }

        menu.Append( wxID_HIGHEST + 1 + static_cast<int>( ii ), text, action.reason )->Enable( action.allowed );
    }

    int chosen = GetPopupMenuSelectionFromUser( menu ) - wxID_HIGHEST - 1;

    if( chosen >= 0 && chosen < static_cast<int>( actions.size() ) )
        runAction( actions[chosen].id );
}


void PANEL_GIT_WORKSPACE::runAction( WORKSPACE_ACTION::ID aId )
{
    using ID = WORKSPACE_ACTION::ID;

    switch( aId )
    {
    case ID::OPEN: onOpen(); break;
    case ID::EDITING: onEditing(); break;
    case ID::RELEASE_CLAIM: onReleaseClaim(); break;
    case ID::CHECK_OUT: onCheckOut(); break;
    case ID::CHECK_IN: onCheckIn(); break;
    case ID::CHANGE_STATE: onChangeState(); break;
    case ID::SHOW_IN_FOLDER: onShowInFolder(); break;
    }
}


void PANEL_GIT_WORKSPACE::refreshCells()
{
#ifdef __WXMAC__
    // The native outline view caches its cells and reads them again only for the rows it is told changed
    m_model->RefreshCells();
#else
    // The generic control looks up each changed row from the root, which is quadratic over a large workspace
    m_itemsTree->Refresh();
#endif
}


void PANEL_GIT_WORKSPACE::onLockChanged()
{
    refreshCells();
    showState();
}


unsigned PANEL_GIT_WORKSPACE::detailGates() const
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current )
        return 0;

    std::shared_ptr<MDW_WORKSPACE_SERVICE> service = current->Service();
    unsigned                               gates = 0;

    if( service && service->Supports( MDW_CAPABILITY_GROUP::LOCKING ) )
        gates |= 1;

    if( service && service->Supports( MDW_CAPABILITY_GROUP::REVISIONS ) )
        gates |= 2;

    return gates;
}


void PANEL_GIT_WORKSPACE::onCapabilitiesChanged()
{
    bool lifecycle = m_caps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE );

    showColumns();
    refreshCells();

    if( m_selected.IsEmpty() )
        return;

    // Both the backend and the re-read announce a change, and only a new gate needs the versions read again
    if( detailGates() == m_detailGates )
    {
        // LIFECYCLE coming or going changes whether the item has states to offer
        if( lifecycle != m_caps.Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
            readTransitions();

        showState();
        return;
    }

    MDW_ITEM_INFO selected = m_selectedItem;
    showDetail( selected );
}


void PANEL_GIT_WORKSPACE::hideDetail()
{
    m_selected.clear();
    m_selectedItem = MDW_ITEM_INFO();
    m_transitionsOp = MDW_OP();
    m_transitionsReading = false;
    m_transitions.clear();
    m_versions->Clear();

    if( m_workspaceSplitter->IsSplit() )
    {
        m_sashPos = m_workspaceSplitter->GetSashPosition();
        m_workspaceSplitter->Unsplit( m_itemDetail );
    }
}


void PANEL_GIT_WORKSPACE::onEditing()
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() )
        return;

    MDW_LOCK_INFO lock = current->LockTracker().Get( m_selected );

    MDW_ITEM_REF item;
    item.item_id = m_selected;

    MdwSetEditing( this, MDW_CLIENT::Get().ProjectSource(), item, !( lock.is_locked && lock.locked_by_me ), false,
                   wxEmptyString,
                   [this]()
                   {
                       fillTree();
                   } );
}


void PANEL_GIT_WORKSPACE::onReleaseClaim()
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() )
        return;

    // The claim may have changed hands since the command was offered
    std::optional<MDW_CLAIM_ACTION> release = MdwClaimRelease( current->LockTracker().Get( m_selected ) );

    if( !release )
        return;

    MDW_ITEM_REF item;
    item.item_id = m_selected;

    MdwRunClaimAction( this, MDW_CLIENT::Get().ProjectSource(), item, *release,
                       [this]()
                       {
                           fillTree();
                       } );
}


void PANEL_GIT_WORKSPACE::onChangeState()
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    if( !source || m_selected.IsEmpty() || m_transitions.empty() || m_transitionPending )
        return;

    const wxString                                           itemId = m_selected;
    const wxString                                           name = m_selectedItem.display_name;
    const std::vector<kiapi::workspace::LifecycleTransition> transitions = m_transitions;
    std::vector<MDW_PART_ACTION>                             actions;

    for( const kiapi::workspace::LifecycleTransition& transition : transitions )
    {
        bool denied = transition.permission().state() == kiapi::workspace::PS_DENIED;

        MDW_PART_ACTION action( MDW_PART_ACTION::ID::TRANSITION,
                                denied ? MDW_PART_ACTION::STATE::DENIED : MDW_PART_ACTION::STATE::ALLOWED,
                                wxString::FromUTF8( transition.permission().reason() ) );
        action.target = wxString::FromUTF8( transition.target_state_id() );
        action.label = wxString::FromUTF8( transition.display_name() );
        actions.push_back( action );
    }

    wxWeakRef<PANEL_GIT_WORKSPACE> self( this );
    std::optional<wxString>        target = MdwChooseTransition( this, name, actions );

    if( !self || IsBeingDeleted() || !target )
        return;

    std::optional<wxString> comment = MdwTransitionComment( this, name, transitions, *target );

    if( !self || IsBeingDeleted() || !comment )
        return;

    MDW_ITEM_REF item;
    item.item_id = itemId;

    MDW_PART part( source, item );

    m_transitionPending = true;
    m_transitionOp = part.Transition( wxEmptyString, *target, *comment,
                                      [this, name, itemId]( const MDW_RESULT& aResult, const MDW_TRANSITION& )
                                      {
                                          wxString what =
                                                  wxString::Format( _( "Could not change the state of %s." ), name );

                                          m_transitionPending = false;

                                          if( !aResult.IsOk() )
                                              MdwReportFailure( this, what, aResult );

                                          // The states offered were those of the old state, so they are read again
                                          if( aResult.IsOk() && itemId == m_selected )
                                          {
                                              m_transitions.clear();
                                              readTransitions();
                                          }

                                          showState();
                                          Populate();
                                      } );
    showState();
}


void PANEL_GIT_WORKSPACE::onOpen()
{
    wxString path = selectedPath();

    if( path.IsEmpty() )
        return;

    EDA_BASE_FRAME* manager = projectManager();

    if( !manager || !manager->OpenProjectTreeFile( path ) )
        DisplayErrorMessage( this, wxString::Format( _( "KiCad cannot open '%s'." ), path ) );
}


EDA_BASE_FRAME* PANEL_GIT_WORKSPACE::projectManager() const
{
    EDA_BASE_FRAME* top = dynamic_cast<EDA_BASE_FRAME*>( m_frame->Kiway().GetTop() );

    // Only the project manager has the file tree whose double click Open matches
    return top && top->GetFrameType() == KICAD_MAIN_FRAME_T ? top : nullptr;
}


void PANEL_GIT_WORKSPACE::onShowInFolder()
{
    wxString path = selectedPath();

    if( !path.IsEmpty() )
        LaunchExternal( wxFileName( path ).GetPath() );
}


void PANEL_GIT_WORKSPACE::SelectItem( const wxString& aItemId )
{
    m_pendingSelect = aItemId;
    m_search->ChangeValue( wxEmptyString );

    // ChangeValue sends no event, and a running search must end with the box emptied
    onSearch();
}


bool PANEL_GIT_WORKSPACE::editsInPlace() const
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    return source && source->Info().backend.checkin_model == MDW_CHECKIN_MODEL::WORKING_COPY;
}


void PANEL_GIT_WORKSPACE::onCheckOut()
{
    if( !session() || m_selected.IsEmpty() || !m_checkOutPending.expired() )
        return;

    wxString itemId = m_selected;
    wxString destination = WorkspaceCheckoutPath( m_frame->Prj().GetProjectPath(), m_selectedItem );

    // An empty path would still take the lock, leaving nothing to check in
    if( destination.IsEmpty() )
    {
        DisplayErrorMessage( this, _( "The workspace gives this file no path in the project to check out to." ) );
        return;
    }

    wxString holder = m_frame->Prj().GetLocalSettings().GetWorkspaceItemId( destination );

    // Replacing another item's file would make its next check in upload this item
    if( !holder.IsEmpty() && holder != itemId && wxFileName::Exists( destination ) )
    {
        DisplayErrorMessage( this, wxString::Format( _( "'%s' already holds the checkout of %s." ), destination,
                                                     holder ) );
        return;
    }

    // The backend replaces the local file outright, saved edits included
    if( wxFileName::Exists( destination ) )
    {
        if( DocumentEditorsDirty( m_frame->Kiway() ) )
        {
            DisplayErrorMessage( this, _( "Save your changes before checking out over an existing file." ) );
            return;
        }

        if( !IsOK( this, wxString::Format( _( "Replace '%s' with the checked out version?" ), destination ) ) )
            return;
    }

    checkOut( itemId, destination );
}


void PANEL_GIT_WORKSPACE::checkOut( const wxString& aItemId, const wxString& aDestination )
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || !m_checkOutPending.expired() )
        return;

    wxString                           projectPath = m_frame->Prj().GetProjectFullName();
    std::string                        snapshot = MDW_SAVE_GUARD::ProjectFileSnapshot( m_frame->Prj() );
    std::weak_ptr<PANEL_GIT_WORKSPACE> lifetime = m_lifetime;
    EDITOR_SNAPSHOT                    docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );
    KIWAY*                             kiway = &m_frame->Kiway();

    auto pending = std::make_shared<int>( 0 );
    m_checkOutPending = pending;
    showState();

    // The app owns the completion so the project learns of the new file even if this window closes first
    current->RunAsync<CHECKOUT>(
            wxTheApp,
            [itemId = aItemId, destination = aDestination, source = MDW_CLIENT::Get().ProjectSource()]()
            {
                CHECKOUT             checkout;
                MDW_CHECKOUT_REQUEST request;

                request.item_id = itemId;
                request.local_path = destination;
                request.lock = true;

                if( !source )
                {
                    checkout.status = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
                    return checkout;
                }

                std::tie( checkout.status, checkout.result ) = MdwSync<MDW_CHECKOUT_RESULT>(
                        [&]( MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
                        {
                            return source->Checkout( request, std::move( aDone ) );
                        } );

                return checkout;
            },
            [itemId = aItemId, destination = aDestination, projectPath, snapshot, lifetime, pending, docsBefore,
             kiway]( CHECKOUT aCheckout )
            {
                // Taken on the UI thread, so it also sees the panel deleted inside a modal error dialog
                wxWeakRef<PANEL_GIT_WORKSPACE> panel( lifetime.lock().get() );
                wxWindow*                      parent = panel && !panel->IsBeingDeleted() ? panel.get() : nullptr;

                if( parent )
                {
                    panel->m_checkOutPending.reset();
                    panel->showState();
                }

                if( !aCheckout.status.IsOk() )
                {
                    DisplayErrorMessage( parent, _( "Could not check out the file." ),
                                         MdwErrorText( aCheckout.status ) );
                }
                else if( PROJECT* project = Pgm().GetSettingsManager().GetProject( projectPath ) )
                {
                    const wxString&      localPath = aCheckout.result.local_path;
                    const MDW_ITEM_INFO& item = aCheckout.result.item;

                    // Also rereads the settings when the checkout replaced the project file
                    bool recorded = MDW_SAVE_GUARD::RecordCheckout( Pgm().GetSettingsManager(), *project, localPath,
                                                                    itemId, { item.revision_id, item.revision_label },
                                                                    snapshot );

                    if( !recorded )
                    {
                        DisplayErrorMessage( parent,
                                             _( "Checked out, but the project's settings could not be saved." ) );
                    }
                    else if( parent )
                    {
                        panel->m_frame->SetStatusText( wxString::Format( _( "Checked out to %s" ), destination ) );
                    }

                    // The open editors belong to the active project, and reloading another's would drop their undo
                    if( project == &Pgm().GetSettingsManager().Prj() )
                    {
                        EDITOR_SNAPSHOT before = docsBefore;

                        // The record already brought the settings in, and rereading them would drop unsaved ones
                        before.settings = CaptureOpenEditorDocs( *kiway ).settings;

                        // An editor left on the replaced file would save it back over the checkout
                        ReloadOpenEditors( *kiway, before );
                    }
                }

                if( panel && !panel->IsBeingDeleted() )
                    panel->Populate();
            } );
}


void PANEL_GIT_WORKSPACE::onCheckIn()
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() || !m_checkInPending.expired() )
        return;

    MDW_CHECKIN_REQUEST request;
    request.item_id = m_selected;
    request.local_path = WorkspaceCheckinPath( m_frame->Prj().GetLocalSettings().WorkspaceItems(),
                                               m_frame->Prj().GetProjectPath(), m_selectedItem );

    if( !wxFileName::FileExists( request.local_path ) )
    {
        DisplayErrorMessage( this, _( "Check out the file before checking it in." ), request.local_path );
        return;
    }

    MDW_LOCK_INFO lock = current->LockTracker().Get( m_selected );

    // Check In is offered under a teammate's advisory claim, whose holder will have to merge
    if( ( ( lock.is_locked && !lock.locked_by_me ) || !lock.known )
        && !IsOK( this, MdwClaimCommitWarning( lock, wxFileName( request.local_path ).GetFullName(), false ) ) )
    {
        return;
    }

    wxTextEntryDialog commentDlg( this, _( "Check-in comment:" ), _( "Check In" ) );

    if( commentDlg.ShowModal() != wxID_OK )
        return;

    request.comment = commentDlg.GetValue();
    request.base_revision_id = MDW_SAVE_GUARD::CheckinBase( m_frame->Prj(), request.local_path, request.item_id );

    checkIn( request, m_selectedItem );
}


void PANEL_GIT_WORKSPACE::checkIn( const MDW_CHECKIN_REQUEST& aRequest, const MDW_ITEM_INFO& aItem )
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current )
        return;

    wxString                           projectPath = m_frame->Prj().GetProjectFullName();
    std::string                        snapshot = MDW_SAVE_GUARD::ProjectFileSnapshot( m_frame->Prj() );
    std::weak_ptr<PANEL_GIT_WORKSPACE> lifetime = m_lifetime;

    auto pending = std::make_shared<int>( 0 );
    m_checkInPending = pending;
    showState();

    // The app owns the completion so the file's new base is recorded even if this window closes first
    current->RunAsync<CHECKIN>(
            wxTheApp,
            [request = aRequest, source = MDW_CLIENT::Get().ProjectSource()]()
            {
                CHECKIN checkin;

                if( !source )
                {
                    checkin.status = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
                    return checkin;
                }

                std::tie( checkin.status, checkin.revision ) = MdwSync<MDW_REVISION>(
                        [&]( MDW_DONE<MDW_REVISION> aDone )
                        {
                            return source->Checkin( request, std::move( aDone ) );
                        } );

                // A history cut short by the page cap still answers, newest first
                auto readHistory = [&]( const MDW_ITEM_REF& aRef, std::vector<MDW_REVISION>& aOut )
                {
                    auto [result, revisions] = MdwSync<MDW_PAGED<MDW_REVISION>>(
                            [&]( MDW_DONE<MDW_PAGED<MDW_REVISION>> aDone )
                            {
                                return MdwReadAllPages<MDW_REVISION>(
                                        [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_REVISION>> aPageDone )
                                        {
                                            return source->History( aRef, aPage, std::move( aPageDone ) );
                                        },
                                        0, std::move( aDone ), MDW_READ_CAP::PARTIAL );
                            } );

                    aOut.insert( aOut.end(), revisions.rows.begin(), revisions.rows.end() );
                    return result;
                };

                const MDW_ITEM_REF ref{ request.item_id, wxEmptyString, wxEmptyString };

                // Only a known base can be stale, so any other conflict is shown as the backend worded it
                if( checkin.status.error != MDW_ERROR_CODE::VERSION_CONFLICT || request.base_revision_id.IsEmpty() )
                    return checkin;

                auto [found, item] = MdwSync<MDW_ITEM_INFO>(
                        [&]( MDW_DONE<MDW_ITEM_INFO> aDone )
                        {
                            return source->Item( ref, wxEmptyString, std::move( aDone ) );
                        } );

                if( !found.IsOk() || item.revision_id.IsEmpty() || !readHistory( ref, checkin.since ).IsOk() )
                {
                    checkin.since.clear();
                    return checkin;
                }

                auto find = [&]( const wxString& aRevisionId )
                {
                    return std::find_if( checkin.since.begin(), checkin.since.end(),
                                         [&]( const MDW_REVISION& aRevision )
                                         {
                                             return aRevision.revision_id == aRevisionId;
                                         } );
                };

                auto base = find( request.base_revision_id );

                // Without the base in the history only the latest is known to be newer
                if( base == checkin.since.end() )
                {
                    auto                      newest = find( item.revision_id );
                    std::vector<MDW_REVISION> latest;

                    if( newest != checkin.since.end() )
                        latest.push_back( *newest );

                    checkin.since = std::move( latest );
                }
                else
                {
                    checkin.since.erase( base, checkin.since.end() );
                }

                // History orders by id, archived revisions included, so only the item names its latest
                checkin.latest = item.revision_id;
                return checkin;
            },
            [request = aRequest, item = aItem, projectPath, snapshot, lifetime, pending]( CHECKIN aCheckin )
            {
                wxWeakRef<PANEL_GIT_WORKSPACE> panel( lifetime.lock().get() );
                wxWindow*                      parent = panel && !panel->IsBeingDeleted() ? panel.get() : nullptr;

                if( parent )
                {
                    panel->m_checkInPending.reset();
                    panel->showState();
                }

                if( parent && !aCheckin.since.empty() )
                {
                    panel->resolveStaleCheckin( request, item, aCheckin.since, aCheckin.latest );
                }
                else if( !aCheckin.status.IsOk() )
                {
                    DisplayErrorMessage( parent, _( "Could not check in the file." ),
                                         MdwErrorText( aCheckin.status ) );
                }
                else if( PROJECT* project = Pgm().GetSettingsManager().GetProject( projectPath ) )
                {
                    const MDW_REVISION& revision = aCheckin.revision;

                    bool recorded = MDW_SAVE_GUARD::RecordBase( Pgm().GetSettingsManager(), *project,
                                                                request.local_path, request.item_id,
                                                                { revision.revision_id, revision.label }, snapshot );

                    if( !recorded )
                    {
                        DisplayErrorMessage( parent,
                                             _( "Checked in, but the project's settings could not be saved." ) );
                    }
                    else if( parent )
                    {
                        panel->m_frame->SetStatusText( WorkspaceCheckinReport( item, revision ) );
                    }
                }

                if( panel && !panel->IsBeingDeleted() )
                    panel->Populate();
            } );
}


void PANEL_GIT_WORKSPACE::resolveStaleCheckin( const MDW_CHECKIN_REQUEST& aRequest, const MDW_ITEM_INFO& aItem,
                                               const std::vector<MDW_REVISION>& aSince, const wxString& aLatest )
{
    wxWeakRef<PANEL_GIT_WORKSPACE> self( this );
    const wxString                 keep = WorkspaceKeepBothPath( aRequest.local_path );
    wxString                       since;

    for( const MDW_REVISION& revision : aSince )
        since << wxS( "\n" ) << wxString::Format( _( "%s by %s" ), WorkspaceRevisionText( revision ), revision.author );

    KIDIALOG dlg( this,
                  wxString::Format( _( "%s has newer revisions than your copy." ),
                                    wxFileName( aRequest.local_path ).GetFullName() ),
                  _( "Check In" ), wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING );

    dlg.SetExtendedMessage( wxString::Format( _( "Checked in since your copy:%s\n\n"
                                                 "Keep Both saves your file as %s and checks out the latest revision "
                                                 "for you to merge. Check In Anyway replaces those changes with "
                                                 "yours." ),
                                              since, wxFileName( keep ).GetFullName() ) );
    dlg.SetYesNoCancelLabels( _( "&Keep Both" ), _( "Check In &Anyway" ), _( "Cancel" ) );

    int answer = dlg.ShowModal();

    if( !self || IsBeingDeleted() )
        return;

    if( answer == wxID_NO )
    {
        // Naming the latest as the base is what makes the overwrite explicit
        MDW_CHECKIN_REQUEST anyway = aRequest;
        anyway.base_revision_id = aLatest;
        checkIn( anyway, aItem );
    }
    else if( answer == wxID_YES )
    {
        if( DocumentEditorsDirty( m_frame->Kiway() ) )
            DisplayErrorMessage( this, _( "Save your changes before checking out over an existing file." ) );
        else if( !m_checkOutPending.expired() )
            DisplayErrorMessage( this, _( "Wait for the running check out to finish, then check in again." ) );
        else if( !wxCopyFile( aRequest.local_path, keep, false ) )
            DisplayErrorMessage( this, wxString::Format( _( "Could not save your file as '%s'." ), keep ) );
        else
            checkOut( aRequest.item_id, aRequest.local_path );
    }
}
