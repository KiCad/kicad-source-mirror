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

#include <mdw/client/mdw_revision_copy.h>

#include <mdw/client/mdw_op_state.h>
#include <mdw/client/mdw_source.h>

#include <gestfich.h>
#include <kiplatform/io.h>

#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <mutex>
#include <random>
#include <set>
#include <system_error>
#include <thread>
#include <vector>


namespace
{

/// Part files a backend may still write, swept once no backend or client worker is left to write them
std::mutex         s_partsMutex;
std::set<wxString> s_parts;


// Beside the target, so the final rename never crosses a filesystem
wxString partFileFor( const wxString& aTarget )
{
    std::random_device random;

    return aTarget + wxString::Format( wxS( ".part%08x" ), static_cast<unsigned>( random() ) );
}


void holdPart( const wxString& aPath )
{
    std::lock_guard<std::mutex> lock( s_partsMutex );
    s_parts.insert( aPath );
}


/// Remove the part file at aPath, true once it is gone
bool removeExactPart( const wxString& aPath )
{
    std::error_code ec;

    std::filesystem::remove( ToFsPath( aPath ), ec );
    return !ec;
}


/// Remove the part file at aPath and the staging files a backend writes beside it, true once none is left
bool removePart( const wxString& aPath )
{
    namespace fs = std::filesystem;

    const fs::path  part = ToFsPath( aPath );
    std::error_code ec;
    bool            gone = removeExactPart( aPath );

    // A backend killed mid-write leaves <part>.kicad-mdw-<decimal nonce>.tmp. Only the ASCII tail is matched, since a
    // listing may spell the name before it in another Unicode form
    const wxString        marker = FromFsPath( part.extension() ) + wxS( ".kicad-mdw-" );
    std::vector<wxString> nonces;

    for( fs::directory_iterator it( part.parent_path(), ec ), end; !ec && it != end; it.increment( ec ) )
    {
        const wxString name = FromFsPath( it->path().filename() );
        const size_t   at = name.rfind( marker );
        wxString       nonce;

        if( at == wxString::npos || !name.Mid( at + marker.length() ).EndsWith( wxS( ".tmp" ), &nonce )
            || nonce.IsEmpty() )
        {
            continue;
        }

        if( std::all_of( nonce.begin(), nonce.end(),
                         []( const wxUniChar& c )
                         {
                             return c >= '0' && c <= '9';
                         } ) )
        {
            nonces.push_back( nonce );
        }
    }

    gone = gone && !ec;

    // Built from the registered spelling, so the filesystem decides which file that names
    for( const wxString& nonce : nonces )
    {
        fs::path staging = part;

        staging += ToFsPath( wxS( ".kicad-mdw-" ) + nonce + wxS( ".tmp" ) );
        fs::remove( staging, ec );
        gone = gone && !ec;
    }

    return gone;
}


/// Remove aPath and, once aResult shows the backend has finished with it and it is gone, stop tracking it
void settlePart( const wxString& aPath, const MDW_RESULT& aResult )
{
    // A successful reply means the backend already renamed its staging file onto the part
    const bool gone = aResult.IsOk() ? removeExactPart( aPath ) : removePart( aPath );

    // Kept while a file lingers, or while a transport that gave up leaves the backend free to land its write
    if( !gone || aResult.error == MDW_ERROR_CODE::TIMEOUT || aResult.error == MDW_ERROR_CODE::CANCELLED
        || aResult.error == MDW_ERROR_CODE::BACKEND_LOST )
    {
        return;
    }

    std::lock_guard<std::mutex> lock( s_partsMutex );
    s_parts.erase( aPath );
}


/// Move the downloaded aPart onto aTarget, which must not exist unless aReplace
MDW_RESULT commitPart( const wxString& aPart, const wxString& aTarget, bool aReplace )
{
    wxString error;

    if( aReplace )
    {
        if( !KIPLATFORM::IO::CommitTempFile( aPart, aTarget, &error ) )
            return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, error );

        return MDW_RESULT::Ok();
    }

    bool exists = false;

    if( !KIPLATFORM::IO::RenameNoReplace( aPart, aTarget, &exists, &error ) )
    {
        // Made by another program during the download, after the user chose a name nothing held
        if( exists )
        {
            return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                     wxString::Format( _( "%s now exists. Save the copy again to choose whether to "
                                                          "replace it." ),
                                                       aTarget ) );
        }

