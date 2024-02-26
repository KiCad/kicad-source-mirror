/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_git_repository.h"
#include <confirm.h>

#include <git2.h>
#include <gestfich.h>

#include <cerrno>
#include <cstring>
#include <fstream>

#include <wx/clipbrd.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>
#include <wx/stdpaths.h>


DIALOG_GIT_REPOSITORY::DIALOG_GIT_REPOSITORY( wxWindow* aParent, git_repository* aRepository,
                                              wxString aURL ) :
        DIALOG_GIT_REPOSITORY_BASE( aParent ),
        m_repository( aRepository ),
        m_prevFile( wxEmptyString ),
        m_tested( 0 ),
        m_failedTest( false ),
        m_testError( wxEmptyString ),
        m_tempRepo( false ),
        m_repoType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
{
    m_txtURL->SetFocus();

    if( !m_repository )
    {
        // Make a temporary repository to test the connection
        m_tempRepo = true;
        m_tempPath = wxFileName::CreateTempFileName( "kicadtestrepo" );

        git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_NO_REINIT;
        git_repository_init_ext( &m_repository, m_tempPath.ToStdString().c_str(), &options );
    }

    if( !aURL.empty() )
        m_txtURL->SetValue( aURL );
    else
        extractClipboardData();

    if( !m_txtURL->GetValue().IsEmpty() )
        updateURLData();

    SetupStandardButtons();
    updateAuthControls();

    Layout();
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
    if( wxTheClipboard->Open() && wxTheClipboard->IsSupported( wxDF_TEXT ) )
    {
        wxString clipboardText;
        wxTextDataObject textData;

        if( wxTheClipboard->GetData( textData ) && !( clipboardText = textData.GetText() ).empty() )
        {
            if( std::get<0>( isValidHTTPS( clipboardText ) )
                || std::get<0>( isValidSSH( clipboardText ) ) )
            {
                m_txtURL->SetValue( clipboardText );
            }
        }

        wxTheClipboard->Close();
    }

    return false;
}


void DIALOG_GIT_REPOSITORY::setDefaultSSHKey()
{
    wxFileName sshKey;
    sshKey.SetPath( wxGetUserHome() );
    wxString retval;

    sshKey.AppendDir( ".ssh" );
    sshKey.SetFullName( "id_rsa" );

    if( sshKey.FileExists() )
    {
        retval = sshKey.GetFullPath();
    }
    else if( sshKey.SetFullName( "id_dsa" ); sshKey.FileExists() )
    {
        retval = sshKey.GetFullPath();
    }
    else if( sshKey.SetFullName( "id_ecdsa" ); sshKey.FileExists() )
    {
        retval = sshKey.GetFullPath();
    }

    if( !retval.empty() )
    {
        m_fpSSHKey->SetFileName( retval );
        wxFileDirPickerEvent evt;
        evt.SetPath( retval );
        OnFileUpdated( evt );
    }
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

std::tuple<bool,wxString,wxString,wxString> DIALOG_GIT_REPOSITORY::isValidHTTPS( const wxString& url )
{
    wxRegEx regex( R"((https?:\/\/)(([^:]+)(:([^@]+))?@)?([^\/]+\/[^\s]+))" );

    if( regex.Matches( url ) )
    {
        wxString username = regex.GetMatch( url, 3 );
        wxString password = regex.GetMatch( url, 5 );
        wxString repoAddress = regex.GetMatch( url, 1 ) + regex.GetMatch( url, 6 );
        return std::make_tuple( true, username, password, repoAddress );
    }

    return std::make_tuple( false, "", "", "" );
}


std::tuple<bool,wxString, wxString> DIALOG_GIT_REPOSITORY::isValidSSH( const wxString& url )
{
    wxRegEx regex( R"((?:ssh:\/\/)?([^@]+)@([^\/]+\/[^\s]+))" );

    if( regex.Matches( url ) )
    {
        wxString username = regex.GetMatch( url, 1 );
        wxString repoAddress = regex.GetMatch( url, 2 );
        return std::make_tuple( true, username, repoAddress );
    }

    return std::make_tuple( false, "", "" );
}


static wxString get_repo_name( wxString& aRepoAddr )
{
    wxString retval;
    size_t last_slash = aRepoAddr.find_last_of( '/' );
    bool ends_with_dot_git = aRepoAddr.EndsWith( ".git" );

    if( ends_with_dot_git )
        retval = aRepoAddr.substr( last_slash + 1, aRepoAddr.size() - last_slash - 5 );
    else
        retval = aRepoAddr.substr( last_slash + 1, aRepoAddr.size() - last_slash );

    return retval;
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

    if( url.Contains( "https://" ) || url.Contains( "http://" ) )
    {
        auto [valid, username, password, repoAddress] = isValidHTTPS( url );

        if( valid )
        {
            m_ConnType->SetSelection( static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS ) );
            SetUsername( username );
            SetPassword( password );
            m_txtURL->SetValue( repoAddress );

            if( m_txtName->GetValue().IsEmpty() )
                m_txtName->SetValue( get_repo_name( repoAddress ) );

        }
    }
    else if( url.Contains( "ssh://" ) || url.Contains( "git@" ) )
    {
        auto [valid, username, repoAddress] = isValidSSH( url );

        if( valid )
        {
            m_ConnType->SetSelection( static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH ) );
            m_txtUsername->SetValue( username );
            m_txtURL->SetValue( repoAddress );

            if( m_txtName->GetValue().IsEmpty() )
                m_txtName->SetValue( get_repo_name( repoAddress ) );

            setDefaultSSHKey();
        }
    }
}


