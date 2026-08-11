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

#include <kiface_services.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_backend.h>
#include <mdw/mdw_backend_registry.h>
#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_session.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <thread_pool.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>

#include "client/mdw_call.h"

#include <diff_merge/kicad_diff_types.h>
#include <git/kigit_platform.h>
#include <git/kigit_symbol_split.h>
#include <gestfich.h>
#include <kiplatform/io.h>
#include <mdw_runtime/mdw_paths.h>
#include <mdw_runtime/mdw_staged_folder.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <tuple>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/intl.h>

namespace
{

constexpr int CONNECT_TIMEOUT_MS = 8000;

// Spans the login and a retry after trusting a host, so a test never outlasts what a person will wait
constexpr int TEST_DEADLINE_MS = 20000;

// What a stopped test waits for its backend to fail the call in flight
constexpr int TEST_STOP_GRACE_MS = 6000;

// A library's changes are listed in full, so only a backend that never ends its pages reaches this
constexpr int CHANGE_PAGES = 1000;

// The launch and the login each give up within CONNECT_TIMEOUT_MS, so only a hung open reaches this
constexpr int OPEN_DEADLINE_MS = 3 * CONNECT_TIMEOUT_MS;

// Spans the longest respawn backoff and the new backend's handshake and login
constexpr int RESPAWN_WAIT_MS = 30000;

/// Setup works before the workspace preference is turned on, which is often why it is being opened. Leased,
/// since turning integration off destroys the framework while setup may still be using it
MDW_FRAMEWORK_LEASE framework()
{
    PGM_BASE* pgm = PgmOrNull();

    if( !pgm )
        return {};

    pgm->StartMdwFramework();
    return pgm->LeaseMdwFramework( false );
}


/// The framework already running, for calls on a session it holds, which none started now would
MDW_FRAMEWORK_LEASE runningFramework()
{
    PGM_BASE* pgm = PgmOrNull();

    return pgm ? pgm->LeaseMdwFramework( false ) : MDW_FRAMEWORK_LEASE();
}


/// Where a library sits inside its copy, or empty when the backend named a place outside it
wxString libraryPath( const wxString& aLocalRoot, const wxString& aRelativePath )
{
    wxFileName path;

    // Protocol paths use '/', so a backslash or a drive can only be an attempt to leave the copy
    if( aRelativePath.find_first_of( wxS( "\\:" ) ) != wxString::npos
        || !WX_FILENAME::ResolveArchiveEntryPath( aLocalRoot, aRelativePath, path ) )
    {
        return wxEmptyString;
    }

    return path.GetFullPath();
}


/// The file every writer of the copy at aLocalRoot locks
wxString copyLockPath( const wxString& aLocalRoot )
{
    // The lock sits in the reserved .kicad folder, where no source's copy can be
    wxFileName lockFile( aLocalRoot + wxS( ".lock" ) );

    lockFile.AppendDir( wxS( ".kicad" ) );

    if( !lockFile.DirExists() )
        wxFileName::Mkdir( lockFile.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    return lockFile.GetFullPath();
}


/// Take the lock every writer of the copy at aLocalRoot takes; false while another process holds it
bool lockCopy( const wxString& aLocalRoot, KIPLATFORM::IO::FILE_LOCK& aGuard )
{
    bool created = false;

    return aGuard.Acquire( copyLockPath( aLocalRoot ), created ) == KIPLATFORM::IO::FILE_LOCK::STATE::HELD;
}


/// lockCopy, waiting out another writer for as long as the user can stop the wait
MDW_RESULT waitForCopy( const wxString& aLocalRoot, const wxString& aName, KIPLATFORM::IO::FILE_LOCK& aGuard )
{
    using STATE = KIPLATFORM::IO::FILE_LOCK::STATE;

    const wxString path = copyLockPath( aLocalRoot );
    bool           created = false;
    STATE          state = aGuard.Acquire( path, created );

    while( state == STATE::BUSY && MDW_CANCEL_SCOPE::IsOpen() )
    {
        if( MDW_CANCEL_SCOPE::Requested() )
            return MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );

        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        state = aGuard.Acquire( path, created );
    }

    if( state != STATE::HELD )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BUSY,
                                 wxString::Format( _( "'%s' is already being updated." ), aName ) );
    }

    return MDW_RESULT::Ok();
}


/// The marker the rows of aCopy carry
wxString markerOf( const MDW_LIBRARY_COPY& aCopy )
{
    MDW_LIBRARY_SOURCE_CONFIG owner;
    owner.id = aCopy.source_id;
    owner.backend_id = aCopy.backend_id;
    owner.locator = aCopy.locator;

    return MDW_LIBRARY_SOURCES::Marker( owner, aCopy.pinned ? aCopy.snapshot : MDW_SNAPSHOT() );
}


/// A change the backend applied, under the item id and revision its reply gave
struct APPLIED_CHANGE
{
    const MDW_LOCAL_CHANGE* change;
    wxString                item_id;
    wxString                revision_id;
};


/// Record in aCopy's index what the applied changes made of their members
void recordCheckIns( MDW_LIBRARY_COPY& aCopy, const std::vector<APPLIED_CHANGE>& aApplied )
{
    // Every member leaves before any arrives, so an add into a name a removal vacates keeps the new member
    for( const APPLIED_CHANGE& applied : aApplied )
    {
        const MDW_LOCAL_CHANGE& change = *applied.change;

        const bool leaves = change.change == MDW_CHANGE_TYPE::REMOVED;
        const bool arrives = change.change == MDW_CHANGE_TYPE::ADDED;

        std::erase_if( aCopy.members,
                       [&]( const MDW_COPY_MEMBER& aMember )
                       {
                           return ( leaves && aMember.member_id == change.member.item_id )
                                  || ( arrives && aMember.library_id == change.library_id
                                       && aMember.name == change.name );
                       } );
    }

    for( const APPLIED_CHANGE& applied : aApplied )
    {
        const MDW_LOCAL_CHANGE& change = *applied.change;

        if( change.change == MDW_CHANGE_TYPE::ADDED )
        {
            aCopy.members.push_back(
                    { applied.item_id, change.library_id, change.name, change.relative_path, applied.revision_id } );
            continue;
        }

        if( change.change != MDW_CHANGE_TYPE::MODIFIED )
            continue;

        // The listed id, which a reply may have replaced with the member's canonical one
        for( MDW_COPY_MEMBER& member : aCopy.members )
        {
            if( member.member_id == change.member.item_id )
            {
                member.member_id = applied.item_id;
                member.checked_in = applied.revision_id;
            }
        }
    }
}


/**
 * Add to aResolutions each of aConflicts that is only the copy's own check-in coming back, since its file
 * already holds theirs. False when none was added, so an update retries only while it makes progress
 */
bool resolveCheckedIn( const MDW_LIBRARY_COPY& aCopy, const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                       std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions )
{
    bool added = false;

    for( const MDW_UPDATE_CONFLICT& conflict : aConflicts )
    {
        bool own = std::any_of( aCopy.members.begin(), aCopy.members.end(),
                                [&]( const MDW_COPY_MEMBER& aMember )
                                {
                                    return aMember.member_id == conflict.item.item_id && !aMember.checked_in.IsEmpty()
                                           && aMember.checked_in == conflict.theirs_revision_id;
                                } );

        bool resolved = std::any_of( aResolutions.begin(), aResolutions.end(),
                                     [&]( const MDW_CONFLICT_RESOLUTION& aResolution )
                                     {
                                         return aResolution.item_id == conflict.item.item_id
                                                && aResolution.theirs_revision_id == conflict.theirs_revision_id;
                                     } );

        if( own && !resolved )
        {
            aResolutions.push_back( { conflict.item.item_id, conflict.theirs_revision_id, std::nullopt, wxEmptyString,
                                      wxEmptyString } );
            added = true;
        }
    }

    return added;
}


void announce( const std::shared_ptr<MDW_SOURCE>& aSource, const std::set<wxString>& aLibraryIds )
{
    std::shared_ptr<MDW_SESSION> session = aSource->Session();

    if( !session || !session->Service() )
        return;

    // A local announcement, so no echo is expected for it
    MDW_TRACKED_OP op;
    op.service = session->Service();
    op.id = MdwNewOperationId();

    for( const wxString& libraryId : aLibraryIds )
        op.Emit( MDW_EVENT_TYPE::LIBRARY_CHANGED, libraryId );
}


/// Bring the copy of aConfig at aPin up to date, rewrite its index and announce each library changed
MDW_LIBRARY_SYNC_RESULT syncLibraryCopy( MDW_LIBRARY_SOURCES& aSources, const std::shared_ptr<MDW_SOURCE>& aSource,
                                         const MDW_LIBRARY_SOURCE_CONFIG& aConfig, const MDW_SNAPSHOT& aPin,
                                         const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions )
{
    MDW_LIBRARY_SYNC_RESULT sync;

    std::shared_ptr<MDW_SESSION> session = aSource ? aSource->Session() : nullptr;

    // One locator can name sources on two backends, and each keeps its own copy
    if( !aSource || aSource->Info().locator != aConfig.locator
        || ( session && session->Binding().backend_id != aConfig.backend_id ) )
    {
        sync.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
        return sync;
    }

    const bool     pinned = !aPin.token.IsEmpty();
    const wxString localRoot = aSources.LocalRoot( aConfig, aPin );

    // Another sync of this copy in any process would leave its files and index disagreeing
    KIPLATFORM::IO::FILE_LOCK guard;

    if( MDW_RESULT locked = waitForCopy( localRoot, aConfig.name, guard ); !locked.IsOk() )
    {
        sync.result = locked;
        return sync;
    }

    // A set-up rolled back while this one waited may have removed the source, and its copy must not come back
    std::optional<MDW_LIBRARY_SOURCE_CONFIG> current = aSources.Find( aConfig.id );

    if( !current || current->backend_id != aConfig.backend_id || current->locator != aConfig.locator
        || current->name != aConfig.name || current->prefix != aConfig.prefix )
    {
        sync.result = MDW_RESULT::Fail(
                MDW_ERROR_CODE::ITEM_NOT_FOUND,
                wxString::Format( _( "'%s' was removed while this set-up waited. Set it up again." ), aConfig.name ) );
        return sync;
    }

    std::optional<MDW_LIBRARY_COPY> existing = aSources.Copy( MDW_LIBRARY_SOURCES::Marker( aConfig, aPin ) );
    MDW_LIBRARY                     library = aSource->Libraries();
    MDW_LIBRARY_LIST                list;
    std::set<wxString>              changedIds;

    if( existing )
    {
        for( const MDW_COPY_LIBRARY& previous : existing->libraries )
            changedIds.insert( previous.library_id );
    }
    else if( !wxFileName::DirExists( localRoot ) )
    {
        // Decided under the lock, since a copy another process writes first is not this set-up's to roll back
        sync.created = MDW_CREATED_COPY{ aPin, std::nullopt };
    }

    bool filesChanged = false;

    auto writeIndex = [&]( const MDW_LIBRARY_COPY& aCopy )
    {
        wxString generation;

        sync.result = aSources.WriteCopy( aCopy, &generation );

        if( sync.result.IsOk() && sync.created )
            sync.created->indexed = generation;
    };

    auto run = [&]()
    {
        // A pinned copy never changes once written, but a fresh generation tells a set-up that wrote it and
        // now rolls back that this one took it up
        if( pinned && existing && existing->snapshot.token == aPin.token )
        {
            wxString generation;

            sync.copy = *existing;
            sync.result = aSources.WriteCopy( sync.copy, &generation );
            sync.copy.generation = generation;
            return;
        }

        // A folder no index vouches for is updated, never written over, so only the backend that made it takes it
        const bool adopting = !pinned && !existing && wxFileName::DirExists( localRoot );

        if( !pinned && ( existing || adopting ) )
        {
            MDW_LIBRARY_COPY                     previous = existing.value_or( MDW_LIBRARY_COPY() );
            std::vector<MDW_CONFLICT_RESOLUTION> resolutions = aResolutions;
            MDW_RESULT                           updated;
            MDW_LIBRARY_UPDATE                   update;

            previous.local_root = localRoot;

            do
            {
                std::tie( updated, update ) = MdwSync<MDW_LIBRARY_UPDATE>(
                        [&]( MDW_DONE<MDW_LIBRARY_UPDATE> aDone )
                        {
                            return library.Update( localRoot, resolutions, std::move( aDone ) );
                        } );
            } while( updated.IsOk() && resolveCheckedIn( previous, update.conflicts, resolutions ) );

            sync.result = updated;
            sync.changes = update.changes;
            sync.conflicts = update.conflicts;

            // The backend refuses a folder that is not its copy, which may hold someone's work
            if( adopting && updated.error == MDW_ERROR_CODE::INVALID_PARAMETER )
            {
                sync.result =
                        MDW_RESULT::Fail( MDW_ERROR_CODE::VERSION_CONFLICT,
                                          wxString::Format( _( "'%s' already exists. Move it aside to set up '%s'." ),
                                                            localRoot, aConfig.name ) );
            }

            if( !sync.result.IsOk() || !update.conflicts.empty() )
            {
                sync.copy = previous;
                return;
            }

            filesChanged = true;
            sync.filesChanged = !update.changes.empty();
            list = { update.libraries, update.snapshot, update.members };
        }
        else
        {
            auto [written, materialized] = MdwSync<MDW_LIBRARY_LIST>(
                    [&]( MDW_DONE<MDW_LIBRARY_LIST> aDone )
                    {
                        return library.Materialize( localRoot, aPin.token, std::move( aDone ) );
                    } );

            sync.result = written;

            if( !written.IsOk() )
                return;

            filesChanged = true;
            sync.filesChanged = true;

            list = materialized;
        }

        // Every library the files now hold, before any later step can fail
        for( const MDW_LIBRARY_INFO& info : list.libraries )
            changedIds.insert( info.library_id );

        MDW_LIBRARY_COPY& copy = sync.copy;
        copy.source_id = aConfig.id;
        copy.backend_id = aConfig.backend_id;
        copy.locator = aConfig.locator;
        copy.local_root = localRoot;
        copy.snapshot = list.snapshot;
        copy.pinned = pinned;

        std::set<wxString> registered;

        for( const MDW_LIBRARY_INFO& info : list.libraries )
        {
            if( info.type == MDW_LIBRARY_TYPE::UNKNOWN || libraryPath( localRoot, info.relative_path ).IsEmpty() )
                continue;

            copy.libraries.push_back(
                    { info.library_id, aConfig.prefix + info.display_name, info.type, info.relative_path } );
            registered.insert( info.library_id );
        }

        for( const MDW_LIBRARY_MEMBER& member : list.members )
        {
            if( registered.contains( member.library_id ) )
                copy.members.push_back( { member.member_id, member.library_id, member.name, member.path_hint, {} } );
        }

        writeIndex( copy );
    };

    run();

    // The source's session may have started before this copy existed, when its backend could list no claims
    if( sync.result.IsOk() && !existing && !pinned && session )
        session->RefreshLocks();

    // Whatever failed after the files changed, readers of every library touched must look again
    if( filesChanged )
        announce( aSource, changedIds );

    return sync;
}


