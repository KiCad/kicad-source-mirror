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

#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <wx/utils.h>


PANEL_GIT_REPOSITORY::PANEL_GIT_REPOSITORY( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_REPOSITORY_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_repoUserName->SetEditable( false );
    m_repoUserEmail->SetEditable( false );
    m_identityNote->SetLabel( _( "Edits apply to this repository only." ) );
    m_saveButton->SetLabel( _( "Edit Author..." ) );
    m_saveButton->Bind( wxEVT_BUTTON,
                        [this]( wxCommandEvent& )
                        {
                            onEditAuthor();
                        } );
}


void PANEL_GIT_REPOSITORY::Populate()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    auto identity = std::make_shared<KIGIT::GIT_IDENTITY>();

    m_configQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [identity]( KIGIT_COMMON& aCommon )
            {
                *identity = KIGIT::ReadGitIdentity( aCommon );
            },
            [this, identity]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                GitUserConfig config =
                        KIGIT::ResolveGitIdentity( *identity, Pgm().GetCommonSettings()->m_Git, wxGetUserName() );
                m_repoUserName->ChangeValue( config.authorName );
                m_repoUserEmail->ChangeValue( config.authorEmail );
            } );
}


void PANEL_GIT_REPOSITORY::onEditAuthor()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    auto                          identity = std::make_shared<KIGIT::GIT_IDENTITY>();
    m_saveButton->Disable();

    m_editQuery.Run(
            common,
            [identity]( KIGIT_COMMON& aCommon )
            {
                *identity = KIGIT::ReadGitIdentity( aCommon );
            },
            [this, common, identity]()
            {
                m_saveButton->Enable();

                if( m_dialog->SessionStillCurrent() )
                    KIGIT::EditGitIdentity( this, m_frame, common, *identity );
            } );
}