void DIALOG_GIT_REPOSITORY::OnTestClick( wxCommandEvent& event )
{
    git_remote* remote = nullptr;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;

    // We track if we have already tried to connect.
    // If we have, the server may come back to offer another connection
    // type, so we need to keep track of how many times we have tried.
    m_tested = 0;

    callbacks.credentials = []( git_cred** aOut, const char* aUrl, const char* aUsername,
                                unsigned int aAllowedTypes, void* aPayload ) -> int
    {
        DIALOG_GIT_REPOSITORY* dialog = static_cast<DIALOG_GIT_REPOSITORY*>( aPayload );

        if( dialog->GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
            return GIT_PASSTHROUGH;

        if( aAllowedTypes & GIT_CREDTYPE_USERNAME
            && !( dialog->GetTested() & GIT_CREDTYPE_USERNAME ) )
        {
            wxString username = dialog->GetUsername().Trim().Trim( false );
            git_cred_username_new( aOut, username.ToStdString().c_str() );
            dialog->GetTested() |= GIT_CREDTYPE_USERNAME;
        }
        else if( dialog->GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS
                 && ( aAllowedTypes & GIT_CREDTYPE_USERPASS_PLAINTEXT )
                 && !( dialog->GetTested() & GIT_CREDTYPE_USERPASS_PLAINTEXT ) )
        {
            wxString username = dialog->GetUsername().Trim().Trim( false );
            wxString password = dialog->GetPassword().Trim().Trim( false );

            git_cred_userpass_plaintext_new( aOut, username.ToStdString().c_str(),
                                             password.ToStdString().c_str() );
            dialog->GetTested() |= GIT_CREDTYPE_USERPASS_PLAINTEXT;
        }
        else if( dialog->GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
                 && ( aAllowedTypes & GIT_CREDTYPE_SSH_KEY )
                 && !( dialog->GetTested() & GIT_CREDTYPE_SSH_KEY ) )
        {
            // SSH key authentication
            wxString sshKey = dialog->GetRepoSSHPath();
            wxString sshPubKey = sshKey + ".pub";
            wxString username = dialog->GetUsername().Trim().Trim( false );
            wxString password = dialog->GetPassword().Trim().Trim( false );

            git_cred_ssh_key_new( aOut, username.ToStdString().c_str(),
                                  sshPubKey.ToStdString().c_str(), sshKey.ToStdString().c_str(),
                                  password.ToStdString().c_str() );
            dialog->GetTested() |= GIT_CREDTYPE_SSH_KEY;
        }
        else
        {
            return GIT_PASSTHROUGH;
        }

        return GIT_OK;
    };

    callbacks.payload = this;

    wxString txtURL = m_txtURL->GetValue();
    git_remote_create_with_fetchspec( &remote, m_repository, "origin", txtURL.ToStdString().c_str(),
                                      "+refs/heads/*:refs/remotes/origin/*" );

    if( git_remote_connect( remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr ) != GIT_OK )
        SetTestResult( true, git_error_last()->message );
    else
        SetTestResult( false, wxEmptyString );

    git_remote_disconnect( remote );
    git_remote_free( remote );

    auto dlg = wxMessageDialog( this, wxEmptyString, _( "Test connection" ), wxOK | wxICON_INFORMATION );

    if( !m_failedTest )
    {
        dlg.SetMessage( _( "Connection successful" ) );
    }
    else
    {
        dlg.SetMessage( wxString::Format( _( "Could not connect to '%s' " ), m_txtURL->GetValue() ) );
        dlg.SetExtendedMessage( m_testError );
    }

    dlg.ShowModal();
}


void DIALOG_GIT_REPOSITORY::OnFileUpdated( wxFileDirPickerEvent& aEvent )
{
    wxString file = aEvent.GetPath();

    if( file.ends_with( wxS( ".pub" ) ) )
        file = file.Left( file.size() - 4 );

    std::ifstream ifs( file.fn_str() );

    if( !ifs.good() || !ifs.is_open() )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Could not open private key '%s'" ), file ),
                             wxString::Format( "%s: %d", std::strerror( errno ), errno ) );
        return;
    }

    std::string line;
    std::getline( ifs, line );

    bool isValid = ( line.find( "PRIVATE KEY" ) != std::string::npos );
    bool isEncrypted = ( line.find( "ENCRYPTED" ) != std::string::npos );

    if( !isValid )
    {
        DisplayErrorMessage( this, _( "Invalid SSH Key" ),
                             _( "The selected file is not a valid SSH private key" ) );
        CallAfter( [this] { SetRepoSSHPath( m_prevFile ); } );
        return;
    }

    if( isEncrypted )
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

    ifs.close();

    wxString      pubFile = file + wxS( ".pub" );
    std::ifstream pubIfs( pubFile.fn_str() );

    if( !pubIfs.good() || !pubIfs.is_open() )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Could not open public key '%s'" ),
                                                     file + ".pub" ),
                             wxString::Format( "%s: %d", std::strerror( errno ), errno ) );
        aEvent.SetPath( wxEmptyString );
        CallAfter( [this] { SetRepoSSHPath( m_prevFile ); } );
        return;
    }

    m_prevFile = file;
    pubIfs.close();
}


void DIALOG_GIT_REPOSITORY::OnOKClick( wxCommandEvent& event )
{
    // Save the repository details

    if( m_txtName->GetValue().IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Missing information" ),
                             _( "Please enter a name for the repository" ) );
        return;
    }

    if( m_txtURL->GetValue().IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Missing information" ),
                             _( "Please enter a URL for the repository" ) );
        return;
    }

    EndModal( wxID_OK );
}


void DIALOG_GIT_REPOSITORY::updateAuthControls()
{
    if( m_ConnType->GetSelection() == static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL ) )
    {
        m_panelAuth->Show( false );
    }
    else
    {
        m_panelAuth->Show( true );

        if( m_ConnType->GetSelection() == static_cast<int>( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH ) )
        {
            m_fpSSHKey->Show( true );
            m_labelSSH->Show( true );
            m_labelPass1->SetLabel( _( "SSH Key Password" ) );
        }
        else
        {
            m_fpSSHKey->Show( false );
            m_labelSSH->Show( false );
            m_labelPass1->SetLabel( _( "Password" ) );
            setDefaultSSHKey();
        }
    }

    Layout();
}


void DIALOG_GIT_REPOSITORY::OnSelectConnType( wxCommandEvent& event )
{
    updateAuthControls();
}
