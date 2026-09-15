/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kigit_transport.h"

#include "kigit_ssh_keys.h"

#include <git/kicad_git_compat.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>

#include <wx/log.h>
#include <wx/translation.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>


namespace KIGIT
{

bool SetServerTimeouts()
{
#if ( LIBGIT2_VER_MAJOR > 1 ) || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 7 )
    return git_libgit2_opts( GIT_OPT_SET_SERVER_CONNECT_TIMEOUT, SERVER_CONNECT_TIMEOUT_MS ) == GIT_OK
           && git_libgit2_opts( GIT_OPT_SET_SERVER_TIMEOUT, SERVER_TRANSFER_TIMEOUT_MS ) == GIT_OK;
#else
    return true;
#endif
}


KIGIT_ERROR_INFO GetLastGitErrorInfo( int aCode )
{
    KIGIT_ERROR_INFO info;
    info.code = aCode;

    // libgit2 before 1.8 answers null rather than "no error"
    if( const git_error* error = git_error_last() )
    {
        info.klass = error->klass;
        info.message = wxString::FromUTF8( error->message ? error->message : "" );
    }

    return info;
}


/// True when a leased update names a ref the remote holds at another commit than the lease expects
static bool LeaseBroken( const git_push_update** aUpdates, size_t aCount, const std::vector<GIT_REF_UPDATE>& aWanted )
{
    for( size_t i = 0; i < aCount; ++i )
    {
        const git_push_update* update = aUpdates[i];
        wxString               remoteRef = wxString::FromUTF8( update->dst_refname );
        wxString               remoteOld;

        if( !git_oid_is_zero( &update->src ) )
            remoteOld = wxString::FromUTF8( git_oid_tostr_s( &update->src ) );

        for( const GIT_REF_UPDATE& wanted : aWanted )
        {
            if( wanted.leased && wanted.remoteRef == remoteRef && !wanted.expectedOld.IsSameAs( remoteOld, false ) )
                return true;
        }
    }

    return false;
}


struct REMOTE_TRANSPORT::STATE
{
    TRANSPORT_SESSION* session = nullptr;
    unsigned           tested = 0; ///< The git_credential_t kinds this call has offered
    bool               agentTried = false;
    KIGIT_ERROR_INFO   error;

    /// Set only while PushRefs runs
    const std::vector<GIT_REF_UPDATE>* updates = nullptr;
    std::vector<GIT_REF_RESULT>*       results = nullptr;
    bool                               leaseBroken = false;

    /// A checkout in batches reports each batch's progress past the files the batches before it wrote
    size_t checkoutDone = 0;
    size_t checkoutTotal = 0;

    /// Every call offers every credential again and keeps nothing from an earlier failure
    void Begin()
    {
        tested = 0;
        agentTried = false;
        error = KIGIT_ERROR_INFO();
        leaseBroken = false;
        session->ResetTransferState();
    }

    int Fail( int aCode )
    {
        error = GetLastGitErrorInfo( aCode );
        return aCode;
    }
};


namespace
{
    using STATE = REMOTE_TRANSPORT::STATE;


    wxString lastGitError()
    {
        // libgit2 quotes config lines and URLs verbatim, passwords included
        return StripEmbeddedUserinfo( GetLastGitErrorInfo( 0 ).message );
    }


    int refuseAuthentication( STATE& aState )
    {
        // Answered as a refusal rather than "no callback set", so the caller prompts and retries
        git_error_clear();
        git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
        aState.session->SetAuthFailure();
        return GIT_EAUTH;
    }


    // The server accepted the key's public half. Refusing to sign ends the request before any login is tried.
    // Recorded here because the server may disconnect before libgit2 asks for another credential
    extern "C" int probeSignCb( LIBSSH2_SESSION*, unsigned char**, size_t*, const unsigned char*, size_t,
                                void** aAbstract )
    {
        if( !aAbstract || !*aAbstract )
            return -1;

        KIGIT_AUTH_ATTEMPT* attempt = static_cast<KIGIT_AUTH_ATTEMPT*>( *aAbstract );
        attempt->lastKeyRefused = false;

        if( attempt->acceptedKey.IsEmpty() )
        {
            attempt->acceptedKey = attempt->probing;
            attempt->passphraseGiven = false;
        }

        return -1;
    }


