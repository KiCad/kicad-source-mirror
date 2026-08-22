/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/dialog_git_credentials.h>
#include <dialogs/git/dialog_git_identity.h>
#include <dialogs/git/git_ui_utils.h>
#include <confirm.h>
#include <eda_base_frame.h>
#include <wx/app.h>
#include <wx/utils.h>
#include <wx/weakref.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <mutex>
#include <git/git_backend.h>
#include <widgets/wx_progress_reporters.h>
#include <git/git_clone_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_platform.h>
#include <git/kigit_ssh_keys.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <trace_helpers.h>
#include <widgets/kistatusbar.h>


// A session that never read its remote would show no address and offer no transport
static void resolveMissingRemote( KIGIT_COMMON* aCommon )
{
    if( !aCommon->GetRemote().IsEmpty() || !aCommon->GetRepo() )
        return;

    // A status read holds the lock for moments at a time, so a short wait usually gets it
    wxBusyCursor                 busy;
    KIGIT::ACTION_LOCK           lock = aCommon->TryLockActionFor( KIGIT_COMMON::ACTION_LOCK_WAIT );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "Credential prompt opens without an address, the repository is busy" );
        return;
    }

    wxString    remoteName = aCommon->GetRemoteNameOrDefault();
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, aCommon->GetRepo(), remoteName.utf8_string().c_str() ) != GIT_OK )
        return;

    KIGIT::GitRemotePtr remotePtr( remote );

    // The retry's credential callback compares against this, so both see one identity
    if( const char* url = git_remote_url( remote ) )
        aCommon->SyncRemoteUrl( wxString::FromUTF8( url ) );
}


// The window that asked can close while a prompt waits, freeing it and its project with it
static bool askerGone( const wxWeakRef<wxWindow>& aParent, bool aHadParent )
{
    if( !aHadParent )
        return false;

    if( !aParent )
        return true;

    // Destroy() on a top level window only queues the delete, so its weak ref still answers
    for( wxWindow* window = aParent.get(); window; window = window->GetParent() )
    {
        if( window->IsBeingDeleted() || wxTheApp->IsScheduledForDestruction( window ) )
            return true;
    }

    return false;
}


// Any modal here can outlast its parent, so every answer short of acceptance says whether it did
static KIGIT::PROMPT_ANSWER refusal( const wxWeakRef<wxWindow>& aParent, bool aHadParent )
{
    return askerGone( aParent, aHadParent ) ? KIGIT::PROMPT_ANSWER::ABANDONED : KIGIT::PROMPT_ANSWER::DECLINED;
}


// The project's key outlives the session's list, which a host change regathers, but a deleted one is never offered
static wxString initialSSHKey( KIGIT_COMMON* aCommon, PROJECT* aProject )
{
    // The server named this key as the one it takes, so its passphrase is the one to ask for
    if( wxString accepted = aCommon->GetAcceptedSSHKey(); KIGIT::IsOfferableSSHKey( accepted ) )
        return accepted;

    if( aProject && KIGIT::IsOfferableSSHKey( aProject->GetLocalSettings().m_GitSSHKey ) )
        return aProject->GetLocalSettings().m_GitSSHKey;

    return aCommon->GetFirstOfferedSSHKey();
}


