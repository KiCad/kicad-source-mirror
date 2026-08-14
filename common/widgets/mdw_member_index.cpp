/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/mdw_member_index.h>
#include <widgets/mdw_part_state.h>

#include <lib_id.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_source.h>
#include <confirm.h>
#include <mdw_runtime/mdw_member_facts.h>
#include <pgm_base.h>
#include <trace_helpers.h>
#include <core/leak_at_exit.h>

#include <algorithm>
#include <set>

#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/app.h>
#include <wx/log.h>


namespace
{

// How long landed listings wait for the rest of a burst
constexpr int NOTIFY_HOLD_MS = 2000;

// A listing is read in full, so only a backend that never ends its pages reaches this
constexpr int MEMBER_PAGES = 1000;

MDW_MEMBER_INDEX* s_instance = nullptr;

wxString keyOf( const wxString& aLocator, const wxString& aLibraryId, const wxString& aSnapshot )
{
    return aLocator + wxS( "\n" ) + aLibraryId + wxS( "\n" ) + aSnapshot;
}

} // namespace


std::optional<MDW_TEAM_LIBRARY> MdwTeamLibrary( MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow )
{
    // The row's URI is expanded only for a row that can bind
    if( !MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() )
        return std::nullopt;

    return MdwTeamLibrary( MDW_LIBRARY_SOURCES::TeamLibrariesEnabled(), MDW_LIBRARY_SOURCES::Get(),
                           []( const wxString& aLocator )
                           {
                               return MDW_CLIENT::Get().FindSource( aLocator );
                           },
                           aType, aRow.Options(), LIBRARY_MANAGER::GetFullURI( &aRow, true ) );
}


std::optional<MDW_TEAM_LIBRARY>
MdwTeamLibrary( bool aEnabled, const MDW_LIBRARY_SOURCES& aSources,
                const std::function<std::shared_ptr<MDW_SOURCE>( const wxString& )>& aFindSource,
                MDW_LIBRARY_TYPE aType, const wxString& aRowOptions, const wxString& aRowPath )
{
    if( !aEnabled )
        return std::nullopt;

    std::optional<MDW_LIBRARY_ROW> row = aSources.ForRowAt( aType, aRowOptions, aRowPath );

    if( !row )
        return std::nullopt;

    std::shared_ptr<MDW_SOURCE> source = aFindSource( row->locator );

    if( !source )
        return std::nullopt;

    // Listing at the copy's snapshot keeps each member's release the one on disk, pinned or not
    return MDW_TEAM_LIBRARY{ source, row->library_id, row->snapshot.token, row->local_root };
}


bool MdwSaveNeedsCheckIn( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    // Capabilities are known only once logged in, and an offline edit is the one most in need of the warning
    if( !aSource || aSource->Info().connection.phase != MDW_CONNECTION::ONLINE )
        return true;

    return aSource->Has( MDW_CAPABILITY_GROUP::SHARED_LIBRARIES )
           && !aSource->Has( MDW_CAPABILITY_GROUP::LIFECYCLE );
}


namespace
{

int g_silentSaves = 0;

} // namespace


MDW_SILENT_SAVE::MDW_SILENT_SAVE()
{
    ++g_silentSaves;
}


MDW_SILENT_SAVE::~MDW_SILENT_SAVE()
{
    --g_silentSaves;
}


bool MDW_SILENT_SAVE::Active()
{
    return g_silentSaves > 0;
}


void MdwWarnLocalSave( wxWindow* aParent, MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow )
{
    if( MDW_SILENT_SAVE::Active() || !MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() )
        return;

    std::optional<MDW_LIBRARY_ROW> team = MDW_LIBRARY_SOURCES::Get().ForRowAt(
            aType, aRow.Options(), LIBRARY_MANAGER::GetFullURI( &aRow, true ) );

    if( !team || !MdwSaveNeedsCheckIn( MDW_CLIENT::Get().FindSource( team->locator ) ) )
        return;

    static std::set<wxString> warned;

    if( !warned.insert( team->locator + wxS( "\n" ) + team->library_id ).second )
        return;

    // Modal, since an info bar is replaced by the editors' own updates before the user reads it
    DisplayInfoMessage( aParent,
                        wxString::Format( _( "Changes saved to %s stay on this computer until they are checked in "
                                             "to the team library." ),
                                          aRow.Nickname() ) );
}