    /// Offer the agent's keys once, then the next key file, recording in the attempt which file was offered
    int offerSSHKey( STATE& aState, git_credential** aOut, const wxString& aUsername )
    {
        KIGIT_AUTH_ATTEMPT& attempt = aState.session->AuthAttempt();

        if( !aState.agentTried )
        {
            aState.agentTried = true;

            if( git_credential_ssh_key_from_agent( aOut, aUsername.utf8_str() ) == GIT_OK )
            {
                attempt.agentTried = true;
                return GIT_OK;
            }

            wxLogTrace( traceKigit, "Failed to create SSH agent credential: %s", lastGitError() );
        }

        if( !attempt.probing.IsEmpty() )
        {
            wxLogTrace( traceKigit, attempt.lastKeyRefused ? "Server refuses %s" : "Server accepts %s",
                        attempt.probing );
            attempt.probing.clear();
        }

        wxString password = aState.session->GetPassword();
        wxString sshKey = aState.session->GetNextPublicKey();

        if( sshKey.IsEmpty() )
        {
            wxLogTrace( traceKigit, "Finished testing all possible ssh keys" );
            aState.tested |= GIT_CREDENTIAL_SSH_KEY;
            return GIT_PASSTHROUGH;
        }

        // Without a passphrase no encrypted key can sign, so asking the server first is the only way to learn which
        // one it takes. With one, the probe's extra round would cost a counted attempt for nothing
        if( password.IsEmpty() && IsPrivateKeyFileEncrypted( sshKey ) )
        {
            const std::string blob = PublicKeyBlob( sshKey );

            if( !blob.empty()
                && git_credential_ssh_custom_new( aOut, aUsername.utf8_str(), blob.data(), blob.size(), probeSignCb,
                                                  &attempt )
                           == GIT_OK )
            {
                wxLogTrace( traceKigit, "Asking whether the server takes %s", sshKey );
                attempt.probing = sshKey;
                attempt.lastKeyRefused = true;
                attempt.keysTried.push_back( sshKey );
                return GIT_OK;
            }
        }

        wxString sshPubKey = PublicKeyPath( sshKey );

        wxLogTrace( traceKigit, "Testing %s\n", sshKey );

        // libssh2 opens the key files with fopen, which reads the path in the locale's encoding
        if( git_credential_ssh_key_new( aOut, aUsername.utf8_str(), sshPubKey.mbc_str(), sshKey.mbc_str(),
                                        password.utf8_str() )
            != GIT_OK )
        {
            wxLogTrace( traceKigit, "Failed to create SSH key credential for %s: %s", sshKey, lastGitError() );
            return GIT_PASSTHROUGH;
        }

        // libssh2 reads the file only once authentication starts, so a load failure names this key
        attempt.keysTried.push_back( sshKey );
        attempt.lastKeyRefused = false;
        attempt.passphraseGiven = !password.IsEmpty();
        return GIT_OK;
    }


    extern "C" int credentialsCb( git_credential** aOut, const char* aUrl, const char* aUsername,
                                  unsigned int aAllowedTypes, void* aPayload )
    {
        STATE&             state = *static_cast<STATE*>( aPayload );
        TRANSPORT_SESSION* session = state.session;

        // The transport's URL outranks a cached remote that may be empty or stale
        session->SyncRemoteUrl( wxString::FromUTF8( aUrl ) );

        // The configured remote URL may embed a password. Keep it out of the trace log.
        wxString safeUrl = StripEmbeddedUserinfo( wxString::FromUTF8( aUrl ) );

        wxLogTrace( traceKigit, "Credentials callback for %s, testing %d", safeUrl, aAllowedTypes );

        session->AuthAttempt().connType = session->GetConnType();

        if( session->GetConnType() == GIT_CONN_TYPE::GIT_CONN_LOCAL )
        {
            wxLogTrace( traceKigit, "Local repository, no credentials needed" );
            return GIT_PASSTHROUGH;
        }

        if( !session->MayAuthenticate() )
            return refuseAuthentication( state );

        if( ( aAllowedTypes & GIT_CREDENTIAL_USERNAME ) && !( state.tested & GIT_CREDENTIAL_USERNAME ) )
        {
            wxString username = session->GetUsername().Trim().Trim( false );

            // A token in the URL parses as the username, so no trace names it
            wxLogTrace( traceKigit, "Username credential at %s with allowed type %d", safeUrl, aAllowedTypes );

            // An empty username makes the server answer "unsupported authentication", which tells the user nothing
            if( username.IsEmpty() )
                return refuseAuthentication( state );

            if( int rc = git_credential_username_new( aOut, username.utf8_str() ); rc != GIT_OK )
            {
                wxLogTrace( traceKigit, "Failed to create username credential: %s", lastGitError() );
                return rc;
            }

            state.tested |= GIT_CREDENTIAL_USERNAME;
            return GIT_OK;
        }

        if( session->GetConnType() == GIT_CONN_TYPE::GIT_CONN_HTTPS
            && ( aAllowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT )
            && !( state.tested & GIT_CREDENTIAL_USERPASS_PLAINTEXT ) && !session->GetUsername().IsEmpty() )
        {
            wxLogTrace( traceKigit, "Plaintext authentication at %s with allowed type %d", safeUrl, aAllowedTypes );

            wxString password = session->GetPassword();
            int      rc = git_credential_userpass_plaintext_new( aOut, session->GetUsername().utf8_str(),
                                                                 password.utf8_str() );

            state.tested |= GIT_CREDENTIAL_USERPASS_PLAINTEXT;
            session->AuthAttempt().passwordGiven = !password.IsEmpty();
            return rc;
        }

        if( session->GetConnType() == GIT_CONN_TYPE::GIT_CONN_SSH && ( aAllowedTypes & GIT_CREDENTIAL_SSH_KEY )
            && !( state.tested & GIT_CREDENTIAL_SSH_KEY ) )
        {
            // libgit2 fails a credential whose user differs from the one the SSH session already uses
            wxString sshUser = ( aUsername && *aUsername ) ? wxString::FromUTF8( aUsername ) : session->GetUsername();

            int result = offerSSHKey( state, aOut, sshUser );

            if( result == GIT_PASSTHROUGH )
                return refuseAuthentication( state );

            return result;
        }

        return refuseAuthentication( state );
    }


