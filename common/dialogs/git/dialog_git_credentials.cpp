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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_git_credentials.h"

DIALOG_GIT_CREDENTIALS::DIALOG_GIT_CREDENTIALS( wxWindow* aParent, const wxString& aUrl,
                                                KIGIT_COMMON::GIT_CONN_TYPE aConnType, const wxString& aDefaultUsername,
                                                const wxString& aDefaultSSHKey ) :
        DIALOG_GIT_CREDENTIALS_BASE( aParent )
{
    m_urlLabel->SetLabel( aUrl );
    m_userCtrl->SetValue( aDefaultUsername );

    if( !aDefaultSSHKey.IsEmpty() )
        m_keyPicker->SetFileName( wxFileName( aDefaultSSHKey ) );

    if( aConnType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        m_authChoice->SetSelection( 1 );
    else
        m_authChoice->SetSelection( 0 );

    // Auth type is determined by the remote URL scheme; don't let the user
    // pick a transport that doesn't match.
    m_authChoice->Enable( false );

    updateFieldsForConnType();

    SetupStandardButtons();

    Layout();
    GetSizer()->Fit( this );
    Centre();
}


void DIALOG_GIT_CREDENTIALS::OnConnTypeChanged( wxCommandEvent& aEvent )
{
    updateFieldsForConnType();
    aEvent.Skip();
}


void DIALOG_GIT_CREDENTIALS::updateFieldsForConnType()
{
    bool ssh = ( m_authChoice->GetSelection() == 1 );

    m_passLabel->Enable( !ssh );
    m_passCtrl->Enable( !ssh );
    m_keyLabel->Enable( ssh );
    m_keyPicker->Enable( ssh );

    if( ssh )
        m_passLabel->SetLabel( _( "Key passphrase:" ) );
    else
        m_passLabel->SetLabel( _( "Password / token:" ) );

    // SSH still needs a username (typically "git") but it can stay editable.
}


wxString DIALOG_GIT_CREDENTIALS::GetUsername() const
{
    return m_userCtrl->GetValue();
}


wxString DIALOG_GIT_CREDENTIALS::GetPassword() const
{
    return m_passCtrl->GetValue();
}


wxString DIALOG_GIT_CREDENTIALS::GetSSHKey() const
{
    return m_keyPicker->GetFileName().GetFullPath();
}


KIGIT_COMMON::GIT_CONN_TYPE DIALOG_GIT_CREDENTIALS::GetConnType() const
{
    if( m_authChoice->GetSelection() == 1 )
        return KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;

    return KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS;
}


bool DIALOG_GIT_CREDENTIALS::SaveCredentials() const
{
    return m_saveCheck->IsChecked();
}
