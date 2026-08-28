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
#include <dialogs/git/workspace_item_files.h>
#include <dialogs/git/workspace_search_query.h>
#include <confirm.h>
#include <kidialog.h>
#include <eda_base_frame.h>
#include <mdw/mdw_claim_text.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_lock_tracker.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_paged_read.h>
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

#include <wx/app.h>
#include <wx/event.h>
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
    std::map<wxString, wxString> folderNames;
};


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

    m_itemsList->AppendColumn( _( "Name" ), wxLIST_FORMAT_LEFT, FromDIP( 200 ) );
    m_itemsList->AppendColumn( _( "Editing" ), wxLIST_FORMAT_LEFT, FromDIP( 100 ) );
    m_itemsList->AppendColumn( _( "Last Change" ), wxLIST_FORMAT_LEFT, FromDIP( 110 ) );
    m_itemsList->AppendColumn( _( "Folder" ), wxLIST_FORMAT_LEFT, FromDIP( 140 ) );

    m_editingButton->SetToolTip( _( "Signals editing intent to teammates; others can still edit." ) );

    m_checkOutButton->Bind( wxEVT_BUTTON,
                            [this]( wxCommandEvent& )
                            {
                                onCheckOut();
                            } );

    m_checkInButton->Bind( wxEVT_BUTTON,
                           [this]( wxCommandEvent& )
                           {
                               onCheckIn();
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
                    || aEvent.type == MDW_EVENT_TYPE::LOCKS_CHANGED
                    || aEvent.type == MDW_EVENT_TYPE::REVISION_RELEASED )
                {
                    onLockChanged();
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

    m_itemsList->Bind( wxEVT_LIST_ITEM_SELECTED,
                       [this]( wxListEvent& aEvent )
                       {
                           showDetail( m_items[m_shown[aEvent.GetIndex()]] );
                       } );

    m_itemsList->Bind( wxEVT_LIST_ITEM_DESELECTED,
                       [this]( wxListEvent& )
                       {
                           if( m_itemsList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) == -1 )
                               hideDetail();
                       } );

    m_editingButton->Bind( wxEVT_BUTTON,
                           [this]( wxCommandEvent& )
                           {
                               onEditing();
                           } );

    m_connectLibraryButton->Bind( wxEVT_BUTTON,
                                  [this]( wxCommandEvent& )
                                  {
                                      SetUpLibrarySource( m_frame, m_frame->Kiway(), &m_frame->Prj() );
                                  } );
}


PANEL_GIT_WORKSPACE::~PANEL_GIT_WORKSPACE() = default;


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
    forgetListing();
    hideDetail();
    fillList();

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
        }

        m_queryStatus = WORKSPACE_SEARCH_QUERY::STATUS();
        m_queryHasMore = false;
        applyQueryStatus();

        fillList();

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
        fillList();
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
            fillList();

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

                        if( aFirstPage )
                        {
                            fillList();
                        }
                        else if( !aRows.empty() )
                        {
                            // Reselecting the open row would reload its detail and drop unsaved edits
                            wxEventBlocker blocker( m_itemsList );
                            fillList();
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
        greyRows( m_queryStatus.stale );
}


void PANEL_GIT_WORKSPACE::greyRows( bool aGrey )
{
    const wxColour colour = aGrey ? wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT )
                                  : m_itemsList->GetTextColour();

    for( long row = 0; row < m_itemsList->GetItemCount(); ++row )
        m_itemsList->SetItemTextColour( row, colour );
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
    greyRows( true );
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
        fillList();
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

                for( const MDW_FOLDER& folder : roots )
                {
                    listing.folderNames[folder.folder_id] = WorkspaceFolderName( wxEmptyString, folder );
                    folders.push_back( folder.folder_id );
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

                    for( const MDW_FOLDER& child : children )
                    {
                        listing.folderNames[child.folder_id] =
                                WorkspaceFolderName( listing.folderNames[folder], child );
                        folders.push_back( child.folder_id );
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
                    fillList();
                    setItemCount( MdwErrorMessage( aListing.status ) );
                    m_itemCount->SetToolTip( MdwErrorText( aListing.status ) );
                    return;
                }

                m_listStale = false;
                m_lastGood = wxDateTime::Now();
                m_listingTime = m_lastGood;
                m_listingSession = weak;
                m_listing = aListing.items;
                m_folderNames = std::move( aListing.folderNames );
                m_items = std::move( aListing.items );
                m_versions->RefreshChanges();
                fillList();
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
    m_listingTime = wxDateTime();
    m_listingSession.reset();
}


