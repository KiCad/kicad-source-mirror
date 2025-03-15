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

#include "kigit_pcb_merge.h"
#include <git/kicad_git_blob_reader.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <board.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <richio.h>
#include <trace_helpers.h>

#include <wx/log.h>

void KIGIT_PCB_MERGE::findSetDifferences( const BOARD_ITEM_SET& aAncestorSet, const BOARD_ITEM_SET& aOtherSet,
                                          std::vector<BOARD_ITEM*>& aAdded, std::vector<BOARD_ITEM*>& aRemoved,
                                          std::vector<BOARD_ITEM*>& aChanged )
{
    auto it1 = aAncestorSet.begin();
    auto it2 = aOtherSet.begin();

    while( it1 != aAncestorSet.end() && it2 != aOtherSet.end() )
    {
        BOARD_ITEM* item1 = *it1;
        BOARD_ITEM* item2 = *it2;

        if( item1->m_Uuid < item2->m_Uuid )
        {
            aRemoved.push_back( item1 );
            ++it1;
        }
        else if( item1->m_Uuid > item2->m_Uuid )
        {
            aAdded.push_back( item2 );
            ++it2;
        }
        else
        {
            if( !( *item1 == *item2 ) )
                aChanged.push_back( item1 );

            ++it1;
            ++it2;
        }
    }
}


KIGIT_PCB_MERGE_DIFFERENCES KIGIT_PCB_MERGE::compareBoards( BOARD* aAncestor, BOARD* aOther )
{
    KIGIT_PCB_MERGE_DIFFERENCES differences;

    const auto ancestor_set = aAncestor->GetItemSet();
    const auto other_set = aOther->GetItemSet();

    findSetDifferences( ancestor_set, other_set, differences.m_added, differences.m_removed, differences.m_changed );

    return differences;
}


int KIGIT_PCB_MERGE::Merge()
{
    auto repo = const_cast<git_repository*>( git_merge_driver_source_repo( m_mergeDriver ) );
    const git_index_entry* ancestor = git_merge_driver_source_ancestor( m_mergeDriver );
    const git_index_entry* ours = git_merge_driver_source_ours( m_mergeDriver );
    const git_index_entry* theirs = git_merge_driver_source_theirs( m_mergeDriver );

    // Read ancestor index entry into a buffer

    git_blob* ancestor_blob;
    git_blob* ours_blob;
    git_blob* theirs_blob;

    if( git_blob_lookup( &ancestor_blob, repo, &ancestor->id ) != 0 )
    {
        wxLogTrace( traceGit, "Could not find ancestor blob: %s", KIGIT_COMMON::GetLastGitError() );
        return GIT_ENOTFOUND;
    }

    KIGIT::GitBlobPtr ancestor_blob_ptr( ancestor_blob );

    if( git_blob_lookup( &ours_blob, repo, &ours->id ) != 0 )
    {
        wxLogTrace( traceGit, "Could not find ours blob: %s", KIGIT_COMMON::GetLastGitError() );
        return GIT_ENOTFOUND;
    }

    KIGIT::GitBlobPtr ours_blob_ptr( ours_blob );

    if( git_blob_lookup( &theirs_blob, repo, &theirs->id ) != 0 )
    {
        wxLogTrace( traceGit, "Could not find theirs blob: %s", KIGIT_COMMON::GetLastGitError() );
        return GIT_ENOTFOUND;
    }

    KIGIT::GitBlobPtr theirs_blob_ptr( theirs_blob );

    // Get the raw data from the blobs
    BLOB_READER ancestor_reader( ancestor_blob );
    PCB_IO_KICAD_SEXPR_PARSER ancestor_parser( &ancestor_reader, nullptr, nullptr );
    BLOB_READER ours_reader( ours_blob );
    PCB_IO_KICAD_SEXPR_PARSER ours_parser( &ours_reader, nullptr, nullptr );
    BLOB_READER theirs_reader( theirs_blob );
    PCB_IO_KICAD_SEXPR_PARSER theirs_parser( &theirs_reader, nullptr, nullptr );

    std::unique_ptr<BOARD> ancestor_board;
    std::unique_ptr<BOARD> ours_board;
    std::unique_ptr<BOARD> theirs_board;

    try
    {
        ancestor_board.reset( static_cast<BOARD*>( ancestor_parser.Parse() ) );
        ours_board.reset( static_cast<BOARD*>( ours_parser.Parse() ) );
        theirs_board.reset( static_cast<BOARD*>( theirs_parser.Parse() ) );
    }
    catch(const IO_ERROR& e)
    {
        wxLogTrace( traceGit, "Could not parse board: %s", e.What() );
        return GIT_EUSER;
    }
    catch( ... )
    {
        wxLogTrace( traceGit, "Could not parse board: unknown error" );
        return GIT_EUSER;
    }

    // Parse the differences between the ancestor and ours
    KIGIT_PCB_MERGE_DIFFERENCES ancestor_ours_differences = compareBoards( ancestor_board.get(), ours_board.get() );
    KIGIT_PCB_MERGE_DIFFERENCES ancestor_theirs_differences = compareBoards( ancestor_board.get(), theirs_board.get() );

    // Find the items that we modified and they deleted
    std::set_intersection( ancestor_ours_differences.m_changed.begin(), ancestor_ours_differences.m_changed.end(),
                           ancestor_theirs_differences.m_removed.begin(), ancestor_theirs_differences.m_removed.end(),
                           std::inserter( we_modified_they_deleted, we_modified_they_deleted.begin() ) );

    // Find the items that they modified and we deleted
    std::set_intersection( ancestor_theirs_differences.m_changed.begin(), ancestor_theirs_differences.m_changed.end(),
                           ancestor_ours_differences.m_removed.begin(), ancestor_ours_differences.m_removed.end(),
                           std::inserter( they_modified_we_deleted, they_modified_we_deleted.begin() ) );

    // Find the items that both we and they modified
    std::set_intersection( ancestor_ours_differences.m_changed.begin(), ancestor_ours_differences.m_changed.end(),
                           ancestor_theirs_differences.m_changed.begin(), ancestor_theirs_differences.m_changed.end(),
                           std::inserter( both_modified, both_modified.begin() ) );

    return 0;
}


std::unique_ptr<BOARD> readBoard( wxString& aFilename )
{
    // Take input from stdin
    FILE_LINE_READER reader( aFilename );

    PCB_IO_KICAD_SEXPR_PARSER             parser( &reader, nullptr, nullptr );
    std::unique_ptr<BOARD> board;

    try
    {
        board.reset( static_cast<BOARD*>( parser.Parse() ) );
    }
    catch( const IO_ERROR& )
    {
    }

    return board;
}
