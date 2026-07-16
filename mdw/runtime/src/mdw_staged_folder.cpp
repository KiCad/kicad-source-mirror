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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <mdw_runtime/mdw_staged_folder.h>
#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_file_io.h>
#include <mdw_runtime/mdw_paths.h>

#include "mdw_runtime_internal.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined( _WIN32 )
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace ws = kiapi::workspace;

namespace
{

/// Unlike fs::remove, never deletes a file
std::error_code removeEmptyFolder( const fs::path& aPath )
{
#if defined( _WIN32 )
    int rc = _wrmdir( aPath.c_str() );
#else
    int rc = ::rmdir( aPath.c_str() );
#endif

    if( rc == 0 || errno == ENOENT )
        return std::error_code();

    return std::error_code( errno, std::generic_category() );
}


const char STAGING_TAG[] = ".kicad-mdw-";
const char STAGING_EXT[] = ".staging";
const char STAGING_MARKER[] = ".kicad-mdw-staging";
const char STAGING_COPY[] = "copy";
const size_t NONCE_DIGITS = 16;

const char PERMIT_FOLDER[] = ".kicad";


/// Every removal of a staging sibling, by its own fill or by a later sweep, goes through here
void removeStaging( const fs::path& aStaging )
{
    // The copy may already wear the root's read-only mode, which would leave its content behind
    const fs::perm_options writable = fs::perm_options::add | fs::perm_options::nofollow;
    std::error_code        ignored;

    fs::permissions( aStaging, fs::perms::owner_all, writable, ignored );
    fs::permissions( aStaging / STAGING_COPY, fs::perms::owner_all, writable, ignored );

    // A scanner may still hold a file the fill just wrote, which Windows will not delete meanwhile
    for( int attempt = 0; attempt < 20; ++attempt )
    {
        std::error_code ec;

        // The copy goes first, so a removal cut short keeps the marker a later sweep needs
        fs::remove_all( aStaging / STAGING_COPY, ec );
        fs::remove_all( aStaging, ec );

        if( !ec || !fs::exists( fs::symlink_status( aStaging, ignored ) ) )
            return;

        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    }
}


fs::path stagingPath( const fs::path& aRoot, std::uint64_t aNonce )
{
    char nonce[NONCE_DIGITS + 1];
    std::snprintf( nonce, sizeof( nonce ), "%016llx", static_cast<unsigned long long>( aNonce ) );

    fs::path staging = aRoot;
    staging += STAGING_TAG + std::string( nonce ) + STAGING_EXT;
    return staging;
}


/// Only a name stagingPath could have made, so a folder of the user's that merely looks alike is never taken
bool isStagingName( const fs::path::string_type& aName, const fs::path::string_type& aPrefix )
{
    const fs::path::string_type ext = fs::path( STAGING_EXT ).native();

    if( aName.size() != aPrefix.size() + NONCE_DIGITS + ext.size() || aName.compare( 0, aPrefix.size(), aPrefix ) != 0
        || aName.compare( aPrefix.size() + NONCE_DIGITS, ext.size(), ext ) != 0 )
    {
        return false;
    }

    for( size_t i = aPrefix.size(); i < aPrefix.size() + NONCE_DIGITS; ++i )
    {
        if( !( ( aName[i] >= '0' && aName[i] <= '9' ) || ( aName[i] >= 'a' && aName[i] <= 'f' ) ) )
            return false;
    }

    return true;
}


/// The name of the root whose staging sibling aName could be, or empty when it could be none
fs::path::string_type stagingRootName( const fs::path::string_type& aName )
{
    const fs::path::string_type tag = fs::path( STAGING_TAG ).native();
    const size_t                tail = NONCE_DIGITS + fs::path( STAGING_EXT ).native().size();

    if( aName.size() <= tag.size() + tail )
        return {};

    fs::path::string_type prefix = aName.substr( 0, aName.size() - tail );

    if( prefix.compare( prefix.size() - tag.size(), tag.size(), tag ) != 0 || !isStagingName( aName, prefix ) )
        return {};

    return prefix.substr( 0, prefix.size() - tag.size() );
}


/// Whether aName could be a staging sibling of some other root
bool isStagingLeaf( const fs::path::string_type& aName )
{
    return !stagingRootName( aName ).empty();
}


/// Remove every sibling of aRoot that a fill left when its process died. The caller holds aRoot's permit,
/// so no fill of aRoot is alive anywhere
void removeDeadStaging( const fs::path& aRoot )
{
    const fs::path::string_type prefix = aRoot.filename().native() + fs::path( STAGING_TAG ).native();

    std::error_code       ec;
    std::vector<fs::path> dead;

    for( fs::directory_iterator it( aRoot.parent_path(), ec ), end; !ec && it != end; it.increment( ec ) )
    {
        std::error_code statusError;

        // The marker, not the name, shows a fill made it, since a copy may sit at a root named like one
        if( isStagingName( it->path().filename().native(), prefix )
            && fs::is_directory( fs::symlink_status( it->path(), statusError ) )
            && fs::is_regular_file( fs::symlink_status( it->path() / STAGING_MARKER, statusError ) ) )
        {
            dead.push_back( it->path() );
        }
    }

    for( const fs::path& staging : dead )
        removeStaging( staging );
}


/// Held by every fill of aRoot through its cleanup and never deleted, so a lock file is never swapped under a holder
/// Kept in .kicad beside aRoot with the copy indexes, so it never sits among the user's libraries
fs::path permitPath( const fs::path& aRoot )
{
    fs::path lockPath = aRoot.parent_path() / PERMIT_FOLDER / aRoot.filename();
    lockPath += ".kicad-mdw.lock";
    return lockPath;
}


/// What a fill thread hands back to the call that started it
struct FILL_STATE
{
    std::mutex              mutex;
    std::condition_variable cv;
    bool                    done = false;
    bool                    abandoned = false;
    ws::WorkspaceError      result;
    std::exception_ptr      error;
    std::atomic<bool>       finished = false; ///< The thread has nothing left to do and joins at once
};


struct ABANDONED_FILL
{
    std::thread                 thread;
    std::shared_ptr<FILL_STATE> state;
};


std::mutex                  s_abandonedMutex;
std::vector<ABANDONED_FILL> s_abandoned;


void keepAbandoned( std::thread aThread, std::shared_ptr<FILL_STATE> aState )
{
    std::lock_guard<std::mutex> lock( s_abandonedMutex );

    std::erase_if( s_abandoned,
                   []( ABANDONED_FILL& aFill )
                   {
                       if( !aFill.state->finished )
                           return false;

                       aFill.thread.join();
                       return true;
                   } );

    s_abandoned.push_back( { std::move( aThread ), std::move( aState ) } );
}

} // namespace


