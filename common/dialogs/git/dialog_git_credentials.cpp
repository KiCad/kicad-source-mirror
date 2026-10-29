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

#include "dialog_git_credentials.h"

#include <diff_merge/diff_scene.h>
#include <git/kigit_failure.h>
#include <git/kigit_ssh_keys.h>
#include <kiplatform/ui.h>

#include <wx/dcclient.h>

DIALOG_GIT_CREDENTIALS::DIALOG_GIT_CREDENTIALS( wxWindow* aParent, const wxString& aUrl, const wxString& aExplanation,
                                                KIGIT_COMMON::GIT_CONN_TYPE aConnType, const wxString& aDefaultUsername,
                                                const wxString& aDefaultSSHKey ) :
        DIALOG_GIT_CREDENTIALS_BASE( aParent ),
        m_connType( aConnType )
{
    OptOut( this );

    if( aUrl.IsEmpty() )
    {
        m_promptLabel->SetLabelText( _( "Authentication is required to access the remote repository." ) );
    }
    else
    {
        // Long remote paths would otherwise widen the dialog past the screen
        wxClientDC dc( m_promptLabel );
        dc.SetFont( m_promptLabel->GetFont() );
        const wxString safeUrl = KIGIT::StripEmbeddedUserinfo( aUrl );
        wxString url = wxControl::Ellipsize( safeUrl, dc, wxELLIPSIZE_MIDDLE, FromDIP( 400 ), wxELLIPSIZE_FLAGS_NONE );

        m_promptLabel->SetLabelText( wxString::Format( _( "Authentication is required to access %s." ), url ) );
        m_promptLabel->Wrap( FromDIP( 500 ) );
        m_promptLabel->SetToolTip( safeUrl );
    }

    if( !aExplanation.IsEmpty() )
    {
        KICAD_DIFF::DIFF_COLOR_THEME theme = KICAD_DIFF::DIFF_COLOR_THEME::ForText( KIPLATFORM::UI::IsDarkTheme() );

        m_explanationLabel->SetLabelText( aExplanation );
        m_explanationLabel->SetForegroundColour( theme.removed.ToColour() );
        m_explanationLabel->Wrap( FromDIP( 500 ) );
        m_explanationLabel->Show();
    }

    if( m_connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        m_urlUsername = KIGIT_COMMON::ParseRemoteURL( aUrl ).username;

    m_userCtrl->SetValue( m_urlUsername.IsEmpty() ? aDefaultUsername : m_urlUsername );

    // Otherwise the picker opens wherever the last file dialog was, far from any key
    m_keyPicker->SetInitialDirectory( KIGIT::SSHKeyFolder() );

    if( !aDefaultSSHKey.IsEmpty() )
        m_keyPicker->SetFileName( wxFileName( aDefaultSSHKey ) );

    // Keyed on the address, not the session, so a type chosen here earlier cannot lock the choice
    KIGIT_COMMON::GIT_CONN_TYPE urlConnType = KIGIT_COMMON::ParseRemoteURL( aUrl ).connType;
    m_authChoice->Enable( urlConnType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL );

    if( m_connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
        m_connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS;

    m_authChoice->SetSelection( m_connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH ? 1 : 0 );

    updateFieldsForConnType();

    SetupStandardButtons();

    Layout();
    GetSizer()->Fit( this );
    Centre();
}


void DIALOG_GIT_CREDENTIALS::OnConnTypeChanged( wxCommandEvent& aEvent )
{
    if( m_authChoice->GetSelection() == 1 )
        m_connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;
    else
        m_connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS;

    updateFieldsForConnType();
    aEvent.Skip();
}


void DIALOG_GIT_CREDENTIALS::updateFieldsForConnType()
{
    bool ssh = ( m_connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );

    m_keyLabel->Enable( ssh );
    m_keyPicker->Enable( ssh );

    // The server refuses any user other than the one the SSH address names
    m_userCtrl->Enable( !ssh || m_urlUsername.IsEmpty() );

    if( ssh )
    {
        m_passLabel->SetLabel( _( "Key passphrase:" ) );
        m_passCtrl->SetHint( _( "Leave empty if the key has no passphrase." ) );
    }
    else
    {
        m_passLabel->SetLabel( _( "Password / token:" ) );
        m_passCtrl->SetHint( wxEmptyString );
    }
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
    return KIGIT::PrivateKeyPath( m_keyPicker->GetFileName().GetFullPath() );
}


KIGIT_COMMON::GIT_CONN_TYPE DIALOG_GIT_CREDENTIALS::GetConnType() const
{
    return m_connType;
}


bool DIALOG_GIT_CREDENTIALS::SaveCredentials() const
{
    return m_saveCheck->IsChecked();
}
