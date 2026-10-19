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

#include <memory>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/git_backend.h>
#include <git/git_config_handler.h>
#include <git/kicad_git_common.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>


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
    if( m_identityEdited || !m_dialog->SessionStillCurrent() )
        return;

    auto userConfig = std::make_shared<GitUserConfig>();

    // The fallback is application state, so copy it before the worker reads the repository.
    userConfig->authorName = Pgm().GetCommonSettings()->m_Git.authorName;
    userConfig->authorEmail = Pgm().GetCommonSettings()->m_Git.authorEmail;

    m_configQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [userConfig]( KIGIT_COMMON& aCommon )
            {
                GIT_CONFIG_HANDLER configHandler( &aCommon );
                wxString           value;

                if( GetGitBackend()->GetConfigString( &configHandler, "user.name", value ) )
                    userConfig->authorName = value;

                if( GetGitBackend()->GetConfigString( &configHandler, "user.email", value ) )
                    userConfig->authorEmail = value;
            },
            [this, userConfig]()
            {
                // The user may have begun editing after the query was queued.
                if( !m_dialog->SessionStillCurrent() || m_identityEdited )
                    return;

                m_repoUserName->ChangeValue( userConfig->authorName );
                m_repoUserEmail->ChangeValue( userConfig->authorEmail );
            } );
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

    m_configQuery.Cancel();

    m_dialog->RunGitAction(
            [this, name, email]( KIGIT_COMMON& aCommon )
            {
                if( !aCommon.SetUserConfig( name, email ) )
                    return false;

                // Follow the repository again only after saving succeeds. A busy or failed
                // save must leave the edited identity protected from a later refresh.
                m_identityEdited = false;
                return true;
            },
            _( "Could not save the git configuration." ) );
}