KIGIT::PROMPT_ANSWER KIGIT::PromptForCredentials( wxWindow* aParent, KIGIT_COMMON* aCommon, PROJECT* aProject,
                                                  const wxString& aExplanation )
{
    resolveMissingRemote( aCommon );

    KIGIT_COMMON::CREDENTIAL_IDENTITY identity = aCommon->GetCredentialIdentity();
    wxString                          shownUsername = identity.username;

    if( shownUsername.IsEmpty() && aProject )
        shownUsername = aProject->GetLocalSettings().m_GitRepoUsername;

    if( shownUsername.IsEmpty() )
        shownUsername = PlatformServices().getAccount( CredentialRealm( identity.remote ) );

    wxWeakRef<wxWindow>    parent( aParent );
    DIALOG_GIT_CREDENTIALS dlg( aParent, identity.remote, aExplanation, identity.connType, shownUsername,
                                initialSSHKey( aCommon, aProject ) );

    if( dlg.ShowModal() != wxID_OK || askerGone( parent, aParent != nullptr ) )
        return refusal( parent, aParent != nullptr );

    bool ssh = dlg.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;

    // An ssh remote's user comes from its address, and the server refuses a different one.
    bool urlNamesUser = ssh && !KIGIT_COMMON::ParseRemoteURL( identity.remote ).username.IsEmpty();

    KIGIT_COMMON::PROMPTED_CREDENTIALS entered;
    entered.connType = dlg.GetConnType();
    entered.password = dlg.GetPassword();

    if( !urlNamesUser )
        entered.username = dlg.GetUsername();

    if( ssh )
        entered.sshKey = dlg.GetSSHKey();

    // A background fetch can move the session to another remote while the dialog is open
    if( !aCommon->ApplyPromptedCredentials( identity, entered ) )
    {
        wxLogTrace( traceGit, "Credential prompt discarded, the remote changed while it was open" );
        DisplayInfoMessage( aParent, _( "The remote changed while this window was open. Try the operation again." ) );
        return refusal( parent, aParent != nullptr );
    }

    // Held by the transfer's worker, so it names no window; the prompt may be long gone when it runs
    KIGIT_COMMON::CREDENTIAL_SAVER save =
            []( const wxString& aUrl, const wxString& aUsername, const wxString& aPassword )
    {
        SaveCredentialAsync( aUrl, aUsername, aPassword,
                             []( bool aStored )
                             {
                                 if( !aStored )
                                 {
                                     DisplayInfoMessage( nullptr, _( "The credentials could not be saved to the "
                                                                     "system keychain, so they are kept for this "
                                                                     "session only." ) );
                                 }
                             } );
    };

    // A typo the server refuses never reaches the keychain
    aCommon->SaveWhenAccepted( dlg.SaveCredentials() ? save : KIGIT_COMMON::CREDENTIAL_SAVER() );

    // A correction only in the session would be reverted by the next attach.
    if( aProject )
    {
        if( !urlNamesUser )
            aProject->GetLocalSettings().m_GitRepoUsername = dlg.GetUsername();

        if( ssh && !dlg.GetSSHKey().IsEmpty() )
            aProject->GetLocalSettings().m_GitSSHKey = dlg.GetSSHKey();

        if( KIGIT_COMMON::ParseRemoteURL( identity.remote ).connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
            aProject->GetLocalSettings().m_GitRepoType = ssh ? wxS( "ssh" ) : wxS( "https" );
    }

    return PROMPT_ANSWER::ACCEPTED;
}


static wxString hostLabel( const KIGIT::HOST_KEY_FAILURE& aFailure )
{
    wxString host = aFailure.host;

    if( aFailure.port != 22 )
        host << wxS( ":" ) << aFailure.port;

    return host;
}


KIGIT::PROMPT_ANSWER KIGIT::AskToTrustHost( wxWindow* aParent, const HOST_KEY_FAILURE& aFailure,
                                            bool aRetriedAfterTrust )
{
    wxWeakRef<wxWindow> parent( aParent );

    if( !MayOfferTrust( aFailure, aRetriedAfterTrust ) )
    {
        DisplayErrorMessage( aParent, HostKeyRefusalMessage( aFailure ) );
        return refusal( parent, aParent != nullptr );
    }

    wxString question;

    if( aFailure.state == KNOWN_HOST_STATE::OTHER_KEY_TYPE )
    {
        question = _( "%s is already known by a different key type. The key it presented has fingerprint "
                      "SHA256:%s. Connect and remember this key too?" );
    }
    else
    {
        question = _( "This is the first time connecting to %s. Its key fingerprint is SHA256:%s. Connect and "
                      "remember this server?" );
    }

    // Enter must never trust a server
    auto dlg = KICAD_MESSAGE_DIALOG( aParent, wxString::Format( question, hostLabel( aFailure ), aFailure.fingerprint ),
                                     _( "Verify Server" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetYesNoLabels( _( "&Connect" ), _( "Cancel" ) );

    if( dlg.ShowModal() != wxID_YES || askerGone( parent, aParent != nullptr ) )
        return refusal( parent, aParent != nullptr );

    return PROMPT_ANSWER::ACCEPTED;
}


KIGIT::PROMPT_ANSWER KIGIT::PromptToTrustHost( wxWindow* aParent, const HOST_KEY_FAILURE& aFailure,
                                               bool aRetriedAfterTrust )
{
    wxWeakRef<wxWindow> parent( aParent );
    PROMPT_ANSWER       answer = AskToTrustHost( aParent, aFailure, aRetriedAfterTrust );

    if( answer != PROMPT_ANSWER::ACCEPTED || AddKnownHost( aFailure ) )
        return answer;

    DisplayErrorMessage( aParent,
                         wxString::Format( _( "The key for %s could not be saved, so the connection was not "
                                              "made. Check that your .ssh folder can be written to." ),
                                           hostLabel( aFailure ) ),
                         KnownHostsPath() );
    return refusal( parent, aParent != nullptr );
}


KIGIT::TRANSFER_RETRY KIGIT::AnswerTransferFailure( wxWindow* aParent, const std::weak_ptr<bool>& aAlive,
                                                    KIGIT_COMMON* aCommon, PROJECT* aProject,
                                                    const TRANSFER_FAILURE& aFailure, bool aRetriedAfterTrust,
                                                    int& aPrompts )
{
    if( aFailure.cancelled )
    {
        aPrompts = 0;
        return TRANSFER_RETRY::CANCELLED;
    }

    // A prompt runs an event loop in which the window can close
    if( aFailure.hostKey )
    {
        PROMPT_ANSWER answer = PromptToTrustHost( aParent, *aFailure.hostKey, aRetriedAfterTrust );

        if( aAlive.expired() || answer == PROMPT_ANSWER::ABANDONED )
            return TRANSFER_RETRY::GONE;

        if( answer == PROMPT_ANSWER::DECLINED )
        {
            aPrompts = 0;
            return TRANSFER_RETRY::HOST_DECLINED;
        }

        return TRANSFER_RETRY::RETRY_TRUSTED;
    }

    // A remote that keeps changing could otherwise send the request back to the keyring forever
    if( !aFailure.authFailure || aPrompts >= MAX_CREDENTIAL_PROMPTS )
    {
        aPrompts = 0;
        return TRANSFER_RETRY::REPORT;
    }

    ++aPrompts;

    // A stored secret answers before anyone is asked to type one
    if( !aCommon->SecretRead() )
        return TRANSFER_RETRY::RETRY;

    PROMPT_ANSWER answer = PromptForCredentials( aParent, aCommon, aProject, aFailure.error );

    if( aAlive.expired() || answer == PROMPT_ANSWER::ABANDONED )
        return TRANSFER_RETRY::GONE;

    if( answer == PROMPT_ANSWER::DECLINED )
    {
        aPrompts = 0;
        return TRANSFER_RETRY::CANCELLED;
    }

    return TRANSFER_RETRY::RETRY;
}


KIGIT::CLONE_OUTCOME KIGIT::CloneWithPrompts( wxWindow* aParent, KIGIT_COMMON& aCommon, const wxString& aUrl,
                                              const wxString& aPath, const wxString& aUsername,
                                              const wxString& aPassword, const wxString& aSSHKey )
{
    bool retriedAfterTrust = false;

    while( true )
    {
        bool     cloned = false;
        bool     cancelled = false;
        wxString error;
        wxString details;

        std::optional<KIGIT::HOST_KEY_FAILURE> hostKey;

        // Scoped, so the progress window is gone before a prompt below
        {
            GIT_CLONE_HANDLER cloneHandler( &aCommon );

            cloneHandler.SetRemote( aUrl );
            cloneHandler.SetClonePath( aPath );
            cloneHandler.SetUsername( aUsername );
            cloneHandler.SetPassword( aPassword );
            cloneHandler.SetSSHKey( aSSHKey );

            cloneHandler.SetProgressReporter(
                    std::make_unique<WX_PROGRESS_REPORTER>( aParent, _( "Clone Repository" ), 1, PR_CAN_ABORT ) );

            cloned = cloneHandler.PerformClone();
            cancelled = !cloned && cloneHandler.IsThisOperationCancelled();

            if( !cloned )
            {
                KIGIT_ERROR_MESSAGE message = cloneHandler.PopError();
                error = message.summary;
                details = message.details;
                hostKey = cloneHandler.GetHostKeyFailure();
            }
        }

        if( cloned )
            return CLONE_OUTCOME::CLONED;

        // Cancelling from the progress dialog is not an error
        if( cancelled )
            return CLONE_OUTCOME::CANCELLED;

        if( hostKey )
        {
            if( PromptToTrustHost( aParent, *hostKey, retriedAfterTrust ) != PROMPT_ANSWER::ACCEPTED )
                return CLONE_OUTCOME::CANCELLED;

            retriedAfterTrust = true;
            continue;
        }

        DisplayErrorMessage( aParent, error, details );
        return CLONE_OUTCOME::FAILED;
    }
}


void KIGIT::ClearSyncWarnings( wxWindow* aWindow )
{
    for( wxWindow* window = aWindow; window; window = window->GetParent() )
    {
        if( wxFrame* frame = dynamic_cast<wxFrame*>( window ) )
        {
            if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( frame->GetStatusBar() ) )
                statusBar->ClearWarningMessages( SYNC_WARNING_SOURCE );

            return;
        }
    }
}


void KIGIT::ClearSyncWarnings( const KIGIT_COMMON* aCommon )
{
    // The manager lists the warnings, and an editor's pull or push answers them as well
    for( wxWindow* window : wxTopLevelWindows )
    {
        EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( window );

        if( frame && frame->HasKiway() && frame->Prj().GetGitCommon().get() == aCommon )
            ClearSyncWarnings( frame );
    }
}


KIGIT::GIT_IDENTITY KIGIT::ReadGitIdentity( KIGIT_COMMON& aCommon )
{
    GIT_IDENTITY identity;

    ReadAuthorConfig( aCommon.GetRepo(), identity.local, identity.global );
    return identity;
}


GitUserConfig KIGIT::PrefillAuthor( const GIT_IDENTITY& aIdentity )
{
    GitUserConfig author = ResolveAuthor( Pgm().GetCommonSettings()->m_Git, aIdentity.local, aIdentity.global );

    if( author.authorName.IsEmpty() )
        author.authorName = wxGetUserName();

    return author;
}


namespace
{

bool identitySessionCurrent( EDA_BASE_FRAME* aFrame, const std::shared_ptr<KIGIT_COMMON>& aCommon )
{
    return aFrame && !aFrame->IsBeingDeleted() && aCommon && !aCommon->IsShuttingDown() && aCommon->GetRepo()
           && aFrame->Prj().GetGitCommon() == aCommon && Pgm().GetCommonSettings()->m_Git.enableGit
           && !aFrame->Prj().GetLocalSettings().m_GitIntegrationDisabled;
}

} // namespace


bool KIGIT::EditGitIdentity( wxWindow* aParent, EDA_BASE_FRAME* aFrame, const std::shared_ptr<KIGIT_COMMON>& aCommon,
                             const GIT_IDENTITY& aIdentity )
{
    if( !identitySessionCurrent( aFrame, aCommon ) )
        return false;

    wxWeakRef<EDA_BASE_FRAME> frame( aFrame );
    wxWeakRef<wxWindow>       parent( aParent );
    GitUserConfig config = PrefillAuthor( aIdentity );

    {
        DIALOG_GIT_IDENTITY dlg( aParent, _( "Set Author" ), config.authorName, config.authorEmail );

        if( dlg.ShowModal() != wxID_OK )
            return false;

        config.authorName = dlg.GetAuthorName();
        config.authorEmail = dlg.GetEmail();
    }

    if( !parent || parent->IsBeingDeleted() || !identitySessionCurrent( frame.get(), aCommon ) )
        return false;

    bool saved = false;

    {
        KIGIT::ACTION_LOCK lock = KIGIT::WaitForActionLock( *aCommon, aParent );

        if( !lock.owns_lock() )
            return false;

        if( !identitySessionCurrent( frame.get(), aCommon ) )
            return false;

        saved = aCommon->SetUserConfig( config.authorName, config.authorEmail );
    }

    if( !saved )
    {
        DisplayErrorMessage( aParent, _( "Could not save the name and email to the repository." ) );
        return false;
    }

    aCommon->NotifyRefreshListeners();
    return true;
}


bool KIGIT::EnsureGitIdentity( wxWindow* aParent, EDA_BASE_FRAME* aFrame, const std::shared_ptr<KIGIT_COMMON>& aCommon,
                               const GIT_IDENTITY& aIdentity )
{
    if( !identitySessionCurrent( aFrame, aCommon ) )
        return false;

    GitUserConfig author = ResolveAuthor( Pgm().GetCommonSettings()->m_Git, aIdentity.local, aIdentity.global );

    return ( author.hasName && author.hasEmail ) || EditGitIdentity( aParent, aFrame, aCommon, aIdentity );
}