/// Why aSource is not connected, without trying a refused login again, which could lock the user out
MDW_RESULT notConnected( const MDW_SOURCE& aSource )
{
    const MDW_RESULT reason = aSource.Info().connection.reason;

    return reason.IsOk() ? MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_AUTHENTICATED ) : reason;
}


/// Whether aSource is online, asking an offline server again first, since the probe backoff can leave a server
/// that came back unasked for a minute
bool online( const MDW_SOURCE& aSource )
{
    std::shared_ptr<MDW_SESSION> session = aSource.Session();

    if( session && session->Status().phase == MDW_CONNECTION::OFFLINE )
        session->Probe( CONNECT_TIMEOUT_MS );

    return aSource.Info().connection.phase == MDW_CONNECTION::ONLINE;
}


/// aRun, and once more after a respawn replaced the backend that failed it. A backend killed while idle fails the
/// first call sent to it, which the user would otherwise have to repeat
template <typename RESULT>
RESULT afterRespawn( const std::shared_ptr<MDW_SOURCE>& aSource, const std::function<RESULT()>& aRun )
{
    std::shared_ptr<MDW_SESSION> session = aSource ? aSource->Session() : nullptr;
    const std::uint64_t          generation = session ? session->BackendGeneration() : 0;
    RESULT                       result = aRun();

    if( !session || result.result.error != MDW_ERROR_CODE::BACKEND_LOST )
        return result;

    if( session->AwaitRespawn( generation, std::chrono::milliseconds( RESPAWN_WAIT_MS ) ) )
        return aRun();

    // A cancel ended the wait, and the lost backend is not what the user should hear about
    if( MDW_CANCEL_SCOPE::Requested() )
        result.result = MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );

    return result;
}


/**
 * For each of aResults, the index in aChanges of the change it answers. A result names its member by library and
 * name, by item id only when it has no name, and each change answers one result at most
 */
std::vector<std::optional<size_t>> matchResults( const std::vector<MDW_LOCAL_CHANGE>&  aChanges,
                                                 const std::vector<MDW_MEMBER_RESULT>& aResults )
{
    std::vector<std::optional<size_t>> matches( aResults.size() );
    std::vector<bool>                  claimed( aChanges.size(), false );

    for( size_t r = 0; r < aResults.size(); ++r )
    {
        const MDW_MEMBER_RESULT& result = aResults[r];

        for( size_t c = 0; c < aChanges.size(); ++c )
        {
            const MDW_LOCAL_CHANGE& change = aChanges[c];

            // An item id can name one member before a move and another after it, so the name decides
            bool answers = result.name.IsEmpty() ? change.member.item_id == result.item_id
                                                 : change.library_id == result.library_id && change.name == result.name;

            if( !claimed[c] && answers )
            {
                claimed[c] = true;
                matches[r] = c;
                break;
            }
        }
    }

    return matches;
}


/// The check-in permission each source last answered, by locator, for a menu that cannot wait for one
struct KNOWN_CHECKIN
{
    std::mutex                                       mutex;
    std::map<wxString, kiapi::workspace::Permission> permissions;
};


KNOWN_CHECKIN& knownCheckIn()
{
    static KNOWN_CHECKIN known;
    return known;
}


/// Whether aSource lets this user check in at all, as far as it knows
kiapi::workspace::Permission checkInPermission( MDW_SOURCE& aSource )
{
    kiapi::workspace::Permission written;

    if( !aSource.Has( MDW_CAPABILITY_GROUP::PERMISSIONS ) )
        return written;

    auto [result, permissions] = MdwSync<std::vector<kiapi::workspace::Permission>>(
            [&]( MDW_DONE<std::vector<kiapi::workspace::Permission>> aDone )
            {
                return aSource.Permissions( MDW_ITEM_REF(), wxEmptyString, std::move( aDone ) );
            } );

    for( const kiapi::workspace::Permission& permission : permissions )
    {
        if( result.IsOk() && permission.name() == "checkin" )
            written = permission;
    }

    if( result.IsOk() )
    {
        std::lock_guard<std::mutex> lock( knownCheckIn().mutex );
        knownCheckIn().permissions[aSource.Info().locator] = written;
    }

    return written;
}


/// The symbol library files a listing read, by local path
struct SYMBOL_FILES
{
    std::map<wxString, std::optional<KIGIT::SYMBOL_FILE>> split;
    std::map<wxString, std::string>                       texts;

    /// aLocal split into its symbols, read only once
    const std::optional<KIGIT::SYMBOL_FILE>& Split( const wxString& aLocal )
    {
        if( !split.contains( aLocal ) )
        {
            std::ifstream in( ToFsPath( aLocal ), std::ios::binary );

            if( in.is_open() )
            {
                texts[aLocal] =
                        std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
                split[aLocal] = KIGIT::SplitSymbols( texts[aLocal] );
            }
            else
            {
                split[aLocal] = std::nullopt;
            }
        }

        return split[aLocal];
    }
};


