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
#include <confirm.h>
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

    // An ssh remote's user comes from its address, and the server refuses a different one.
    if( dlg.GetConnType() != KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
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
        if( dlg.GetConnType() != KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
            aProject->GetLocalSettings().m_GitRepoUsername = dlg.GetUsername();

        if( dlg.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH && !dlg.GetSSHKey().IsEmpty() )
            aProject->GetLocalSettings().m_GitSSHKey = dlg.GetSSHKey();
    }

    aCommon->ClearAuthFailure();
    aCommon->TestedTypes() = 0;
    aCommon->ResetNextKey();
    return true;
}


bool KIGIT::HasGitIdentity( KIGIT_COMMON& aCommon )
{
    if( !aCommon.GetRepo() )
        return false;

    git_signature* signature = nullptr;
    int            result = git_signature_default( &signature, aCommon.GetRepo() );
    git_signature_free( signature );
    return result == GIT_OK;
}


bool KIGIT::EnsureGitIdentity( wxWindow* aParent, KIGIT_COMMON* aCommon, bool aHasIdentity )
{
    if( !aCommon || !aCommon->GetRepo() || aHasIdentity )
        return true;

    wxString name = wxGetUserName();
    wxString email;

    if( !PromptGitIdentity( aParent, _( "Introduce Yourself" ), name, email ) )
        return false;

    bool saved = false;

    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayErrorMessage( aParent, _( "Another git operation is in progress, so the name and email were "
                                             "not saved. Try again in a moment." ) );
            return false;
        }

        saved = aCommon->SetUserConfig( name, email );
    }

    if( !saved )
    {
        DisplayErrorMessage( aParent, _( "Could not save the name and email to the repository." ) );
        return false;
    }

    return true;
}


bool KIGIT::PromptGitIdentity( wxWindow* aParent, const wxString& aTitle, wxString& aName, wxString& aEmail )
{
    DIALOG_GIT_IDENTITY dlg( aParent, aTitle, aName, aEmail );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aName = dlg.GetName();
    aEmail = dlg.GetEmail();
    return true;
}