void PANEL_GIT_WORKSPACE::fillList()
{
    // A provider already matched its rows, possibly on fields the list does not show
    wxString search = m_query ? wxString() : m_search->GetValue().Lower();
    wxString keep = m_pendingSelect.IsEmpty() ? m_selected : m_pendingSelect;

    m_itemsList->DeleteAllItems();
    m_shown.clear();

    for( size_t ii = 0; ii < m_items.size(); ++ii )
    {
        const MDW_ITEM_INFO& item = m_items[ii];

        if( !search.IsEmpty() && !item.display_name.Lower().Contains( search )
            && !folderText( item ).Lower().Contains( search ) )
        {
            continue;
        }

        long row = m_itemsList->InsertItem( m_itemsList->GetItemCount(), item.display_name );

        m_itemsList->SetItem( row, 1, editingText( item.item_id ) );
        m_itemsList->SetItem( row, 2, item.modified.IsValid() ? item.modified.FormatDate() : wxString() );
        m_itemsList->SetItem( row, 3, folderText( item ) );
        m_shown.push_back( ii );

        if( item.item_id == keep )
        {
            m_pendingSelect.clear();
            m_itemsList->SetItemState( row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
            m_itemsList->EnsureVisible( row );
        }
    }

    // Folder takes room only once an item sits in one
    bool inFolder = std::any_of( m_items.begin(), m_items.end(),
                                 []( const MDW_ITEM_INFO& aItem )
                                 {
                                     return !aItem.folder_id.IsEmpty();
                                 } );

    if( !inFolder )
        m_itemsList->SetColumnWidth( 3, 0 );
    else if( m_itemsList->GetColumnWidth( 3 ) == 0 )
        m_itemsList->SetColumnWidth( 3, FromDIP( 140 ) );

    // A listing that failed or went offline keeps its line, and its rows stay grey
    if( m_query || !m_listStale )
        setItemCount( wxString::Format( wxPLURAL( "%zu file", "%zu files", m_shown.size() ), m_shown.size() ) );

    if( m_query )
        applyQueryStatus();
    else
        greyRows( m_listStale );

    if( m_itemsList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) == -1 )
        hideDetail();
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

    std::shared_ptr<MDW_WORKSPACE_SERVICE> service = current->Service();
    bool                                   locking = service && service->Supports( MDW_CAPABILITY_GROUP::LOCKING );

    m_detailGates = detailGates();
    m_selected = aItem.item_id;
    m_selectedItem = aItem;
    m_itemTitle->SetLabel( aItem.display_name );
    m_itemFolder->SetLabel( wxString::Format( _( "Folder: %s" ),
                                              aItem.folder_id.IsEmpty() ? _( "(top)" ) : folderText( aItem ) ) );
    m_itemEditing->Show( locking );
    m_itemLastChange->SetLabel( aItem.modified.IsValid()
                                        ? wxString::Format( _( "Last change: %s" ), aItem.modified.FormatDate() )
                                        : wxString() );

    bool inPlace = editsInPlace();

    m_checkOutButton->Show( !inPlace );
    m_checkInButton->Show( !inPlace );
    m_editingButton->Show( locking && inPlace );

    showLock();

    if( !m_workspaceSplitter->IsSplit() )
        m_workspaceSplitter->SplitVertically( m_itemListPanel, m_itemDetail, m_sashPos );

    bool revisions = service && service->Supports( MDW_CAPABILITY_GROUP::REVISIONS );

    m_versions->Show( revisions );
    m_itemDetail->Layout();

    if( !revisions )
    {
        m_versions->Clear();
        return;
    }

    MDW_ITEM_REF item;
    item.item_id = aItem.item_id;

    m_versions->SetItem( MDW_CLIENT::Get().ProjectSource(), item, m_frame->Prj().GetProjectDirectory(),
                         WorkspaceCheckoutPath( m_frame->Prj().GetProjectPath(), aItem ), m_frame,
                         wxString() );
}


void PANEL_GIT_WORKSPACE::showLock()
{
    std::shared_ptr<MDW_SESSION> current = session();

    if( !current || m_selected.IsEmpty() )
        return;

    MDW_LOCK_INFO    lock = current->LockTracker().Get( m_selected );
    wxString         editing = editingText( m_selected );
    MDW_CLAIM_ACTION claim = MdwClaimActions( lock ).front();

    if( !lock.is_locked && !lock.known )
        m_itemEditing->SetLabel( editing );
    else
        m_itemEditing->SetLabel( wxString::Format( _( "Editing: %s" ), editing.IsEmpty() ? _( "nobody" ) : editing ) );

    m_checkOutButton->Enable( m_checkOutPending.expired() && !lock.is_locked );
    m_checkInButton->Enable( m_checkInPending.expired() && ( !lock.is_locked || lock.locked_by_me || lock.advisory ) );
    m_editingButton->SetLabel( claim.label );
    m_editingButton->Enable( claim.enabled );

    m_itemDetail->Layout();
}


void PANEL_GIT_WORKSPACE::onLockChanged()
{
    for( size_t row = 0; row < m_shown.size(); ++row )
        m_itemsList->SetItem( static_cast<long>( row ), 1, editingText( m_items[m_shown[row]].item_id ) );

    showLock();
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
    // Both the backend and the re-read announce a change
    if( m_selected.IsEmpty() || detailGates() == m_detailGates )
        return;

    MDW_ITEM_INFO selected = m_selectedItem;
    showDetail( selected );
}


void PANEL_GIT_WORKSPACE::hideDetail()
{
    m_selected.clear();
    m_selectedItem = MDW_ITEM_INFO();
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
                       fillList();
                   } );
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
    m_checkOutButton->Disable();

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

                if( panel )
                    panel->m_checkOutPending.reset();

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
    m_checkInButton->Disable();

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

                if( panel )
                    panel->m_checkInPending.reset();

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
