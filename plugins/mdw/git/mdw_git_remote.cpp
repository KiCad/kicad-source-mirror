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

#include "mdw_git_remote.h"

#include <git/git_repo_mixin.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_failure.h>
#include <git/kigit_lock_retry.h>
#include <git/kigit_remote_url.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <git2.h>

#include <condition_variable>

namespace ws = kiapi::workspace;


/// The application's transfer session, answering with the credentials Authenticate kept
class MDW_GIT_SESSION : public KIGIT_REPO_MIXIN
{
public:
    MDW_GIT_SESSION( KIGIT_COMMON* aCommon, MDW_GIT_CREDENTIALS aCredentials,
                     std::shared_ptr<const std::atomic<bool>> aCancel, bool aMayAuthenticate = true ) :
            KIGIT_REPO_MIXIN( aCommon ),
            m_credentials( std::move( aCredentials ) ),
            m_mayAuthenticate( aMayAuthenticate ),
            m_request( std::move( aCancel ) ),
            m_progress( MdwCurrentRequestProgressReporter() )
    {
        aCommon->SetSSHKey( wxString::FromUTF8( m_credentials.sshKeyPath ) );
    }

    // A new address resets the session to the URL's credentials, and Authenticate's take precedence
    void SyncRemoteUrl( const wxString& aUrl ) override
    {
        KIGIT_REPO_MIXIN::SyncRemoteUrl( aUrl );

        const KIGIT::REMOTE_URL parsed = KIGIT::ParseRemoteURL( aUrl );

        // The credential callback syncs again with the address a withheld login left
        m_urlLogin |= parsed.connType == KIGIT::GIT_CONN_TYPE::GIT_CONN_HTTPS && parsed.url != aUrl;

        const bool ssh = GetConnType() == KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH;

        // The sign-in form has one secret field, so over SSH its password is the key's passphrase
        const std::string& secret =
                ssh && !m_credentials.sshPassphrase.empty() ? m_credentials.sshPassphrase : m_credentials.password;

        if( !m_credentials.username.empty() )
            GetCommon()->SetUsername( wxString::FromUTF8( m_credentials.username ) );

        if( !secret.empty() )
            GetCommon()->SetPassword( wxString::FromUTF8( secret ) );
    }

    // The sync reapplies Authenticate's credentials, where keeping the session's would drop the URL's username
    void FollowRewrittenUrl( const wxString& aUrl ) override { SyncRemoteUrl( aUrl ); }

    void UpdateStep( KIGIT::TRANSFER_STEP aStep, size_t aDone, size_t aTotal ) override
    {
        if( m_progress )
            m_progress( aDone, aTotal, aStep == KIGIT::TRANSFER_STEP::RECEIVE ? "receive" : "checkout" );
    }

    // libgit2 packs a local source on threads of its own, where the request is not the current one
    bool IsThisOperationCancelled() const override
    {
        return ( m_request && m_request->load() ) || KIGIT_REPO_MIXIN::IsThisOperationCancelled();
    }

    bool MayAuthenticate() const override { return m_mayAuthenticate; }

    bool HasOwnCredentials() const override
    {
        return !m_credentials.username.empty() && !m_credentials.password.empty();
    }

    /// The HTTP(S) address carries a login, which libgit2 sends before asking for one
    bool UrlLogin() const { return m_urlLogin; }

private:
    MDW_GIT_CREDENTIALS                      m_credentials;
    bool                                     m_mayAuthenticate;
    bool                                     m_urlLogin = false;
    std::shared_ptr<const std::atomic<bool>> m_request;
    MDW_PROGRESS_REPORTER                    m_progress; ///< Taken here, as libgit2's threads serve no request
};


struct MDW_GIT_REMOTE::CHECK_OUTCOME
{
    std::mutex                        mutex;
    std::condition_variable           cv;
    std::optional<ws::WorkspaceError> result;
};


