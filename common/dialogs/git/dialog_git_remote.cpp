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

#include <dialogs/git/dialog_git_remote.h>

#include <confirm.h>


DIALOG_GIT_REMOTE::DIALOG_GIT_REMOTE( wxWindow* aParent, const wxString& aName, const wxString& aUrl ) :
        DIALOG_GIT_REMOTE_BASE( aParent, wxID_ANY, aName.IsEmpty() ? _( "Add Remote" ) : _( "Edit Remote" ) )
{
    OptOut( this );

    m_nameCtrl->SetValue( aName );
    m_urlCtrl->SetValue( aUrl );

    if( !aName.IsEmpty() )
    {
        m_nameCtrl->Disable();
        m_urlCtrl->SetFocus();
    }

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_GIT_REMOTE::TransferDataFromWindow()
{
    m_name = m_nameCtrl->GetValue();
    m_name.Trim( true ).Trim( false );
    m_url = m_urlCtrl->GetValue();
    m_url.Trim( true ).Trim( false );

    if( m_name.IsEmpty() )
    {
        DisplayInfoMessage( this, _( "Enter a name for the remote." ) );
        m_nameCtrl->SetFocus();
        return false;
    }

    if( m_url.IsEmpty() )
    {
        DisplayInfoMessage( this, _( "Enter the remote's URL." ) );
        m_urlCtrl->SetFocus();
        return false;
    }

    return true;
}