    extern "C" int certificateCb( git_cert* aCert, int aValid, const char* aHost, void* aPayload )
    {
        if( !aCert || !aHost || !aPayload || aCert->cert_type != GIT_CERT_HOSTKEY_LIBSSH2 )
            return GIT_PASSTHROUGH;

        const git_cert_hostkey* cert = reinterpret_cast<const git_cert_hostkey*>( aCert );
        STATE&                  state = *static_cast<STATE*>( aPayload );
        TRANSPORT_SESSION*      session = state.session;

        auto refuse = [&]( const wxString& aReason )
        {
            // libgit2 v1.9.0 ssh_libssh2.c:758-760 replaces any error the callback sets
            session->SetHostKeyRefusal( aReason );
            return GIT_ECERTIFICATE;
        };

        // libgit2 1.9 connects on a passthrough even when nothing was verified
        auto refuseUnreadable = [&]()
        {
            wxLogTrace( traceKigit, "Host key from %s cannot be read, refusing it", aHost );
            return refuse( wxString::Format( _( "KiCad could not read the key %s presented, so the connection was "
                                                "not made. Ask your administrator which host key the server uses." ),
                                             wxString::FromUTF8( aHost ) ) );
        };

        if( !( cert->type & GIT_CERT_SSH_RAW ) || !cert->hostkey || cert->hostkey_len == 0 )
            return refuseUnreadable();

        HOST_KEY_FAILURE key;
        key.rawKey.assign( cert->hostkey, cert->hostkey_len );
        key.host = wxString::FromUTF8( aHost );

        // An SCP address hands over an IPv6 host still in its brackets
        if( key.host.StartsWith( wxS( "[" ) ) && key.host.EndsWith( wxS( "]" ) ) )
            key.host = key.host.Mid( 1, key.host.length() - 2 );

        key.port = session->SSHPortFor( key.host );

        // Checking another port's entries would let a wildcard or plain-host line vouch for this server
        if( key.port < 0 )
        {
            wxLogTrace( traceKigit, "Host %s does not match the synced remote, refusing its key", key.host );
            return refuse( wxString::Format( _( "The address of %s could not be matched to the remote, so its key "
                                                "was not checked. Check the remote URL." ),
                                             key.host ) );
        }

        key.keyType = HostKeyType( key.rawKey );
        key.fingerprint = HostKeyFingerprint( key.rawKey );

        if( key.keyType.IsEmpty() )
            return refuseUnreadable();

        key.state = LookupKnownHost( ReadKnownHosts(), key.host, key.port, key.keyType, key.rawKey );

        wxLogTrace( traceKigit, "Host key %s SHA256:%s for %s port %d, known_hosts state %d, libgit2 valid %d",
                    key.keyType, key.fingerprint, key.host, key.port, static_cast<int>( key.state ), aValid );

        // libssh2 accepts revoked keys and port-22 lines for other ports, and 1.5.0 reads no known_hosts at all
        if( key.state == KNOWN_HOST_STATE::MATCH )
            return 0;

        session->SetHostKeyFailure( key );
        return GIT_ECERTIFICATE;
    }


    extern "C" int sidebandCb( const char* aData, int aLength, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.session->IsThisOperationCancelled() )
        {
            wxLogTrace( traceKigit, "Progress CB cancelled" );
            return GIT_EUSER;
        }

        state.session->ReceiveSideband( aData, aLength );
        return 0;
    }


    extern "C" int transferCb( const git_transfer_progress* aStats, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.session->IsThisOperationCancelled() )
        {
            wxLogTrace( traceKigit, "Transfer progress cancelled" );
            return GIT_EUSER;
        }

