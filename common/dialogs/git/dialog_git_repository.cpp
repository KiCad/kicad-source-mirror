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

#include "dialog_git_repository.h"
#include <confirm.h>
#include <dialogs/git/git_credentials_prompt.h>

#include <git2.h>
#include <git/kicad_git_memory.h>
#include <git/kicad_git_common.h>
#include <git/kigit_ssh_keys.h>
#include <git/git_repo_mixin.h>
#include <gestfich.h>

#include <cerrno>
#include <cstring>
#include <fstream>

#include <wx/clipbrd.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>
#include <wx/stdpaths.h>


DIALOG_GIT_REPOSITORY::DIALOG_GIT_REPOSITORY( wxWindow* aParent, git_repository* aRepository, wxString aURL ) :
        DIALOG_GIT_REPOSITORY_BASE( aParent ),
        m_repository( aRepository ),
        m_incomeURL( aURL ),
        m_tempRepo( false )
{
    OptOut( this );

    m_txtURL->SetFocus();

    if( !m_repository )
    {
        // Make a temporary repository to test the connection
        m_tempRepo = true;
        m_tempPath = wxFileName::CreateTempFileName( "kicadtestrepo" );

        // CreateTempFileName leaves a file at the path, where git cannot make a repository
        wxRemoveFile( m_tempPath );

        git_repository_init_options options;
        git_repository_init_init_options( &options, GIT_REPOSITORY_INIT_OPTIONS_VERSION );
        options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_NO_REINIT;
        git_repository_init_ext( &m_repository, m_tempPath.ToStdString().c_str(), &options );
    }

    SetupStandardButtons( { { wxID_HELP, _( "Test Connection" ) } } );
    Layout();
    finishDialogSettings();

    m_txtURL->Bind( wxEVT_TEXT,
                    [this]( wxCommandEvent& aEvent )
                    {
                        updateURLData();
                        updateAuthControls();
                        aEvent.Skip();
                    } );
}


DIALOG_GIT_REPOSITORY::~DIALOG_GIT_REPOSITORY()
{
    if( m_tempRepo )
    {
        git_repository_free( m_repository );
        RmDirRecursive( m_tempPath );
    }
}


bool DIALOG_GIT_REPOSITORY::extractClipboardData()
{
    wxClipboardLocker lock;
    if( !lock )
        return false;

    if( !wxTheClipboard->IsSupported( wxDF_TEXT ) )
        return false;

    wxTextDataObject textData;
    if( !wxTheClipboard->GetData( textData ) )
        return false;

    wxString clipboardText = textData.GetText();
    if( clipboardText.empty() )
        return false;

    KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( clipboardText );

    if( parsed.connType != KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL && !parsed.hostname.IsEmpty()
        && clipboardText.find_first_of( " \t\r\n" ) == wxString::npos )
    {
        m_txtURL->SetValue( clipboardText );
        return true;
    }

    return false;
}


void DIALOG_GIT_REPOSITORY::setDefaultSSHKey( const wxString& aHost )
{
    std::vector<wxString> keys = KIGIT::DefaultSSHKeyCandidates( KIGIT::HomeDir(), aHost );

    if( keys.empty() )
        return;

    // Unticked greys out the picker and passphrase, hiding the key the connection will use
    m_cbCustom->SetValue( true );
    m_fpSSHKey->SetFileName( keys.front() );

    wxFileDirPickerEvent evt;
    evt.SetPath( keys.front() );
    OnFileUpdated( evt );
}


void DIALOG_GIT_REPOSITORY::onCbCustom( wxCommandEvent& event )
{
    updateAuthControls();
    event.Skip();
}


void DIALOG_GIT_REPOSITORY::OnUpdateUI( wxUpdateUIEvent& event )
{
    // event.Enable( !m_txtName->GetValue().IsEmpty() && !m_txtURL->GetValue().IsEmpty() );
}


void DIALOG_GIT_REPOSITORY::SetEncrypted( bool aEncrypted )
{
    if( aEncrypted )
    {
        m_txtPassword->Enable();
        m_txtPassword->SetToolTip( _( "Enter the password for the SSH key" ) );
    }
    else
    {
        m_txtPassword->SetValue( wxEmptyString );
        m_txtPassword->SetToolTip( wxEmptyString );
        m_txtPassword->Disable();
    }
}