ws::AuthChallenge MdwGitAuthChallenge( const std::string& aUsernameHint, bool aSsh )
{
    ws::AuthChallenge challenge;
    challenge.set_username_hint( aUsernameHint );

    for( auto [id, secret] : { std::pair( "username", false ), std::pair( "password", true ),
                               std::pair( "ssh_key_path", false ), std::pair( "ssh_passphrase", true ) } )
    {
        ws::AuthField* field = challenge.add_fields();
        field->set_field_id( id );
        field->set_secret( secret );
    }

    if( aSsh )
        challenge.mutable_fields( 1 )->set_display_name( "Passphrase" );

    return challenge;
}


MDW_GIT_REMOTE::~MDW_GIT_REMOTE()
{
    for( auto& [thread, outcome] : m_abandoned )
        thread.join();
}


void MDW_GIT_REMOTE::SetCredentials( const std::map<std::string, std::string>& aFields )
{
    auto field = [&aFields]( const char* aName )
    {
        auto it = aFields.find( aName );
        return it == aFields.end() ? std::string() : it->second;
    };

    MDW_GIT_CREDENTIALS fresh;

    fresh.username = field( "username" );
    fresh.password = field( "password" );
    fresh.sshKeyPath = field( "ssh_key_path" );
    fresh.sshPassphrase = field( "ssh_passphrase" );

    std::lock_guard<std::mutex> lock( m_mutex );

    // A sign-in may retry the very login refused before, as after a directory outage
    ++m_signIns;

    if( fresh.username != m_credentials.username || fresh.password != m_credentials.password
        || fresh.sshKeyPath != m_credentials.sshKeyPath || fresh.sshPassphrase != m_credentials.sshPassphrase )
    {
        ++m_generation;
    }

    m_credentials = std::move( fresh );
}


std::uint64_t MDW_GIT_REMOTE::CredentialGeneration() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_generation;
}


std::string MDW_GIT_REMOTE::Username() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_credentials.username;
}


// Some summaries send the user to the details, so there is always something there to read
static std::string detailOf( const KIGIT_FAILURE_REPORT& aReport, int aCode )
{
    if( aReport.details.IsEmpty() )
        return wxString::Format( wxS( "libgit2 gave no reason (error %d)" ), aCode ).utf8_string();

    return aReport.details.utf8_string();
}


// A call before any transfer failed, such as on a malformed address, so only libgit2 can say why
static ws::WorkspaceError setUpFailure( int aCode, KIGIT_OPERATION aOperation, const wxString& aRemote,
                                        const wxString& aUrl )
{
    KIGIT_FAILURE_CONTEXT context;
    KIGIT_ERROR_INFO      info = KIGIT::GetLastGitErrorInfo( aCode );

    context.operation = aOperation;
    context.remoteName = aRemote;
    context.url = aUrl;

    const KIGIT_FAILURE_REPORT report = KIGIT::DescribeGitFailure( info, KIGIT_AUTH_ATTEMPT(), context );
    ws::WorkspaceError         error = MdwFail( ws::WE_NETWORK, report.summary.utf8_string() );

    error.set_detail( detailOf( report, info.code ) );
    return error;
}


ws::WorkspaceError MDW_GIT_REMOTE::Transfer( git_repository* aRepo, const std::string& aRemoteName, const CALL& aCall )
{
    return transfer(
            aRepo, aRemoteName,
            [&aCall]( KIGIT::REMOTE_TRANSPORT& aTransport, MDW_GIT_SESSION& )
            {
                return aCall( aTransport );
            },
            nullptr );
}


ws::WorkspaceError MDW_GIT_REMOTE::transfer( git_repository* aRepo, const std::string& aRemoteName,
                                             const SESSION_CALL& aCall, bool* aForbidden )
{
    git_remote* remote = nullptr;

    const int lookup = aRepo ? git_remote_lookup( &remote, aRepo, aRemoteName.c_str() ) : GIT_OK;

    if( lookup != GIT_OK )
    {
        return setUpFailure( lookup, KIGIT_OPERATION::SET_UP_REMOTE, wxString::FromUTF8( aRemoteName ),
                             wxString() );
    }

    KIGIT::GitRemotePtr remotePtr( remote );
    return run( aRepo, remote, aCall, MdwCurrentRequestCancelToken(), aForbidden );
}