        state.session->UpdateStep( TRANSFER_STEP::RECEIVE, aStats->received_objects, aStats->total_objects );
        state.session->UpdateProgress(
                aStats->received_objects, aStats->total_objects,
                wxString::Format( _( "Received %u of %u objects" ), aStats->received_objects, aStats->total_objects ) );
        return 0;
    }


    extern "C" int updateTipsCb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                                 void* aPayload )
    {
        constexpr int cstring_len = 8;
        char          a_str[cstring_len + 1];
        char          b_str[cstring_len + 1];

        STATE&   state = *static_cast<STATE*>( aPayload );
        wxString status;

        git_oid_tostr( b_str, cstring_len, aSecond );

#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
        if( !git_oid_is_zero( aFirst ) )
#else
        if( !git_oid_iszero( aFirst ) )
#endif
        {
            git_oid_tostr( a_str, cstring_len, aFirst );
            status = wxString::Format( _( "* [updated] %s..%s %s" ), a_str, b_str, aRefname );
        }
        else
        {
            status = wxString::Format( _( "* [new] %s %s" ), b_str, aRefname );
        }

        state.session->UpdateProgress( 0, 0, status );
        return 0;
    }


    extern "C" int pushTransferCb( unsigned int aCurrent, unsigned int aTotal, size_t aBytes, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.session->IsThisOperationCancelled() )
        {
            wxLogTrace( traceKigit, "Push progress cancelled" );
            return GIT_EUSER;
        }

        long long progress = aTotal ? ( aCurrent * 100ll ) / aTotal : 100;

        state.session->UpdateProgress( aCurrent, aTotal,
                                       wxString::Format( _( "Writing objects: %lld%% (%u/%u), %zu bytes" ), progress,
                                                         aCurrent, aTotal, aBytes ) );
        return 0;
    }


    extern "C" int pushUpdateReferenceCb( const char* aRefname, const char* aStatus, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        GIT_REF_RESULT result;
        result.remoteRef = wxString::FromUTF8( aRefname );
        result.accepted = !aStatus || !*aStatus;

        if( result.accepted )
        {
            state.session->UpdateProgress( 0, 0, wxString::Format( _( "[updated] %s" ), aRefname ) );

            if( state.results )
                state.results->push_back( result );

            return 0;
        }

        const wxString status = DecodeServerText( aStatus, strlen( aStatus ) );

        // Every ref is answered, so the caller reports a refusal per ref
        if( state.results )
        {
            state.session->UpdateProgress(
                    0, 0, wxString::Format( _( "* [rejected] %s (%s)" ), aRefname, PrintableServerText( status ) ) );

            result.message = status;
            state.results->push_back( result );
            return 0;
        }

        state.session->SetPushRefusal( wxString::Format( wxS( "%s: %s" ), aRefname, status ) );
        state.session->UpdateProgress(
                0, 0, wxString::Format( _( "* [rejected] %s (%s)" ), aRefname, PrintableServerText( status ) ) );

        // Without this the push reports success even though the remote refused it
        git_error_set_str( GIT_ERROR_NET, aStatus );
        return GIT_ERROR;
    }


    extern "C" int pushNegotiationCb( const git_push_update** aUpdates, size_t aCount, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        // The last moment a cancel can stop a push that sends no objects, such as a deletion
        if( state.session->IsThisOperationCancelled() )
            return GIT_EUSER;

        if( state.updates && LeaseBroken( aUpdates, aCount, *state.updates ) )
        {
            state.leaseBroken = true;
            git_error_set_str( GIT_ERROR_NET, "remote ref moved since it was read" );
            return GIT_EUSER;
        }

        return 0;
    }


    extern "C" void checkoutCb( const char* aPath, size_t aDone, size_t aTotal, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.checkoutTotal )
        {
            state.session->UpdateStep( TRANSFER_STEP::CHECKOUT, state.checkoutDone + aDone, state.checkoutTotal );
            state.session->UpdateProgress( state.checkoutDone + aDone, state.checkoutTotal, wxString( aPath ) );
        }
        else
        {
            state.session->UpdateStep( TRANSFER_STEP::CHECKOUT, aDone, aTotal );
            state.session->UpdateProgress( aDone, aTotal, wxString( aPath ) );
        }
    }


    struct CHECKOUT_PATH
    {
        std::string path;
        size_t      size = 0;
    };


    struct CHECKOUT_WALK
    {
        git_odb*                    odb = nullptr;
        std::vector<CHECKOUT_PATH>* paths = nullptr;
    };


    extern "C" int checkoutWalkCb( const char* aRoot, const git_tree_entry* aEntry, void* aPayload )
    {
        CHECKOUT_WALK& walk = *static_cast<CHECKOUT_WALK*>( aPayload );

        if( git_tree_entry_type( aEntry ) == GIT_OBJECT_TREE )
            return 0;

        CHECKOUT_PATH entry;
        entry.path = std::string( aRoot ) + git_tree_entry_name( aEntry );

        size_t       size = 0;
        git_object_t type = GIT_OBJECT_INVALID;

        if( git_tree_entry_type( aEntry ) == GIT_OBJECT_BLOB
            && git_odb_read_header( &size, &type, walk.odb, git_tree_entry_id( aEntry ) ) == GIT_OK )
        {
            entry.size = size;
        }

        walk.paths->push_back( std::move( entry ) );
        return 0;
    }


    /// Check out HEAD of a clone made without a working tree, in batches that a cancel can fall between
    int checkoutInBatches( git_repository* aRepo, STATE& aState )
    {
        // A batch stays short enough to stop within about a second
        const size_t BATCH_BYTES = 16 * 1024 * 1024;
        const size_t BATCH_FILES = 2048;

        if( git_repository_head_unborn( aRepo ) == 1 )
            return GIT_OK;

        git_object* treeObject = nullptr;

        if( int rc = git_revparse_single( &treeObject, aRepo, "HEAD^{tree}" ) )
            return rc;

        GitObjectPtr tree( treeObject );
        git_odb*     odbHandle = nullptr;

        if( int rc = git_repository_odb( &odbHandle, aRepo ) )
            return rc;

        std::unique_ptr<git_odb, decltype( &git_odb_free )> odb( odbHandle, &git_odb_free );
        std::vector<CHECKOUT_PATH>                          paths;
        CHECKOUT_WALK                                       walk{ odb.get(), &paths };

        if( int rc = git_tree_walk( reinterpret_cast<git_tree*>( tree.get() ), GIT_TREEWALK_PRE, checkoutWalkCb,
                                    &walk ) )
        {
            return rc;
        }

        // Attributes filter the files after them, as in a checkout of the whole tree
        std::stable_partition( paths.begin(), paths.end(),
                               []( const CHECKOUT_PATH& aPath )
                               {
                                   const std::string suffix = "/.gitattributes";
                                   const std::string name = "/" + aPath.path;

                                   return name.size() >= suffix.size()
                                          && name.compare( name.size() - suffix.size(), suffix.size(), suffix ) == 0;
                               } );

        aState.checkoutDone = 0;
        aState.checkoutTotal = paths.size();

        auto cancelled = [&]()
        {
            if( !aState.session->IsThisOperationCancelled() )
                return false;

            git_error_set_str( GIT_ERROR_CHECKOUT, "the checkout was cancelled" );
            return true;
        };

        for( size_t first = 0; first < paths.size(); )
        {
            if( cancelled() )
                return GIT_EUSER;

            size_t                   bytes = 0;
            size_t                   last = first;
            std::vector<const char*> batch;

            while( last < paths.size() && batch.size() < BATCH_FILES && ( batch.empty() || bytes < BATCH_BYTES ) )
            {
                batch.push_back( paths[last].path.c_str() );
                bytes += paths[last].size;
                ++last;
            }

            git_checkout_options options;
            git_checkout_options_init( &options, GIT_CHECKOUT_OPTIONS_VERSION );

            // The clone's folder held nothing before, so forcing overwrites nothing of the user's
            options.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
            options.paths.strings = const_cast<char**>( batch.data() );
            options.paths.count = batch.size();
            options.progress_cb = checkoutCb;
            options.progress_payload = &aState;

            if( int rc = git_checkout_tree( aRepo, tree.get(), &options ) )
                return rc;

            aState.checkoutDone += batch.size();
            first = last;
        }

        // A cancel during the last batch still means the caller does not want the clone
        return cancelled() ? GIT_EUSER : GIT_OK;
    }


    std::filesystem::path fsPath( const wxString& aPath )
    {
#ifdef __WINDOWS__
        return std::filesystem::path( aPath.ToStdWstring() );
#else
        return std::filesystem::path( aPath.utf8_string() );
#endif
    }


    /// Fail with libgit2's own words when aUrl is a local path that does not exist. libgit2 would read a missing
    /// drive path as an SCP address on the host its letter names, and report a server it cannot find
    int refuseMissingPath( const char* aUrl )
    {
        const wxString  path = aUrl ? LocalRemotePath( wxString::FromUTF8( aUrl ) ) : wxString();
        std::error_code ec;

        if( path.IsEmpty() || std::filesystem::exists( fsPath( path ), ec ) || ec )
            return GIT_OK;

        git_error_set_str( GIT_ERROR_REPOSITORY,
                           ( "could not find repository at '" + path.utf8_string() + "'" ).c_str() );
        return GIT_ENOTFOUND;
    }


    /// Remove what a clone wrote into aPath, and aPath itself unless it was there before
    void removeClone( const wxString& aPath, bool aExisted )
    {
        const std::filesystem::path root = fsPath( aPath );
        std::error_code             ec;

        if( !aExisted )
        {
            std::filesystem::remove_all( root, ec );
            return;
        }

        for( std::filesystem::directory_iterator it( root, ec ), end; !ec && it != end; it.increment( ec ) )
        {
            std::error_code ignored;
            std::filesystem::remove_all( it->path(), ignored );
        }
    }


    /// libgit2 sends a login written into an HTTP(S) URL before asking the callback, so a session that may not
    /// authenticate, or has its own login, dials the URL without it
    int withholdUrlLogin( git_remote* aRemote, TRANSPORT_SESSION& aSession, bool aPush )
    {
        const char* raw = aPush ? git_remote_pushurl( aRemote ) : git_remote_url( aRemote );

        if( !raw || ( aSession.MayAuthenticate() && !aSession.HasOwnCredentials() ) )
            return GIT_OK;

        const wxString   url = wxString::FromUTF8( raw );
        const REMOTE_URL parsed = ParseRemoteURL( url );

        if( parsed.connType != GIT_CONN_TYPE::GIT_CONN_HTTPS || parsed.url == url )
            return GIT_OK;

        const std::string bare = parsed.url.utf8_string();

        return aPush ? git_remote_set_instance_pushurl( aRemote, bare.c_str() )
                     : git_remote_set_instance_url( aRemote, bare.c_str() );
    }


    // url.insteadOf can rewrite the clone URL, which would otherwise read as a new identity and drop the
    // credentials entered for the original
    extern "C" int cloneRemoteCb( git_remote** aOut, git_repository* aRepo, const char* aName, const char* aUrl,
                                  void* aPayload )
    {
        if( int rc = git_remote_create( aOut, aRepo, aName, aUrl ); rc != GIT_OK )
            return rc;

        TRANSPORT_SESSION* session = static_cast<STATE*>( aPayload )->session;

        if( const char* url = git_remote_url( *aOut ) )
            session->FollowRewrittenUrl( wxString::FromUTF8( url ) );

        int rc = refuseMissingPath( git_remote_url( *aOut ) );

        if( rc == GIT_OK )
            rc = withholdUrlLogin( *aOut, *session, false );

        if( rc != GIT_OK )
        {
            git_remote_free( *aOut );
            *aOut = nullptr;
            return rc;
        }

        return GIT_OK;
    }


    git_remote_callbacks callbacksFor( STATE& aState )
    {
        git_remote_callbacks callbacks;
        git_remote_init_callbacks( &callbacks, GIT_REMOTE_CALLBACKS_VERSION );
        callbacks.credentials = credentialsCb;
        callbacks.certificate_check = certificateCb;
        callbacks.sideband_progress = sidebandCb;
        callbacks.transfer_progress = transferCb;
        callbacks.update_tips = updateTipsCb;
        callbacks.push_transfer_progress = pushTransferCb;
        callbacks.push_update_reference = pushUpdateReferenceCb;
        callbacks.push_negotiation = pushNegotiationCb;
        callbacks.payload = &aState;

        return callbacks;
    }


    /// libgit2's view of a list of refspecs, valid while this lives
    class REFSPEC_ARRAY
    {
    public:
        explicit REFSPEC_ARRAY( const std::vector<wxString>& aRefspecs )
        {
            for( const wxString& refspec : aRefspecs )
                m_specs.push_back( refspec.utf8_string() );

            for( std::string& spec : m_specs )
                m_ptrs.push_back( spec.data() );

            m_array = { m_ptrs.data(), m_ptrs.size() };
        }

        REFSPEC_ARRAY( const REFSPEC_ARRAY& ) = delete;
        REFSPEC_ARRAY& operator=( const REFSPEC_ARRAY& ) = delete;

        const git_strarray* Get() const { return &m_array; }

    private:
        std::vector<std::string> m_specs;
        std::vector<char*>       m_ptrs;
        git_strarray             m_array{};
    };


    git_proxy_options autoProxy()
    {
        git_proxy_options proxy;
        git_proxy_init_options( &proxy, GIT_PROXY_OPTIONS_VERSION );
        proxy.type = GIT_PROXY_AUTO;

        return proxy;
    }
} // namespace