static wxString get_repo_name( const wxString& aRepoAddr )
{
    wxString addr = aRepoAddr;

    // Strip GitHub/GitLab web-UI path suffixes so that pasting a browser URL
    // (e.g. .../repo/tree/master, .../repo/blob/main/README.md, .../repo/pulls)
    // still gives the repository name rather than a branch/page name.
    static wxRegEx webSuffix(
        R"((/-)?/(tree|blob|commits?|raw|releases|tags|branches|pulls|pull|issues|merge_requests|wiki|actions)/.*$)",
        wxRE_ADVANCED );

    webSuffix.ReplaceAll( &addr, wxEmptyString );

    while( addr.EndsWith( "/" ) )
        addr.RemoveLast();

    if( addr.EndsWith( ".git" ) )
        addr.RemoveLast( 4 );

    size_t last_slash = addr.find_last_of( "/\\" );

    if( last_slash == wxString::npos )
        return addr.AfterLast( ':' );

    return addr.substr( last_slash + 1 );
}


void DIALOG_GIT_REPOSITORY::OnLocationExit( wxFocusEvent& event )
{
    updateURLData();
    updateAuthControls();
    event.Skip();
}


void DIALOG_GIT_REPOSITORY::updateURLData()
{
    wxString url = m_txtURL->GetValue();

    if( url.IsEmpty() )
        return;

    KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( url );
    bool                     wasSSH = GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;
    m_ConnType->SetSelection( static_cast<int>( parsed.connType ) );

    if( parsed.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH && !wasSSH && m_fpSSHKey->GetPath().IsEmpty() )
        setDefaultSSHKey( parsed.hostname );

    if( !parsed.username.IsEmpty() )
        SetUsername( parsed.username );

    if( !parsed.password.IsEmpty() )
        SetPassword( parsed.password );

    // Keep the SSH username and port in the address. Passwords belong in the password field.
    if( parsed.url != url )
        m_txtURL->ChangeValue( parsed.url );

    m_txtName->SetValue( get_repo_name( parsed.url ) );
}


void DIALOG_GIT_REPOSITORY::OnTestClick( wxCommandEvent& event )
{
    wxString url = GetRepoURL();

    if( wxString( url ).Trim().Trim( false ).IsEmpty() || reportUnusableKey() )
        return;

    wxString error;
    wxString details;
    bool success = false;
    git_remote* remote = nullptr;
    git_remote_callbacks callbacks;
    git_remote_init_callbacks( &callbacks, GIT_REMOTE_CALLBACKS_VERSION );

    // We track if we have already tried to connect.
    // If we have, the server may come back to offer another connection
    // type, so we need to keep track of how many times we have tried.

    KIGIT_COMMON common( m_repository );
    common.SetRemote( url );

    // SetUsername clears the fetched-secret flag, so it comes first. The other order tests the
    // keychain's secret instead of the one just typed.
    common.SetUsername( GetUsername() );
    common.SetPassword( m_txtPassword->GetValue() );
    common.SetSSHKey( GetRepoSSHPath() );

    KIGIT_REPO_MIXIN repoMixin( &common );
    callbacks.payload = &repoMixin;
    callbacks.credentials = credentials_cb;
    callbacks.certificate_check = certificate_check_cb;

    git_proxy_options proxyOpts;
    git_proxy_init_options( &proxyOpts, GIT_PROXY_OPTIONS_VERSION );
    proxyOpts.type = GIT_PROXY_AUTO;
    proxyOpts.url = "";

    int                 result = git_remote_create_anonymous( &remote, m_repository, url.utf8_str() );
    KIGIT::GitRemotePtr remotePtr( remote );

    if( result == GIT_OK )
    {
        // url.insteadOf may have rewritten the URL, which would otherwise drop the credentials typed here
        if( const char* remoteUrl = git_remote_url( remote ) )
            common.FollowRewrittenUrl( wxString::FromUTF8( remoteUrl ) );

        bool retriedAfterTrust = false;

        while( true )
        {
            repoMixin.ResetTransferState();
            result = git_remote_connect( remote, GIT_DIRECTION_FETCH, &callbacks, &proxyOpts, nullptr );

            if( result == GIT_OK )
            {
                git_remote_disconnect( remote );
                success = true;
                break;
            }

            KIGIT_FAILURE_REPORT report = repoMixin.ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( result ),
                                                                           KIGIT_OPERATION::CONNECT, wxEmptyString );
            git_remote_disconnect( remote );
            error = report.summary;
            details = report.details;

            std::optional<KIGIT::HOST_KEY_FAILURE> hostKey = repoMixin.GetHostKeyFailure();

            if( !hostKey )
                break;

            // The prompt has told the user why, so a declined key ends the test here
            if( !KIGIT::PromptToTrustHost( this, *hostKey, retriedAfterTrust ) )
                return;

            retriedAfterTrust = true;
        }
    }
    else
    {
        KIGIT_FAILURE_CONTEXT context;
        context.operation = KIGIT_OPERATION::CONNECT;
        context.url = url;

        KIGIT_FAILURE_REPORT report =
                KIGIT::DescribeGitFailure( KIGIT_COMMON::GetLastGitErrorInfo( result ), {}, context );
        error = report.summary;
        details = report.details;
    }

    if( !success )
    {
        DisplayErrorMessage( this, error, details );
        return;
    }

    auto dlg = KICAD_MESSAGE_DIALOG( this, _( "Connection successful" ), _( "Test Connection" ),
                                     wxOK | wxICON_INFORMATION );
    dlg.ShowModal();
}


