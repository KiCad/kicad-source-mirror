/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <git2.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/log.h>


PANEL_GIT_REPOS::PANEL_GIT_REPOS( wxWindow* aParent ) : PANEL_GIT_REPOS_BASE( aParent)
{

    m_btnAddRepo->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_btnEditRepo->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_btnDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

}

PANEL_GIT_REPOS::~PANEL_GIT_REPOS()
{
}


void PANEL_GIT_REPOS::ResetPanel()
{
    m_grid->ClearGrid();
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
        printf( "Failed to open default Git config: %s\n", giterr_last()->message );
        return std::make_pair( name, email );
    }

    if( git_config_get_entry( &name_c, config, "user.name" ) != 0 )
    {
        printf( "Failed to get user.name from Git config: %s\n", giterr_last()->message );
    }
    if( git_config_get_entry( &email_c, config, "user.email" ) != 0 )
    {
        printf( "Failed to get user.email from Git config: %s\n", giterr_last()->message );
    }

    if( name_c )
        name = name_c->value;

    if( email_c )
        email = email_c->value;

    git_config_entry_free( name_c );
    git_config_entry_free( email_c );
    git_config_free( config );

    return std::make_pair( name, email );
}

bool PANEL_GIT_REPOS::TransferDataFromWindow()
{
    COMMON_SETTINGS*                              settings = Pgm().GetCommonSettings();
    std::vector<COMMON_SETTINGS::GIT_REPOSITORY>& repos = settings->m_Git.repositories;

    repos.clear();

    for( int row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        COMMON_SETTINGS::GIT_REPOSITORY repo;

        repo.active = m_grid->GetCellValue( row, COL_ACTIVE ) == "1";
        repo.name = m_grid->GetCellValue( row, COL_NAME );
        repo.path = m_grid->GetCellValue( row, COL_PATH );
        repo.authType = m_grid->GetCellValue( row, COL_AUTH_TYPE );
        repo.username = m_grid->GetCellValue( row, COL_USERNAME );

        KIPLATFORM::SECRETS::StoreSecret( repo.path, repo.username,
                                          m_grid->GetCellValue( row, COL_PASSWORD ) );
        repo.ssh_path = m_grid->GetCellValue( row, COL_SSH_PATH );
        repo.checkValid = m_grid->GetCellValue( row, COL_STATUS ) == "1";
        repos.push_back( repo );
    }

    settings->m_Git.useDefaultAuthor = m_cbDefault->GetValue();
    settings->m_Git.authorName = m_author->GetValue();
    settings->m_Git.authorEmail = m_authorEmail->GetValue();

    return true;
}

static bool testRepositoryConnection( COMMON_SETTINGS::GIT_REPOSITORY& repository)
{
    git_libgit2_init();

    git_remote_callbacks callbacks;
    callbacks.version = GIT_REMOTE_CALLBACKS_VERSION;

    typedef struct
    {
        COMMON_SETTINGS::GIT_REPOSITORY* repo;
        bool success;
    } callbacksPayload;

    callbacksPayload cb_data( { &repository, true } );  // If we don't need authentication, then,
                                                        // we are successful
    callbacks.payload = &cb_data;
    callbacks.credentials =
            [](git_cred** out, const char* url, const char* username, unsigned int allowed_types,
                void* payload) -> int
            {

                // If we are asking for credentials, then, we need authentication

                callbacksPayload* data = static_cast<callbacksPayload*>(payload);

                data->success = false;

                if( allowed_types & GIT_CREDTYPE_USERNAME )
                {
                    data->success = true;
                }
                else if( data->repo->authType == "ssh" && ( allowed_types & GIT_CREDTYPE_SSH_KEY ) )
                {
                    wxString sshKeyPath = data->repo->ssh_path;

                    // Check if the SSH key exists and is readable
                    if( wxFileExists( sshKeyPath ) && wxFile::Access( sshKeyPath, wxFile::read ) )
                        data->success = true;
                }
                else if( data->repo->authType == "password" )
                {
                    data->success = ( allowed_types & GIT_CREDTYPE_USERPASS_PLAINTEXT );
                }

                return 0;
            };

    // Create a temporary directory to initialize the Git repository
    wxString tempDirPath = wxFileName::CreateTempFileName(wxT("kigit_temp"));

    if( !wxFileName::Mkdir( tempDirPath, wxS_DIR_DEFAULT ) )
    {
        git_libgit2_shutdown();
        wxLogError( "Failed to create temporary directory for Git repository (%s): %s", tempDirPath,
                    wxSysErrorMsg() );
        return false;
    }

    // Initialize the Git repository
    git_repository* repo = nullptr;
    int result = git_repository_init( &repo, tempDirPath.mb_str( wxConvUTF8 ), 0 );

    if (result != 0)
    {
        git_repository_free(repo);
        git_libgit2_shutdown();
        wxRmdir(tempDirPath);
        return false;
    }

    git_remote* remote = nullptr;
    result = git_remote_create_anonymous( &remote, repo, tempDirPath.mb_str( wxConvUTF8 ) );

    if (result != 0)
    {
        git_remote_free(remote);
        git_repository_free(repo);
        git_libgit2_shutdown();
        wxRmdir(tempDirPath);
        return false;
    }

    // We don't really care about the result of this call, the authentication callback
    // will set the return values we need
    git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr);

    git_remote_disconnect(remote);
    git_remote_free(remote);
    git_repository_free(repo);

    git_libgit2_shutdown();

    // Clean up the temporary directory
    wxRmdir(tempDirPath);

    return cb_data.success;
}