REMOTE_TRANSPORT::REMOTE_TRANSPORT( TRANSPORT_SESSION* aSession, git_remote* aRemote, bool aFollowRedirects ) :
        m_state( std::make_unique<STATE>() ),
        m_remote( aRemote ),
        m_followRedirects( aFollowRedirects )
{
    m_state->session = aSession;
}


REMOTE_TRANSPORT::~REMOTE_TRANSPORT() = default;


int REMOTE_TRANSPORT::connect( git_direction aDirection, const git_remote_callbacks& aCallbacks )
{
    if( m_state->session->IsThisOperationCancelled() )
        return m_state->Fail( GIT_EUSER );

    const char* pushUrl = aDirection == GIT_DIRECTION_PUSH ? git_remote_pushurl( m_remote ) : nullptr;

    const char* url = pushUrl ? pushUrl : git_remote_url( m_remote );

    // The host key is checked before any credential is asked for, so the session must already name this host
    if( url )
        m_state->session->SyncRemoteUrl( wxString::FromUTF8( url ) );

    if( int rc = refuseMissingPath( url ) )
        return m_state->Fail( rc );

    if( int rc = withholdUrlLogin( m_remote, *m_state->session, pushUrl != nullptr ) )
        return m_state->Fail( rc );

#if LIBGIT2_VER_MAJOR > 1 || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 4 )
    git_remote_connect_options options;
    git_remote_connect_options_init( &options, GIT_REMOTE_CONNECT_OPTIONS_VERSION );
    options.callbacks = aCallbacks;
    options.proxy_opts = autoProxy();

    // Left unset, libgit2 honours http.followRedirects
    if( !m_followRedirects )
        options.follow_redirects = GIT_REMOTE_REDIRECT_NONE;

    if( int rc = git_remote_connect_ext( m_remote, aDirection, &options ) )
        return m_state->Fail( rc );
#else
    // No per-connection redirect control.
    git_proxy_options proxy = autoProxy();

    if( int rc = git_remote_connect( m_remote, aDirection, &aCallbacks, &proxy, nullptr ) )
        return m_state->Fail( rc );
#endif

    // libgit2 asks for nothing more once a credential works, so the key or password just offered is the one that did
    m_state->session->RememberWorkingCredential();

    if( m_state->session->IsThisOperationCancelled() )
    {
        git_remote_disconnect( m_remote );
        return m_state->Fail( GIT_EUSER );
    }

    return GIT_OK;
}