MDW_ITEM_REF MdwMemberRef( const MDW_TEAM_LIBRARY& aBinding, const MDW_LIBRARY_MEMBER& aMember )
{
    return MDW_ITEM_REF{ aMember.member_id, aMember.path_hint, aBinding.source->Info().locator };
}


bool MDW_MEMBER_CHANGES::Has( const wxString& aLocator, const wxString& aLibraryId ) const
{
    return all || libraries.count( { aLocator, aLibraryId } ) || libraries.count( { aLocator, wxString() } );
}


MDW_MEMBER_INDEX& MDW_MEMBER_INDEX::Get()
{
    static MDW_MEMBER_INDEX index;
    return index;
}


MDW_MEMBER_INDEX::MDW_MEMBER_INDEX() :
        MDW_MEMBER_INDEX(
                []( const wxString& aLocator )
                {
                    return MDW_CLIENT::Get().FindSource( aLocator );
                },
                []( wxEvtHandler* aOwner, std::function<void( const wxString& )> aChanged )
                {
                    return MDW_CLIENT::Get().SubscribeConnections( aOwner, std::move( aChanged ) );
                } )
{
    s_instance = this;
}


MDW_MEMBER_INDEX::MDW_MEMBER_INDEX( FIND_SOURCE aFindSource, CONNECTIONS aConnections ) :
        m_findSource( std::move( aFindSource ) ),
        m_subscribeConnections( std::move( aConnections ) )
{
    // Constructed first, the client outlives this index's subscription at exit
    MDW_CLIENT::Get();
}


MDW_MEMBER_INDEX::~MDW_MEMBER_INDEX()
{
    if( s_instance == this )
        s_instance = nullptr;

    // Past wx shutdown the timer cannot be destroyed safely, and with no event loop it never fires
    if( m_notifyTimer && !wxTheApp )
        LSAN_IGNORE( m_notifyTimer.release() );
}


void MDW_MEMBER_INDEX::Shutdown()
{
    if( !s_instance )
        return;

    s_instance->m_shutDown = true;
    s_instance->m_notifyTimer.reset();
}


std::optional<MDW_MEMBER_ENTRY> MDW_MEMBER_INDEX::Find( const MDW_TEAM_LIBRARY& aBinding, const wxString& aName )
{
    const std::map<wxString, MDW_MEMBER_ENTRY>* members = Members( aBinding );

    if( !members )
        return std::nullopt;

    auto it = members->find( aName );

    if( it == members->end() )
        return std::nullopt;

    return it->second;
}


const std::map<wxString, MDW_MEMBER_ENTRY>* MDW_MEMBER_INDEX::Members( const MDW_TEAM_LIBRARY& aBinding )
{
    LIBRARY* lib = library( aBinding );

    if( !lib )
        return nullptr;

    if( lib->listed )
        return &lib->members;

    // A release moves the copy to a snapshot not listed yet, and the part must not drop out until it is
    auto earlier = m_lastListed.find( keyOf( lib->source->Info().locator, lib->libraryId, wxEmptyString ) );

    if( earlier != m_lastListed.end() && earlier->second->source == lib->source )
        return &earlier->second->members;

    return nullptr;
}


std::optional<MDW_PART_FACTS> MdwListedFacts( const MDW_LIBRARY_LISTING& aListing, const wxString& aName,
                                              const wxString& aDigest )
{
    if( !aListing.team )
        return std::nullopt;

    MDW_PART_FACTS facts;

    if( !aListing.members )
    {
        facts.statuses.push_back( kiapi::mdw::PPS_UNVERIFIED );
        facts.message = _( "Its library's listing has not arrived" );
        return facts;
    }

    auto entry = aListing.members->find( aName );

    if( entry == aListing.members->end() )
    {
        facts.statuses.push_back( kiapi::mdw::PPS_UNKNOWN );
        facts.message = _( "Not checked in" );
        return facts;
    }

    const std::vector<MDW_MEMBER_RELEASE>& releases = entry->second.member.releases;
    std::vector<MDW_RELEASE_VIEW>          views;

    for( const MDW_MEMBER_RELEASE& release : releases )
    {
        views.push_back( { release.content_digest.utf8_string(),
                           release.state.category == MDW_LIFECYCLE_CATEGORY::RELEASED, release.published,
                           !release.state.not_for_new_designs } );
    }

    const MDW_MEMBER_VERDICT verdict = MdwWeighMember( views, aDigest.utf8_string() );

    if( aDigest.IsEmpty() && verdict.statuses.front() == kiapi::mdw::PPS_UNKNOWN )
        facts.message = _( "The placed part carries no content digest" );

    if( !releases.empty() )
        facts.latest_revision_label = releases.back().label;

    if( verdict.matched >= 0 )
    {
        facts.matched_revision_id = releases[verdict.matched].revision_id;
        facts.matched_revision_label = releases[verdict.matched].label;
        facts.matched_state = releases[verdict.matched].state;
    }

    facts.statuses = verdict.statuses;

    return facts;
}


