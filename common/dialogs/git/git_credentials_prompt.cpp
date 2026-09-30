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
#include <confirm.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <mutex>
#include <git/kicad_git_common.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <kiplatform/secrets.h>


bool KIGIT::PromptForCredentials( wxWindow* aParent, KIGIT_COMMON* aCommon, PROJECT* aProject )
{
    DIALOG_GIT_CREDENTIALS dlg( aParent, aCommon->GetRemote(), aCommon->GetConnType(), aCommon->GetUsername(),
                                wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aCommon->SetUsername( dlg.GetUsername() );
    aCommon->SetPassword( dlg.GetPassword() );

    if( dlg.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH && !dlg.GetSSHKey().IsEmpty() )
        aCommon->SetSSHKey( dlg.GetSSHKey() );

    if( dlg.SaveCredentials()
        && !KIPLATFORM::SECRETS::StoreSecret( aCommon->GetRemote(), aCommon->GetUsername(), dlg.GetPassword() ) )
    {
        DisplayInfoMessage( aParent, _( "The credentials could not be saved to the system keychain, so they "
                                        "are kept for this session only." ) );
    }

    // A correction only in the session would be reverted by the next attach.
    if( aProject )
    {
        aProject->GetLocalSettings().m_GitRepoUsername = dlg.GetUsername();

        if( dlg.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH && !dlg.GetSSHKey().IsEmpty() )
            aProject->GetLocalSettings().m_GitSSHKey = dlg.GetSSHKey();
    }

    aCommon->ClearAuthFailure();
    aCommon->TestedTypes() = 0;
    aCommon->ResetNextKey();
    return true;
}


bool KIGIT::EnsureGitIdentity( wxWindow* aParent, KIGIT_COMMON* aCommon )
{
    if( !aCommon || !aCommon->GetRepo() )
        return true;

    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        // Contended means another operation is running, and the caller's own will report it.
        if( !lock.owns_lock() )
            return true;

        git_signature* sig = nullptr;

        if( git_signature_default( &sig, aCommon->GetRepo() ) == GIT_OK )
        {
            git_signature_free( sig );
            return true;
        }
    }

    wxDialog dlg( aParent, wxID_ANY, _( "Introduce Yourself" ) );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* message =
            new wxStaticText( &dlg, wxID_ANY, _( "Commits record who made them. Enter the name and email to use." ) );
    mainSizer->Add( message, 0, wxALL, 10 );

    wxFlexGridSizer* fields = new wxFlexGridSizer( 2, 2, 6, 6 );
    fields->AddGrowableCol( 1 );

    fields->Add( new wxStaticText( &dlg, wxID_ANY, _( "Name:" ) ), 0, wxALIGN_CENTER_VERTICAL );
    wxTextCtrl* nameCtrl = new wxTextCtrl( &dlg, wxID_ANY, wxGetUserName(), wxDefaultPosition, wxSize( 320, -1 ) );
    fields->Add( nameCtrl, 1, wxEXPAND );

    fields->Add( new wxStaticText( &dlg, wxID_ANY, _( "Email:" ) ), 0, wxALIGN_CENTER_VERTICAL );
    wxTextCtrl* emailCtrl = new wxTextCtrl( &dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 320, -1 ) );
    fields->Add( emailCtrl, 1, wxEXPAND );

    mainSizer->Add( fields, 1, wxEXPAND | wxLEFT | wxRIGHT, 10 );
    mainSizer->Add( dlg.CreateStdDialogButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 10 );

    dlg.SetSizerAndFit( mainSizer );
    dlg.Centre();

    while( true )
    {
        if( dlg.ShowModal() != wxID_OK )
            return false;

        if( nameCtrl->GetValue().Trim().IsEmpty() )
        {
            DisplayInfoMessage( &dlg, _( "Enter your name. It is recorded as the author of your commits." ) );
            nameCtrl->SetFocus();
            continue;
        }

        if( !emailCtrl->GetValue().Contains( wxS( "@" ) ) )
        {
            DisplayInfoMessage( &dlg, _( "Enter a valid email address. It is recorded as the author of "
                                         "your commits." ) );
            emailCtrl->SetFocus();
            continue;
        }

        break;
    }

    std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

    if( !lock.owns_lock() )
    {
        DisplayErrorMessage( aParent, _( "Another git operation is in progress, so the name and email were "
                                         "not saved. Try again in a moment." ) );
        return false;
    }

    if( !aCommon->SetUserConfig( nameCtrl->GetValue().Trim(), emailCtrl->GetValue().Trim() ) )
    {
        DisplayErrorMessage( aParent, _( "Could not save the name and email to the repository." ) );
        return false;
    }

    return true;
}