int REMOTE_TRANSPORT::Fetch( std::vector<wxString>* aBranches )
{
    m_state->Begin();

    git_fetch_options options;
    git_fetch_init_options( &options, GIT_FETCH_OPTIONS_VERSION );
    options.callbacks = callbacksFor( *m_state );
    options.proxy_opts = autoProxy();

    if( int rc = connect( GIT_DIRECTION_FETCH, options.callbacks ) )
        return rc;

    const git_remote_head** heads = nullptr;
    size_t                  count = 0;

    // The advertisement, not local tracking refs, because a custom refspec or stale refs mislead those
    if( aBranches && git_remote_ls( &heads, &count, m_remote ) == GIT_OK )
    {
        for( size_t ii = 0; ii < count; ++ii )
        {
            if( std::strncmp( heads[ii]->name, "refs/heads/", 11 ) == 0 )
                aBranches->push_back( wxString::FromUTF8( heads[ii]->name + 11 ) );
        }
    }

    int rc = git_remote_fetch( m_remote, nullptr, &options, nullptr );

    if( rc )
        m_state->Fail( rc );

    git_remote_disconnect( m_remote );
    return rc;
}


int REMOTE_TRANSPORT::Push( const std::function<wxString()>& aConnected )
{
    m_state->Begin();

    git_push_options options;
    git_push_init_options( &options, GIT_PUSH_OPTIONS_VERSION );
    options.callbacks = callbacksFor( *m_state );
    options.proxy_opts = autoProxy();

#if LIBGIT2_VER_MAJOR > 1 || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 4 )
    if( !m_followRedirects )
        options.follow_redirects = GIT_REMOTE_REDIRECT_NONE;
#endif

    if( int rc = connect( GIT_DIRECTION_PUSH, options.callbacks ) )
        return rc;

    wxString refspec = aConnected();

    if( refspec.IsEmpty() )
    {
        git_remote_disconnect( m_remote );
        return GIT_EUSER;
    }

    std::string        refspecUtf8 = refspec.utf8_string();
    const char*        refs[1] = { refspecUtf8.c_str() };
    const git_strarray refspecs = { const_cast<char**>( refs ), 1 };

    // The connection is reused, so the push goes to the server the caller just checked against
    int rc = git_remote_push( m_remote, &refspecs, &options );

    if( rc )
        m_state->Fail( rc );

    git_remote_disconnect( m_remote );
    return rc;
}