std::optional<MDW_PART_FACTS> MdwListedLatestFacts( const MDW_LIBRARY_LISTING& aListing, const wxString& aName )
{
    wxString digest;

    if( aListing.members )
    {
        if( auto entry = aListing.members->find( aName ); entry != aListing.members->end() )
        {
            if( !entry->second.member.releases.empty() )
                digest = entry->second.member.releases.back().content_digest;
        }
    }

    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( aListing, aName, digest );

    // A tampered newest release has no digest to weigh, which is not the part's doing
    if( facts && digest.IsEmpty() && aListing.members && aListing.members->count( aName )
        && !aListing.members->at( aName ).member.releases.empty() )
    {
        facts->message = _( "The newest release carries no content digest" );
    }

    return facts;
}


bool MdwReleaseNeedsConfirmation( const MDW_PART_FACTS& aFacts )
{
    return std::ranges::any_of( aFacts.statuses,
                                []( kiapi::mdw::PlacedPartStatus aStatus )
                                {
                                    return aStatus == kiapi::mdw::PPS_NEVER_RELEASED
                                           || aStatus == kiapi::mdw::PPS_UNVERIFIED
                                           || aStatus == kiapi::mdw::PPS_UNKNOWN;
                                } );
}


MDW_LIBRARY_LISTING MDW_MEMBER_INDEX::Listing( MDW_LIBRARY_TYPE aType, const wxString& aNickname )
{
    const LIBRARY_TABLE_TYPE tableType =
            aType == MDW_LIBRARY_TYPE::SYMBOL ? LIBRARY_TABLE_TYPE::SYMBOL : LIBRARY_TABLE_TYPE::FOOTPRINT;

    if( !MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() )
        return {};

    std::optional<LIBRARY_TABLE_ROW*> row = Pgm().GetLibraryManager().GetRow( tableType, aNickname );

    if( !row )
        return {};

    std::optional<MDW_TEAM_LIBRARY> binding = MdwTeamLibrary( aType, **row );

    if( !binding )
        return {};

    return { true, Members( *binding ) };
}


void MDW_MEMBER_INDEX::Prime( const std::vector<MDW_TEAM_LIBRARY>& aBindings )
{
    for( const MDW_TEAM_LIBRARY& binding : aBindings )
        library( binding );
}


std::optional<MDW_PART_FACTS> MDW_MEMBER_INDEX::Facts( MDW_LIBRARY_TYPE aType, const LIB_ID& aId,
                                                       const wxString& aDigest )
{
    return MdwListedFacts( Listing( aType, aId.GetLibNickname() ), aId.GetUniStringLibItemName(), aDigest );
}


wxString MDW_MEMBER_INDEX::StateName( const MDW_CAPABILITIES& aCapabilities, const MDW_LIFECYCLE_STATE& aState )
{
    for( const kiapi::workspace::CustomState& state : aCapabilities.custom_states )
    {
        if( !aState.state_id.IsEmpty() && state.state_id() == aState.state_id.utf8_string() )
            return wxString::FromUTF8( state.display_name() );
    }

    return MdwCategoryName( aState.category );
}


std::shared_ptr<MDW_MEMBER_INDEX::LISTENER> MDW_MEMBER_INDEX::Listen( LISTENER aListener )
{
    // A reader that registers before any source connects must still hear when one does
    subscribe();

    auto token = std::make_shared<LISTENER>( std::move( aListener ) );
    m_listeners.push_back( token );
    return token;
}


void MDW_MEMBER_INDEX::Forget( const wxString& aLocator )
{
    forget( aLocator, wxEmptyString );
}