ws::WorkspaceError MDW_GIT_REMOTE::Check( const std::string& aUrl )
{
    return check( aUrl, MdwCurrentRequestCancelToken() );
}


ws::WorkspaceError MDW_GIT_REMOTE::Check( const std::string& aUrl, std::chrono::milliseconds aLimit )
{
    auto outcome = std::make_shared<CHECK_OUTCOME>();
    auto stop = std::make_shared<std::atomic<bool>>( false );

    std::thread worker(
            [this, aUrl, outcome, stop]()
            {
                ws::WorkspaceError result;

                try
                {
                    result = check( aUrl, stop );
                }
                catch( const std::exception& error )
                {
                    result = MdwFail( ws::WE_INTERNAL, error.what() );
                }

                std::lock_guard<std::mutex> lock( outcome->mutex );
                outcome->result = std::move( result );
                outcome->cv.notify_all();
            } );

    const auto                   deadline = std::chrono::steady_clock::now() + aLimit;
    std::unique_lock<std::mutex> lock( outcome->mutex );

    // Polled, since nothing wakes this wait when the caller cancels
    while( !outcome->result && std::chrono::steady_clock::now() < deadline && !MdwCurrentRequestCancelled() )
        outcome->cv.wait_for( lock, std::chrono::milliseconds( 50 ) );

    if( outcome->result )
    {
        ws::WorkspaceError result = std::move( *outcome->result );
        lock.unlock();
        worker.join();
        return result;
    }

    lock.unlock();
    stop->store( true );

    {
        std::lock_guard<std::mutex> abandoned( m_abandonedMutex );

        std::erase_if( m_abandoned,
                       []( auto& aCheck )
                       {
                           {
                               std::lock_guard<std::mutex> done( aCheck.second->mutex );

                               if( !aCheck.second->result )
                                   return false;
                           }

                           aCheck.first.join();
                           return true;
                       } );

        m_abandoned.emplace_back( std::move( worker ), outcome );
    }

    if( MdwCurrentRequestCancelled() )
        return MdwCancelled();

    return MdwFail( ws::WE_TIMEOUT, "The remote did not answer in time" );
}


ws::WorkspaceError MDW_GIT_REMOTE::check( const std::string& aUrl, const CANCEL& aCancel )
{
    git_remote* remote = nullptr;

    const int created = git_remote_create_detached( &remote, aUrl.c_str() );

    if( created != GIT_OK )
        return setUpFailure( created, KIGIT_OPERATION::CONNECT, wxString(), wxString::FromUTF8( aUrl ) );

    KIGIT::GitRemotePtr remotePtr( remote );

    return run(
            nullptr, remote,
            []( KIGIT::REMOTE_TRANSPORT& aTransport, MDW_GIT_SESSION& )
            {
                return aTransport.Check();
            },
            aCancel );
}


