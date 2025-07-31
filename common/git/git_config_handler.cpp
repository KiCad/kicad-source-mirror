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

#include "git_config_handler.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <trace_helpers.h>
#include <wx/log.h>

GIT_CONFIG_HANDLER::GIT_CONFIG_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}

GIT_CONFIG_HANDLER::~GIT_CONFIG_HANDLER()
{}

GitUserConfig GIT_CONFIG_HANDLER::GetUserConfig()
{
    GitUserConfig userConfig;

    // Try to get from git config first
    userConfig.hasName = GetConfigString( "user.name", userConfig.authorName );
    userConfig.hasEmail = GetConfigString( "user.email", userConfig.authorEmail );

    // Fall back to common settings if not found in git config
    if( !userConfig.hasName )
    {
        userConfig.authorName = Pgm().GetCommonSettings()->m_Git.authorName;
    }

    if( !userConfig.hasEmail )
    {
        userConfig.authorEmail = Pgm().GetCommonSettings()->m_Git.authorEmail;
    }

    return userConfig;
}

wxString GIT_CONFIG_HANDLER::GetWorkingDirectory()
{
    git_repository* repo = GetRepo();

    if( !repo )
        return wxEmptyString;

    const char* workdir = git_repository_workdir( repo );

    if( !workdir )
        return wxEmptyString;

    return wxString( workdir );
}

bool GIT_CONFIG_HANDLER::GetConfigString( const wxString& aKey, wxString& aValue )
{
    git_repository* repo = GetRepo();

    if( !repo )
        return false;

    git_config* config = nullptr;

    if( git_repository_config( &config, repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get repository config: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitConfigPtr configPtr( config );

    git_config_entry* entry = nullptr;
    int result = git_config_get_entry( &entry, config, aKey.mb_str() );
    KIGIT::GitConfigEntryPtr entryPtr( entry );

    if( result != GIT_OK || entry == nullptr )
    {
        wxLogTrace( traceGit, "Config key '%s' not found", aKey );
        return false;
    }

    aValue = wxString( entry->value );
    return true;
}

void GIT_CONFIG_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}