/// Set aRewriteOnly when aChange only reformatted a symbol, as a format upgrade does; returns the download result
MDW_RESULT rewriteOnly( MDW_SOURCE& aSource, const MDW_LIBRARY_COPY& aCopy, const MDW_LOCAL_CHANGE& aChange,
                        const KIGIT::LIBRARY_DIFF& aDiff, const wxString& aFolder, SYMBOL_FILES& aFiles,
                        bool& aRewriteOnly )
{
    const wxString local = libraryPath( aCopy.local_root, aChange.relative_path );

    aRewriteOnly = false;

    if( aChange.change != MDW_CHANGE_TYPE::MODIFIED || aChange.base_revision_id.IsEmpty() || local.IsEmpty()
        || !local.EndsWith( wxS( ".kicad_sym" ) ) )
    {
        return MDW_RESULT::Ok();
    }

    auto readText = []( const wxString& aPath ) -> std::optional<std::string>
    {
        std::ifstream in( ToFsPath( aPath ), std::ios::binary );

        if( !in.is_open() )
            return std::nullopt;

        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    const std::optional<KIGIT::SYMBOL_FILE>& file = aFiles.Split( local );

    if( !file )
        return MDW_RESULT::Ok();

    auto span = file->spans.find( aChange.name.ToStdString( wxConvUTF8 ) );

    // Neither side of a derived symbol can be extracted without its parent
    if( span == file->spans.end() || span->second.derived )
        return MDW_RESULT::Ok();

    const wxString base = aFolder + wxFILE_SEP_PATH + wxS( "base.kicad_sym" );

    auto [downloaded, revision] = MdwSync<MDW_REVISION>(
            [&]( MDW_DONE<MDW_REVISION> aDone )
            {
                return aSource.Download( aChange.member, aChange.base_revision_id, base, std::move( aDone ) );
            } );

    if( !downloaded.IsOk() )
        return downloaded;

    std::optional<std::string> before = readText( base );

    if( !before )
        return MDW_RESULT::Ok();

    const std::string after =
            file->header + aFiles.texts[local].substr( span->second.start, span->second.length ) + "\n)\n";
    KICAD_DIFF::DOCUMENT_DIFF diff;

    aRewriteOnly = aDiff( *before, after, &diff ) && diff.changes.empty();
    return MDW_RESULT::Ok();
}


/// Every local change under aCopy, and those aSource may check in
MDW_LIBRARY_CHANGES_RESULT libraryChanges( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_LIBRARY_COPY& aCopy,
                                           const KIGIT::LIBRARY_DIFF& aDiffSymbols )
{
    MDW_LIBRARY_CHANGES_RESULT listed;

    if( !aSource || aSource->Info().locator != aCopy.locator )
    {
        listed.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
        return listed;
    }

    // Empty rows would read as nothing new rather than signed out
    if( !online( *aSource ) )
    {
        listed.result = notConnected( *aSource );
        return listed;
    }

    // Asked each time, since a push since the last list may have taught the source more
    const kiapi::workspace::Permission written = checkInPermission( *aSource );

    MDW_LIBRARY library = aSource->Libraries();

    auto [result, changes] = MdwSync<MDW_PAGED<MDW_LOCAL_CHANGE>>(
            [&]( MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone )
            {
                return MdwReadAllPages<MDW_LOCAL_CHANGE>(
                        [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aPageDone )
                        {
                            return library.LocalChanges( aCopy.local_root, aPage, std::move( aPageDone ) );
                        },
                        0, std::move( aDone ), MDW_READ_CAP::FAIL, CHANGE_PAGES );
            } );

    listed.result = result;

    if( !listed.result.IsOk() )
        return listed;

    listed.changes = std::move( changes.rows );

    SCOPED_TEMP_PATH folder;

    // Saving one symbol rewrites a whole library in the current format, which no one changed
    if( aDiffSymbols && folder.MakeDirectory( wxS( "kicad_libcheckin_" ), 0700 ) )
    {
        SYMBOL_FILES                         files;
        std::vector<MDW_LOCAL_CHANGE>        kept;
        std::map<wxString, MDW_LOCAL_CHANGE> reformatted;
        std::set<wxString>                   edited;

        for( MDW_LOCAL_CHANGE& change : listed.changes )
        {
            bool       unchanged = false;
            MDW_RESULT downloaded =
                    rewriteOnly( *aSource, aCopy, change, aDiffSymbols, folder.Path(), files, unchanged );

            // Kept as an edit, a member whose base never arrived would open the dialog after a cancel
            if( !downloaded.IsOk() )
            {
                listed.result = downloaded;
                listed.changes.clear();
                return listed;
            }

            change.moved = unchanged && !change.base_relative_path.IsEmpty()
                           && change.base_relative_path != change.relative_path;

            if( unchanged )
                reformatted.try_emplace( change.relative_path, change );

            if( !unchanged || change.moved )
            {
                edited.insert( change.relative_path );
                edited.insert( change.base_relative_path );
                kept.push_back( std::move( change ) );
            }
        }

        // Edits saved in a newer format than the team's file would be spliced under its header, which is refused,
        // so their file goes whole with them like any other re-save
        for( const MDW_LOCAL_CHANGE& change : kept )
        {
            const wxString& file =
                    change.base_relative_path.IsEmpty() ? change.relative_path : change.base_relative_path;
            const wxString local = libraryPath( aCopy.local_root, file );

            if( !aSource->Capabilities().adopts_whole_files || change.base_format.IsEmpty()
                || reformatted.contains( file ) || !local.EndsWith( wxS( ".kicad_sym" ) ) )
            {
                continue;
            }

            const std::optional<KIGIT::SYMBOL_FILE>& split = files.Split( local );

            if( split && KIGIT::NewerFormat( KIGIT::FormatVersion( split->header ), change.base_format.utf8_string() ) )
                reformatted.try_emplace( file );
        }

        // A source that cannot adopt a file is left to read the re-save as before
        for( auto& [file, row] : reformatted )
        {
            if( !aSource->Capabilities().adopts_whole_files )
                continue;

            const wxString digest =
                    wxString::FromUTF8( KIGIT::BlobDigest( files.texts[libraryPath( aCopy.local_root, file )] ) );

            // A file with a real edit goes whole with its edits, or its re-save would stay behind after them
            if( edited.contains( file ) )
            {
                auto crosses = [&]( const MDW_LOCAL_CHANGE& aChange )
                {
                    const wxString& from =
                            aChange.base_relative_path.IsEmpty() ? aChange.relative_path : aChange.base_relative_path;

                    return ( aChange.relative_path == file ) != ( from == file );
                };

                // A member moved from or to another file has no row of this file to carry its digest
                if( std::none_of( kept.begin(), kept.end(), crosses ) )
                {
                    size_t edits = std::count_if( kept.begin(), kept.end(),
                                                  [&]( const MDW_LOCAL_CHANGE& aChange )
                                                  {
                                                      return aChange.relative_path == file;
                                                  } );

                    for( MDW_LOCAL_CHANGE& change : kept )
                    {
                        if( change.relative_path == file )
                        {
                            change.file_digest = digest;
                            change.file_edits = edits;
                        }
                    }
                }

                continue;
            }

            row.format_digest = digest;
            row.member = MDW_ITEM_REF();
            row.name = file;
            row.change = MDW_CHANGE_TYPE::MODIFIED;
            row.provisional = false;
            row.moved = false;

            // A released member's denial would hold back a file whose members only changed format
            row.checkin = kiapi::workspace::Permission();
            kept.push_back( std::move( row ) );
        }

        listed.changes = std::move( kept );
    }

    // A refused push taught the source that this user cannot write, whichever member is listed. An earlier
    // refusal only warns
    for( MDW_LOCAL_CHANGE& change : listed.changes )
    {
        if( written.state() == kiapi::workspace::PS_DENIED )
            change.checkin = written;
        else if( change.checkin.state() != kiapi::workspace::PS_DENIED )
            change.checkin.set_reason( written.reason() );
    }

    return listed;
}


/// Why aSource cannot take a check-in of aCopy, or Ok
MDW_RESULT checkInRefusal( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_LIBRARY_COPY& aCopy )
{
    std::shared_ptr<MDW_SESSION> session = aSource ? aSource->Session() : nullptr;

    // A pinned copy is a snapshot, and one locator can name sources on two backends
    if( !aSource || aCopy.pinned || aSource->Info().locator != aCopy.locator
        || ( session && session->Binding().backend_id != aCopy.backend_id ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
    }

    if( !online( *aSource ) )
        return notConnected( *aSource );

    return MDW_RESULT::Ok();
}


/// Check in aChanges for a caller that holds the copy's lock, and record what the reply accepted in the index
MDW_LIBRARY_CHECKIN_RESULT commitLocked( MDW_LIBRARY_SOURCES& aSources, const std::shared_ptr<MDW_SOURCE>& aSource,
                                         const MDW_LIBRARY_COPY& aCopy, const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                         const wxString& aComment )
{
    MDW_LIBRARY_CHECKIN_RESULT checkIn;
    checkIn.copy = aCopy;

    // A sync since aCopy was read may have rewritten the copy, and its changes would no longer apply
    std::optional<MDW_LIBRARY_COPY> current = aSources.Copy( markerOf( aCopy ) );

    if( !current || current->snapshot.token != aCopy.snapshot.token )
    {
        checkIn.result = MDW_RESULT::Fail( MDW_ERROR_CODE::VERSION_CONFLICT,
                                           _( "The library copy changed. List its changes again." ) );
        return checkIn;
    }

    checkIn.copy = *current;

    std::vector<wxString>        itemIds;
    std::map<wxString, wxString> wholeFiles;
    const bool                   adopts = aSource->Capabilities().adopts_whole_files;

    std::map<wxString, std::set<wxString>> fileEdits;

    for( const MDW_LOCAL_CHANGE& change : aChanges )
    {
        if( change.format_digest.IsEmpty() )
        {
            itemIds.push_back( change.member.item_id );

            if( !change.file_digest.IsEmpty() )
                fileEdits[change.relative_path].insert( change.member.item_id );
        }
        else if( adopts )
        {
            wholeFiles[change.relative_path] = change.format_digest;
        }
        else
        {
            checkIn.result = MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED );
            return checkIn;
        }
    }

    // A file goes whole only with every edit listed in it, so an unticked edit never rides along
    for( const MDW_LOCAL_CHANGE& change : aChanges )
    {
        if( adopts && !change.file_digest.IsEmpty() && fileEdits[change.relative_path].size() == change.file_edits )
            wholeFiles[change.relative_path] = change.file_digest;
    }

    // With no member named, every whole file only reformats its members
    const bool formatUpgrade = itemIds.empty() && !wholeFiles.empty();

    std::tie( checkIn.result, checkIn.checkin ) = MdwSync<MDW_LIBRARY_CHECKIN>(
            [&]( MDW_DONE<MDW_LIBRARY_CHECKIN> aDone )
            {
                return aSource->Libraries().CheckIn( aCopy.local_root, itemIds, aComment, wholeFiles, formatUpgrade,
                                                     std::move( aDone ) );
            } );

    if( !checkIn.result.IsOk() )
        return checkIn;

    std::set<wxString>          changedIds;
    std::vector<APPLIED_CHANGE> applied;

    std::vector<std::optional<size_t>> matches = matchResults( aChanges, checkIn.checkin.results );

    for( size_t r = 0; r < matches.size(); ++r )
    {
        const MDW_MEMBER_RESULT& result = checkIn.checkin.results[r];

        // A pending change may still be refused, so the index keeps what was there until it is applied
        if( !matches[r] || !result.status.IsOk() || result.item_id.IsEmpty()
            || result.change.outcome != MDW_CHANGE_OUTCOME::APPLIED )
        {
            continue;
        }

        const MDW_LOCAL_CHANGE& change = aChanges[*matches[r]];

        applied.push_back( { &change, result.item_id, result.change.revision.revision_id } );
        changedIds.insert( change.library_id );
    }

    // A file adopted whole names no member, but the commit still moved the copy
    if( applied.empty() && ( wholeFiles.empty() || checkIn.checkin.copy_snapshot.token.IsEmpty() ) )
        return checkIn;

    // Members list at the copy's snapshot, which must now hold what this check-in committed
    if( !checkIn.checkin.copy_snapshot.token.IsEmpty() )
        checkIn.copy.snapshot = checkIn.checkin.copy_snapshot;

    recordCheckIns( checkIn.copy, applied );

    // The backend kept what it accepted even if the index cannot be written, so the outcome stays in checkin
    if( MDW_RESULT written = aSources.WriteCopy( checkIn.copy ); !written.IsOk() )
        checkIn.result = written;

    announce( aSource, changedIds );
    return checkIn;
}


/// Check in aChanges and record what the reply accepted in the index. No file is rewritten
MDW_LIBRARY_CHECKIN_RESULT checkInLibraryCopy( MDW_LIBRARY_SOURCES&                 aSources,
                                               const std::shared_ptr<MDW_SOURCE>&   aSource,
                                               const MDW_LIBRARY_COPY&              aCopy,
                                               const std::vector<MDW_LOCAL_CHANGE>& aChanges, const wxString& aComment )
{
    MDW_LIBRARY_CHECKIN_RESULT checkIn;
    checkIn.copy = aCopy;
    checkIn.result =
            aChanges.empty() ? MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER ) : checkInRefusal( aSource, aCopy );

    if( !checkIn.result.IsOk() )
        return checkIn;

    // Enforced here as well as in the UI, so nothing publishes a member the source denied
    auto denied = std::find_if( aChanges.begin(), aChanges.end(),
                                []( const MDW_LOCAL_CHANGE& aChange )
                                {
                                    return aChange.checkin.state() == kiapi::workspace::PS_DENIED;
                                } );

    if( denied != aChanges.end() )
    {
        checkIn.result =
                MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER, wxString::FromUTF8( denied->checkin.reason() ) );
        return checkIn;
    }

    KIPLATFORM::IO::FILE_LOCK guard;

    if( !lockCopy( aCopy.local_root, guard ) )
    {
        checkIn.result = MDW_RESULT::Fail( MDW_ERROR_CODE::BUSY, _( "The library copy is being updated." ) );
        return checkIn;
    }

    return commitLocked( aSources, aSource, aCopy, aChanges, aComment );
}


struct LIBRARY_ENTRY
{
    wxString nickname;
    wxString uri;
    wxString description;
};

const std::string TABLE_MARKER_KEY = "mdw";


wxString markerOptions( const wxString& aMarker )
{
    std::map<std::string, UTF8> options;
    options[TABLE_MARKER_KEY] = UTF8( aMarker );

    return wxString( LIBRARY_TABLE::FormatOptions( &options ).wx_str() );
}


bool isMarked( const wxString& aOptions, const wxString& aMarker )
{
    wxString id = MDW_LIBRARY_SOURCES::MarkerOf( aOptions );

    return !id.IsEmpty() && id == aMarker;
}


/**
 * Make the rows marked aMarker match aEntries, adding, updating and removing as needed. Only rows carrying the
 * option mdw=<marker> are touched, so rows added by hand are left alone. A wanted nickname held by a row without
 * that marker, or named in aTaken, is not touched and is returned in aConflicts. True when the table changed.
 */