        return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, error );
    }

    // The copy is in place, so a directory that cannot be flushed only risks it on a power loss
    KIPLATFORM::IO::FlushDirectory( wxFileName( aTarget ).GetPath() );
    return MDW_RESULT::Ok();
}


/// Set while a sweep runs, which may outlive the wait for it
std::atomic<bool> s_sweeping = false;


void sweepParts( const std::set<wxString>& aParts )
{
    for( const wxString& part : aParts )
    {
        // One that could not be removed is kept for a later stop to retry
        if( removePart( part ) )
        {
            std::lock_guard<std::mutex> lock( s_partsMutex );
            s_parts.erase( part );
        }
    }

    s_sweeping = false;
}


/// The download and its holder, freed once the download's callback has run or it was cancelled
struct COPY_RUN
{
    std::shared_ptr<MDW_OP_STATE> state = std::make_shared<MDW_OP_STATE>();
    wxString                      part;

    std::mutex mutex;
    MDW_OP     download;
    bool       closed = false;

    /// Mark the run over and hand back the download, which the caller destroys outside the mutex.
    /// False when the run was already over
    bool Close( MDW_OP& aDownload )
    {
        std::lock_guard<std::mutex> lock( mutex );

        if( closed )
            return false;

        closed = true;
        aDownload = std::move( download );
        return true;
    }

    void Cancel()
    {
        MDW_OP stale;

        if( !Close( stale ) )
            return;

        // Left running to its callback, which removes what a late write leaves behind. Never touching the
        // destination here keeps a dead network share from hanging the UI thread
        stale.RequestStop();
        stale.Detach();

        // No callback is left to retire this op
        state->Complete( {} );
    }
};

} // namespace


MDW_OP MdwSaveRevisionCopy( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                            const wxString& aRevisionId, const wxString& aTarget, bool aReplace,
                            MDW_DONE<MDW_REVISION> aDone )
{
    auto run = std::make_shared<COPY_RUN>();
    run->part = partFileFor( aTarget );
    holdPart( run->part );

    // The download answers on the client worker, which must never copy or end the caller's callback
    run->state->Hold<MDW_REVISION>( std::move( aDone ) );

    run->state->SetCanceller(
            [weak = std::weak_ptr<COPY_RUN>( run )]()
            {
                if( std::shared_ptr<COPY_RUN> live = weak.lock() )
                    live->Cancel();
            } );

    // Completes on the client worker whatever the delivery mode, so the part file is settled even when no UI loop
    // runs to deliver a cancelled copy's late answer
    MDW_SYNC_SCOPE scope;

    MDW_OP download = aSource->Download(
            aItem, aRevisionId, run->part,
            [run, aTarget, aReplace]( const MDW_RESULT& aResult, const MDW_REVISION& aRevision )
            {
                // A cancel that got here first owns the cleanup, so nothing is committed after it
                MDW_OP finished;
                const bool live = run->Close( finished );
                finished = MDW_OP();

                if( !live )
                {
                    settlePart( run->part, aResult );
                    return;
                }

                const MDW_RESULT result = aResult.IsOk() ? commitPart( run->part, aTarget, aReplace ) : aResult;

                settlePart( run->part, aResult );

                run->state->Answer( result, result.IsOk() ? aRevision : MDW_REVISION() );
            } );

    {
        std::lock_guard<std::mutex> lock( run->mutex );

        if( !run->closed )
            run->download = std::move( download );
    }

    return MDW_OP( run->state );
}


bool MdwSweepRevisionCopies( std::chrono::milliseconds aTimeout )
{
    if( s_sweeping.exchange( true ) )
        return false;

    std::set<wxString> parts;

    {
        std::lock_guard<std::mutex> lock( s_partsMutex );
        parts = s_parts;
    }

    if( parts.empty() )
    {
        s_sweeping = false;
        return true;
    }

    auto              swept = std::make_shared<std::promise<void>>();
    std::future<void> done = swept->get_future();

    // Detached, so a destination share that never answers cannot hold the caller past aTimeout
    try
    {
        std::thread(
                [swept, parts = std::move( parts )]()
                {
                    sweepParts( parts );
                    swept->set_value();
                } )
                .detach();
    }
    catch( const std::system_error& )
    {
        s_sweeping = false;
        return false;
    }

    return done.wait_for( aTimeout ) == std::future_status::ready;
}


bool MdwRevisionSweepRunning()
{
    return s_sweeping;
}