void MdwSweepDeadStaging( const fs::path& aFolder )
{
    std::set<fs::path> roots;
    std::error_code    ec;

    for( fs::directory_iterator it( aFolder, ec ), end; !ec && it != end; it.increment( ec ) )
    {
        fs::path::string_type root = stagingRootName( it->path().filename().native() );

        if( !root.empty() )
            roots.insert( aFolder / root );
    }

    for( const fs::path& root : roots )
    {
        std::error_code missing;

        // Every fill takes the permit before it makes a sibling, so a root without one had no fill
        if( !fs::is_regular_file( permitPath( root ), missing ) )
            continue;

        MDW_FILE_LOCK permit( permitPath( root ), false );

        if( permit.Locked() )
            removeDeadStaging( root );
    }
}


void MdwJoinAbandonedFills()
{
    std::vector<ABANDONED_FILL> fills;

    {
        std::lock_guard<std::mutex> lock( s_abandonedMutex );
        fills.swap( s_abandoned );
    }

    for( ABANDONED_FILL& fill : fills )
        fill.thread.join();
}


ws::WorkspaceError MdwFillStagedFolder( const fs::path& aRoot, MDW_FOLDER_FILL aFill )
{
    thread_local std::mt19937_64 random( std::random_device{}() );

    // A trailing separator or "." would put the sibling inside the root. ".." is kept, as a link may precede it
    fs::path root = aRoot;

    while( root.has_relative_path() && ( root.filename().empty() || root.filename() == "." ) )
        root = root.parent_path();

    if( !root.has_relative_path() || root.filename() == ".." )
        return MdwFail( ws::WE_INVALID_PARAMETER, MdwUtf8Path( aRoot ) + " cannot hold a library copy" );

    if( isStagingLeaf( root.filename().native() ) )
    {
        return MdwFail( ws::WE_INVALID_PARAMETER,
                        MdwUtf8Path( aRoot ) + " is named like a temporary folder and cannot hold a library copy" );
    }

    if( root.filename() == PERMIT_FOLDER )
    {
        return MdwFail( ws::WE_INVALID_PARAMETER,
                        MdwUtf8Path( aRoot ) + " is reserved and cannot hold a library copy" );
    }

    std::error_code ec;
    fs::file_status existing = fs::status( root, ec );
    const bool      replacing = fs::exists( existing );

    if( ec && !replacing && existing.type() != fs::file_type::not_found )
        return MdwFail( ws::WE_INTERNAL, "Cannot read " + MdwUtf8Path( root ) + ": " + ec.message() );

    fs::path       staging = stagingPath( root, random() );
    const fs::path permits = permitPath( root ).parent_path();
    ec.clear();
    fs::create_directories( permits, ec );

    if( ec )
        return MdwFail( ws::WE_INTERNAL, "Cannot create " + MdwUtf8Path( permits ) + ": " + ec.message() );

    // One fill per root, so cancel and retry never stack copies. An abandoned fill keeps it until it is gone
    auto permit = std::make_shared<MDW_FILE_LOCK>( permitPath( root ), false );

    while( !permit->Locked() )
    {
        if( !permit->Busy() )
            return MdwFail( ws::WE_INTERNAL, "Cannot lock " + MdwUtf8Path( permitPath( root ) ) );

        if( MdwCurrentRequestCancelled() )
            return MdwCancelled();

        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        permit = std::make_shared<MDW_FILE_LOCK>( permitPath( root ), false );
    }

    removeDeadStaging( root );

    // Created exclusively, so a folder this call did not make is never filled or removed
    if( !fs::create_directory( staging, ec ) )
    {
        return MdwFail( ws::WE_INTERNAL,
                        "Cannot create " + MdwUtf8Path( staging ) + ( ec ? ": " + ec.message() : "" ) );
    }

    auto abandon = [&]( const ws::WorkspaceError& aError )
    {
        removeStaging( staging );
        return aError;
    };

    // The marker goes in first and never moves into the root, and the fill gets an empty folder beside it
    const fs::path copy = staging / STAGING_COPY;

    if( !std::ofstream( staging / STAGING_MARKER ) || !fs::create_directory( copy, ec ) )
        return abandon( MdwFail( ws::WE_INTERNAL, "Cannot set up " + MdwUtf8Path( staging ) ) );

    // The fill must be able to write, so the root's exact mode waits until it is done
    if( replacing )
        fs::permissions( copy, existing.permissions() | fs::perms::owner_all, ec );

    if( ec )
        return abandon( MdwFail( ws::WE_INTERNAL, "Cannot set up " + MdwUtf8Path( staging ) + ": " + ec.message() ) );

    auto state = std::make_shared<FILL_STATE>();

    auto run = [state, staging, copy, fill = std::move( aFill ), cancelled = MdwCurrentRequestCancelToken(),
                progress = MdwCurrentRequestProgress(), permit]() mutable
    {
        MDW_REQUEST_CANCEL_SCOPE   scope( cancelled );
        MDW_REQUEST_PROGRESS_SCOPE progressScope( progress );
        ws::WorkspaceError         result;
        std::exception_ptr         error;
        bool                       abandoned = false;

        try
        {
            result = fill( copy );
        }
        catch( ... )
        {
            error = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lock( state->mutex );
            abandoned = state->abandoned;
            state->result = std::move( result );
            state->error = error;
            state->done = true;
            state->cv.notify_all();
        }

        if( abandoned )
        {
            removeStaging( staging );
            permit.reset();
        }

        state->finished = true;
    };

    // A fill may see a cancel only between steps, so a cancelled call answers at once and the fill cleans up alone
    std::thread worker;

    try
    {
        worker = std::thread( std::move( run ) );
    }
    catch( ... )
    {
        abandon( MdwOk() );
        throw;
    }

    std::unique_lock<std::mutex> lock( state->mutex );

    while( !state->cv.wait_for( lock, std::chrono::milliseconds( 50 ),
                                [&state]()
                                {
                                    return state->done;
                                } ) )
    {
        if( MdwCurrentRequestCancelled() )
        {
            state->abandoned = true;
            lock.unlock();
            keepAbandoned( std::move( worker ), std::move( state ) );
            return MdwCancelled();
        }
    }

    lock.unlock();
    worker.join();

    if( state->error )
    {
        abandon( MdwOk() );
        std::rethrow_exception( state->error );
    }

    ws::WorkspaceError result = state->result;

    if( result.code() == ws::WE_OK && MdwCurrentRequestCancelled() )
        result = MdwCancelled();

    if( result.code() != ws::WE_OK )
        return abandon( result );

    // The copy takes the place of the folder it replaces, so a private one stays private
    if( replacing )
        fs::permissions( copy, existing.permissions(), ec );

    if( ec )
    {
        return abandon( MdwFail( ws::WE_INTERNAL,
                                 "Cannot keep the permissions of " + MdwUtf8Path( root ) + ": " + ec.message() ) );
    }

    // Only an empty folder is removed, and the rename refuses anything that appeared there meanwhile
    ec = removeEmptyFolder( root );

    if( !ec )
        fs::rename( copy, root, ec );

    if( ec )
    {
        return abandon( MdwFail( ws::WE_VERSION_CONFLICT,
                                 MdwUtf8Path( root ) + " changed while the copy was made: " + ec.message() ) );
    }

    // Only the marker is left, which a later sweep removes if this fails
    removeStaging( staging );
    return result;
}