// False with aError and aDetails set when aPath is not a private key with its .pub beside it
static bool checkSSHKey( wxString aPath, wxString& aError, wxString& aDetails, bool& aEncrypted )
{
    if( aPath.ends_with( wxS( ".pub" ) ) )
        aPath = aPath.Left( aPath.size() - 4 );

    std::ifstream ifs( aPath.fn_str() );

    if( !ifs.good() || !ifs.is_open() )
    {
        aError = wxString::Format( _( "Could not open private key '%s'" ), aPath );
        aDetails = wxString::Format( "%s: %d", std::strerror( errno ), errno );
        return false;
    }

    std::string keyText( ( std::istreambuf_iterator<char>( ifs ) ), std::istreambuf_iterator<char>() );
    wxString    key = wxString::FromUTF8( keyText );

    if( !key.BeforeFirst( '\n' ).Contains( wxS( "PRIVATE KEY" ) ) )
    {
        aError = _( "Invalid SSH Key" );
        aDetails = _( "The selected file is not a valid SSH private key" );
        return false;
    }

    std::ifstream pubIfs( ( aPath + wxS( ".pub" ) ).fn_str() );

    if( !pubIfs.good() || !pubIfs.is_open() )
    {
        aError = wxString::Format( _( "Could not open public key '%s'" ), aPath + wxS( ".pub" ) );
        aDetails = wxString::Format( "%s: %d", std::strerror( errno ), errno );
        return false;
    }

    aEncrypted = KIGIT::IsPrivateKeyEncrypted( key );
    return true;
}


void DIALOG_GIT_REPOSITORY::OnFileUpdated( wxFileDirPickerEvent& aEvent )
{
    wxString error;
    wxString details;
    bool     encrypted = false;

    // The picker fires on every keystroke, so a half-typed path is not an error until OK
    if( checkSSHKey( aEvent.GetPath(), error, details, encrypted ) )
        SetEncrypted( encrypted );
}


bool DIALOG_GIT_REPOSITORY::reportUnusableKey()
{
    if( GetRepoType() != KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH || !m_cbCustom->IsChecked()
        || GetRepoSSHPath().IsEmpty() )
    {
        return false;
    }

    wxString error;
    wxString details;
    bool     encrypted = false;

    if( checkSSHKey( GetRepoSSHPath(), error, details, encrypted ) )
        return false;

    DisplayErrorMessage( this, error, details );
    return true;
}