bool syncMarked( LIBRARY_TABLE& aTable, const wxString& aMarker, const std::vector<LIBRARY_ENTRY>& aEntries,
                 std::vector<wxString>& aConflicts, const std::set<wxString>& aTaken )
{
    bool changed = false;

    std::map<wxString, const LIBRARY_ENTRY*> wanted;

    for( const LIBRARY_ENTRY& entry : aEntries )
        wanted[entry.nickname] = &entry;

    auto& rows = aTable.Rows();

    for( auto it = rows.begin(); it != rows.end(); )
    {
        if( !isMarked( it->Options(), aMarker ) )
        {
            ++it;
            continue;
        }

        auto match = wanted.find( it->Nickname() );

        if( match == wanted.end() )
        {
            it = rows.erase( it );
            changed = true;
            continue;
        }

        if( it->URI() != match->second->uri || it->Description() != match->second->description )
        {
            it->SetURI( match->second->uri );
            it->SetDescription( match->second->description );
            changed = true;
        }

        wanted.erase( match );
        ++it;
    }

    for( const LIBRARY_ENTRY& entry : aEntries )
    {
        if( !wanted.count( entry.nickname ) )
            continue;

        if( aTable.HasRow( entry.nickname ) || aTaken.count( entry.nickname ) )
        {
            aConflicts.push_back( entry.nickname );
            continue;
        }

        LIBRARY_TABLE_ROW& row = aTable.InsertRow();
        row.SetNickname( entry.nickname );
        row.SetURI( entry.uri );
        row.SetDescription( entry.description );
        row.SetType( wxS( "KiCad" ) );
        row.SetOptions( markerOptions( aMarker ) );
        row.SetOk( true );
        changed = true;
    }

    return changed;
}


/// Make the rows of aTable of type aType match the libraries of aCopies, removing only marked rows aOwns accepts
bool syncRows( LIBRARY_TABLE& aTable, MDW_LIBRARY_TYPE aType,
               const std::vector<std::shared_ptr<const MDW_LIBRARY_COPY>>& aCopies, std::vector<wxString>& aConflicts,
               const std::set<wxString>& aTaken, const std::function<bool( const wxString& )>& aOwns,
               const std::set<wxString>& aReportSources, const MDW_LIBRARY_SOURCES& aStore )
{
    std::map<wxString, std::vector<LIBRARY_ENTRY>> wanted;
    std::set<wxString>                             reported;

    for( const std::shared_ptr<const MDW_LIBRARY_COPY>& shared : aCopies )
    {
        const MDW_LIBRARY_COPY&                  copy = *shared;
        std::vector<LIBRARY_ENTRY>&              entries = wanted[markerOf( copy )];
        const wxString                           folder = wxFileName( copy.local_root ).GetFullName();
        std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = aStore.Find( copy.source_id );

        // A pinned row is committed in the project's table, so its name cannot be one this machine chose
        const wxString shown =
                copy.pinned ? MDW_LIBRARY_SOURCES::DefaultName( copy.locator ) : ( source ? source->name : wxString() );
        const wxString description = ( shown.IsEmpty() ? folder : shown ) + wxS( " (team library)" );

        if( aReportSources.contains( copy.source_id ) )
            reported.insert( markerOf( copy ) );

        for( const MDW_COPY_LIBRARY& library : copy.libraries )
        {
            if( library.type != aType || libraryPath( copy.local_root, library.relative_path ).IsEmpty() )
                continue;

            // The variable names the copy as every machine does, so a shared table or a moved root still resolves
            entries.push_back( { library.nickname,
                                 wxS( "${KICAD_TEAM_LIBRARIES}/" ) + folder + wxS( "/" ) + library.relative_path,
                                 description } );
        }
    }

    // Unwanted rows go first, so a nickname moving between sources is free before it is added
    auto unwanted = [&]( const LIBRARY_TABLE_ROW& aRow )
    {
        wxString marker = MDW_LIBRARY_SOURCES::MarkerOf( aRow.Options() );

        if( marker.IsEmpty() || !aOwns( marker ) )
            return false;

        auto it = wanted.find( marker );

        return it == wanted.end()
               || std::none_of( it->second.begin(), it->second.end(),
                                [&]( const LIBRARY_ENTRY& aEntry )
                                {
                                    return aEntry.nickname == aRow.Nickname();
                                } );
    };

    bool changed = std::erase_if( aTable.Rows(), unwanted ) != 0;

    for( const auto& [marker, entries] : wanted )
    {
        std::vector<wxString> conflicts;

        changed |= syncMarked( aTable, marker, entries, conflicts, aTaken );

        if( reported.contains( marker ) )
            aConflicts.insert( aConflicts.end(), conflicts.begin(), conflicts.end() );
    }

    return changed;
}


/// OpenSession on a thread of its own, since its launch and login cannot see a cancel and a cancel must not wait
/// them out. An open that outlives the cancel still publishes its session, unless aKey was closed meanwhile
std::shared_ptr<MDW_SESSION> openSession( const wxString& aKey, const MDW_PROJECT_BINDING& aBinding,
                                          const wxString& aAccount )
{
    auto opened = std::make_shared<std::shared_ptr<MDW_SESSION>>();

    MDW_RESULT waited = MdwRunUntilStopped(
            [opened, aKey, aBinding, aAccount]()
            {
                if( MDW_FRAMEWORK_LEASE fw = runningFramework() )
                    *opened = fw->OpenSession( aKey, aBinding, CONNECT_TIMEOUT_MS, aAccount );

                return MDW_RESULT::Ok();
            },
            std::chrono::milliseconds( OPEN_DEADLINE_MS ), std::chrono::milliseconds( 0 ),
            []()
            {
                return MDW_CANCEL_SCOPE::Requested();
            },
            nullptr );

    return waited.IsOk() ? *opened : nullptr;
}


/// A source served by the library session of aSource, opened again when it cannot answer
std::pair<MDW_RESULT, std::shared_ptr<MDW_SOURCE>> librarySource( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
{
    MDW_FRAMEWORK_LEASE fw = framework();

    if( !fw )
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), nullptr };

    MDW_LIBRARY_SOURCES& sources = MDW_LIBRARY_SOURCES::Get();
    MDW_PROJECT_BINDING  binding{ aSource.backend_id, aSource.name, aSource.locator };

    // Keyed as MDW_CLIENT::LibrarySources() keys it, so the client adopts the session opened here
    wxString                     key = sources.SessionKey( aSource );
    std::shared_ptr<MDW_SESSION> session = fw->Session( key );

    // A task that only started once the cancel came must not launch a backend
    if( MDW_CANCEL_SCOPE::Requested() )
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), nullptr };

    const bool replace = !session || !session->Supervisor()->Online()
                         || session->Binding().backend_id != binding.backend_id
                         || session->Binding().binding != binding.binding;
    const bool signedOut = !replace && session->Status().phase == MDW_CONNECTION::SIGN_IN_NEEDED;

    // Signs in again in place as the source's account, which never resends a login the server refused
    if( signedOut )
    {
        session->SetUsername( aSource.account );

        if( fw->HasUnrefusedStoredLogin( *session ) )
            session->Reconnect( CONNECT_TIMEOUT_MS );

        if( session->Status().phase == MDW_CONNECTION::SIGN_IN_NEEDED )
            return { session->Status().reason, nullptr };
    }

    // A dead backend never comes back for a library session, and a rebound one serves another workspace
    if( replace )
        session = openSession( key, binding, aSource.account );

    // The open cannot see a cancel, so one pressed while it ran is told from a failed connection only here
    if( MDW_CANCEL_SCOPE::Requested() )
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), nullptr };

    // Opening reaches only the backend, never the location, so a failure is the backend not starting
    if( !session )
    {
        const MDW_BACKEND_MANIFEST* manifest = fw->Registry().Find( aSource.backend_id );

        if( !manifest )
            return { MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), nullptr };

        return { MDW_RESULT::Fail( MDW_ERROR_CODE::NETWORK,
                                   wxString::Format( _( "%s could not be started." ), manifest->display_name ) ),
                 nullptr };
    }

    return { MDW_RESULT::Ok(), std::make_shared<MDW_SOURCE>( session ) };
}


std::optional<std::string> readText( const wxString& aFile )
{
    std::ifstream in( ToFsPath( aFile ), std::ios::binary );

    if( !in.is_open() )
        return std::nullopt;

    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


/// True when aRelativePath, a member's file under its copy, is a symbol library its members share
bool symbolLibrary( const wxString& aRelativePath )
{
    return wxFileName( aRelativePath ).GetExt() == wxString( FILEEXT::KiCadSymbolLibFileExtension );
}


/// aText with each CRLF as LF, since a library saved on Windows holds CRLF and the blob it came from LF
std::string withLf( std::string aText )
{
    size_t out = 0;

    for( size_t in = 0; in < aText.size(); ++in )
    {
        if( aText[in] != '\r' || in + 1 == aText.size() || aText[in + 1] != '\n' )
            aText[out++] = aText[in];
    }

    aText.resize( out );
    return aText;
}


/// aText with each line ending as CRLF
std::string withCrLf( const std::string& aText )
{
    std::string out;

    for( char ch : withLf( aText ) )
    {
        if( ch == '\n' )
            out += '\r';

        out += ch;
    }

    return out;
}


/// The text of the member aName in aText, which is all of it unless aText is a symbol library
std::optional<std::string> memberText( const std::string& aText, const wxString& aName, bool aSymbol )
{
    if( !aSymbol )
        return aText;

    const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( aText );
    auto                     span = file.spans.find( aName.utf8_string() );

    if( !file.complete || span == file.spans.end() )
        return std::nullopt;

    return aText.substr( span->second.start, span->second.length );
}


/// Symbol library text aText without its symbol aName, or nothing when aText is not a list of symbols
std::optional<std::string> withoutMember( const std::string& aText, const wxString& aName )
{
    const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( aText );
    auto                     span = file.spans.find( aName.utf8_string() );

    if( !file.complete )
        return std::nullopt;

    if( span == file.spans.end() )
        return aText;

    std::size_t       start = span->second.start;
    const std::size_t indent = start ? aText.find_last_not_of( " \t", start - 1 ) : std::string::npos;

    // Its line goes with it, so the file reads as if it never held the symbol
    if( indent != std::string::npos && aText[indent] == '\n' )
        start = indent;

    std::string text = aText;
    text.erase( start, span->second.start + span->second.length - start );
    return text;
}


/// The symbol library file aPath, or every symbol file of the .kicad_symdir folder aPath
std::vector<wxString> symbolLibraryFiles( const wxString& aPath )
{
    std::vector<wxString> files;

    if( wxFileName::FileExists( aPath ) )
    {
        files.push_back( aPath );
        return files;
    }

    wxDir    dir( aPath );
    wxString name;

    const wxString pattern = wxS( "*." ) + wxString( FILEEXT::KiCadSymbolLibFileExtension );

    for( bool more = dir.IsOpened() && dir.GetFirst( &name, pattern, wxDIR_FILES ); more; more = dir.GetNext( &name ) )
    {
        files.push_back( wxFileName( aPath, name ).GetFullPath() );
    }

    return files;
}


/// The texts of the other files of the .kicad_symdir folder holding symbol file aFile, empty outside one. Nothing
/// when one cannot be read
std::optional<std::vector<std::string>> folderTexts( const wxString& aFile )
{
    const wxFileName         file( aFile );
    std::vector<std::string> texts;

    if( !file.GetPath().EndsWith( wxS( ".kicad_symdir" ) ) )
        return texts;

    for( const wxString& other : symbolLibraryFiles( file.GetPath() ) )
    {
        if( wxFileName( other ).SameAs( file ) )
            continue;

        std::optional<std::string> text = readText( other );

        if( !text )
            return std::nullopt;

        texts.push_back( std::move( *text ) );
    }

    return texts;
}


/// The format version the header of the symbol library text aText names, 0 for none, or nothing for other text
std::optional<long> symbolTextVersion( const std::string& aText )
{
    if( aText.find( "(kicad_symbol_lib" ) == std::string::npos )
        return std::nullopt;

    const std::string version = KIGIT::FormatVersion( aText );

    return version.empty() ? 0 : std::strtol( version.c_str(), nullptr, 10 );
}


/// symbolTextVersion() of the file aPath, reading only as much as a header takes
std::optional<long> symbolFileVersion( const wxString& aPath )
{
    std::ifstream in( ToFsPath( aPath ), std::ios::binary );
    std::string   head( 1024, '\0' );

    in.read( head.data(), static_cast<std::streamsize>( head.size() ) );
    head.resize( static_cast<size_t>( in.gcount() ) );

    return symbolTextVersion( head );
}


/// aPath as a protocol path under aLocalRoot
wxString relativePath( const wxString& aLocalRoot, const wxString& aPath )
{
    wxFileName file( aPath );
    file.MakeRelativeTo( aLocalRoot );

    return file.GetFullPath( wxPATH_UNIX );
}


/// Write aText to aPath unless aPath no longer holds aExpected, as after a save since. Empty when done
wxString replaceIfUnchanged( const wxString& aPath, const std::string& aExpected, const std::string& aText )
{
    bool     changed = false;
    wxString error;

    // Checked once the new bytes are on disk, so only the rename lies between the check and the replacement
    auto unchanged = [&]()
    {
        changed = readText( aPath ) != aExpected;
        return !changed;
    };

    if( KIPLATFORM::IO::AtomicWriteFile( aPath, aText.data(), aText.size(), &error, unchanged ) )
        return wxEmptyString;

    if( changed )
        return wxString::Format( _( "'%s' changed since, so it was left as it is." ), aPath );

    return wxString::Format( _( "Could not write '%s': %s" ), aPath, error );
}

} // namespace


