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

#include <dialogs/git/dialog_git_new_tag.h>

#include <wx/settings.h>

#include <confirm.h>


DIALOG_GIT_NEW_TAG::DIALOG_GIT_NEW_TAG( wxWindow* aParent ) :
        DIALOG_GIT_NEW_TAG_BASE( aParent )
{
    OptOut( this );

    m_hint->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_GIT_NEW_TAG::TransferDataFromWindow()
{
    m_name = m_nameCtrl->GetValue();
    m_name.Trim( true ).Trim( false );
    m_message = m_messageCtrl->GetValue();

    if( m_name.IsEmpty() )
    {
        DisplayInfoMessage( this, _( "Enter a name for the tag." ) );
        m_nameCtrl->SetFocus();
        return false;
    }

    return true;
}