void MDW_MEMBER_INDEX::forget( const wxString& aLocator, const wxString& aLibraryId )
{
    DIRTY& dirty = m_dirty[aLocator];

    if( aLibraryId.IsEmpty() )
        dirty.all = true;
    else
        dirty.libraryIds.insert( aLibraryId );

    if( !wxTheApp )
    {
        relistDirty();
        return;
    }

    // A setup announces every library, so relisting per event sends libraries times events requests
    if( !m_relistQueued )
    {
        m_relistQueued = true;
        m_owner.CallAfter(
                [this]()
                {
                    relistDirty();
                } );
    }
}


void MDW_MEMBER_INDEX::relistDirty()
{
    m_relistQueued = false;

    std::map<wxString, DIRTY> dirty;
    dirty.swap( m_dirty );

    for( auto& [key, lib] : m_libraries )
    {
        auto it = dirty.find( lib->source->Info().locator );

        if( it != dirty.end() && ( it->second.all || it->second.libraryIds.count( lib->libraryId ) ) )
            relist( *lib );
    }

    for( const auto& [locator, unused] : dirty )
        relistLocks( locator );
}


void MDW_MEMBER_INDEX::relist( LIBRARY& aLibrary )
{
    // The listing in flight starts over when it answers, so nothing is cancelled and sent again meanwhile
    if( aLibrary.listing )
    {
        aLibrary.stale = true;
        return;
    }

    // What was shown stays until the new listing replaces it, so the tree does not flicker
    listMembers( aLibrary );
}


void MDW_MEMBER_INDEX::relistLocks( const wxString& aLocator )
{
    for( auto& [key, lib] : m_libraries )
    {
        if( lib->source->Info().locator != aLocator )
            continue;

        // A source may gain or lose locking across a rebind or a capability change
        if( !lib->source->Has( MDW_CAPABILITY_GROUP::LOCKING ) )
        {
            m_locks.erase( aLocator );
            applyLocks( aLocator );
            notify( aLocator, wxEmptyString );
            return;
        }

        std::unique_ptr<SOURCE_LOCKS>& locks = m_locks[aLocator];

        if( !locks )
            locks = std::make_unique<SOURCE_LOCKS>();

        listLocks( lib->source );
        return;
    }
}


void MDW_MEMBER_INDEX::subscribe()
{
    if( !m_events.IsActive() )
    {
        m_events = MDW_CLIENT::Get().Subscribe( &m_owner,
                [this]( const MDW_EVENT& aEvent )
                {
                    onEvent( aEvent );
                } );
    }

    // A library connected after the tree was built is only shown once readers look again
    if( !m_connections.IsActive() && m_subscribeConnections )
    {
        m_connections = m_subscribeConnections( &m_owner,
                [this]( const wxString& )
                {
                    notifyAll();
                } );
    }
}


void MDW_MEMBER_INDEX::rebind( const wxString& aLocator )
{
    std::shared_ptr<MDW_SOURCE> source = m_findSource ? m_findSource( aLocator ) : nullptr;

    if( !source )
        return;

    bool moved = false;

    // The old source never answers again, so each library keeps its listing on screen until the new one lands
    for( auto& [key, lib] : m_libraries )
    {
        if( lib->source != source && lib->source->Info().locator == aLocator )
        {
            lib->source = source;
            lib->stale = false;
            listMembers( *lib );
            moved = true;
        }
    }

    m_boundSources[aLocator] = source;

    if( moved )
        relistLocks( aLocator );
}