int REMOTE_TRANSPORT::FetchRefspecs( const std::vector<wxString>& aRefspecs, bool aPrune, bool aWriteFetchHead )
{
    m_state->Begin();

    // libgit2 reads no refspecs as the remote's configured ones, which would fetch and prune refs nobody asked for
    if( aRefspecs.empty() )
        return GIT_OK;

    git_fetch_options options;
    git_fetch_init_options( &options, GIT_FETCH_OPTIONS_VERSION );
    options.callbacks = callbacksFor( *m_state );
    options.proxy_opts = autoProxy();
    options.prune = aPrune ? GIT_FETCH_PRUNE : GIT_FETCH_NO_PRUNE;
    options.update_fetchhead = aWriteFetchHead ? 1 : 0;

    // Automatic tag following would write tags under names this caller did not ask for
    options.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_NONE;

    if( int rc = connect( GIT_DIRECTION_FETCH, options.callbacks ) )
        return rc;

    REFSPEC_ARRAY refspecs( aRefspecs );

    int rc = git_remote_fetch( m_remote, refspecs.Get(), &options, nullptr );

    if( rc )
        m_state->Fail( rc );

    git_remote_disconnect( m_remote );
    return rc;
}


int REMOTE_TRANSPORT::DownloadRefspecs( const std::vector<wxString>&  aRefspecs,
                                        std::map<wxString, wxString>& aAdvertised )
{
    m_state->Begin();
    aAdvertised.clear();

    if( aRefspecs.empty() )
        return GIT_OK;

    git_fetch_options options;
    git_fetch_init_options( &options, GIT_FETCH_OPTIONS_VERSION );
    options.callbacks = callbacksFor( *m_state );
    options.proxy_opts = autoProxy();
    options.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_NONE;

    if( int rc = connect( GIT_DIRECTION_FETCH, options.callbacks ) )
        return rc;

    const git_remote_head** heads = nullptr;
    size_t                  count = 0;
    int                     rc = git_remote_ls( &heads, &count, m_remote );

    if( rc == GIT_OK )
    {
        for( size_t ii = 0; ii < count; ++ii )
        {
            // A peeled tag's entry names no ref
            if( std::string_view( heads[ii]->name ).ends_with( "^{}" ) )
                continue;

            aAdvertised[wxString::FromUTF8( heads[ii]->name )] =
                    wxString::FromUTF8( git_oid_tostr_s( &heads[ii]->oid ) );
        }

        REFSPEC_ARRAY refspecs( aRefspecs );

        // Reuses the connection, so the objects match the advertisement just read
        rc = git_remote_download( m_remote, refspecs.Get(), &options );
    }

    if( rc )
    {
        m_state->Fail( rc );
        aAdvertised.clear();
    }

    git_remote_disconnect( m_remote );
    return rc;
}