RefPushResult MDW_GIT_REMOTE::Push( git_repository* aRepo, const std::string& aRemoteName,
                                    const std::vector<GIT_REF_UPDATE>& aUpdates, std::vector<GIT_REF_RESULT>& aResults,
                                    ws::WorkspaceError& aError )
{
    RefPushResult result = RefPushResult::Error;
    bool          moved = false;
    bool          unread = false;
    bool          forbidden = false;
    wxString      serverMessage;

    aError = transfer(
            aRepo, aRemoteName,
            [&]( KIGIT::REMOTE_TRANSPORT& aTransport, MDW_GIT_SESSION& aSession )
            {
                result = aTransport.PushRefs( aUpdates, aResults );

                // Reading the ref again resets the session, and the hook's reason with it
                serverMessage = aSession.ServerMessage();

                // A refusal with the ref where the push expected it is the server's decision. Else it lost a race
                for( const GIT_REF_RESULT& ref : aResults )
                {
                    for( const GIT_REF_UPDATE& update : aUpdates )
                    {
                        if( result != RefPushResult::Rejected || ref.accepted || !update.leased
                            || update.remoteRef != ref.remoteRef )
                        {
                            continue;
                        }

                        std::optional<wxString> now = aTransport.ReadRemoteRef( ref.remoteRef );

                        unread = unread || !now;
                        moved = moved || ( now && !now->IsSameAs( update.expectedOld, false ) );
                    }
                }

                return result == RefPushResult::Error ? GIT_ERROR : GIT_OK;
            },
            &forbidden );

    auto describe = [&]( int aCode, const wxString& aRefusal )
    {
        KIGIT_ERROR_INFO      info;
        KIGIT_FAILURE_CONTEXT context;
        git_remote*           remote = nullptr;

        if( git_remote_lookup( &remote, aRepo, aRemoteName.c_str() ) == 0 )
            context.url = wxString::FromUTF8( git_remote_url( remote ) );

        KIGIT::GitRemotePtr remotePtr( remote );

        info.code = aCode;
        context.operation = KIGIT_OPERATION::PUSH;
        context.remoteName = wxString::FromUTF8( aRemoteName );
        context.pushRefusal = aRefusal;
        context.serverMessage = serverMessage;

        return std::make_pair( KIGIT::DescribeGitFailure( info, KIGIT_AUTH_ATTEMPT(), context ), aCode );
    };

    // A broken lease is a push that was behind, so it reads as one
    if( result == RefPushResult::LeaseBroken )
    {
        auto [report, code] = describe( GIT_ENONFASTFORWARD, wxString() );

        aError = MdwFail( ws::WE_VERSION_CONFLICT, report.summary.utf8_string() );
        aError.set_detail( detailOf( report, code ) );
    }

    for( const GIT_REF_RESULT& ref : aResults )
    {
        if( result == RefPushResult::Rejected && !ref.accepted )
        {
            const int code = ( moved && !unread ) ? GIT_ENONFASTFORWARD : 0;
            auto [report, unused] = describe( code, ref.remoteRef + wxS( ": " ) + ref.message );

            // Without the ref the refusal cannot be told from a lost race, so it is neither
            aError = MdwFail( unread ? ws::WE_NETWORK : ( moved ? ws::WE_VERSION_CONFLICT : ws::WE_PERMISSION_DENIED ),
                              unread ? "The push was refused and the remote could not be read to tell why"
                                     : report.summary.utf8_string() );
            aError.set_detail( detailOf( report, code ) );

            // A forge such as Gitea takes a reader's push and only its hook declines it. A declined tag may be a
            // protected-tag rule, which says nothing about writing the library
            forbidden = !moved && !unread && ref.remoteRef.StartsWith( wxS( "refs/heads/" ) );
            break;
        }
    }

    if( m_pushObserver && result == RefPushResult::Success )
        m_pushObserver( aUpdates, ws::PS_ALLOWED );
    else if( m_pushObserver && forbidden )
        m_pushObserver( aUpdates, ws::PS_DENIED );

    return result;
}


RefPushResult MDW_GIT_REMOTE::PushBranch( git_repository* aRepo, const std::string& aRemoteName,
                                          const std::string& aBranch, const std::optional<std::string>& aUpstream,
                                          ws::WorkspaceError& aError )
{
    GIT_REF_UPDATE              push;
    std::vector<GIT_REF_RESULT> results;

    push.localRef = wxString::FromUTF8( "refs/heads/" + aBranch );
    push.remoteRef = push.localRef;
    push.leased = true;
    push.expectedOld = aUpstream ? wxString::FromUTF8( *aUpstream ) : wxString();

    return Push( aRepo, aRemoteName, { push }, results, aError );
}


