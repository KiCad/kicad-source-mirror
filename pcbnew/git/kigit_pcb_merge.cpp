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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kigit_pcb_merge.h"
#include <git/kigit_merge_blob_utils.h>

#include <diff_merge/kicad_merge_engine.h>
#include <diff_merge/pcb_differ.h>
#include <diff_merge/pcb_merge_applier.h>

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
    // The structural 3-way merge needs all three sides. A null ancestor entry
    // means one side added a file that doesn't exist on the other; without a
    // common ancestor there is nothing to diff against, so decline and let
    // libgit2 fall back to its default handling. LoadMergeBlobs tolerates a
    // missing ancestor, so this guard is kept explicit to preserve behavior.
    if( !git_merge_driver_source_ancestor( m_mergeDriver ) )
    {
        wxLogTrace( traceGit, "PCB merge driver: no common ancestor, declining merge" );
        return GIT_PASSTHROUGH;
    }

    KIGIT::MERGE_BLOBS blobs;

    if( int rc = KIGIT::LoadMergeBlobs( m_mergeDriver, blobs ); rc != 0 )
    {
        wxLogTrace( traceGit, "PCB merge driver: could not load merge blobs (rc=%d)", rc );
        return rc;
    }

    // Parse each side from its decoded blob contents. LoadMergeBlobs has freed
    // the underlying git_blob objects, so the boards are built from the strings.
    STRING_LINE_READER         ancestor_reader( blobs.ancestor, wxS( "ancestor" ) );
    PCB_IO_KICAD_SEXPR_PARSER  ancestor_parser( &ancestor_reader, nullptr, nullptr );
    STRING_LINE_READER         ours_reader( blobs.ours, wxS( "ours" ) );
    PCB_IO_KICAD_SEXPR_PARSER  ours_parser( &ours_reader, nullptr, nullptr );
    STRING_LINE_READER         theirs_reader( blobs.theirs, wxS( "theirs" ) );
    PCB_IO_KICAD_SEXPR_PARSER  theirs_parser( &theirs_reader, nullptr, nullptr );

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

    // Keep the legacy difference sets populated for any external consumer
    // (none today, but the accessors are public API). The full 3-way merge
    // logic runs through the new diff/merge engine.
    KIGIT_PCB_MERGE_DIFFERENCES ancestor_ours_differences =
            compareBoards( ancestor_board.get(), ours_board.get() );
    KIGIT_PCB_MERGE_DIFFERENCES ancestor_theirs_differences =
            compareBoards( ancestor_board.get(), theirs_board.get() );

    std::set_intersection( ancestor_ours_differences.m_changed.begin(),
                           ancestor_ours_differences.m_changed.end(),
                           ancestor_theirs_differences.m_removed.begin(),
                           ancestor_theirs_differences.m_removed.end(),
                           std::inserter( we_modified_they_deleted, we_modified_they_deleted.begin() ) );
    std::set_intersection( ancestor_theirs_differences.m_changed.begin(),
                           ancestor_theirs_differences.m_changed.end(),
                           ancestor_ours_differences.m_removed.begin(),
                           ancestor_ours_differences.m_removed.end(),
                           std::inserter( they_modified_we_deleted, they_modified_we_deleted.begin() ) );
    std::set_intersection( ancestor_ours_differences.m_changed.begin(),
                           ancestor_ours_differences.m_changed.end(),
                           ancestor_theirs_differences.m_changed.begin(),
                           ancestor_theirs_differences.m_changed.end(),
                           std::inserter( both_modified, both_modified.begin() ) );

    // Run the structured diff/merge pipeline so we can actually emit a
    // merged blob into git's buffer. Project pointers from the parser
    // belong to whichever PROJECT the loader supplied — for blob loads
    // there is no project, so this is safe.
    KICAD_DIFF::PCB_DIFFER ourDiffer( ancestor_board.get(), ours_board.get() );
    KICAD_DIFF::PCB_DIFFER theirDiffer( ancestor_board.get(), theirs_board.get() );

    KICAD_DIFF::KICAD_MERGE_ENGINE engine;
    KICAD_DIFF::MERGE_PLAN         plan = engine.Plan( ourDiffer.Diff(), theirDiffer.Diff() );

    // Captured before the plan is moved into the applier; drives the
    // conflict-or-clean return below without a second Plan() recompute.
    const bool planResolved = plan.Resolved();

    KICAD_DIFF::PCB_MERGE_APPLIER applier( ancestor_board.get(), ours_board.get(),
                                           theirs_board.get(), std::move( plan ) );
    std::unique_ptr<BOARD>        merged = applier.Apply();

    if( !merged )
    {
        wxLogTrace( traceGit, "PCB merge applier failed to produce a board" );
        return GIT_EUSER;
    }

    // Serialize the merged board into a std::string via PCB_IO_KICAD_SEXPR.
    std::string serialized;

    try
    {
        STRING_FORMATTER formatter;
        PCB_IO_KICAD_SEXPR pcbIO;
        pcbIO.SetOutputFormatter( &formatter );
        pcbIO.Format( merged.get() );
        serialized = formatter.GetString();
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogTrace( traceGit, "Failed to serialize merged board: %s", ioe.What() );
        return GIT_EUSER;
    }

    if( KIGIT::WriteToGitBuf( m_result, serialized ) != 0 )
    {
        wxLogTrace( traceGit, "Failed to allocate git_buf for merged board" );
        return GIT_EUSER;
    }

    // The merged board is fully serialized above, so the working-tree file is
    // always valid. If conflicts remained unresolved, tell libgit2 to record a
    // conflict; the user resolves it via the mergetool, which recomputes the
    // plan from ancestor/ours/theirs.
    if( !planResolved )
        return GIT_EMERGECONFLICT;

    return 0;
}


/**
 * Trampoline used by the libgit2 merge-driver registry. Constructs a
 * KIGIT_PCB_MERGE, runs Merge(), and forwards the path/mode out of the
 * libgit2 source struct.
 */
int KIGIT_PCB_MERGE::Apply( const git_merge_driver_source* aSrc, const char** aPathOut,
                            unsigned int* aModeOut, git_buf* aMergedOut )
{
    return KIGIT::ApplyMergeDriver<KIGIT_PCB_MERGE>( aSrc, aPathOut, aModeOut, aMergedOut );
}