RefPushResult REMOTE_TRANSPORT::PushRefs( const std::vector<GIT_REF_UPDATE>& aUpdates,
                                          std::vector<GIT_REF_RESULT>&       aResults )
{
    m_state->Begin();
    aResults.clear();

    // libgit2 reads no refspecs as the remote's configured push refspecs, which nobody leased
    if( aUpdates.empty() )
        return RefPushResult::Success;

    git_push_options options;
    git_push_init_options( &options, GIT_PUSH_OPTIONS_VERSION );
    options.callbacks = callbacksFor( *m_state );
    options.proxy_opts = autoProxy();

    if( !m_followRedirects )
        options.follow_redirects = GIT_REMOTE_REDIRECT_NONE;

    if( connect( GIT_DIRECTION_PUSH, options.callbacks ) )
        return RefPushResult::Error;

    std::vector<wxString> refspecs;

    for( const GIT_REF_UPDATE& update : aUpdates )
        refspecs.push_back( ( update.force ? wxS( "+" ) : wxS( "" ) ) + update.localRef + wxS( ":" )
                            + update.remoteRef );

    REFSPEC_ARRAY array( refspecs );

    m_state->updates = &aUpdates;
    m_state->results = &aResults;
    int rc = git_remote_push( m_remote, array.Get(), &options );
    m_state->updates = nullptr;
    m_state->results = nullptr;

    if( rc )
        m_state->Fail( rc );

    git_remote_disconnect( m_remote );

    if( m_state->leaseBroken )
        return RefPushResult::LeaseBroken;

    if( rc )
        return RefPushResult::Error;

    for( const GIT_REF_RESULT& result : aResults )
    {
        if( !result.accepted )
            return RefPushResult::Rejected;
    }

    return RefPushResult::Success;
}


std::optional<wxString> REMOTE_TRANSPORT::ReadRemoteRef( const wxString& aRef )
{
    m_state->Begin();

    git_remote_callbacks callbacks = callbacksFor( *m_state );

    if( connect( GIT_DIRECTION_FETCH, callbacks ) )
        return std::nullopt;

    const git_remote_head** heads = nullptr;
    size_t                  count = 0;
    std::optional<wxString> oid;

    if( git_remote_ls( &heads, &count, m_remote ) == GIT_OK )
    {
        oid = wxString();

        for( size_t ii = 0; ii < count; ++ii )
        {
            if( aRef == wxString::FromUTF8( heads[ii]->name ) )
                oid = wxString::FromUTF8( git_oid_tostr_s( &heads[ii]->oid ) );
        }
    }

    git_remote_disconnect( m_remote );
    return oid;
}


int REMOTE_TRANSPORT::Check()
{
    m_state->Begin();

    if( int rc = connect( GIT_DIRECTION_FETCH, callbacksFor( *m_state ) ) )
        return rc;

    git_remote_disconnect( m_remote );
    return GIT_OK;
}


int REMOTE_TRANSPORT::Clone( const wxString& aUrl, const wxString& aPath, git_repository** aRepo, bool aBare,
                             const std::function<int( git_repository* )>& aPrepare )
{
    m_state->Begin();

    if( m_state->session->IsThisOperationCancelled() )
        return m_state->Fail( GIT_EUSER );

    git_clone_options options;
    git_clone_init_options( &options, GIT_CLONE_OPTIONS_VERSION );
    options.bare = aBare ? 1 : 0;

    // A checkout cannot stop midway, and a clone from a local path transfers nothing a cancel could stop
    options.checkout_opts.checkout_strategy = GIT_CHECKOUT_NONE;
    options.fetch_opts.callbacks = callbacksFor( *m_state );
    options.fetch_opts.proxy_opts = autoProxy();
    options.remote_cb = cloneRemoteCb;
    options.remote_cb_payload = m_state.get();

    std::string urlUtf8 = aUrl.utf8_string();
    std::string pathUtf8 = aPath.utf8_string();

    std::error_code ec;
    const bool      existed = std::filesystem::exists( fsPath( aPath ), ec );

    if( int rc = git_clone( aRepo, urlUtf8.c_str(), pathUtf8.c_str(), &options ) )
        return m_state->Fail( rc );

    // As libgit2 does for a clone that fails, so a retry finds the folder as it was. An open handle keeps Windows
    // from deleting it
    auto abandon = [&]( int aRc )
    {
        int failed = m_state->Fail( aRc );

        git_repository_free( *aRepo );
        *aRepo = nullptr;
        removeClone( aPath, existed );
        return failed;
    };

    if( aPrepare )
    {
        if( int rc = aPrepare( *aRepo ) )
            return abandon( rc );
    }

    if( !aBare )
    {
        if( int rc = PROJECT_GIT_UTILS::AllowLongPaths( *aRepo ) )
            return abandon( rc );

        if( int rc = checkoutInBatches( *aRepo, *m_state ) )
            return abandon( rc );
    }

    m_state->session->RememberWorkingCredential();
    return GIT_OK;
}


const KIGIT_ERROR_INFO& REMOTE_TRANSPORT::Error() const
{
    return m_state->error;
}

} // namespace KIGIT
