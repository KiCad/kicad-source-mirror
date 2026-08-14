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

/**
 * @file mdw_member_index.h
 * @brief The members of the open team libraries, as their sources last listed them.
 *
 * Library editors, the library tree's Lifecycle column and Assign Part need a member's item id and state
 * the moment they draw, so they read this cache synchronously. A library not yet listed answers nothing and
 * starts a listing; listeners hear when it lands. A source's events drop what it told the cache before.
 * UI thread only.
 */

#ifndef MDW_MEMBER_INDEX_H
#define MDW_MEMBER_INDEX_H

#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_events.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>

#include <wx/event.h>
#include <wx/string.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

class LIB_ID;
class LIBRARY_TABLE_ROW;
class wxTimer;
class wxWindow;
class MDW_LIBRARY_SOURCES;
class MDW_SOURCE;


/// Where one library table row's members live
struct KICOMMON_API MDW_TEAM_LIBRARY
{
    std::shared_ptr<MDW_SOURCE> source;
    wxString                    library_id;
    wxString                    snapshot;       ///< What the local copy holds; empty for the latest
    wxString                    local_root;
};


/**
 * The binding behind the aType library table row aRow; nothing for a row no source serves. The row's marker and
 * expanded URI name the library, so a renamed row still resolves.
 */
KICOMMON_API std::optional<MDW_TEAM_LIBRARY> MdwTeamLibrary( MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow );


/// MdwTeamLibrary for the row with options aRowOptions and expanded URI aRowPath, with its settings, sources and
/// source lookup given rather than the process-wide ones
KICOMMON_API std::optional<MDW_TEAM_LIBRARY>
MdwTeamLibrary( bool aEnabled, const MDW_LIBRARY_SOURCES& aSources,
                const std::function<std::shared_ptr<MDW_SOURCE>( const wxString& )>& aFindSource,
                MDW_LIBRARY_TYPE aType, const wxString& aRowOptions, const wxString& aRowPath );


/// True when a save into a team library copy from aSource, null without a session, stays on this machine until it
/// is checked in. Only a logged-in source that takes no check-ins or has a lifecycle to release through says no.
KICOMMON_API bool MdwSaveNeedsCheckIn( const std::shared_ptr<MDW_SOURCE>& aSource );


/// Warn over aParent that a save into the aType library table row aRow stays on this machine until it is checked
/// in, when MdwSaveNeedsCheckIn holds. Shown once per session for each library. UI thread only.
KICOMMON_API void MdwWarnLocalSave( wxWindow* aParent, MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow );


/// Keeps MdwWarnLocalSave silent while it lives, for a write the user did not ask for as a save, such as one an
/// API client makes with nobody at the window, one a check-in follows at once, or a new footprint written to its
/// library before it is edited. A silenced write leaves the library's warning for the next save the user makes.
class KICOMMON_API MDW_SILENT_SAVE
{
public:
    MDW_SILENT_SAVE();
    ~MDW_SILENT_SAVE();

    MDW_SILENT_SAVE( const MDW_SILENT_SAVE& ) = delete;
    MDW_SILENT_SAVE& operator=( const MDW_SILENT_SAVE& ) = delete;

    static bool Active();
};


/// The reference to aMember of aBinding's library that its source's requests take
KICOMMON_API MDW_ITEM_REF MdwMemberRef( const MDW_TEAM_LIBRARY& aBinding, const MDW_LIBRARY_MEMBER& aMember );


struct KICOMMON_API MDW_MEMBER_ENTRY
{
    MDW_LIBRARY_MEMBER member;
    MDW_LOCK_INFO      lock;
};


/// What the index holds of one library
struct KICOMMON_API MDW_LIBRARY_LISTING
{
    bool                                        team = false;
    const std::map<wxString, MDW_MEMBER_ENTRY>* members = nullptr; ///< By name, null until the library is listed
};


/**
 * What aListing says of the member aName placed or chosen with the content aDigest. It weighs the member's releases
 * as the placed-part check of Release Design does. Nothing for a library that is no team's. A library not listed
 * yet is UNVERIFIED, and a member the listing lacks is UNKNOWN, never checked in. An empty aDigest knows nothing of
 * the part and is UNKNOWN, as in the runtime.
 */
KICOMMON_API std::optional<MDW_PART_FACTS> MdwListedFacts( const MDW_LIBRARY_LISTING& aListing, const wxString& aName,
                                                           const wxString& aDigest );


/// MdwListedFacts for a part only chosen, which stands for the member's newest release
KICOMMON_API std::optional<MDW_PART_FACTS> MdwListedLatestFacts( const MDW_LIBRARY_LISTING& aListing,
                                                                 const wxString&            aName );


/**
 * True when a release of a symbol that names a footprint with aFacts needs the user's say-so, since the footprint was
 * never released or could not be checked. Other conditions only get a note.
 */
KICOMMON_API bool MdwReleaseNeedsConfirmation( const MDW_PART_FACTS& aFacts );


/// The libraries whose listings changed since listeners last heard
struct KICOMMON_API MDW_MEMBER_CHANGES
{
    bool                                    all = false; ///< Any row may bind or list differently
    std::set<std::pair<wxString, wxString>> libraries;   ///< Source locator and library id, empty id for all

    bool Has( const wxString& aLocator, const wxString& aLibraryId ) const;
    bool Empty() const { return !all && libraries.empty(); }
};