MDW_MEMBER_INDEX::LIBRARY* MDW_MEMBER_INDEX::library( const MDW_TEAM_LIBRARY& aBinding )
{
    if( !aBinding.source || !aBinding.source->Has( MDW_CAPABILITY_GROUP::SHARED_LIBRARIES ) )
        return nullptr;

    subscribe();

    const wxString locator = aBinding.source->Info().locator;
    const wxString key = keyOf( locator, aBinding.library_id, aBinding.snapshot );
    auto           it = m_libraries.find( key );

    auto       bound = m_boundSources.find( locator );
    const bool boundHere = bound != m_boundSources.end() && bound->second.lock() == aBinding.source;

    // Unbinding a project hands its locator to a library source without WORKSPACE_BOUND. Rebinding walks every
    // library, so a tree asking for each of its libraries does it only when the locator has an older source
    if( !boundHere && ( it == m_libraries.end() || it->second->source != aBinding.source ) )
        rebind( locator );

    if( it != m_libraries.end() )
        return it->second.get();

    auto lib = std::make_unique<LIBRARY>();
    lib->source = aBinding.source;
    lib->libraryId = aBinding.library_id;
    lib->snapshot = aBinding.snapshot;

    LIBRARY* raw = lib.get();
    m_libraries.emplace( key, std::move( lib ) );

    // A finder that answered with another source leaves this locator's libraries on more than one
    if( auto shared = m_boundSources.find( locator );
        shared != m_boundSources.end() && shared->second.lock() != aBinding.source )
    {
        m_boundSources.erase( shared );
    }

    listMembers( *raw );

    if( !m_locks.count( locator ) && aBinding.source->Has( MDW_CAPABILITY_GROUP::LOCKING ) )
    {
        m_locks.emplace( locator, std::make_unique<SOURCE_LOCKS>() );
        listLocks( aBinding.source );
    }

    return raw;
}


void MDW_MEMBER_INDEX::listMembers( LIBRARY& aLibrary )
{
    LIBRARY* lib = &aLibrary;

    lib->listing = true;
    lib->op = MdwReadAllPages<MDW_LIBRARY_MEMBER>(
            [lib]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_LIBRARY_MEMBER>> aDone )
            {
                return lib->source->Libraries().Members( lib->libraryId, lib->snapshot, aPage, std::move( aDone ) );
            },
            0,
            [this, lib]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_LIBRARY_MEMBER>& aRead )
            {
                // Pages read before a change may not hold it
                if( lib->stale )
                {
                    lib->stale = false;
                    listMembers( *lib );
                    return;
                }

                // A snapshot the source can no longer page through is listed again from the start
                if( aResult.error == MDW_ERROR_CODE::VERSION_CONFLICT && !aResult.IsOk() )
                {
                    listMembers( *lib );
                    return;
                }

                // Offline keeps whatever was listed before; the source's reconnect event lists again
                if( !aResult.IsOk() )
                {
                    lib->listing = false;
                    queueNotify();
                    return;
                }

                lib->members.clear();

                for( const MDW_LIBRARY_MEMBER& member : aRead.rows )
                    lib->members[member.name] = MDW_MEMBER_ENTRY{ member, MDW_LOCK_INFO() };

                lib->listed = true;
                lib->listing = false;
                m_lastListed[keyOf( lib->source->Info().locator, lib->libraryId, wxEmptyString )] = lib;
                applyLocks( *lib );
                notify( lib->source->Info().locator, lib->libraryId );
            },
            MDW_READ_CAP::FAIL, MEMBER_PAGES );
}


void MDW_MEMBER_INDEX::listLocks( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    const wxString locator = aSource->Info().locator;
    SOURCE_LOCKS*  locks = m_locks[locator].get();

    locks->listing = true;
    locks->op = MdwReadAllPages<MDW_ITEM_LOCK>(
            [aSource]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_ITEM_LOCK>> aDone )
            {
                return aSource->Locks( wxEmptyString, aPage, std::move( aDone ) );
            },
            0,
            [this, locks, locator]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_ITEM_LOCK>& aRead )
            {
                if( !aResult.IsOk() )
                {
                    locks->listing = false;
                    queueNotify();
                    return;
                }

                locks->locks.clear();

                for( const MDW_ITEM_LOCK& lock : aRead.rows )
                    locks->locks[lock.item_id] = lock.lock;

                locks->listing = false;
                applyLocks( locator );
                notify( locator, wxEmptyString );
            },
            MDW_READ_CAP::FAIL, MEMBER_PAGES );
}


void MDW_MEMBER_INDEX::applyLocks( const wxString& aLocator )
{
    for( auto& [key, lib] : m_libraries )
    {
        if( lib->source->Info().locator == aLocator )
            applyLocks( *lib );
    }
}


void MDW_MEMBER_INDEX::applyLocks( LIBRARY& aLibrary )
{
    auto it = m_locks.find( aLibrary.source->Info().locator );

    for( auto& [name, entry] : aLibrary.members )
    {
        entry.lock = MDW_LOCK_INFO();

        if( it == m_locks.end() )
            continue;

        if( auto lock = it->second->locks.find( entry.member.member_id ); lock != it->second->locks.end() )
            entry.lock = lock->second;
    }
}


