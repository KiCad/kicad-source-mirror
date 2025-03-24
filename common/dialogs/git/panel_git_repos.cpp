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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "panel_git_repos.h"

#include <bitmaps.h>
#include <dialogs/git/dialog_git_repository.h>
#include <kiplatform/secrets.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <trace_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <git2.h>
#include <git/kicad_git_memory.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/log.h>


PANEL_GIT_REPOS::PANEL_GIT_REPOS( wxWindow* aParent ) : PANEL_GIT_REPOS_BASE( aParent)
{
}

PANEL_GIT_REPOS::~PANEL_GIT_REPOS()
{
}


void PANEL_GIT_REPOS::ResetPanel()
{
    m_cbDefault->SetValue( true );
    m_author->SetValue( wxEmptyString );
    m_authorEmail->SetValue( wxEmptyString );
}

static std::pair<wxString, wxString> getDefaultAuthorAndEmail()
{
    wxString name;
    wxString email;
    git_config_entry* name_c = nullptr;
    git_config_entry* email_c = nullptr;

    git_config* config = nullptr;

    if( git_config_open_default( &config ) != 0 )
    {
        wxLogTrace( traceGit, "Failed to open default Git config: %s", KIGIT_COMMON::GetLastGitError() );
        return std::make_pair( name, email );
    }

    KIGIT::GitConfigPtr configPtr( config );

    if( git_config_get_entry( &name_c, config, "user.name" ) != 0 )
    {
        wxLogTrace( traceGit, "Failed to get user.name from Git config: %s", KIGIT_COMMON::GetLastGitError() );
        return std::make_pair( name, email );
    }

    KIGIT::GitConfigEntryPtr namePtr( name_c );

    if( git_config_get_entry( &email_c, config, "user.email" ) != 0 )
    {
        wxLogTrace( traceGit, "Failed to get user.email from Git config: %s", KIGIT_COMMON::GetLastGitError() );
        return std::make_pair( name, email );
    }

    KIGIT::GitConfigEntryPtr emailPtr( email_c );

    if( name_c )
        name = name_c->value;

    if( email_c )
        email = email_c->value;

    return std::make_pair( name, email );
}


bool PANEL_GIT_REPOS::TransferDataFromWindow()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    settings->m_Git.enableGit = m_enableGit->GetValue();
    settings->m_Git.updatInterval = m_updateInterval->GetValue();
    settings->m_Git.useDefaultAuthor = m_cbDefault->GetValue();
    settings->m_Git.authorName = m_author->GetValue();
    settings->m_Git.authorEmail = m_authorEmail->GetValue();

    return true;
}


bool PANEL_GIT_REPOS::TransferDataToWindow()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();
    std::pair<wxString, wxString> defaultAuthor = getDefaultAuthorAndEmail();

    m_enableGit->SetValue( settings->m_Git.enableGit );
    m_updateInterval->SetValue( settings->m_Git.updatInterval );

    m_cbDefault->SetValue( settings->m_Git.useDefaultAuthor );

    if( settings->m_Git.useDefaultAuthor )
    {
        m_author->SetValue( defaultAuthor.first );
        m_authorEmail->SetValue( defaultAuthor.second );
    }
    else
    {
        if( settings->m_Git.authorName.IsEmpty() )
            m_author->SetValue( defaultAuthor.first );
        else
            m_author->SetValue( settings->m_Git.authorName );

        if( settings->m_Git.authorEmail.IsEmpty() )
            m_authorEmail->SetValue( defaultAuthor.second );
        else
            m_authorEmail->SetValue( settings->m_Git.authorEmail );
    }

    wxCommandEvent event;
    onDefaultClick( event );
    onEnableGitClick( event );
    return true;
}

void PANEL_GIT_REPOS::onDefaultClick( wxCommandEvent& event )
{
    m_author->Enable( !m_cbDefault->GetValue() );
    m_authorEmail->Enable( !m_cbDefault->GetValue() );
    m_authorLabel->Enable( !m_cbDefault->GetValue() );
    m_authorEmailLabel->Enable( !m_cbDefault->GetValue() );
}

void PANEL_GIT_REPOS::onEnableGitClick( wxCommandEvent& aEvent )
{
    bool enable = m_enableGit->GetValue();
    m_updateInterval->Enable( enable );
    m_cbDefault->Enable( enable );
    m_author->Enable( enable && !m_cbDefault->GetValue() );
    m_authorEmail->Enable( enable && !m_cbDefault->GetValue() );
    m_authorLabel->Enable( enable && !m_cbDefault->GetValue() );
    m_authorEmailLabel->Enable( enable && !m_cbDefault->GetValue() );
}