ws::WorkspaceError MDW_GIT_REMOTE::run( git_repository* aRepo, git_remote* aRemote, const SESSION_CALL& aCall,
                                        const CANCEL& aCancel, bool* aForbidden )
{
    MDW_GIT_CREDENTIALS credentials;
    std::uint64_t       signIn = 0;
    bool                refused = false;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        credentials = m_credentials;
        signIn = m_signIns;
        refused = m_refused == signIn;
    }

    // Resending a refused login locks the account on most servers, so only a sign-in sends it again
    KIGIT_COMMON            common( aRepo );
    MDW_GIT_SESSION         session( &common, std::move( credentials ), aCancel, !refused );
    KIGIT::REMOTE_TRANSPORT transport( &session, aRemote );

    if( aCall( transport, session ) == GIT_OK )
        return MdwOk();

    ws::WorkspaceError error =
            fail( session, transport.Error(), aRemote ? git_remote_url( aRemote ) : nullptr, aForbidden );

    if( error.code() == ws::WE_AUTH_FAILED )
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        // A sign-in since this transfer began is not the one refused, even with the same fields
        if( m_signIns == signIn )
            m_refused = signIn;
    }

    return error;
}


ws::WorkspaceError MDW_GIT_REMOTE::fail( MDW_GIT_SESSION& aSession, const KIGIT_ERROR_INFO& aError, const char* aUrl,
                                         bool* aForbidden )
{
    if( aSession.IsThisOperationCancelled() )
        return MdwCancelled();

    MDW_GIT_FAILURE_FACTS facts;

    facts.attempt = aSession.AuthAttempt();
    facts.authFailed = aSession.WasAuthFailure();
    facts.hostKey = aSession.GetHostKeyFailure();
    facts.hostKeyRefusal = aSession.GetHostKeyRefusal();
    facts.ssh = aSession.GetConnType() == KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH;
    facts.urlLogin = aSession.UrlLogin();

    return Describe( aError, facts, aUrl, aForbidden );
}