class KICOMMON_API MDW_MEMBER_INDEX
{
public:
    using FIND_SOURCE = std::function<std::shared_ptr<MDW_SOURCE>( const wxString& aLocator )>;
    using CONNECTIONS = std::function<MDW_SUBSCRIPTION( wxEvtHandler* aOwner,
                                                         std::function<void( const wxString& )> aChanged )>;

    static MDW_MEMBER_INDEX& Get();

    /// Drop the hold timer while wx can still stop it. Does nothing when Get() never made the index.
    static void Shutdown();

    /// Finds sources and hears of connections through MDW_CLIENT
    MDW_MEMBER_INDEX();

    MDW_MEMBER_INDEX( FIND_SOURCE aFindSource, CONNECTIONS aConnections );
    ~MDW_MEMBER_INDEX();

    /// aName's entry in what Members() lists
    std::optional<MDW_MEMBER_ENTRY> Find( const MDW_TEAM_LIBRARY& aBinding, const wxString& aName );

    /**
     * Every member of aBinding's library by name. While its snapshot is being listed, the library's last listing
     * at another snapshot, or nothing when there is none.
     */
    const std::map<wxString, MDW_MEMBER_ENTRY>* Members( const MDW_TEAM_LIBRARY& aBinding );

    /// What the index holds of the aType library nicknamed aNickname, which starts its listing
    MDW_LIBRARY_LISTING Listing( MDW_LIBRARY_TYPE aType, const wxString& aNickname );

    /// Starts the listing of each of aBindings' libraries, so a reader that comes later finds it landed
    void Prime( const std::vector<MDW_TEAM_LIBRARY>& aBindings );

    /// MdwListedFacts for aId, nothing when its library is no team's or team libraries are off
    std::optional<MDW_PART_FACTS> Facts( MDW_LIBRARY_TYPE aType, const LIB_ID& aId, const wxString& aDigest );

    /// MdwListedLatestFacts for aId, nothing when its library is no team's or team libraries are off
    std::optional<MDW_PART_FACTS> LatestFacts( MDW_LIBRARY_TYPE aType, const LIB_ID& aId );

    /// The display name of a member's state, from its source's advertised capabilities
    static wxString StateName( const MDW_CAPABILITIES& aCapabilities, const MDW_LIFECYCLE_STATE& aState );

    using LISTENER = std::function<void( const MDW_MEMBER_CHANGES& aChanges )>;

    /// aListener runs once after a burst of listings lands, for as long as the returned token is held
    [[nodiscard]] std::shared_ptr<LISTENER> Listen( LISTENER aListener );

    /// Forget everything aLocator's source said, as after it changed the local copy. The relist runs once
    /// for a whole burst of calls, from the event queue, and what was listed stays readable until it lands.
    void Forget( const wxString& aLocator );

private:
    struct LIBRARY
    {
        std::shared_ptr<MDW_SOURCE>          source;
        wxString                             libraryId;
        wxString                             snapshot;
        bool                                 listed = false;
        bool                                 listing = false; ///< A listing is in flight
        bool                                 stale = false;   ///< Changed since the listing in flight began
        std::map<wxString, MDW_MEMBER_ENTRY> members;
        MDW_OP                               op;
    };

    struct SOURCE_LOCKS
    {
        std::map<wxString, MDW_LOCK_INFO> locks;            ///< By item id
        bool                              listing = false;  ///< A listing is in flight
        MDW_OP                            op;
    };

    /// What a burst of events left to relist for one source
    struct DIRTY
    {
        bool               all = false;
        std::set<wxString> libraryIds;
    };

    void     subscribe();
    void     rebind( const wxString& aLocator );
    LIBRARY* library( const MDW_TEAM_LIBRARY& aBinding );
    void     forget( const wxString& aLocator, const wxString& aLibraryId );
    void     relistDirty();
    void     relist( LIBRARY& aLibrary );
    void     listMembers( LIBRARY& aLibrary );
    void     listLocks( const std::shared_ptr<MDW_SOURCE>& aSource );
    void     relistLocks( const wxString& aLocator );
    void     applyLocks( const wxString& aLocator );
    void     applyLocks( LIBRARY& aLibrary );
    void     onEvent( const MDW_EVENT& aEvent );
    bool     listingInFlight() const;
    void     notify( const wxString& aLocator, const wxString& aLibraryId );
    void     notifyAll();
    void     queueNotify();
    void     flushNotify();

    wxEvtHandler                                         m_owner;
    std::map<wxString, std::unique_ptr<LIBRARY>>         m_libraries;
    std::map<wxString, LIBRARY*>                         m_lastListed; ///< Each library's newest listed snapshot
    std::map<wxString, std::unique_ptr<SOURCE_LOCKS>>    m_locks;      ///< By source locator
    std::map<wxString, std::weak_ptr<MDW_SOURCE>>        m_boundSources; ///< The one source of each locator's libraries
    std::map<wxString, DIRTY>                            m_dirty;      ///< By source locator
    bool                                                 m_relistQueued = false;
    MDW_MEMBER_CHANGES                                   m_changes;    ///< Not yet heard by listeners
    bool                                                 m_notifyQueued = false;
    bool                                                 m_shutDown = false;
    std::unique_ptr<wxTimer>                             m_notifyTimer; ///< Ends a hold a stalled listing would keep
    std::vector<std::weak_ptr<LISTENER>>                 m_listeners;
    MDW_SUBSCRIPTION                                     m_events;
    MDW_SUBSCRIPTION                                     m_connections;
    FIND_SOURCE                                          m_findSource;
    CONNECTIONS                                          m_subscribeConnections;
};

#endif // MDW_MEMBER_INDEX_H
