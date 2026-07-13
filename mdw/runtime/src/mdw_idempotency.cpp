/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mdw_runtime/mdw_file_io.h>
#include <mdw_runtime/mdw_strings.h>

#include "mdw_idempotency.h"
#include "mdw_runtime_internal.h"

#include <api/workspace/workspace_types.pb.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <system_error>

namespace fs = std::filesystem;

namespace
{

/// Keys hash onto a fixed set of lock files, so the lock files never need pruning
const unsigned LOCK_STRIPES = 64;

const char INTENT_EXT[] = ".intent";
const char RESULT_EXT[] = ".result";

/// Leads every record, so a file of any other format never parses as one
const char RECORD_MAGIC[] = "MDWJ1";


void putU64( std::string& aOut, std::uint64_t aValue )
{
    for( int i = 0; i < 8; ++i )
        aOut += static_cast<char>( ( aValue >> ( 8 * i ) ) & 0xff );
}


bool getU64( const std::string& aIn, std::size_t& aPos, std::uint64_t& aValue )
{
    if( aIn.size() - aPos < 8 )
        return false;

    aValue = 0;

    for( int i = 7; i >= 0; --i )
        aValue = ( aValue << 8 ) | static_cast<unsigned char>( aIn[aPos + i] );

    aPos += 8;
    return true;
}


const std::size_t HASH_DIGITS = 16;


/// The fixed-length file stem of a key, whose leading hash also picks its lock stripe
std::string recordName( const std::string& aKey )
{
    return MdwHashHex( aKey ) + MdwHashHex( aKey, 0x6c62272e07bb0142ULL );
}


bool isRecordName( const std::string& aStem )
{
    return aStem.size() == 2 * HASH_DIGITS && aStem.find_first_not_of( "0123456789abcdef" ) == std::string::npos;
}

} // namespace


MDW_IDEMPOTENCY_STORE::MDW_IDEMPOTENCY_STORE( const fs::path& aDir ) :
        m_dir( aDir )
{
    std::error_code ec;
    fs::create_directories( m_dir, ec );

    m_open = fs::is_directory( m_dir, ec );
    m_lastPrune = std::chrono::steady_clock::now();

    if( m_open )
        prune();
}


std::int64_t MDW_IDEMPOTENCY_STORE::nowSeconds() const
{
    return std::chrono::duration_cast<std::chrono::seconds>( std::chrono::system_clock::now().time_since_epoch() )
            .count();
}


bool MDW_IDEMPOTENCY_STORE::expired( const RECORD& aRecord ) const
{
    return aRecord.createdAt < nowSeconds() - std::chrono::duration_cast<std::chrono::seconds>( RETENTION ).count();
}


void MDW_IDEMPOTENCY_STORE::prune()
{
    const fs::file_time_type cutoff = fs::file_time_type::clock::now() - RETENTION;
    std::error_code          ec;

    // The range-for increment throws, which would terminate the worker running a prune
    for( fs::directory_iterator it( m_dir, ec ), end; !ec && it != end; it.increment( ec ) )
    {
        const fs::directory_entry& entry = *it;
        std::error_code            entryEc;
        const fs::path&            path = entry.path();
        std::string                extension = path.extension().string();

        // Lock stripes stay, since removing one another process holds would split its lock
        if( extension == ".lock" || !entry.is_regular_file( entryEc ) )
            continue;

        const std::string stem = path.stem().string();

        if( ( extension == INTENT_EXT || extension == RESULT_EXT ) && isRecordName( stem ) )
        {
            // A request holding the stripe may be writing a fresh record under this name, so one in use
            // is left for a later pass
            MDW_FILE_LOCK stripe( stripePath( stem ), false );
            RECORD    record;

            if( !stripe.Locked() )
                continue;

            LOAD state = load( path, record );

            // An unreadable record has no creation time, so its file age stands in
            if( ( state == LOAD::FOUND && expired( record ) )
                || ( state == LOAD::DAMAGED && entry.last_write_time( entryEc ) < cutoff && !entryEc ) )
            {
                fs::remove( path, entryEc );
            }
        }
        else if( entry.last_write_time( entryEc ) < cutoff && !entryEc )
        {
            // Abandoned temporaries and stray files, which no live request can reuse
            fs::remove( path, entryEc );
        }
    }
}