std::vector<MDW_BACKEND_CHOICE> MDW_SETUP::Backends( std::optional<MDW_CAPABILITY_GROUP> aGroup )
{
    std::vector<MDW_BACKEND_CHOICE> choices;

    if( MDW_FRAMEWORK_LEASE fw = framework() )
    {
        for( const MDW_BACKEND_MANIFEST& manifest : fw->Registry().Manifests() )
        {
            if( aGroup && !manifest.capability_hints.contains( *aGroup ) )
                continue;

            choices.push_back( { manifest.backend_id,
                                 manifest.display_name.IsEmpty() ? manifest.backend_id : manifest.display_name,
                                 manifest.binding_fields, manifest.binding_source, manifest.binding_search } );
        }
    }

    return choices;
}


wxString MDW_SETUP::Realm( const wxString& aProjectPath, const wxString& aLocator )
{
    MDW_FRAMEWORK_LEASE          fw = framework();
    std::shared_ptr<MDW_SESSION> session = fw && aLocator.IsEmpty() ? fw->Session( aProjectPath ) : nullptr;

    return session ? session->Status().realm : MDW_CREDENTIALS::Realm( aLocator );
}


bool MDW_SETUP::HasCredential( const wxString& aRealm, const wxString& aUsername )
{
    MDW_FRAMEWORK_LEASE fw = framework();

    return fw && fw->Credentials().Find( aRealm, aUsername ).has_value();
}


wxString MDW_SETUP::Username( const wxString& aRealm, const wxString& aBindingAccount )
{
    if( aRealm.IsEmpty() )
        return wxString();

    return aBindingAccount.IsEmpty() ? KIGIT::PlatformServices().getAccount( aRealm ) : aBindingAccount;
}


std::pair<MDW_RESULT, MDW_SOURCE_INFO>
MDW_SETUP::TestConnection( const wxString& aBackendId, const wxString& aWorkspaceId, const wxString& aLocator,
                           const wxString& aProjectPath, const MDW_TYPED_CREDENTIALS& aTyped,
                           const std::optional<MDW_HOST_KEY>& aTrust, std::shared_ptr<MDW_SESSION>* aKeep,
                           const wxString& aKeyFile )
{
    MDW_FRAMEWORK_LEASE         fw = framework();
    const MDW_BACKEND_MANIFEST* manifest = fw ? fw->Registry().Find( aBackendId ) : nullptr;

    if( !manifest )
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), MDW_SOURCE_INFO() };

    MDW_BACKEND_CONTEXT context;
    context.projectPath = aProjectPath;
    context.workspaceId = aWorkspaceId;
    context.locator = aLocator;

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( *manifest, context );

    if( !supervisor->Launch( CONNECT_TIMEOUT_MS ) )
    {
        if( MDW_CANCEL_SCOPE::Requested() )
            return { MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), MDW_SOURCE_INFO() };

        return { MDW_RESULT::Fail( MDW_ERROR_CODE::NETWORK,
                                   wxString::Format( _( "%s could not be started." ), manifest->display_name ) ),
                 MDW_SOURCE_INFO() };
    }

    MDW_PROJECT_BINDING binding{ aBackendId, aWorkspaceId, aLocator };
    auto                session = std::make_shared<MDW_SESSION>( binding, std::move( supervisor ) );

    if( auto typedUser = aTyped.fields.find( wxS( "username" ) ); typedUser != aTyped.fields.end() )
        session->SetUsername( typedUser->second );

    session->SetKeyFile( aKeyFile );

    // Leased per call, since the attempt may outlive this wait and the framework with it
    session->SetCredentialSource(
            [held = session.get()]()
            {
                PGM_BASE*           pgm = PgmOrNull();
                MDW_FRAMEWORK_LEASE lease = pgm ? pgm->LeaseMdwFramework( false ) : MDW_FRAMEWORK_LEASE();

                return lease ? lease->StoredCredentials( *held ) : MDW_TYPED_CREDENTIALS();
            } );

    const bool typed = aTyped.fields.contains( wxS( "password" ) ) && !aTyped.fields.at( wxS( "password" ) ).IsEmpty();

    if( MDW_CANCEL_SCOPE::Requested() )
    {
        session->Supervisor()->Shutdown();
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), MDW_SOURCE_INFO() };
    }

    // Owns what it uses, as it may still run after the wait gave up on it
    auto attempt = [session, aTrust, aTyped, typed]()
    {
        auto connect = [&]()
        {
            return typed ? session->Authenticate( aTyped, CONNECT_TIMEOUT_MS )
                         : session->Reconnect( CONNECT_TIMEOUT_MS );
        };

        MDW_RESULT tested = connect();

        // A fresh backend can only trust the key this connection met, so it must still be the one the user saw
        if( aTrust && tested.error == MDW_ERROR_CODE::UNTRUSTED_HOST && tested.host_key == aTrust )
        {
            kiapi::mdw::TrustHostKeyRequest request;
            kiapi::mdw::TrustHostKeyReply   reply;
            MdwToProto( request.mutable_host_key(), *aTrust );

            std::shared_ptr<MDW_BACKEND> backend = session->Backend();
            tested = backend ? backend->CallSync( request, reply, CONNECT_TIMEOUT_MS )
                             : MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );

            if( tested.IsOk() )
                tested = connect();
        }

        return tested;
    };

    // Shutting the backend down closes its transport, which fails a call still pending
    MDW_RESULT tested = MdwRunUntilStopped(
            attempt, std::chrono::milliseconds( TEST_DEADLINE_MS ), std::chrono::milliseconds( TEST_STOP_GRACE_MS ),
            []()
            {
                return MDW_CANCEL_SCOPE::Requested();
            },
            [session]()
            {
                session->Supervisor()->Shutdown();
            } );

    // Only an answer the wait saw is kept, never one that came after it gave up
    if( typed && tested.IsOk() )
    {
        fw->Credentials().Save( session->Status().realm, aTyped );

        if( fw->Credentials().Refused( session->Status().realm, aTyped ) )
            fw->Credentials().JudgeStored( session->Status().realm, aTyped, false );
    }

    MDW_SOURCE_INFO info = MDW_SOURCE( session ).Info();

    if( aKeep && tested.IsOk() )
        *aKeep = session;
    else
        session->Supervisor()->Shutdown();

    return { tested, info };
}


std::pair<MDW_RESULT, std::vector<MDW_BINDING_TARGET>>
MDW_SETUP::FindBindingTargets( const std::shared_ptr<MDW_SESSION>& aSession, const wxString& aText )
{
    std::shared_ptr<MDW_BACKEND> backend = aSession ? aSession->Backend() : nullptr;

    if( !backend )
        return { MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST ), {} };

    kiapi::mdw::FindBindingTargetsRequest request;
    kiapi::mdw::FindBindingTargetsReply   reply;
    request.set_text( MdwToUtf8( aText ) );

    MDW_RESULT result = backend->CallSync( request, reply, CONNECT_TIMEOUT_MS );

    std::vector<MDW_BINDING_TARGET> targets;

    for( const kiapi::mdw::BindingTarget& target : reply.targets() )
    {
        targets.push_back( { MdwFromUtf8( target.display_name() ), MdwFromUtf8( target.code() ),
                             MdwFromUtf8( target.locator() ) } );
    }

    return { result, targets };
}


namespace
{

/// aLocator with every query item of key aKey replaced by aItem, or dropped when aItem is empty
wxString replaceLocatorItem( const wxString& aLocator, const wxString& aKey, const wxString& aItem )
{
    wxString base = aLocator.BeforeFirst( '#' );
    wxString fragment = aLocator.Mid( base.length() );
    wxString query = base.Contains( '?' ) ? base.AfterFirst( '?' ) : wxString();

    wxString result = base.BeforeFirst( '?' );
    wxString sep = wxS( "?" );
    bool     replaced = false;

    for( const wxString& pair : wxSplit( query, '&' ) )
    {
        if( pair.IsEmpty() )
            continue;

        const bool ours = pair.BeforeFirst( '=' ) == aKey;
        replaced |= ours;

        if( ours && aItem.IsEmpty() )
            continue;

        result << sep << ( ours ? aItem : pair );
        sep = wxS( "&" );
    }

    // The text is the source's identity, so it is rewritten only when it had the item
    if( !replaced && aItem.IsEmpty() )
        return aLocator;

    if( !replaced )
        result << sep << aItem;

    return result + fragment;
}

} // namespace


wxString MDW_SETUP::LocatorWithItem( const wxString& aLocator, const wxString& aItem )
{
    return replaceLocatorItem( aLocator, aItem.BeforeFirst( '=' ), aItem );
}


wxString MDW_SETUP::LocatorWithoutItem( const wxString& aLocator, const wxString& aKey )
{
    if( aKey.IsEmpty() )
        return aLocator;

    return replaceLocatorItem( aLocator, aKey, wxEmptyString );
}


MDW_RESULT MDW_SETUP::ReconnectLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                              const MDW_CONNECTION_ANSWER&     aAnswer )
{
    MDW_FRAMEWORK_LEASE fw = runningFramework();

    if( !fw )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED );

    // Keyed as librarySource() keys the session
    return fw->Reconnect( MDW_LIBRARY_SOURCES::Get().LocalRoot( aSource ) + wxFILE_SEP_PATH, aAnswer,
                          CONNECT_TIMEOUT_MS );
}


void MDW_SETUP::CloseProjectSession( const wxString& aProjectPath, bool aWaitForExit )
{
    if( MDW_FRAMEWORK_LEASE fw = runningFramework() )
        fw->CloseSession( aProjectPath, aWaitForExit );
}


void MDW_SETUP::RememberProjectAccount( PROJECT& aProject, const MDW_PROJECT_BINDING& aBinding,
                                        const wxString& aUsername )
{
    if( aUsername.IsEmpty() )
        return;

    if( MDW_CLIENT::BindsGitRemote( aBinding ) )
    {
        aProject.GetLocalSettings().m_GitRepoUsername = aUsername;
        return;
    }

    if( const wxString realm = Realm( aProject.GetProjectFullName(), aBinding.binding ); !realm.IsEmpty() )
        KIGIT::PlatformServices().setAccount( realm, aUsername );
}


MDW_RESULT MDW_SETUP::RememberSourceAccount( MDW_LIBRARY_SOURCE_CONFIG& aSource, const wxString& aUsername )
{
    MDW_LIBRARY_SOURCES& sources = MDW_LIBRARY_SOURCES::Get();

    if( aSource.site )
    {
        if( const wxString realm = Realm( wxEmptyString, aSource.locator ); !realm.IsEmpty() )
            KIGIT::PlatformServices().setAccount( realm, aUsername );
    }
    else if( MDW_RESULT recorded = sources.SetAccount( aSource.id, aUsername ); recorded.IsOk() )
    {
        aSource.account = aUsername;
    }
    else
    {
        return recorded;
    }

    CloseProjectSession( sources.SessionKey( aSource ), false );
    return MDW_RESULT::Ok();
}