// Empty when the address is one git can try; otherwise says what is wrong with it
static wxString describeUnusableURL( git_repository* aRepo, const wxString& aURL )
{
    git_remote* rawRemote = nullptr;

    if( git_remote_create_anonymous( &rawRemote, aRepo, aURL.utf8_str() ) != GIT_OK )
        return wxString::Format( _( "'%s' is not a repository address." ), aURL );

    KIGIT::GitRemotePtr remote( rawRemote );

    // A url.insteadOf alias is judged by the address it expands to
    wxString effective = wxString::FromUTF8( git_remote_url( remote.get() ) );
    wxString checked = effective;

    // The parser has no git:// case, so its server part is checked the way an HTTPS one is
    if( checked.Lower().StartsWith( wxS( "git://" ) ) )
        checked = wxS( "https://" ) + checked.Mid( 6 );

    KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( checked );

    if( parsed.connType != KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
    {
        const wxString& host = parsed.hostname;
        wxString        authority = checked.AfterFirst( ':' ).Mid( 2 ).BeforeFirst( '/' );

        // The parser drops an unclosed IPv6 bracket, so look for it in the address itself
        if( host.IsEmpty() || host.find_first_of( wxS( " \t" ) ) != wxString::npos
            || ( checked.Contains( wxS( "://" ) ) && authority.Contains( wxS( "[" ) )
                 && !authority.Contains( wxS( "]" ) ) ) )
        {
            return wxString::Format( _( "'%s' does not name a server." ), aURL );
        }

        if( parsed.port < 0 )
            return wxString::Format( _( "The port in '%s' is not a number." ), aURL );

        return wxEmptyString;
    }

    wxString lower = effective.Lower();

    if( lower.Contains( wxS( "://" ) ) && !lower.StartsWith( wxS( "file://" ) ) )
        return wxString::Format( _( "'%s' is not an HTTPS, SSH or folder address." ), aURL );

    return wxEmptyString;
}


void DIALOG_GIT_REPOSITORY::OnOKClick( wxCommandEvent& event )
{
    // Save the repository details

    if( m_txtURL->GetValue().IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Missing information" ),
                            _( "Please enter a URL for the repository" ) );
        return;
    }

    wxString problem = describeUnusableURL( m_repository, GetRepoURL() );

    if( !problem.IsEmpty() )
    {
        DisplayErrorMessage( this, problem,
                             _( "Use an address such as https://server/owner/project.git or "
                                "git@server:owner/project.git, or the path of a repository folder." ) );
        return;
    }

    if( reportUnusableKey() )
        return;

    EndModal( wxID_OK );
}


void DIALOG_GIT_REPOSITORY::updateAuthControls()
{
    if( m_ConnType->GetSelection() == static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL ) )
    {
        m_panelAuth->Enable( false );
    }
    else
    {
        m_panelAuth->Enable( true );

        if( m_ConnType->GetSelection() == static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH ) )
        {
            KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( GetRepoURL() );
            m_cbCustom->Enable( true );
            m_fpSSHKey->Enable( m_cbCustom->IsChecked() );

            // Edit an embedded SSH username in the address so the two fields cannot disagree.
            m_txtUsername->Enable( m_cbCustom->IsChecked() && parsed.username.IsEmpty() );
            m_txtPassword->Enable( m_cbCustom->IsChecked() );
            m_labelPass1->SetLabel( _( "SSH key password:" ) );
        }
        else
        {
            m_cbCustom->Enable( false );
            m_fpSSHKey->Enable( false );
            m_txtUsername->Enable( true );
            m_txtPassword->Enable( true );
            m_labelPass1->SetLabel( _( "Password:" ) );
        }
    }
}


void DIALOG_GIT_REPOSITORY::OnSelectConnType( wxCommandEvent& event )
{
    updateAuthControls();
}

bool DIALOG_GIT_REPOSITORY::TransferDataToWindow()
{
    if( !m_incomeURL.empty() )
        m_txtURL->SetValue( m_incomeURL );
    else
        extractClipboardData();

    if( !m_txtURL->GetValue().IsEmpty() )
        updateURLData();
    else
        m_ConnType->SetSelection( static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL ) );

    updateAuthControls();

    return true;
}
