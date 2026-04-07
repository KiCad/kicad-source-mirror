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

#include "project_git_utils.h"
#include "git_backend.h"

#include <wx/filename.h>
#include <wx/string.h>
#include <string_utils.h>

#ifndef __WINDOWS__
#include <climits>
#include <cstdlib>
#endif

namespace KIGIT
{

git_repository* PROJECT_GIT_UTILS::GetRepositoryForFile( const char* aFilename )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend )
        return nullptr;

    return backend->GetRepositoryForFile( aFilename );
}


int PROJECT_GIT_UTILS::CreateBranch( git_repository* aRepo, const wxString& aBranchName )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend )
        return -1;

    return backend->CreateBranch( aRepo, aBranchName );
}


bool PROJECT_GIT_UTILS::RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                                   bool aRemoveGitDir, wxString* aErrors )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend )
        return false;

    return backend->RemoveVCS( aRepo, aProjectPath, aRemoveGitDir, aErrors );
}


wxString PROJECT_GIT_UTILS::GetCurrentHash( const wxString& aProjectFile, bool aShort )
{
    wxString result = wxT( "no hash" );
    git_repository* repo = PROJECT_GIT_UTILS::GetRepositoryForFile( TO_UTF8( aProjectFile ) );

    if( repo )
    {
        git_reference* head = nullptr;

        if( git_repository_head( &head, repo ) == 0 )
        {
            const git_oid* oid = git_reference_target( head );

            if( oid )
            {
                char buf[GIT_OID_HEXSZ + 1];
                size_t len = aShort ? 9 : GIT_OID_HEXSZ + 1; // 8 chars + null terminator
                git_oid_tostr( buf, len, oid );
                result = wxString::FromUTF8( buf );
            }

            git_reference_free( head );
        }

        git_repository_free( repo );
    }

    return result;
}


wxString PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir( const wxString& aUserProjectPath,
                                                             const wxString& aCanonicalWorkDir )
{
#ifdef __WINDOWS__
    return aCanonicalWorkDir;
#else
    if( aUserProjectPath.IsEmpty() || aCanonicalWorkDir.IsEmpty() )
        return aCanonicalWorkDir;

    char resolvedPath[PATH_MAX];

    if( realpath( aUserProjectPath.mb_str(), resolvedPath ) == nullptr )
        return aCanonicalWorkDir;

    wxString canonicalUserPath = wxString::FromUTF8( resolvedPath );

    if( !canonicalUserPath.EndsWith( wxFileName::GetPathSeparator() ) )
        canonicalUserPath += wxFileName::GetPathSeparator();

    wxString canonicalWorkDirNorm = aCanonicalWorkDir;

    if( !canonicalWorkDirNorm.EndsWith( wxFileName::GetPathSeparator() ) )
        canonicalWorkDirNorm += wxFileName::GetPathSeparator();

    // The user project path resolves to the same directory as the git workdir (e.g., the
    // project is at the repo root but opened via a symlinked path). Return the user path
    // to preserve symlinks.  When the project is in a subdirectory of the workdir, the
    // paths will differ and we fall through to extract only the workdir portion below.
    if( canonicalUserPath == canonicalWorkDirNorm )
    {
        return aUserProjectPath.EndsWith( wxFileName::GetPathSeparator() )
                   ? aUserProjectPath
                   : aUserProjectPath + wxFileName::GetPathSeparator();
    }

    // Walk both paths upward in lockstep until the canonical user path matches the canonical
    // workdir. This correctly handles symlinks that compress multiple canonical path
    // components into fewer visible components (e.g. /work/repo -> /real/deep/path/root).
    wxFileName userFn;
    userFn.AssignDir( aUserProjectPath );

    wxFileName canonicalUserFn;
    canonicalUserFn.AssignDir( canonicalUserPath );

    wxFileName canonicalWorkDirFn;
    canonicalWorkDirFn.AssignDir( canonicalWorkDirNorm );

    while( canonicalUserFn.GetFullPath() != canonicalWorkDirFn.GetFullPath() )
    {
        if( canonicalUserFn.GetDirCount() == 0 || userFn.GetDirCount() == 0 )
            return aCanonicalWorkDir;

        canonicalUserFn.RemoveLastDir();
        userFn.RemoveLastDir();
    }

    return userFn.GetPathWithSep();
#endif
}

} // namespace KIGIT