fs::path MDW_IDEMPOTENCY_STORE::recordPath( const std::string& aKey, const char* aExtension ) const
{
    // A digest keeps client bytes out of the name and the path short enough for Windows
    return m_dir / ( recordName( aKey ) + aExtension );
}


fs::path MDW_IDEMPOTENCY_STORE::stripePath( const std::string& aName ) const
{
    const std::uint64_t hash = std::stoull( aName.substr( 0, HASH_DIGITS ), nullptr, 16 );

    return m_dir / ( "stripe-" + std::to_string( hash % LOCK_STRIPES ) + ".lock" );
}


MDW_IDEMPOTENCY_STORE::LOAD MDW_IDEMPOTENCY_STORE::load( const fs::path& aPath, RECORD& aRecord )
{
    std::error_code ec;

    // libstdc++ reports a missing file through ec as well, so the type is what tells absence apart
    if( fs::symlink_status( aPath, ec ).type() == fs::file_type::not_found )
        return LOAD::ABSENT;

    std::optional<std::string> read = MDW_ReadFile( aPath );

    if( !read )
        return LOAD::DAMAGED;

    const std::string& data = *read;

    const std::size_t magicSize = sizeof( RECORD_MAGIC ) - 1;
    std::size_t       pos = magicSize;
    std::uint64_t     createdAt = 0;
    std::uint64_t     requestSize = 0;

    if( data.compare( 0, magicSize, RECORD_MAGIC ) != 0 || !getU64( data, pos, createdAt )
        || !getU64( data, pos, requestSize ) || requestSize > data.size() - pos )
    {
        return LOAD::DAMAGED;
    }

    aRecord.createdAt = static_cast<std::int64_t>( createdAt );
    aRecord.request = data.substr( pos, requestSize );
    aRecord.body = data.substr( pos + requestSize );
    return LOAD::FOUND;
}


bool MDW_IDEMPOTENCY_STORE::save( const fs::path& aPath, const RECORD& aRecord ) const
{
    std::string data = RECORD_MAGIC;
    putU64( data, static_cast<std::uint64_t>( aRecord.createdAt ) );
    putU64( data, aRecord.request.size() );
    data += aRecord.request;
    data += aRecord.body;

    return !MDW_ReplaceFile( aPath, data, 0600 ) && MDW_SyncDirectory( m_dir );
}


void MDW_IDEMPOTENCY_STORE::Run( const std::string& aKey, const std::string& aRequestBytes,
                                 kiapi::mdw::MdwResponse&                                aResponse,
                                 const std::function<void( const std::string& aToken )>& aHandler,
                                 const RECOVERY&                                         aRecovery )
{
    if( aKey.size() > MAX_KEY_LENGTH )
    {
        aResponse.mutable_error()->set_code( kiapi::workspace::WE_INVALID_PARAMETER );
        aResponse.mutable_error()->set_message( "idempotency_id is too long" );
        return;
    }

    bool pruneDue = false;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( std::chrono::steady_clock::now() - m_lastPrune > std::chrono::hours( 1 ) )
        {
            m_lastPrune = std::chrono::steady_clock::now();
            pruneDue = true;
        }
    }

    if( pruneDue )
        prune();

    // A retry that arrives while the original is still running waits for its result
    MDW_FILE_LOCK stripe( stripePath( recordName( aKey ) ) );

    if( !stripe.Locked() )
    {
        aResponse.mutable_error()->set_code( kiapi::workspace::WE_INTERNAL );
        aResponse.mutable_error()->set_message( "Cannot lock the idempotency store" );
        return;
    }

    runLocked( aKey, aRequestBytes, aResponse, aHandler, aRecovery );
}


