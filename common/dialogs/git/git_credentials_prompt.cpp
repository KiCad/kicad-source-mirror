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
#include <wx/utils.h>
#include <wx/weakref.h>
#include <mutex>
#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_ssh_keys.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <kiplatform/secrets.h>
#include <trace_helpers.h>
#include <widgets/kistatusbar.h>


// A session that never read its remote would show no address and offer no transport
static void resolveMissingRemote( KIGIT_COMMON* aCommon )
{
    if( !aCommon->GetRemote().IsEmpty() || !aCommon->GetRepo() )
        return;

    // A status read holds the lock for moments at a time, so a short wait usually gets it
    wxBusyCursor                 busy;
    std::unique_lock<std::mutex> lock = aCommon->TryLockActionFor( KIGIT_COMMON::ACTION_LOCK_WAIT );

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


// The project's key outlives the session's list, which a host change regathers, but a deleted one is never offered
static wxString initialSSHKey( KIGIT_COMMON* aCommon, PROJECT* aProject )
{
    if( aProject && KIGIT::IsOfferableSSHKey( aProject->GetLocalSettings().m_GitSSHKey ) )
        return aProject->GetLocalSettings().m_GitSSHKey;

    return aCommon->GetFirstOfferedSSHKey();
}


bool KIGIT::PromptForCredentials( wxWindow* aParent, KIGIT_COMMON* aCommon, PROJECT* aProject,
                                  const wxString& aExplanation )
{
    resolveMissingRemote( aCommon );

    KIGIT_COMMON::CREDENTIAL_IDENTITY identity = aCommon->GetCredentialIdentity();
    wxString                          shownUsername = identity.username;

    if( shownUsername.IsEmpty() && aProject )
        shownUsername = aProject->GetLocalSettings().m_GitRepoUsername;

    DIALOG_GIT_CREDENTIALS dlg( aParent, identity.remote, aExplanation, identity.connType, shownUsername,
                                initialSSHKey( aCommon, aProject ) );

    if( dlg.ShowModal() != wxID_OK )
        return false;

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
        return false;
    }

    wxString username = entered.username ? *entered.username : identity.username;

    if( dlg.SaveCredentials() && !KIPLATFORM::SECRETS::StoreSecret( identity.remote, username, entered.password ) )
    {
        DisplayInfoMessage( aParent, _( "The credentials could not be saved to the system keychain, so they "
                                        "are kept for this session only." ) );
    }

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

    return true;
}


bool KIGIT::PromptToTrustHost( wxWindow* aParent, const HOST_KEY_FAILURE& aFailure, bool aRetriedAfterTrust )
{
    wxString host = aFailure.host;

    if( aFailure.port != 22 )
        host << wxS( ":" ) << aFailure.port;

    if( !MayOfferTrust( aFailure, aRetriedAfterTrust ) )
    {
        DisplayErrorMessage( aParent, HostKeyRefusalMessage( aFailure ) );
        return false;
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
    auto dlg = KICAD_MESSAGE_DIALOG( aParent, wxString::Format( question, host, aFailure.fingerprint ),
                                     _( "Verify Server" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetYesNoLabels( _( "&Connect" ), _( "Cancel" ) );

    if( dlg.ShowModal() != wxID_YES )
        return false;

    if( !AddKnownHost( aFailure ) )
    {
        DisplayErrorMessage( aParent,
                             wxString::Format( _( "The key for %s could not be saved, so the connection was not "
                                                  "made. Check that your .ssh folder can be written to." ),
                                               host ),
                             KnownHostsPath() );
        return false;
    }

    return true;
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

    if( !aCommon.GetRepo() )
        return identity;

    GIT_CONFIG_HANDLER handler( &aCommon );
    identity.config.hasName = GetGitBackend()->GetConfigString( &handler, "user.name", identity.config.authorName );
    identity.config.hasEmail = GetGitBackend()->GetConfigString( &handler, "user.email", identity.config.authorEmail );

    git_signature* signature = nullptr;
    identity.configured = git_signature_default( &signature, aCommon.GetRepo() ) == GIT_OK;
    GitSignaturePtr signaturePtr( signature );
    return identity;
}


GitUserConfig KIGIT::ResolveGitIdentity( const GIT_IDENTITY& aIdentity, const COMMON_SETTINGS::GIT& aPreferences,
                                         const wxString& aUserName )
{
    GitUserConfig config = aIdentity.config;

    if( !aPreferences.useDefaultAuthor )
    {
        if( config.authorName.IsEmpty() )
            config.authorName = aPreferences.authorName;

        if( config.authorEmail.IsEmpty() )
            config.authorEmail = aPreferences.authorEmail;
    }

    if( config.authorName.IsEmpty() )
        config.authorName = aUserName;

    return config;
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
    GitUserConfig config = ResolveGitIdentity( aIdentity, Pgm().GetCommonSettings()->m_Git, wxGetUserName() );

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
        std::unique_lock<std::mutex> lock = KIGIT::WaitForActionLock( *aCommon, aParent );

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

    return aIdentity.configured || EditGitIdentity( aParent, aFrame, aCommon, aIdentity );
}