namespace
{
bool setDisabledHere( PROJECT& aProject, bool aDisabled )
{
    PROJECT_LOCAL_SETTINGS& local = aProject.GetLocalSettings();
    const bool              was = local.m_WorkspaceDisabledHere;

    local.m_WorkspaceDisabledHere = aDisabled;
    local.SaveToFile( aProject.GetProjectPath() );

    // The next open would read the old choice, so the session stays as that choice left it
    if( local.LastSaveFailed() )
    {
        local.m_WorkspaceDisabledHere = was;
        return false;
    }

    // The project file is left alone, since the binding in it belongs to the team
    MDW_SETUP::CloseProjectSession( aProject.GetProjectFullName() );
    return true;
}
} // namespace


bool MDW_SETUP::DisconnectHere( PROJECT& aProject )
{
    return setDisabledHere( aProject, true );
}


bool MDW_SETUP::ReconnectHere( PROJECT& aProject )
{
    return setDisabledHere( aProject, false );
}


void MDW_SETUP::SweepDeadCopies()
{
    const std::filesystem::path folder = MdwFsPath( MDW_LIBRARY_SOURCES::DefaultRoot().utf8_string() );

    // A partial copy can be large, and startup must not wait for it
    GetKiCadThreadPool().detach_task(
            [folder]()
            {
                MdwSweepDeadStaging( folder );
            } );
}


MDW_RESULT MDW_SETUP::DiscardLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG&     aSource,
                                            const std::vector<MDW_CREATED_COPY>& aCopies )
{
    using LOCK_STATE = KIPLATFORM::IO::FILE_LOCK::STATE;

    MDW_LIBRARY_SOURCES&                   sources = MDW_LIBRARY_SOURCES::Get();
    std::vector<KIPLATFORM::IO::FILE_LOCK> guards( aCopies.size() );
    std::vector<size_t>                    drop;
    std::vector<wxString>                  owned;
    bool                                   taken = false;

    // Every lock is taken before any index goes, so a lock that fails leaves every copy as it was
    for( size_t ii = 0; ii < aCopies.size(); ++ii )
    {
        const MDW_CREATED_COPY& created = aCopies[ii];
        const wxString          marker = MDW_LIBRARY_SOURCES::Marker( aSource, created.pin );
        const wxString          lockPath = copyLockPath( sources.LocalRoot( aSource, created.pin ) );
        bool                    lockCreated = false;
        LOCK_STATE              state = guards[ii].Acquire( lockPath, lockCreated );

        // Another set-up of this source is filling the copy, so the copy and the source are now its own
        if( state == LOCK_STATE::BUSY )
        {
            taken = true;
            continue;
        }

        if( state != LOCK_STATE::HELD )
        {
            return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                     wxString::Format( _( "Could not lock '%s'." ), lockPath ) );
        }

        // Another process may have taken the copy up between the set-up's sync and now, even at the same snapshot
        if( std::optional<MDW_LIBRARY_COPY> current = sources.Copy( marker );
            current && current->generation != created.indexed )
        {
            continue;
        }

        drop.push_back( ii );
    }

    for( size_t ii : drop )
    {
        const MDW_SNAPSHOT& pin = aCopies[ii].pin;

        // Without an index the client has no copy to reconnect a backend for once the session closes
        MDW_RESULT dropped = sources.DropCopy( MDW_LIBRARY_SOURCES::Marker( aSource, pin ) );

        if( !dropped.IsOk() )
            return dropped;

        owned.push_back( sources.LocalRoot( aSource, pin ) );
    }

    const bool deleting = std::any_of( owned.begin(), owned.end(),
                                       []( const wxString& aRoot )
                                       {
                                           return wxFileName::DirExists( aRoot );
                                       } );

    // The backend keeps the copy open, and Windows will not delete an open file. Without a copy there is no
    // reason to wait while the backend finishes a fill the cancel abandoned
    CloseProjectSession( sources.SessionKey( aSource ), deleting );

    for( const wxString& localRoot : owned )
    {
        wxString why;

        if( wxFileName::DirExists( localRoot ) && !RmDirRecursive( localRoot, &why ) )
            return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, why );
    }

    // A copy left in place still needs its source
    if( taken || sources.HasCopy( aSource ) )
        return MDW_RESULT::Ok();

    return sources.Remove( aSource.id );
}


MDW_LIBRARY_SYNC_RESULT MDW_SETUP::SyncLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG&            aSource,
                                                      const MDW_SNAPSHOT&                         aPin,
                                                      const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions )
{
    MDW_LIBRARY_SYNC_RESULT sync;

    auto [connected, source] = librarySource( aSource );

    if( !source )
    {
        sync.result = connected;
        return sync;
    }

    return SyncLibrarySource( MDW_LIBRARY_SOURCES::Get(), source, aSource, aPin, aResolutions );
}


MDW_LIBRARY_SYNC_RESULT MDW_SETUP::SyncLibrarySource( MDW_LIBRARY_SOURCES&                        aSources,
                                                      const std::shared_ptr<MDW_SOURCE>&          aSource,
                                                      const MDW_LIBRARY_SOURCE_CONFIG&            aConfig,
                                                      const MDW_SNAPSHOT&                         aPin,
                                                      const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions )
{
    return afterRespawn<MDW_LIBRARY_SYNC_RESULT>( aSource,
                                                  [&]()
                                                  {
                                                      return syncLibraryCopy( aSources, aSource, aConfig, aPin,
                                                                              aResolutions );
                                                  } );
}


