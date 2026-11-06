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

#include "git_repo_mixin.h"
#include "kicad_git_common.h"
#include "kigit_failure.h"
#include "kigit_ssh_keys.h"

#include <git/kicad_git_compat.h>
#include <trace_helpers.h>

#include <wx/log.h>
#include <wx/translation.h>

#include <cstring>


namespace KIGIT
{

struct REMOTE_TRANSPORT::STATE
{
    KIGIT_REPO_MIXIN* handler = nullptr;
    unsigned          tested = 0; ///< The git_credential_t kinds this call has offered
    bool              agentTried = false;
    KIGIT_ERROR_INFO  error;

    KIGIT_COMMON* Common() const { return handler->GetCommon(); }

    /// Every call offers every credential again and keeps nothing from an earlier failure
    void Begin()
    {
        tested = 0;
        agentTried = false;
        error = KIGIT_ERROR_INFO();
        handler->ResetTransferState();
    }

    int Fail( int aCode )
    {
        error = KIGIT_COMMON::GetLastGitErrorInfo( aCode );
        return aCode;
    }
};


namespace
{
    using STATE = REMOTE_TRANSPORT::STATE;


    int refuseAuthentication( STATE& aState )
    {
        // Answered as a refusal rather than "no callback set", so the caller prompts and retries
        git_error_clear();
        git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
        aState.handler->SetAuthFailure();
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
        KIGIT_AUTH_ATTEMPT& attempt = aState.handler->AuthAttempt();
        KIGIT_COMMON*       common = aState.Common();

        if( !aState.agentTried )
        {
            aState.agentTried = true;

            if( git_credential_ssh_key_from_agent( aOut, aUsername.mbc_str() ) == GIT_OK )
            {
                attempt.agentTried = true;
                return GIT_OK;
            }

            wxLogTrace( traceGit, "Failed to create SSH agent credential: %s", KIGIT_COMMON::GetLastGitError() );
        }

        if( !attempt.probing.IsEmpty() )
        {
            wxLogTrace( traceGit, attempt.lastKeyRefused ? "Server refuses %s" : "Server accepts %s", attempt.probing );
            attempt.probing.clear();
        }

        wxString password = common->GetPassword();
        wxString sshKey = common->GetNextPublicKey();

        if( sshKey.IsEmpty() )
        {
            wxLogTrace( traceGit, "Finished testing all possible ssh keys" );
            aState.tested |= GIT_CREDENTIAL_SSH_KEY;
            return GIT_PASSTHROUGH;
        }

        // Without a passphrase no encrypted key can sign, so asking the server first is the only way to learn which
        // one it takes. With one, the probe's extra round would cost a counted attempt for nothing
        if( password.IsEmpty() && IsPrivateKeyFileEncrypted( sshKey ) )
        {
            const std::string blob = PublicKeyBlob( sshKey );

            if( !blob.empty()
                && git_credential_ssh_custom_new( aOut, aUsername.mbc_str(), blob.data(), blob.size(), probeSignCb,
                                                  &attempt )
                           == GIT_OK )
            {
                wxLogTrace( traceGit, "Asking whether the server takes %s", sshKey );
                attempt.probing = sshKey;
                attempt.lastKeyRefused = true;
                attempt.keysTried.push_back( sshKey );
                return GIT_OK;
            }
        }

        wxString sshPubKey = PublicKeyPath( sshKey );

        wxLogTrace( traceGit, "Testing %s\n", sshKey );

        if( git_credential_ssh_key_new( aOut, aUsername.mbc_str(), sshPubKey.mbc_str(), sshKey.mbc_str(),
                                        password.mbc_str() )
            != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to create SSH key credential for %s: %s", sshKey,
                        KIGIT_COMMON::GetLastGitError() );
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
        STATE&            state = *static_cast<STATE*>( aPayload );
        KIGIT_REPO_MIXIN* handler = state.handler;

        // The transport's URL outranks a cached remote that may be empty or stale
        state.Common()->SyncRemoteUrl( wxString::FromUTF8( aUrl ) );

        // The configured remote URL may embed a password. Keep it out of the trace log.
        wxString safeUrl = StripEmbeddedUserinfo( wxString::FromUTF8( aUrl ) );

        wxLogTrace( traceGit, "Credentials callback for %s, testing %d", safeUrl, aAllowedTypes );

        handler->AuthAttempt().connType = handler->GetConnType();

        if( handler->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
        {
            wxLogTrace( traceGit, "Local repository, no credentials needed" );
            return GIT_PASSTHROUGH;
        }

        if( ( aAllowedTypes & GIT_CREDENTIAL_USERNAME ) && !( state.tested & GIT_CREDENTIAL_USERNAME ) )
        {
            wxString username = handler->GetUsername().Trim().Trim( false );

            // A token in the URL parses as the username, so no trace names it
            wxLogTrace( traceGit, "Username credential at %s with allowed type %d", safeUrl, aAllowedTypes );

            // An empty username makes the server answer "unsupported authentication", which tells the user nothing
            if( username.IsEmpty() )
                return refuseAuthentication( state );

            if( int rc = git_credential_username_new( aOut, username.ToStdString().c_str() ); rc != GIT_OK )
            {
                wxLogTrace( traceGit, "Failed to create username credential: %s", KIGIT_COMMON::GetLastGitError() );
                return rc;
            }

            state.tested |= GIT_CREDENTIAL_USERNAME;
            return GIT_OK;
        }

        if( handler->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS
            && ( aAllowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT )
            && !( state.tested & GIT_CREDENTIAL_USERPASS_PLAINTEXT ) && !handler->GetUsername().IsEmpty() )
        {
            wxLogTrace( traceGit, "Plaintext authentication at %s with allowed type %d", safeUrl, aAllowedTypes );

            wxString password = state.Common()->GetPassword();
            int rc = git_credential_userpass_plaintext_new( aOut, handler->GetUsername().mbc_str(), password.mbc_str() );

            state.tested |= GIT_CREDENTIAL_USERPASS_PLAINTEXT;
            handler->AuthAttempt().passwordGiven = !password.IsEmpty();
            return rc;
        }

        if( handler->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
            && ( aAllowedTypes & GIT_CREDENTIAL_SSH_KEY ) && !( state.tested & GIT_CREDENTIAL_SSH_KEY ) )
        {
            // libgit2 fails a credential whose user differs from the one the SSH session already uses
            wxString sshUser = ( aUsername && *aUsername ) ? wxString::FromUTF8( aUsername ) : handler->GetUsername();

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
        KIGIT_REPO_MIXIN*       handler = state.handler;

        auto refuse = [&]( const wxString& aReason )
        {
            // libgit2 v1.9.0 ssh_libssh2.c:758-760 replaces any error the callback sets
            handler->SetHostKeyRefusal( aReason );
            return GIT_ECERTIFICATE;
        };

        // libgit2 1.9 connects on a passthrough even when nothing was verified
        auto refuseUnreadable = [&]()
        {
            wxLogTrace( traceGit, "Host key from %s cannot be read, refusing it", aHost );
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

        key.port = state.Common()->SSHPortFor( key.host );

        // Checking another port's entries would let a wildcard or plain-host line vouch for this server
        if( key.port < 0 )
        {
            wxLogTrace( traceGit, "Host %s does not match the synced remote, refusing its key", key.host );
            return refuse( wxString::Format( _( "The address of %s could not be matched to the remote, so its key "
                                                "was not checked. Check the remote URL." ),
                                             key.host ) );
        }

        key.keyType = HostKeyType( key.rawKey );
        key.fingerprint = HostKeyFingerprint( key.rawKey );

        if( key.keyType.IsEmpty() )
            return refuseUnreadable();

        key.state = LookupKnownHost( ReadKnownHosts(), key.host, key.port, key.keyType, key.rawKey );

        wxLogTrace( traceGit, "Host key %s SHA256:%s for %s port %d, known_hosts state %d, libgit2 valid %d",
                    key.keyType, key.fingerprint, key.host, key.port, static_cast<int>( key.state ), aValid );

        // libssh2 accepts revoked keys and port-22 lines for other ports, and 1.5.0 reads no known_hosts at all
        if( key.state == KNOWN_HOST_STATE::MATCH )
            return 0;

        handler->SetHostKeyFailure( key );
        return GIT_ECERTIFICATE;
    }


    extern "C" int sidebandCb( const char* aData, int aLength, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.handler->IsThisOperationCancelled() )
        {
            wxLogTrace( traceGit, "Progress CB cancelled" );
            return GIT_EUSER;
        }

        state.handler->ReceiveSideband( aData, aLength );
        return 0;
    }


    extern "C" int transferCb( const git_transfer_progress* aStats, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.handler->IsThisOperationCancelled() )
        {
            wxLogTrace( traceGit, "Transfer progress cancelled" );
            return GIT_EUSER;
        }

        state.handler->UpdateProgress( aStats->received_objects, aStats->total_objects,
                                       wxString::Format( _( "Received %u of %u objects" ), aStats->received_objects,
                                                         aStats->total_objects ) );
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

        state.handler->UpdateProgress( 0, 0, status );
        return 0;
    }


    extern "C" int pushTransferCb( unsigned int aCurrent, unsigned int aTotal, size_t aBytes, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( state.handler->IsThisOperationCancelled() )
        {
            wxLogTrace( traceGit, "Push progress cancelled" );
            return GIT_EUSER;
        }

        long long progress = aTotal ? ( aCurrent * 100ll ) / aTotal : 100;

        state.handler->UpdateProgress( aCurrent, aTotal,
                                       wxString::Format( _( "Writing objects: %lld%% (%u/%u), %zu bytes" ), progress,
                                                         aCurrent, aTotal, aBytes ) );
        return 0;
    }


    extern "C" int pushUpdateReferenceCb( const char* aRefname, const char* aStatus, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        if( !aStatus || !*aStatus )
        {
            state.handler->UpdateProgress( 0, 0, wxString::Format( _( "[updated] %s" ), aRefname ) );
            return 0;
        }

        const wxString status = DecodeServerText( aStatus, strlen( aStatus ) );

        state.handler->SetPushRefusal( wxString::Format( wxS( "%s: %s" ), aRefname, status ) );
        state.handler->UpdateProgress( 0, 0, wxString::Format( _( "* [rejected] %s (%s)" ), aRefname,
                                                               PrintableServerText( status ) ) );

        // Without this the push reports success even though the remote refused it
        git_error_set_str( GIT_ERROR_NET, aStatus );
        return GIT_ERROR;
    }


    extern "C" void checkoutCb( const char* aPath, size_t aDone, size_t aTotal, void* aPayload )
    {
        STATE& state = *static_cast<STATE*>( aPayload );

        state.handler->UpdateProgress( aDone, aTotal, wxString( aPath ) );
    }


    // url.insteadOf can rewrite the clone URL, which would otherwise read as a new identity and drop the
    // credentials entered for the original
    extern "C" int cloneRemoteCb( git_remote** aOut, git_repository* aRepo, const char* aName, const char* aUrl,
                                  void* aPayload )
    {
        if( int rc = git_remote_create( aOut, aRepo, aName, aUrl ); rc != GIT_OK )
            return rc;

        if( const char* url = git_remote_url( *aOut ) )
            static_cast<STATE*>( aPayload )->Common()->FollowRewrittenUrl( wxString::FromUTF8( url ) );

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
        callbacks.payload = &aState;

        return callbacks;
    }


    git_proxy_options autoProxy()
    {
        git_proxy_options proxy;
        git_proxy_init_options( &proxy, GIT_PROXY_OPTIONS_VERSION );
        proxy.type = GIT_PROXY_AUTO;

        return proxy;
    }
} // namespace


REMOTE_TRANSPORT::REMOTE_TRANSPORT( KIGIT_REPO_MIXIN* aHandler, git_remote* aRemote, bool aFollowRedirects ) :
        m_state( std::make_unique<STATE>() ),
        m_remote( aRemote ),
        m_followRedirects( aFollowRedirects )
{
    m_state->handler = aHandler;
}


REMOTE_TRANSPORT::~REMOTE_TRANSPORT() = default;


int REMOTE_TRANSPORT::connect( git_direction aDirection, const git_remote_callbacks& aCallbacks )
{
    git_remote_connect_options options;
    git_remote_connect_options_init( &options, GIT_REMOTE_CONNECT_OPTIONS_VERSION );
    options.callbacks = aCallbacks;
    options.proxy_opts = autoProxy();

    // Left unset, libgit2 honours http.followRedirects
    if( !m_followRedirects )
        options.follow_redirects = GIT_REMOTE_REDIRECT_NONE;

    if( int rc = git_remote_connect_ext( m_remote, aDirection, &options ) )
        return m_state->Fail( rc );

    // libgit2 asks for nothing more once a credential works, so the key just offered is the one to try first
    m_state->handler->RememberWorkingSSHKey();
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

    if( !m_followRedirects )
        options.follow_redirects = GIT_REMOTE_REDIRECT_NONE;

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


int REMOTE_TRANSPORT::Check()
{
    m_state->Begin();

    if( int rc = connect( GIT_DIRECTION_FETCH, callbacksFor( *m_state ) ) )
        return rc;

    git_remote_disconnect( m_remote );
    return GIT_OK;
}


int REMOTE_TRANSPORT::Clone( const wxString& aUrl, const wxString& aPath, git_repository** aRepo )
{
    m_state->Begin();

    git_clone_options options;
    git_clone_init_options( &options, GIT_CLONE_OPTIONS_VERSION );
    options.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    options.checkout_opts.progress_cb = checkoutCb;
    options.checkout_opts.progress_payload = m_state.get();
    options.fetch_opts.callbacks = callbacksFor( *m_state );
    options.fetch_opts.proxy_opts = autoProxy();
    options.remote_cb = cloneRemoteCb;
    options.remote_cb_payload = m_state.get();

    std::string urlUtf8 = aUrl.utf8_string();

    if( int rc = git_clone( aRepo, urlUtf8.c_str(), aPath.mbc_str(), &options ) )
        return m_state->Fail( rc );

    m_state->handler->RememberWorkingSSHKey();
    return GIT_OK;
}


const KIGIT_ERROR_INFO& REMOTE_TRANSPORT::Error() const
{
    return m_state->error;
}

} // namespace KIGIT
