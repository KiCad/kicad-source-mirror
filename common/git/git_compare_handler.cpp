/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <git/git_compare_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <git2.h>

#include <trace_helpers.h>

#include <wx/log.h>


namespace KIGIT
{

const char* FileChangeStatusToString( FILE_CHANGE_STATUS aStatus )
{
    switch( aStatus )
    {
    case FILE_CHANGE_STATUS::UNCHANGED:  return "unchanged";
    case FILE_CHANGE_STATUS::ADDED:      return "added";
    case FILE_CHANGE_STATUS::REMOVED:    return "removed";
    case FILE_CHANGE_STATUS::MODIFIED:   return "modified";
    case FILE_CHANGE_STATUS::RENAMED:    return "renamed";
    case FILE_CHANGE_STATUS::COPIED:     return "copied";
    case FILE_CHANGE_STATUS::TYPECHANGE: return "typechange";
    }

    return "unknown";
}


namespace
{

FILE_CHANGE_STATUS deltaStatusToEnum( git_delta_t aDelta )
{
    switch( aDelta )
    {
    case GIT_DELTA_UNMODIFIED: return FILE_CHANGE_STATUS::UNCHANGED;
    case GIT_DELTA_ADDED:      return FILE_CHANGE_STATUS::ADDED;
    case GIT_DELTA_DELETED:    return FILE_CHANGE_STATUS::REMOVED;
    case GIT_DELTA_MODIFIED:   return FILE_CHANGE_STATUS::MODIFIED;
    case GIT_DELTA_RENAMED:    return FILE_CHANGE_STATUS::RENAMED;
    case GIT_DELTA_COPIED:     return FILE_CHANGE_STATUS::COPIED;
    case GIT_DELTA_TYPECHANGE: return FILE_CHANGE_STATUS::TYPECHANGE;
    default:                   return FILE_CHANGE_STATUS::MODIFIED;
    }
}

} // namespace


std::vector<CHANGED_FILE> CompareRefs( git_repository* aRepo,
                                       const wxString& aBaseRef,
                                       const wxString& aHeadRef )
{
    std::vector<CHANGED_FILE> result;

    if( !aRepo )
        return result;

    GitTreePtr baseTree( ResolveRefToTree( aRepo, aBaseRef ) );
    GitTreePtr headTree( ResolveRefToTree( aRepo, aHeadRef ) );

    if( !baseTree || !headTree )
        return result;

    git_diff*        diff = nullptr;
    git_diff_options opts;
    git_diff_options_init( &opts, GIT_DIFF_OPTIONS_VERSION );

    if( git_diff_tree_to_tree( &diff, aRepo, baseTree.get(), headTree.get(), &opts ) != 0 )
    {
        wxLogTrace( traceGit, "git_diff_tree_to_tree failed: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return result;
    }

    GitDiffPtr diffPtr( diff );

    // Find renames so the UI can show "renamed from X" instead of (REMOVED+ADDED).
    git_diff_find_options findOpts;
    git_diff_find_options_init( &findOpts, GIT_DIFF_FIND_OPTIONS_VERSION );
    findOpts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
    git_diff_find_similar( diff, &findOpts );

    result.reserve( git_diff_num_deltas( diff ) );

    CollectDiffDeltas( diff,
            [&result]( const git_diff_delta& aDelta )
            {
                CHANGED_FILE entry;
                entry.status = deltaStatusToEnum( aDelta.status );
                entry.path   = wxString::FromUTF8( aDelta.new_file.path
                                                           ? aDelta.new_file.path
                                                           : aDelta.old_file.path );

                if( aDelta.status == GIT_DELTA_RENAMED || aDelta.status == GIT_DELTA_COPIED )
                    entry.oldPath = wxString::FromUTF8( aDelta.old_file.path );

                result.push_back( std::move( entry ) );
            } );

    return result;
}

} // namespace KIGIT