MDW_CONFLICT_SIDES_RESULT MDW_SETUP::ConflictSides( const MDW_LIBRARY_SOURCE_CONFIG&        aSource,
                                                    const wxString&                         aLocalRoot,
                                                    const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                                                    const wxString&                         aFolder )
{
    MDW_CONFLICT_SIDES_RESULT   sides;
    std::shared_ptr<MDW_SOURCE> source;

    std::tie( sides.result, source ) = librarySource( aSource );

    if( !source )
        return sides;

    for( size_t ii = 0; ii < aConflicts.size(); ++ii )
    {
        const MDW_UPDATE_CONFLICT& conflict = aConflicts[ii];
        MDW_CONFLICT_SIDES&        side = sides.sides.emplace_back();
        const wxString             ext = wxS( "." ) + wxFileName( conflict.relative_path ).GetExt();
        const wxString             stem = aFolder + wxFILE_SEP_PATH + wxString::Format( wxS( "%zu_" ), ii );

        side.conflict = conflict;

        auto download = [&]( const wxString& aRevision, const wxString& aName, wxString& aFile )
        {
            if( aRevision.IsEmpty() )
                return true;

            wxString file = stem + aName + ext;

            auto [downloaded, revision] = MdwSync<MDW_REVISION>(
                    [&]( MDW_DONE<MDW_REVISION> aDone )
                    {
                        return source->Download( conflict.item, aRevision, file, std::move( aDone ) );
                    } );

            // A side the backend cannot write, such as a derived symbol, is only left out of the merge
            if( downloaded.IsOk() )
                aFile = file;
            else if( downloaded.error != MDW_ERROR_CODE::NOT_SUPPORTED )
                sides.result = downloaded;

            return sides.result.IsOk();
        };

        if( !download( conflict.base_revision_id, wxS( "ancestor" ), side.ancestorFile )
            || !download( conflict.theirs_revision_id, wxS( "theirs" ), side.theirsFile ) )
        {
            return sides;
        }

        const wxString local = libraryPath( aLocalRoot, conflict.relative_path );

        if( local.IsEmpty() )
        {
            sides.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
            return sides;
        }

        if( !wxFileExists( local ) )
            continue;

        std::ifstream in( ToFsPath( local ), std::ios::binary );

        // Reporting an unreadable local copy as deleted would let the update overwrite it unsaved
        if( !in.is_open() )
        {
            sides.result = MDW_RESULT::Fail( MDW_ERROR_CODE::PERMISSION_DENIED, local );
            return sides;
        }

        std::string text( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

        if( ext == wxS( "." ) + wxString( FILEEXT::KiCadSymbolLibFileExtension ) )
        {
            const KIGIT::SYMBOL_FILE file = KIGIT::SplitSymbols( text );
            auto                     span = file.spans.find( conflict.name.ToStdString( wxConvUTF8 ) );

            side.hasOurs = span != file.spans.end();

            // An extract of a derived symbol would lose its parent
            if( !side.hasOurs || span->second.derived )
                continue;

            text = file.header + text.substr( span->second.start, span->second.length ) + "\n)\n";
        }
        else
        {
            side.hasOurs = true;
        }

        std::ofstream out( ToFsPath( stem + wxS( "ours" ) + ext ), std::ios::binary );

        if( out.write( text.data(), text.size() ) && out.flush() )
            side.oursFile = stem + wxS( "ours" ) + ext;
    }

    return sides;
}


bool MDW_SETUP::SidesMatch( const MDW_CONFLICT_SIDES& aSide )
{
    if( aSide.oursFile.IsEmpty() || aSide.theirsFile.IsEmpty() )
        return false;

    const bool                 symbol = symbolLibrary( aSide.conflict.relative_path );
    std::optional<std::string> ours = readText( aSide.oursFile );
    std::optional<std::string> theirs = readText( aSide.theirsFile );

    if( !ours || !theirs )
        return false;

    // Line endings decide nothing, or a part saved back on Windows never matches its LF blob
    std::optional<std::string> oursMember = memberText( withLf( *ours ), aSide.conflict.name, symbol );

    return oursMember && oursMember == memberText( withLf( *theirs ), aSide.conflict.name, symbol );
}


std::optional<std::string> MDW_SETUP::ReadMember( const wxString& aPath, const MDW_UPDATE_CONFLICT& aConflict )
{
    std::optional<std::string> text = readText( aPath );

    if( !text )
        return std::nullopt;

    return memberText( *text, aConflict.name, symbolLibrary( aConflict.relative_path ) );
}


MDW_SETUP::UPGRADE MDW_SETUP::SymbolUpgrade( KIWAY& aKiway )
{
    return [&aKiway]( const std::string& aDestination, const std::vector<std::string>& aFolder,
                      const std::string& aMemberFile, const wxString& aName ) -> std::optional<std::string>
    {
        KIFACE_CURRENT_SYMBOL_FORMAT_SERVICE::Type upgrade =
                GetKifaceService<KIFACE_CURRENT_SYMBOL_FORMAT_SERVICE>( aKiway.KiFACE( KIWAY::FACE_SCH ) );

        return upgrade ? upgrade( aDestination, aFolder, aMemberFile, aName ) : std::nullopt;
    };
}


std::optional<std::string> MDW_SETUP::SpliceMember( const std::string&              aDestination,
                                                    const std::vector<std::string>& aFolder,
                                                    const std::string& aMemberFile, const wxString& aName,
                                                    const UPGRADE& aUpgrade )
{
    std::optional<std::string> member = memberText( aMemberFile, aName, true );
    const KIGIT::SYMBOL_FILE   file = KIGIT::SplitSymbols( aDestination );
    auto                       span = file.spans.find( aName.utf8_string() );

    // Parser migrations follow the header, so mismatched versions can migrate a symbol twice
    // A missing symbol is placed by a whole save as well
    if( KIGIT::FormatVersion( aDestination ) != KIGIT::FormatVersion( aMemberFile )
        || ( file.complete && span == file.spans.end() ) )
    {
        return aUpgrade ? aUpgrade( aDestination, aFolder, aMemberFile, aName ) : std::nullopt;
    }

    if( !member || !file.complete )
        return std::nullopt;

    // An LF answer spliced into a file saved on Windows would leave it with mixed line endings
    if( aDestination.find( "\r\n" ) != std::string::npos )
        member = withCrLf( *member );

    std::string text = aDestination;
    text.replace( span->second.start, span->second.length, *member );
    return text;
}


bool MDW_SETUP::WriteMember( const wxString& aPath, const MDW_UPDATE_CONFLICT& aConflict, const std::string& aAnswer,
                             const UPGRADE& aUpgrade )
{
    std::optional<std::string> text = aAnswer;

    if( symbolLibrary( aConflict.relative_path ) )
    {
        std::optional<std::string>              destination = readText( aPath );
        std::optional<std::vector<std::string>> folder = folderTexts( aPath );

        text = destination && folder ? SpliceMember( *destination, *folder, aAnswer, aConflict.name, aUpgrade )
                                     : std::nullopt;
    }

    return text && KIPLATFORM::IO::AtomicWriteFile( aPath, text->data(), text->size() );
}


std::optional<std::vector<MDW_CONFLICT_RESOLUTION>>
MDW_SETUP::PlacedAnswers( const wxString& aLocalRoot, const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                          const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions, const UPGRADE& aUpgrade )
{
    struct WHOLE
    {
        std::string              text;
        std::string              version; ///< The version the file was read with
        wxString                 builtFrom;
        std::vector<std::string> folder;
    };

    std::vector<MDW_CONFLICT_RESOLUTION> placed = aResolutions;
    std::map<wxString, WHOLE>            whole;

    auto conflictFor = [&]( const MDW_CONFLICT_RESOLUTION& aResolution ) -> const MDW_UPDATE_CONFLICT*
    {
        if( !aResolution.merged )
            return nullptr;

        auto it = std::find_if( aConflicts.begin(), aConflicts.end(),
                                [&]( const MDW_UPDATE_CONFLICT& aConflict )
                                {
                                    return aConflict.item.item_id == aResolution.item_id;
                                } );

        return it == aConflicts.end() || !symbolLibrary( it->relative_path ) ? nullptr : &*it;
    };

    for( const MDW_CONFLICT_RESOLUTION& resolution : placed )
    {
        const MDW_UPDATE_CONFLICT* conflict = conflictFor( resolution );

        if( !conflict || resolution.merged->empty() || whole.count( conflict->relative_path ) )
            continue;

        const wxString             local = libraryPath( aLocalRoot, conflict->relative_path );
        std::optional<std::string> text = local.IsEmpty() ? std::nullopt : readText( local );
        const KIGIT::SYMBOL_FILE   answer = KIGIT::SplitSymbols( *resolution.merged );
        auto                       span = answer.spans.find( conflict->name.utf8_string() );

        // A derived symbol does not read without its parent, so the backend takes it only inside its whole file
        const bool derived = span != answer.spans.end() && span->second.derived;

        // A file not here yet is written as the answer has it, read by its own header
        if( text && ( derived || KIGIT::FormatVersion( *text ) != KIGIT::FormatVersion( *resolution.merged ) ) )
        {
            std::optional<std::vector<std::string>> folder = folderTexts( local );

            if( !folder )
                return std::nullopt;

            whole[conflict->relative_path] = { *text, KIGIT::FormatVersion( *text ),
                                               wxString::FromUTF8( KIGIT::BlobDigest( *text ) ), *folder };
        }
    }

    // Every answer to a whole file goes into it, or the file would undo the others. Answers in the file's own format
    // go first, spliced before an upgrade leaves none of that format to splice into
    for( bool upgrading : { false, true } )
    {
        for( const MDW_CONFLICT_RESOLUTION& resolution : placed )
        {
            const MDW_UPDATE_CONFLICT* conflict = conflictFor( resolution );
            auto                       file = conflict ? whole.find( conflict->relative_path ) : whole.end();

            if( file == whole.end() )
                continue;

            WHOLE&     target = file->second;
            const bool empty = resolution.merged->empty();

            if( upgrading != ( !empty && KIGIT::FormatVersion( *resolution.merged ) != target.version ) )
                continue;

            std::optional<std::string> next = empty ? withoutMember( target.text, conflict->name )
                                                    : SpliceMember( target.text, target.folder, *resolution.merged,
                                                                    conflict->name, aUpgrade );

            if( !next )
                return std::nullopt;

            target.text = std::move( *next );
        }
    }

    for( MDW_CONFLICT_RESOLUTION& resolution : placed )
    {
        const MDW_UPDATE_CONFLICT* conflict = conflictFor( resolution );
        auto                       file = conflict ? whole.find( conflict->relative_path ) : whole.end();

        if( file != whole.end() )
        {
            resolution.merged = file->second.text;
            resolution.whole_file_of = file->second.builtFrom;
        }
    }

    return placed;
}


MDW_LIBRARY_CHECKIN_RESULT MDW_SETUP::CheckInLibraryCopy( const MDW_LIBRARY_COPY&              aCopy,
                                                          const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                          const wxString&                      aComment )
{
    MDW_LIBRARY_CHECKIN_RESULT checkIn;
    checkIn.copy = aCopy;

    std::optional<MDW_LIBRARY_SOURCE_CONFIG> config = MDW_LIBRARY_SOURCES::Get().Find( aCopy.source_id );

    if( !config )
    {
        checkIn.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
        return checkIn;
    }

    auto [connected, source] = librarySource( *config );

    if( !source )
    {
        checkIn.result = connected;
        return checkIn;
    }

    return checkInLibraryCopy( MDW_LIBRARY_SOURCES::Get(), source, aCopy, aChanges, aComment );
}


MDW_LIBRARY_CHECKIN_RESULT MDW_SETUP::CheckInLibraryCopy( MDW_LIBRARY_SOURCES&                 aSources,
                                                          const std::shared_ptr<MDW_SOURCE>&   aSource,
                                                          const MDW_LIBRARY_COPY&              aCopy,
                                                          const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                          const wxString&                      aComment )
{
    return checkInLibraryCopy( aSources, aSource, aCopy, aChanges, aComment );
}


std::vector<wxString> MDW_SETUP::OutdatedSymbolFiles( const wxString& aPath, long aVersion )
{
    std::vector<wxString> outdated;

    for( const wxString& file : symbolLibraryFiles( aPath ) )
    {
        if( std::optional<long> version = symbolFileVersion( file ); version && *version < aVersion )
            outdated.push_back( file );
    }

    return outdated;
}


bool MDW_SETUP::CheckInKnownDenied( const wxString& aLocator )
{
    std::lock_guard<std::mutex> lock( knownCheckIn().mutex );
    auto                        known = knownCheckIn().permissions.find( aLocator );

    return known != knownCheckIn().permissions.end() && known->second.state() == kiapi::workspace::PS_DENIED;
}


std::optional<MDW_FORMAT_UPGRADE> MDW_SETUP::StageFormatUpgrade( const wxString&                               aPath,
                                                                 const std::function<bool( const wxString& )>& aSave,
                                                                 wxString&                                     aError )
{
    MDW_FORMAT_UPGRADE upgrade;
    upgrade.library = aPath;

    auto readAll = [&]( const wxString& aLibrary, std::map<wxString, std::string>& aTexts )
    {
        for( const wxString& file : symbolLibraryFiles( aLibrary ) )
        {
            std::optional<std::string> text = readText( file );

            if( !text )
            {
                aError = wxString::Format( _( "Could not read '%s'." ), file );
                return false;
            }

            aTexts[wxFileName( file ).GetFullName()] = std::move( *text );
        }

        return true;
    };

    const bool       folder = wxFileName::DirExists( aPath );
    const wxFileName library = folder ? wxFileName( wxFileName::DirName( aPath ).GetPath() ) : wxFileName( aPath );
    SCOPED_TEMP_PATH staging;

    std::map<wxString, std::string> before;
    std::map<wxString, std::string> reread;
    std::map<wxString, std::string> after;

    if( !readAll( aPath, before ) )
        return std::nullopt;

    const wxString output = staging.MakeDirectory( wxS( "kicad_libupgrade_" ), 0700 )
                                    ? staging.Path() + wxFILE_SEP_PATH + library.GetFullName()
                                    : wxString();

    // A folder library is saved into a folder that must already exist
    if( before.empty() || output.IsEmpty() || ( folder && !wxFileName::Mkdir( output ) ) || !aSave( output ) )
    {
        aError = _( "The library could not be saved in the current file format." );
        return std::nullopt;
    }

    // The save read the library from disk, which must still hold what was read before it
    if( !readAll( aPath, reread ) || !readAll( output, after ) )
        return std::nullopt;

    if( reread != before )
    {
        aError = _( "The library changed while it was being upgraded." );
        return std::nullopt;
    }

    for( const auto& [name, text] : before )
    {
        const wxString file = folder ? wxFileName( aPath, name ).GetFullPath() : aPath;
        auto           saved = after.find( name );

        if( saved == after.end() )
        {
            aError = wxString::Format( _( "The upgrade did not save '%s'." ), name );
            return std::nullopt;
        }

        upgrade.before[file] = text;
        upgrade.after[file] = std::move( saved->second );
        after.erase( saved );
    }

    // A file the save added would hold a symbol no one checked in
    if( !after.empty() )
    {
        aError = wxString::Format( _( "The upgrade added '%s'." ), after.begin()->first );
        return std::nullopt;
    }

    return upgrade;
}


MDW_RESULT MDW_SETUP::CheckFormatUpgrade( const MDW_FORMAT_UPGRADE&        aUpgrade,
                                          const KIGIT::LIBRARY_FILES_DIFF& aDiffSymbols )
{
    std::vector<std::string> before;
    std::vector<std::string> after;

    for( const auto& [file, text] : aUpgrade.before )
    {
        auto upgraded = aUpgrade.after.find( file );

        if( upgraded == aUpgrade.after.end() )
            break;

        before.push_back( text );
        after.push_back( upgraded->second );
    }

    KICAD_DIFF::DOCUMENT_DIFF diff;

    // One library of every file, as symbols may derive from symbols in a folder's other files
    if( !aDiffSymbols || before.empty() || before.size() != aUpgrade.after.size()
        || !aDiffSymbols( before, after, &diff ) || !diff.changes.empty() )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                 _( "Saving in the current file format would change a symbol, so the library was "
                                    "not upgraded." ) );
    }

    return MDW_RESULT::Ok();
}


MDW_LIBRARY_CHECKIN_RESULT MDW_SETUP::CheckInFormatUpgrade( const MDW_LIBRARY_COPY&   aCopy,
                                                            const MDW_FORMAT_UPGRADE& aUpgrade, long aVersion,
                                                            const wxString& aComment )
{
    MDW_LIBRARY_CHECKIN_RESULT upgrade;
    upgrade.copy = aCopy;

    std::optional<MDW_LIBRARY_SOURCE_CONFIG> config = MDW_LIBRARY_SOURCES::Get().Find( aCopy.source_id );

    if( !config )
    {
        upgrade.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
        return upgrade;
    }

    auto [connected, source] = librarySource( *config );

    if( !source )
    {
        upgrade.result = connected;
        return upgrade;
    }

    return CheckInFormatUpgrade( MDW_LIBRARY_SOURCES::Get(), source, aCopy, aUpgrade, aVersion, aComment );
}


