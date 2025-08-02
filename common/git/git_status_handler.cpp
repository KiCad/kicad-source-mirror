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

#include "git_status_handler.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <trace_helpers.h>
#include <wx/log.h>

GIT_STATUS_HANDLER::GIT_STATUS_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}

GIT_STATUS_HANDLER::~GIT_STATUS_HANDLER()
{}

bool GIT_STATUS_HANDLER::HasChangedFiles()
{
    git_repository* repo = GetRepo();

    if( !repo )
        return false;

    git_status_options opts;
    git_status_init_options( &opts, GIT_STATUS_OPTIONS_VERSION );

    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
                 | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, repo, &opts ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get status list: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitStatusListPtr status_list_ptr( status_list );
    bool hasChanges = ( git_status_list_entrycount( status_list ) > 0 );

    return hasChanges;
}

std::map<wxString, FileStatus> GIT_STATUS_HANDLER::GetFileStatus( const wxString& aPathspec )
{
    std::map<wxString, FileStatus> fileStatusMap;
    git_repository* repo = GetRepo();

    if( !repo )
        return fileStatusMap;

    git_status_options status_options;
    git_status_init_options( &status_options, GIT_STATUS_OPTIONS_VERSION );
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;

    // Set up pathspec if provided
    std::string pathspec_str;
    std::vector<const char*> pathspec_ptrs;

    if( !aPathspec.IsEmpty() )
    {
        pathspec_str = aPathspec.ToStdString();
        pathspec_ptrs.push_back( pathspec_str.c_str() );

        status_options.pathspec.strings = const_cast<char**>( pathspec_ptrs.data() );
        status_options.pathspec.count = pathspec_ptrs.size();
    }

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, repo, &status_options ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get git status list: %s", KIGIT_COMMON::GetLastGitError() );
        return fileStatusMap;
    }

    KIGIT::GitStatusListPtr statusListPtr( status_list );

    size_t count = git_status_list_entrycount( status_list );
    wxString repoWorkDir( git_repository_workdir( repo ) );

    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( status_list, ii );
        std::string path( entry->head_to_index ? entry->head_to_index->old_file.path
                        : entry->index_to_workdir->old_file.path );

        wxString absPath = repoWorkDir + path;

        FileStatus fileStatus;
        fileStatus.filePath = absPath;
        fileStatus.gitStatus = entry->status;
        fileStatus.status = ConvertStatus( entry->status );

        fileStatusMap[absPath] = fileStatus;
    }

    return fileStatusMap;
}

wxString GIT_STATUS_HANDLER::GetCurrentBranchName()
{
    git_repository* repo = GetRepo();

    if( !repo )
        return wxEmptyString;

    git_reference* currentBranchReference = nullptr;
    int rc = git_repository_head( &currentBranchReference, repo );
    KIGIT::GitReferencePtr currentBranchReferencePtr( currentBranchReference );

    if( currentBranchReference )
    {
        return git_reference_shorthand( currentBranchReference );
    }
    else if( rc == GIT_EUNBORNBRANCH )
    {
        // Unborn branch - could return empty or a default name
        return wxEmptyString;
    }
    else
    {
        wxLogTrace( traceGit, "Failed to lookup current branch: %s", KIGIT_COMMON::GetLastGitError() );
        return wxEmptyString;
    }
}

void GIT_STATUS_HANDLER::UpdateRemoteStatus( const std::set<wxString>& aLocalChanges,
                                              const std::set<wxString>& aRemoteChanges,
                                              std::map<wxString, FileStatus>& aFileStatus )
{
    git_repository* repo = GetRepo();

    if( !repo )
        return;

    wxString repoWorkDir( git_repository_workdir( repo ) );

    // Update status based on local/remote changes
    for( auto& [absPath, fileStatus] : aFileStatus )
    {
        // Convert absolute path to relative path for comparison
        wxString relativePath = absPath;
        if( relativePath.StartsWith( repoWorkDir ) )
        {
            relativePath = relativePath.Mid( repoWorkDir.length() );
#ifdef _WIN32
            relativePath.Replace( wxS( "\\" ), wxS( "/" ) );
#endif
        }

        std::string relativePathStd = relativePath.ToStdString();

        // Only update if the file is not already modified/added/deleted
        if( fileStatus.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT )
        {
            if( aLocalChanges.count( relativePathStd ) )
            {
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_AHEAD;
            }
            else if( aRemoteChanges.count( relativePathStd ) )
            {
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_BEHIND;
            }
        }
    }
}

wxString GIT_STATUS_HANDLER::GetWorkingDirectory()
{
    git_repository* repo = GetRepo();

    if( !repo )
        return wxEmptyString;

    const char* workdir = git_repository_workdir( repo );

    if( !workdir )
        return wxEmptyString;

    return wxString( workdir );
}

KIGIT_COMMON::GIT_STATUS GIT_STATUS_HANDLER::ConvertStatus( unsigned int aGitStatus )
{
    if( aGitStatus & GIT_STATUS_IGNORED )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_IGNORED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED;
    }
    else if( aGitStatus & ( GIT_STATUS_CONFLICTED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CONFLICTED;
    }
    else
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT;
    }
}

void GIT_STATUS_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}