void MDW_IDEMPOTENCY_STORE::runLocked( const std::string& aKey, const std::string& aRequestBytes,
                                       kiapi::mdw::MdwResponse&                                aResponse,
                                       const std::function<void( const std::string& aToken )>& aHandler,
                                       const RECOVERY&                                         aRecovery )
{
    // The envelope fields the server already echoed must survive, so only the outcome is reset
    auto clearOutcome = [&]()
    {
        aResponse.clear_error();
        aResponse.clear_payload();
    };

    auto fail = [&]( kiapi::workspace::WorkspaceErrorCode aCode, const std::string& aMessage )
    {
        clearOutcome();
        aResponse.mutable_error()->set_code( aCode );
        aResponse.mutable_error()->set_message( aMessage );
    };

    const fs::path intentPath = recordPath( aKey, INTENT_EXT );
    const fs::path resultPath = recordPath( aKey, RESULT_EXT );
    RECORD         intent;
    RECORD         result;
    LOAD           intentState = load( intentPath, intent );
    LOAD           resultState = load( resultPath, result );

    // Guessing past a record that cannot be read could apply the operation twice
    if( intentState == LOAD::DAMAGED || resultState == LOAD::DAMAGED )
    {
        fail( kiapi::workspace::WE_INTERNAL, "the idempotency record is unreadable" );
        return;
    }

    if( ( intentState == LOAD::FOUND && expired( intent ) ) || ( resultState == LOAD::FOUND && expired( result ) ) )
    {
        std::error_code ec;
        fs::remove( resultPath, ec );
        fs::remove( intentPath, ec );
        intentState = LOAD::ABSENT;
        resultState = LOAD::ABSENT;
    }

    const RECORD* existing = resultState == LOAD::FOUND ? &result : intentState == LOAD::FOUND ? &intent : nullptr;

    if( existing && existing->request != aRequestBytes )
    {
        fail( kiapi::workspace::WE_INVALID_PARAMETER, "idempotency_id was already used for a different request" );
        return;
    }

    if( resultState == LOAD::FOUND )
    {
        kiapi::mdw::MdwResponse stored;

        if( !stored.ParseFromString( result.body ) )
        {
            fail( kiapi::workspace::WE_INTERNAL, "the idempotency record is unreadable" );
            return;
        }

        *aResponse.mutable_error() = stored.error();
        *aResponse.mutable_payload() = stored.payload();
        return;
    }

    std::int64_t createdAt = nowSeconds();
    bool         replayed = false;
    std::string  token;

    if( intentState == LOAD::FOUND )
    {
        // Only a crash or a throwing handler leaves an intent, and repeating that could undo later work
        if( !aRecovery.recover )
        {
            fail( kiapi::workspace::WE_VERSION_CONFLICT, "interrupted request needs reconciliation" );
            return;
        }

        MDW_RECOVERY found = aRecovery.recover( intent.body, aResponse );

        if( found == MDW_RECOVERY::FAIL )
        {
            if( aResponse.error().code() == kiapi::workspace::WE_OK )
                fail( kiapi::workspace::WE_INTERNAL, "cannot tell whether an earlier attempt took effect" );

            return;
        }

        // The retry inherits the first attempt's age, so recovery never extends retention
        createdAt = intent.createdAt;
        replayed = found == MDW_RECOVERY::REPLAY;
        token = intent.body;

        if( !replayed )
            clearOutcome();
    }
    else
    {
        RECORD fresh;
        fresh.createdAt = createdAt;
        fresh.request = aRequestBytes;

        if( aRecovery.token && !aRecovery.token( token, aResponse ) )
        {
            if( aResponse.error().code() == kiapi::workspace::WE_OK )
                fail( kiapi::workspace::WE_INTERNAL, "cannot read the state the request would change" );

            return;
        }

        fresh.body = token;

        if( !save( intentPath, fresh ) )
        {
            fail( kiapi::workspace::WE_INTERNAL, "cannot record the request before applying it" );
            return;
        }

        clearOutcome();
    }

    if( !replayed )
        aHandler( token );

    // A returned failure promises no effect, so the id is free to run again from scratch
    if( aResponse.error().code() != kiapi::workspace::WE_OK )
    {
        std::error_code ec;
        fs::remove( intentPath, ec );

        // A surviving intent would answer the retry as interrupted, so the client must not be told it may retry
        if( ec || !MDW_SyncDirectory( m_dir ) )
            fail( kiapi::workspace::WE_INTERNAL, "cannot clear the failed request from the idempotency store" );

        return;
    }

    RECORD done;
    done.createdAt = createdAt;
    done.request = aRequestBytes;

    kiapi::mdw::MdwResponse stored;
    *stored.mutable_error() = aResponse.error();
    *stored.mutable_payload() = aResponse.payload();
    done.body = stored.SerializeAsString();

    // Acknowledging a change that a retry could not replay would invite a second application
    if( !save( resultPath, done ) )
        fail( kiapi::workspace::WE_INTERNAL, "the change was applied but could not be recorded; retry to confirm it" );
}