MDW_LIBRARY_CHECKIN_RESULT MDW_SETUP::CheckInFormatUpgrade( MDW_LIBRARY_SOURCES&               aSources,
                                                            const std::shared_ptr<MDW_SOURCE>& aSource,
                                                            const MDW_LIBRARY_COPY&            aCopy,
                                                            const MDW_FORMAT_UPGRADE& aUpgrade, long aVersion,
                                                            const wxString& aComment )
{
    MDW_LIBRARY_CHECKIN_RESULT upgrade;
    upgrade.copy = aCopy;
    upgrade.result = checkInRefusal( aSource, aCopy );

    auto fail = [&]( MDW_ERROR_CODE aCode, const wxString& aWhy )
    {
        upgrade.result = MDW_RESULT::Fail( aCode, aWhy );
        return upgrade;
    };

    if( !upgrade.result.IsOk() )
        return upgrade;

    if( aUpgrade.before.empty() || aUpgrade.before.size() != aUpgrade.after.size() )
        return fail( MDW_ERROR_CODE::INVALID_PARAMETER, wxEmptyString );

    // A save that skipped a file would leave it in the old format while the library reads as upgraded
    for( const auto& [file, text] : aUpgrade.after )
    {
        if( !aUpgrade.before.contains( file ) || symbolTextVersion( text ).value_or( 0 ) < aVersion )
            return fail( MDW_ERROR_CODE::INTERNAL,
                         wxString::Format( _( "'%s' is not in the current file format." ), file ) );
    }

    // Refused up front, or the upgraded files would stay behind as an edit no one can check in
    if( kiapi::workspace::Permission written = checkInPermission( *aSource );
        written.state() == kiapi::workspace::PS_DENIED )
    {
        return fail( MDW_ERROR_CODE::PERMISSION_DENIED, wxString::FromUTF8( written.reason() ) );
    }

    KIPLATFORM::IO::FILE_LOCK guard;

    if( !lockCopy( aCopy.local_root, guard ) )
        return fail( MDW_ERROR_CODE::BUSY, _( "The library copy is being updated." ) );

    // An edit saved since the upgrade read the files would be lost under it
    for( const auto& [file, text] : aUpgrade.before )
    {
        if( readText( file ) != text )
        {
            return fail( MDW_ERROR_CODE::VERSION_CONFLICT,
                         wxString::Format( _( "'%s' changed while it was being upgraded." ), file ) );
        }
    }

    MDW_LIBRARY_CHANGES_RESULT listed = libraryChanges( aSource, aCopy, KIGIT::LIBRARY_DIFF() );

    if( !listed.result.IsOk() )
    {
        upgrade.result = listed.result;
        return upgrade;
    }

    // A folder library's files are its members, so a row in the folder is a row of the library
    auto inLibrary = [&]( const wxString& aRelativePath )
    {
        const wxFileName file( libraryPath( aCopy.local_root, aRelativePath ) );

        return !aRelativePath.IsEmpty()
               && ( file.SameAs( wxFileName( aUpgrade.library ) )
                    || wxFileName::DirName( file.GetPath() ).SameAs( wxFileName::DirName( aUpgrade.library ) ) );
    };

    // The commit says it revised no symbol, so an edit beside the upgrade could never go in with it
    for( const MDW_LOCAL_CHANGE& change : listed.changes )
    {
        if( inLibrary( change.relative_path ) || inLibrary( change.base_relative_path ) )
        {
            return fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                         _( "Check in or undo the changes to this library first, so that the upgrade changes only "
                            "its format." ) );
        }
    }

    std::vector<MDW_LOCAL_CHANGE> rows;
    std::map<wxString, wxString>  installed;
    wxString                      problems;

    // Only what still holds the upgrade is put back, so a save made since is never lost
    auto restore = [&]( const std::function<bool( const wxString& )>& aWanted )
    {
        for( const auto& [relative, file] : installed )
        {
            if( !aWanted( relative ) )
                continue;

            if( wxString problem = replaceIfUnchanged( file, aUpgrade.after.at( file ), aUpgrade.before.at( file ) );
                !problem.IsEmpty() )
            {
                problems << wxS( "\n" ) << problem;
            }
        }
    };

    for( const auto& [file, text] : aUpgrade.after )
    {
        if( text == aUpgrade.before.at( file ) )
            continue;

        if( wxString problem = replaceIfUnchanged( file, aUpgrade.before.at( file ), text ); !problem.IsEmpty() )
        {
            restore(
                    []( const wxString& )
                    {
                        return true;
                    } );

            return fail( MDW_ERROR_CODE::INTERNAL, problem + problems );
        }

        MDW_LOCAL_CHANGE& row = rows.emplace_back();
        row.relative_path = relativePath( aCopy.local_root, file );
        row.name = row.relative_path;
        row.change = MDW_CHANGE_TYPE::MODIFIED;
        row.format_digest = wxString::FromUTF8( KIGIT::BlobDigest( text ) );
        installed[row.relative_path] = file;
    }

    // The files already were in this format, so there is nothing to commit
    if( rows.empty() )
        return upgrade;

    upgrade = commitLocked( aSources, aSource, aCopy, rows, aComment );

    const bool                           committed = !upgrade.checkin.copy_snapshot.token.IsEmpty();
    const std::vector<MDW_SKIPPED_FILE>& skipped = upgrade.checkin.skipped_files;

    auto wasSkipped = [&]( const wxString& aRelative )
    {
        return std::any_of( skipped.begin(), skipped.end(),
                            [&]( const MDW_SKIPPED_FILE& aSkipped )
                            {
                                return aSkipped.path == aRelative;
                            } );
    };

    // A cancelled check-in may still land, and its files must then stay as committed
    if( !committed && upgrade.result.error != MDW_ERROR_CODE::CANCELLED )
    {
        restore(
                []( const wxString& )
                {
                    return true;
                } );
    }
    else if( committed )
    {
        restore( wasSkipped );
    }

    if( upgrade.result.IsOk() && !skipped.empty() )
    {
        upgrade.result =
                MDW_RESULT::Fail( MDW_ERROR_CODE::PERMISSION_DENIED,
                                  wxString::Format( wxS( "%s: %s" ), skipped.front().path, skipped.front().reason ) );
    }
    else if( upgrade.result.IsOk() && !committed )
    {
        upgrade.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, _( "The upgrade was not checked in." ) );
    }

    upgrade.result.message << problems;
    return upgrade;
}


void MDW_SETUP::AdvanceLibraryCopy( const wxString& aMarker, const wxString& aBase, const wxString& aAskedAt,
                                    const MDW_SNAPSHOT& aTo )
{
    MDW_LIBRARY_SOURCES&                    sources = MDW_LIBRARY_SOURCES::Get();
    std::shared_ptr<const MDW_LIBRARY_COPY> indexed = sources.SharedCopy( aMarker );

    if( aTo.token.IsEmpty() || !indexed || indexed->pinned || indexed->snapshot.token == aTo.token )
        return;

    KIPLATFORM::IO::FILE_LOCK guard;

    if( !lockCopy( indexed->local_root, guard ) )
        return;

    std::optional<MDW_LIBRARY_COPY> copy = sources.Copy( aMarker );

    if( !copy )
        return;

    const wxString& at = copy->snapshot.token;

    bool fromBase = !aBase.IsEmpty() && at == aBase;

    // A retry finishing an earlier attempt's commit builds on a HEAD the index never reached
    bool fromAsked = !aAskedAt.IsEmpty() && at == aAskedAt;

    // Anywhere else a sync or another release wrote a newer snapshot, which must not go back
    if( !fromBase && !fromAsked )
        return;

    copy->snapshot = aTo;
    sources.WriteCopy( *copy );
}


MDW_LIBRARY_CHANGES_RESULT MDW_SETUP::LibraryChangesFor( const MDW_LIBRARY_COPY&    aCopy,
                                                         const KIGIT::LIBRARY_DIFF& aDiffSymbols )
{
    MDW_LIBRARY_CHANGES_RESULT listed;

    std::optional<MDW_LIBRARY_SOURCE_CONFIG> config = MDW_LIBRARY_SOURCES::Get().Find( aCopy.source_id );

    if( !config )
    {
        listed.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );
        return listed;
    }

    auto [connected, source] = librarySource( *config );

    if( !source )
    {
        listed.result = connected;
        return listed;
    }

    return LibraryChangesFor( source, aCopy, aDiffSymbols );
}


MDW_LIBRARY_CHANGES_RESULT MDW_SETUP::LibraryChangesFor( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                         const MDW_LIBRARY_COPY&            aCopy,
                                                         const KIGIT::LIBRARY_DIFF&         aDiffSymbols )
{
    return afterRespawn<MDW_LIBRARY_CHANGES_RESULT>( aSource,
                                                     [&]()
                                                     {
                                                         return libraryChanges( aSource, aCopy, aDiffSymbols );
                                                     } );
}


bool MDW_SETUP::SyncLibraryRows( LIBRARY_TABLE& aTable, MDW_LIBRARY_TYPE aType,
                                 const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                 std::vector<wxString>& aConflicts, const std::set<wxString>& aTaken,
                                 const MDW_LIBRARY_SOURCES& aStore )
{
    const MDW_LIBRARY_SOURCES&                           sources = aStore;
    std::vector<std::shared_ptr<const MDW_LIBRARY_COPY>> copies;
    std::set<wxString>                                   markers;
    std::set<wxString>                                   ids;

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
    {
        const wxString marker = MDW_LIBRARY_SOURCES::Marker( source );

        markers.insert( marker );
        ids.insert( source.id );

        if( std::shared_ptr<const MDW_LIBRARY_COPY> copy = sources.SharedCopy( marker ) )
            copies.push_back( std::move( copy ) );
    }

    // Other sources' rows stay as they are, but a row of a source no longer set up is always stale
    return syncRows(
            aTable, aType, copies, aConflicts, aTaken,
            [&]( const wxString& aMarker )
            {
                return markers.contains( aMarker )
                       || ( !MDW_LIBRARY_SOURCES::IsPinned( aMarker ) && !sources.Find( aMarker ) );
            },
            ids, sources );
}


bool MDW_SETUP::SyncPinnedRows( LIBRARY_TABLE& aTable, MDW_LIBRARY_TYPE aType,
                                const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds,
                                const std::set<wxString>& aReportSources, std::vector<wxString>& aConflicts,
                                const std::set<wxString>& aTaken, const MDW_LIBRARY_SOURCES& aStore )
{
    const MDW_LIBRARY_SOURCES&                           sources = aStore;
    std::vector<std::shared_ptr<const MDW_LIBRARY_COPY>> copies;

    for( const MDW_REQUIREMENT_STATE& state : sources.Match( aNeeds ) )
    {
        if( state.status != MDW_REQUIREMENT_STATUS::SATISFIED || state.requirement.pin.token.IsEmpty() )
            continue;

        std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = sources.Find( state.source_id );
        std::shared_ptr<const MDW_LIBRARY_COPY>  copy;

        if( source )
            copy = sources.SharedCopy( MDW_LIBRARY_SOURCES::Marker( *source, state.requirement.pin ) );

        if( copy )
            copies.push_back( std::move( copy ) );
    }

    // Only pinned copies register project rows, so no other marked row is this sync's to remove
    return syncRows( aTable, aType, copies, aConflicts, aTaken, &MDW_LIBRARY_SOURCES::IsPinned, aReportSources,
                     sources );
}


MDW_LIBRARY_STATUS MDW_SETUP::LibraryStatus( const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds, bool aEnabled )
{
    LIBRARY_MANAGER&          manager = Pgm().GetLibraryManager();
    MDW_LIBRARY_REGISTRATIONS registered;

    // Only the rows that resolve count, so a project row hides the global row of its name
    for( auto [tableType, libraryType] : { std::pair( LIBRARY_TABLE_TYPE::SYMBOL, MDW_LIBRARY_TYPE::SYMBOL ),
                                           std::pair( LIBRARY_TABLE_TYPE::FOOTPRINT, MDW_LIBRARY_TYPE::FOOTPRINT ) } )
    {
        for( const LIBRARY_TABLE_ROW* row : manager.Rows( tableType, LIBRARY_TABLE_SCOPE::BOTH, true ) )
        {
            wxString marker = MDW_LIBRARY_SOURCES::MarkerOf( row->Options() );

            if( !marker.IsEmpty() )
            {
                wxString path = MDW_LIBRARY_SOURCES::ComparablePath( LIBRARY_MANAGER::GetFullURI( row, true ) );
                registered.insert( { libraryType, row->Nickname(), marker, path } );
            }
        }
    }

    return MDW_LIBRARY_SOURCES::Get().Status( aNeeds, aEnabled, registered );
}