void MDW_MEMBER_INDEX::onEvent( const MDW_EVENT& aEvent )
{
    switch( aEvent.type )
    {
    case MDW_EVENT_TYPE::ITEM_LOCKED:
    case MDW_EVENT_TYPE::ITEM_UNLOCKED:
        relistLocks( aEvent.source );
        break;

    // Only this event names a library, by its id
    case MDW_EVENT_TYPE::LIBRARY_CHANGED:
        forget( aEvent.source, aEvent.item_id );
        break;

    case MDW_EVENT_TYPE::REVISION_CHECKED_IN:
    case MDW_EVENT_TYPE::REVISION_CHECKED_OUT:
    case MDW_EVENT_TYPE::LIFECYCLE_CATEGORY_CHANGED:
    case MDW_EVENT_TYPE::REVISION_RELEASED:
    case MDW_EVENT_TYPE::PENDING_RESOLVED:
    case MDW_EVENT_TYPE::WORKSPACE_RECONNECTED:
    case MDW_EVENT_TYPE::CAPABILITIES_CHANGED:
        Forget( aEvent.source );
        break;

    // Reconnect brings the locator a new source, not WORKSPACE_RECONNECTED
    case MDW_EVENT_TYPE::WORKSPACE_BOUND:
        rebind( aEvent.source );
        notifyAll();
        break;

    default:
        break;
    }
}


void MDW_MEMBER_INDEX::notify( const wxString& aLocator, const wxString& aLibraryId )
{
    m_changes.libraries.emplace( aLocator, aLibraryId );
    queueNotify();
}


void MDW_MEMBER_INDEX::notifyAll()
{
    m_changes.all = true;
    queueNotify();
}


bool MDW_MEMBER_INDEX::listingInFlight() const
{
    auto inFlight = []( const auto& aEntry )
    {
        return aEntry.second && aEntry.second->listing;
    };

    return std::ranges::any_of( m_libraries, inFlight ) || std::ranges::any_of( m_locks, inFlight );
}


void MDW_MEMBER_INDEX::queueNotify()
{
    if( m_changes.Empty() )
        return;

    if( !wxTheApp )
    {
        flushNotify();
        return;
    }

    // Listeners redraw whole trees and listings land one per event, so a setup is heard once its last one lands
    if( !m_shutDown && listingInFlight() )
    {
        if( !m_notifyTimer )
        {
            m_notifyTimer = std::make_unique<wxTimer>();
            m_notifyTimer->Bind( wxEVT_TIMER,
                                 [this]( wxTimerEvent& )
                                 {
                                     flushNotify();
                                 } );
        }

        // A stalled listing would otherwise hold the others until it answers or times out
        if( !m_notifyTimer->IsRunning() )
            m_notifyTimer->StartOnce( NOTIFY_HOLD_MS );

        return;
    }

    if( !m_notifyQueued )
    {
        m_notifyQueued = true;
        m_owner.CallAfter(
                [this]()
                {
                    flushNotify();
                } );
    }
}


void MDW_MEMBER_INDEX::flushNotify()
{
    m_notifyQueued = false;

    if( m_notifyTimer )
        m_notifyTimer->Stop();

    MDW_MEMBER_CHANGES changes;
    std::swap( changes, m_changes );

    // The hold timer and a queued call can both fire for one burst
    if( changes.Empty() )
        return;

    wxLogTrace( traceMdw, wxS( "Member index notifies %zu libraries%s" ), changes.libraries.size(),
                changes.all ? wxS( " and all" ) : wxS( "" ) );

    std::erase_if( m_listeners,
                   []( const std::weak_ptr<LISTENER>& aListener )
                   {
                       return aListener.expired();
                   } );

    // A listener may register another, so run a copy
    std::vector<std::weak_ptr<LISTENER>> listeners = m_listeners;

    for( const std::weak_ptr<LISTENER>& listener : listeners )
    {
        if( std::shared_ptr<LISTENER> live = listener.lock() )
            ( *live )( changes );
    }
}


std::optional<MDW_PART_FACTS> MDW_MEMBER_INDEX::LatestFacts( MDW_LIBRARY_TYPE aType, const LIB_ID& aId )
{
    return MdwListedLatestFacts( Listing( aType, aId.GetLibNickname() ), aId.GetUniStringLibItemName() );
}
