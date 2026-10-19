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

#include <dialogs/git/panel_git_repository.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/git_config_handler.h>
#include <git/kicad_git_common.h>
#include <project.h>


PANEL_GIT_REPOSITORY::PANEL_GIT_REPOSITORY( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_REPOSITORY_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_saveButton->Bind( wxEVT_BUTTON,
                        [this]( wxCommandEvent& )
                        {
                            onSave();
                        } );

    // Populate uses ChangeValue, which sends no event, so only the user reaches this.
    auto edited = [this]( wxCommandEvent& )
    {
        m_identityEdited = true;
    };

    m_repoUserName->Bind( wxEVT_TEXT, edited );
    m_repoUserEmail->Bind( wxEVT_TEXT, edited );
}


void PANEL_GIT_REPOSITORY::Populate()
{
    // Anything at all signals a refresh, a finished fetch included, and the identity is not
    // what changed. Replacing half typed text would lose it.
    if( m_identityEdited )
        return;

    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    GIT_CONFIG_HANDLER configHandler( common.get() );
    GitUserConfig      userConfig = configHandler.GetUserConfig();

    m_repoUserName->ChangeValue( userConfig.authorName );
    m_repoUserEmail->ChangeValue( userConfig.authorEmail );
}


void PANEL_GIT_REPOSITORY::onSave()
{
    wxString name = m_repoUserName->GetValue().Trim().Trim( false );
    wxString email = m_repoUserEmail->GetValue().Trim().Trim( false );

    wxString reason;

    if( !KIGIT::ValidateIdentity( name, email, reason ) )
    {
        DisplayErrorMessage( this, reason );
        return;
    }

    // The save is what the fields were being held for, so the page follows the repository
    // again from here.
    m_identityEdited = false;

    m_dialog->RunGitAction(
            [name, email]( KIGIT_COMMON& aCommon )
            {
                return aCommon.SetUserConfig( name, email );
            },
            _( "Could not save the git configuration." ) );
}