bool PANEL_GIT_REPOS::TransferDataToWindow()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    m_grid->ClearGrid();

    for( COMMON_SETTINGS::GIT_REPOSITORY& repo : settings->m_Git.repositories )
    {
        if( repo.name.IsEmpty() || repo.path.IsEmpty() )
            continue;

        int row = m_grid->GetNumberRows();
        m_grid->AppendRows( 1 );

        m_grid->SetCellRenderer( row, COL_ACTIVE, new wxGridCellBoolRenderer() );
        m_grid->SetCellEditor( row, COL_ACTIVE, new wxGridCellBoolEditor() );
        m_grid->SetCellValue( row, COL_ACTIVE, repo.active ? "1" : "0" );

        m_grid->SetCellValue( row, COL_NAME, repo.name );
        m_grid->SetCellValue( row, COL_PATH, repo.path );
        m_grid->SetCellValue( row, COL_AUTH_TYPE, repo.authType );
        m_grid->SetCellValue( row, COL_USERNAME, repo.username );

        wxString password;
        KIPLATFORM::SECRETS::GetSecret( repo.path, repo.username, password );
        m_grid->SetCellValue( row, COL_PASSWORD, password );
        m_grid->SetCellValue( row, COL_SSH_PATH, repo.ssh_path );

        if( repo.active )
            m_grid->SetCellValue( row, 3, testRepositoryConnection( repo ) ? "C" : "NC" );

    }

    m_cbDefault->SetValue( settings->m_Git.useDefaultAuthor );

    if( settings->m_Git.useDefaultAuthor )
    {
        std::pair<wxString, wxString> defaultAuthor = getDefaultAuthorAndEmail();
        m_author->SetValue( defaultAuthor.first );
        m_authorEmail->SetValue( defaultAuthor.second );
        m_author->Disable();
        m_authorEmail->Disable();
    }
    else
    {
        m_author->SetValue( settings->m_Git.authorName );
        m_authorEmail->SetValue( settings->m_Git.authorEmail );
    }

    return true;
}

void PANEL_GIT_REPOS::onDefaultClick( wxCommandEvent& event )
{
    m_author->Enable( !m_cbDefault->GetValue() );
    m_authorEmail->Enable( !m_cbDefault->GetValue() );
    m_authorLabel->Enable( !m_cbDefault->GetValue() );
    m_authorEmailLabel->Enable( !m_cbDefault->GetValue() );
}


void PANEL_GIT_REPOS::onGridDClick( wxGridEvent& event )
{
    if( m_grid->GetNumberRows() <= 0 )
    {
        wxCommandEvent evt;
        onAddClick( evt );
        return;
    }

    int row = event.GetRow();

    if( row < 0 || row >= m_grid->GetNumberRows() )
        return;

    DIALOG_GIT_REPOSITORY dialog( this, nullptr );

    dialog.SetRepoName( m_grid->GetCellValue( row, COL_NAME ) );
    dialog.SetRepoURL( m_grid->GetCellValue( row, COL_PATH ) );
    dialog.SetUsername( m_grid->GetCellValue( row, COL_USERNAME ) );
    dialog.SetRepoSSHPath( m_grid->GetCellValue( row, COL_SSH_PATH ) );
    dialog.SetPassword( m_grid->GetCellValue( row, COL_PASSWORD ) );

    wxString type = m_grid->GetCellValue( row, COL_AUTH_TYPE );

    if( type == "password" )
        dialog.SetRepoType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS );
    else if( type == "ssh" )
        dialog.SetRepoType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );
    else
        dialog.SetRepoType( KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL);

    if( dialog.ShowModal() == wxID_OK )
    {
        m_grid->SetCellValue( row, COL_NAME, dialog.GetRepoName() );
        m_grid->SetCellValue( row, COL_PATH, dialog.GetRepoURL() );
        m_grid->SetCellValue( row, COL_USERNAME, dialog.GetUsername() );
        m_grid->SetCellValue( row, COL_SSH_PATH, dialog.GetRepoSSHPath() );
        m_grid->SetCellValue( row, COL_PASSWORD, dialog.GetPassword() );

        if( dialog.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "password" );
        }
        else if( dialog.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "ssh" );
        }
        else
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "none" );
        }
    }

}


void PANEL_GIT_REPOS::onAddClick( wxCommandEvent& event )
{

    DIALOG_GIT_REPOSITORY dialog( m_parent, nullptr );

    if( dialog.ShowModal() == wxID_OK )
    {
        int row = m_grid->GetNumberRows();
        m_grid->AppendRows( 1 );

        m_grid->SetCellValue( row, COL_NAME, dialog.GetRepoName() );
        m_grid->SetCellValue( row, COL_PATH, dialog.GetRepoURL() );
        m_grid->SetCellValue( row, COL_USERNAME, dialog.GetUsername() );
        m_grid->SetCellValue( row, COL_SSH_PATH, dialog.GetRepoSSHPath() );
        m_grid->SetCellValue( row, COL_PASSWORD, dialog.GetPassword() );

        if( dialog.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "password" );
        }
        else if( dialog.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "ssh" );
        }
        else
        {
            m_grid->SetCellValue( row, COL_AUTH_TYPE, "none" );
        }

        m_grid->MakeCellVisible( row, 0 );
    }
}


void PANEL_GIT_REPOS::onEditClick( wxCommandEvent& event )
{
    wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_LEFT_DCLICK, m_grid,
                     m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    onGridDClick( evt );
}


void PANEL_GIT_REPOS::onDeleteClick( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() || m_grid->GetNumberRows() <= 0 )
        return;

    int curRow   = m_grid->GetGridCursorRow();

    m_grid->DeleteRows( curRow );

    curRow = std::max( 0, curRow - 1 );
    m_grid->MakeCellVisible( curRow, m_grid->GetGridCursorCol() );
    m_grid->SetGridCursor( curRow, m_grid->GetGridCursorCol() );
}
