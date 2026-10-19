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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/dialog_git_identity.h>

#include <confirm.h>
#include <dialogs/git/git_ui_utils.h>


DIALOG_GIT_IDENTITY::DIALOG_GIT_IDENTITY( wxWindow* aParent, const wxString& aTitle, const wxString& aName,
                                          const wxString& aEmail ) :
        DIALOG_GIT_IDENTITY_BASE( aParent, wxID_ANY, aTitle )
{
    OptOut( this );

    m_nameCtrl->SetValue( aName );
    m_emailCtrl->SetValue( aEmail );

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_GIT_IDENTITY::TransferDataFromWindow()
{
    m_name = m_nameCtrl->GetValue();
    m_name.Trim( true ).Trim( false );
    m_email = m_emailCtrl->GetValue();
    m_email.Trim( true ).Trim( false );

    wxString reason;

    if( !KIGIT::ValidateIdentity( m_name, m_email, reason ) )
    {
        DisplayInfoMessage( this, reason );

        if( m_name.IsEmpty() )
            m_nameCtrl->SetFocus();
        else
            m_emailCtrl->SetFocus();

        return false;
    }

    return true;
}