ws::WorkspaceError MDW_GIT_REMOTE::Describe( const KIGIT_ERROR_INFO& aError, const MDW_GIT_FAILURE_FACTS& aFacts,
                                             const char* aUrl, bool* aForbidden )
{
    const KIGIT_AUTH_ATTEMPT&                     attempt = aFacts.attempt;
    const std::optional<KIGIT::HOST_KEY_FAILURE>& refused = aFacts.hostKey;

    KIGIT_FAILURE_CONTEXT context;

    context.operation = aForbidden ? KIGIT_OPERATION::PUSH : KIGIT_OPERATION::CONNECT;
    context.url = wxString::FromUTF8( aUrl ? aUrl : "" );
    context.authFailed = aFacts.authFailed;
    context.hostKey = refused;
    context.hostKeyRefusal = aFacts.hostKeyRefusal;

    const KIGIT_FAILURE_REPORT report = KIGIT::DescribeGitFailure( aError, attempt, context );

    auto refusal = [&]( ws::WorkspaceErrorCode aCode, const wxString& aHeadline )
    {
        ws::WorkspaceError error = MdwFail( aCode, aHeadline.utf8_string() );

        error.set_detail( detailOf( report, aError.code ) );
        return error;
    };

    // A forge answers a read-only user's push with a 403 or an SSH refusal, which is no sign-in problem
    if( aForbidden && ( report.kind == KIGIT_FAILURE::PUSH_FORBIDDEN || report.kind == KIGIT_FAILURE::SERVER_REFUSED ) )
    {
        *aForbidden = RecordsDenial( report.kind );

        return refusal( ws::WE_PERMISSION_DENIED, report.summary );
    }

    // The server took this key's public half, so only its passphrase is missing, whatever the disconnect answered
    const bool needsPassphrase = !attempt.acceptedKey.IsEmpty() && !attempt.passphraseGiven;

    if( !refused
        && ( KIGIT::IsCredentialFailure( report.kind ) || aFacts.authFailed || aError.code == GIT_EAUTH
             || needsPassphrase ) )
    {
        MDW_GIT_CREDENTIALS given;

        {
            std::lock_guard<std::mutex> lock( m_mutex );
            given = m_credentials;
        }

        // Nothing given cannot be wrong, so the remote is asking for a sign-in
        const bool none = !aFacts.urlLogin && given.username.empty() && given.password.empty()
                          && given.sshKeyPath.empty() && given.sshPassphrase.empty();

        const wxString     realm = KIGIT::CredentialRealm( context.url );
        ws::WorkspaceError error = refusal( none ? ws::WE_NOT_AUTHENTICATED : ws::WE_AUTH_FAILED,
                                            SignInMessage( report, realm ) );

        *error.mutable_auth_challenge() =
                MdwGitAuthChallenge( given.username, aFacts.ssh );
        error.mutable_auth_challenge()->set_realm( realm.utf8_string() );
        return error;
    }

    if( !refused )
        return refusal( ws::WE_NETWORK, report.summary );

    const KIGIT::HOST_KEY_FAILURE& key = *refused;
    std::lock_guard<std::mutex>    lock( m_mutex );

    // Only the key this refusal met may be trusted afterwards
    m_pendingHostKey.reset();

    // A key that changed or was revoked is never the user's to wave through
    if( !KIGIT::MayOfferTrust( key, false ) )
        return MdwFail( ws::WE_PERMISSION_DENIED, KIGIT::HostKeyRefusalMessage( key ).utf8_string() );

    ws::WorkspaceError error = MdwFail( ws::WE_UNTRUSTED_HOST, "The server's host key is not trusted yet." );
    ws::HostKey*       hostKey = error.mutable_host_key();

    hostKey->set_host( key.host.utf8_string() );
    hostKey->set_port( key.port );
    hostKey->set_key_type( key.keyType.utf8_string() );
    hostKey->set_sha256_fingerprint( key.fingerprint.utf8_string() );

    m_pendingHostKey = key;
    return error;
}


bool MDW_GIT_REMOTE::Trust( const ws::HostKey& aKey )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if( !m_pendingHostKey || m_pendingHostKey->host.utf8_string() != aKey.host()
        || m_pendingHostKey->port != static_cast<int>( aKey.port() )
        || m_pendingHostKey->keyType.utf8_string() != aKey.key_type()
        || m_pendingHostKey->fingerprint.utf8_string() != aKey.sha256_fingerprint() )
    {
        return false;
    }

    if( !KIGIT::AddKnownHost( *m_pendingHostKey ) )
        return false;

    m_pendingHostKey.reset();
    return true;
}


std::string MdwGitError()
{
    wxString message = KIGIT::GetLastGitErrorInfo( 0 ).message;

    return message.IsEmpty() ? std::string( "git error" ) : KIGIT::StripEmbeddedUserinfo( message ).utf8_string();
}


kiapi::workspace::WorkspaceError MdwGitLockActions( KIGIT::REPO_ACTION_LOCK* aLock )
{
    namespace ws = kiapi::workspace;

    if( !aLock )
        return MdwFail( ws::WE_INTERNAL, "The repository has no git directory" );

    int result = aLock->Acquire();

    if( result == GIT_ELOCKED )
        return MdwFail( ws::WE_BUSY, "Another git operation is running; try again" );

    if( result != 0 )
        return MdwFail( ws::WE_INTERNAL, "Cannot lock the repository: " + aLock->Error() );

    return MdwOk();
}


std::string MdwRandomToken()
{
    return boost::uuids::to_string( boost::uuids::random_generator()() );
}


std::string MdwValidUtf8( const std::string& aText )
{
    return wxString::FromUTF8( aText ).empty() && !aText.empty() ? std::string() : aText;
}
