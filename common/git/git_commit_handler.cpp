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

#include "git_commit_handler.h"
#include <git/kicad_git_memory.h>
#include <wx/log.h>

GIT_COMMIT_HANDLER::GIT_COMMIT_HANDLER( git_repository* aRepo ) :
    KIGIT_COMMON( aRepo )
{}


GIT_COMMIT_HANDLER::~GIT_COMMIT_HANDLER()
{}


GIT_COMMIT_HANDLER::CommitResult
GIT_COMMIT_HANDLER::PerformCommit( const std::vector<wxString>& aFiles,
                                   const wxString&               aMessage,
                                   const wxString&               aAuthorName,
                                   const wxString&               aAuthorEmail )
{
    git_repository* repo = GetRepo();

    if( !repo )
        return CommitResult::Error;

    git_index* index = nullptr;

    if( git_repository_index( &index, repo ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to get repository index: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    for( const wxString& file : aFiles )
    {
        if( git_index_add_bypath( index, file.mb_str() ) != 0 )
        {
            AddErrorString( wxString::Format( _( "Failed to add file to index: %s" ),
                                              KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }
    }

    if( git_index_write( index ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to write index: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    git_oid tree_id;

    if( git_index_write_tree( &tree_id, index ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to write tree: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    git_tree* tree = nullptr;

    if( git_tree_lookup( &tree, repo, &tree_id ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to lookup tree: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    KIGIT::GitTreePtr treePtr( tree );
    git_commit* parent = nullptr;

    if( git_repository_head_unborn( repo ) == 0 )
    {
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, repo ) != 0 )
        {
            AddErrorString( wxString::Format( _( "Failed to get HEAD reference: %s" ),
                                              KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }

        KIGIT::GitReferencePtr headRefPtr( headRef );

        if( git_reference_peel( (git_object**) &parent, headRef, GIT_OBJECT_COMMIT ) != 0 )
        {
            AddErrorString( wxString::Format( _( "Failed to get commit: %s" ),
                                              KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }
    }

    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* author = nullptr;

    if( git_signature_now( &author, aAuthorName.mb_str(), aAuthorEmail.mb_str() ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to create author signature: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    KIGIT::GitSignaturePtr authorPtr( author );
    git_oid                oid;

#if( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR == 8 \
    && ( LIBGIT2_VER_REVISION < 2 || LIBGIT2_VER_REVISION == 3 ) )
    git_commit* const parents[1] = { parent };
#else
    const git_commit* parents[1] = { parent };
#endif

    if( git_commit_create( &oid, repo, "HEAD", author, author, nullptr,
                           aMessage.mb_str(), tree, 1, parents ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Failed to create commit: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return CommitResult::Error;
    }

    return CommitResult::Success;
}


wxString GIT_COMMIT_HANDLER::GetErrorString() const
{
    return m_errorString;
}


void GIT_COMMIT_HANDLER::AddErrorString( const wxString& aErrorString )
{
    m_errorString += aErrorString;
}

