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

#include <dialogs/git/merge_conflict_prompt.h>

#include <cli/exit_codes.h>
#include <confirm.h>
#include <diff_merge/diff_doc_kind.h>
#include <diff_merge/merge_dispatch.h>
#include <reporter.h>

#include <set>

#include <wx/choicdlg.h>
#include <wx/datetime.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/translation.h>


MERGE_PROMPT_RESULT PromptMergeConflicts( wxWindow* aParent, KIWAY& aKiway,
                                          const std::vector<MERGE_CONFLICT>& aConflicts,
                                          const MERGE_CONFLICT_TEXT& aText, const wxString& aScratchDir,
                                          std::vector<MERGE_ANSWER>&      aAnswers,
                                          std::vector<MERGE_LOSING_COPY>& aLosingCopies,
                                          const std::function<bool()>&    aCanContinue )
{
    const wxString sep = wxFileName::GetPathSeparator();

    // Hand-merged files live in the scratch directory, which the caller removes
    int handMerged = 0;

    // Each file gets one try at a merge that needs no answers, before the user is asked
    std::set<size_t> autoTried;

    for( size_t ii = 0; ii < aConflicts.size(); )
    {
        if( aCanContinue && !aCanContinue() )
            return MERGE_PROMPT_RESULT::STOPPED;

        const MERGE_CONFLICT& conflict = aConflicts[ii];
        KICAD_DIFF::DOC_KIND  kind = KICAD_DIFF::DocKindFromExtension( conflict.path );

        // The merge tool needs all three sides, so deletions and independent adds are kept or taken whole
        // The footprint merge skips fields such as keywords and could drop an edit unseen
        const bool mergeable = kind != KICAD_DIFF::DOC_KIND::UNKNOWN && kind != KICAD_DIFF::DOC_KIND::FOOTPRINT
                               && conflict.hasAncestor && conflict.hasOurs && conflict.hasTheirs
                               && !conflict.ancestorFile.IsEmpty() && !conflict.oursFile.IsEmpty()
                               && !conflict.theirsFile.IsEmpty();

        // Edits to different symbols of one library collide line by line, and the engine succeeds only when
        // it keeps both sides
        if( mergeable && kind == KICAD_DIFF::DOC_KIND::SYM_LIB && autoTried.insert( ii ).second )
        {
            wxString mergedOut = aScratchDir + sep + wxString::Format( wxS( "a%zu_" ), ii )
                                 + wxFileName( conflict.path ).GetFullName();

            if( KICAD_DIFF::DispatchMerge( aKiway, kind, conflict.ancestorFile, conflict.oursFile, conflict.theirsFile,
                                           mergedOut, false, false, nullptr )
                == CLI::EXIT_CODES::SUCCESS )
            {
                aAnswers.push_back( { MERGE_ANSWER::KEPT::MERGED, mergedOut } );
                ++ii;
                continue;
            }
        }

        wxArrayString choices;

        if( mergeable )
            choices.Add( _( "Open the merge tool" ) );

        // A side with no base was added, not deleted, and a rename arrives as two such entries
        choices.Add( conflict.hasOurs       ? _( "Keep my version" )
                     : conflict.hasAncestor ? _( "Keep it deleted" )
                                            : _( "Leave it out" ) );

        // A side that could not be written out can only be kept as it is here
        if( !conflict.hasTheirs || !conflict.theirsFile.IsEmpty() )
        {
            choices.Add( conflict.hasTheirs     ? aText.useTheirs
                         : conflict.hasAncestor ? aText.deleteAsTheirs
                                                : aText.leaveOutAsTheirs );
        }

        wxString situation;

        if( !conflict.hasOurs && !conflict.hasAncestor )
            situation = aText.onlyTheirs;
        else if( !conflict.hasOurs )
            situation = aText.deletedHere;
        else if( !conflict.hasTheirs && !conflict.hasAncestor )
            situation = aText.onlyOurs;
        else if( !conflict.hasTheirs )
            situation = aText.deletedThere;
        else
            situation = aText.changedBoth;

        int sel = wxGetSingleChoiceIndex( wxString::Format( situation, conflict.name, ii + 1, aConflicts.size() ),
                                          _( "Resolve Conflict" ), choices, aParent );

        if( sel < 0 )
        {
            wxString warning = handMerged == 1 ? _( "Stop resolving? The file you merged by hand is discarded "
                                                    "with the rest." )
                                               : wxString::Format( _( "Stop resolving? The %d files you merged by "
                                                                      "hand are discarded with the rest." ),
                                                                   handMerged );

            if( handMerged > 0 && !IsOK( aParent, warning ) )
                continue;

            return MERGE_PROMPT_RESULT::CANCELLED;
        }

        int action = mergeable ? sel : sel + 1;

        if( action == 0 )
        {
            wxString mergedOut = aScratchDir + sep + wxString::Format( wxS( "m%zu_" ), ii )
                                 + wxFileName( conflict.path ).GetFullName();

            WX_STRING_REPORTER reporter;
            int                rc = KICAD_DIFF::DispatchMerge( aKiway, kind, conflict.ancestorFile, conflict.oursFile,
                                                               conflict.theirsFile, mergedOut,
                                                               true, false, &reporter );

            if( rc != 0 )
            {
                // Offer this file's choices again after a cancel or failure
                if( rc != CLI::EXIT_CODES::ERR_CANCELLED && reporter.HasMessage() )
                    DisplayErrorMessage( aParent, _( "The merge tool did not produce a result." ),
                                         reporter.GetMessages() );

                continue;
            }

            aAnswers.push_back( { MERGE_ANSWER::KEPT::MERGED, mergedOut } );
            ++handMerged;

            // A merged result replaces both originals, so both are kept
            if( conflict.hasOurs && !conflict.oursFile.IsEmpty() )
                aLosingCopies.push_back( { conflict.oursFile, conflict.path + wxS( ".mine" ) } );

            if( conflict.hasTheirs )
                aLosingCopies.push_back( { conflict.theirsFile, conflict.path + wxS( ".theirs" ) } );
        }
        else if( action == 1 )
        {
            // An empty path records that the file is deleted
            aAnswers.push_back( { MERGE_ANSWER::KEPT::OURS, conflict.hasOurs ? conflict.oursFile : wxString() } );

            if( conflict.hasTheirs && !conflict.theirsFile.IsEmpty() )
                aLosingCopies.push_back( { conflict.theirsFile, conflict.path + wxS( ".theirs" ) } );
        }
        else
        {
            aAnswers.push_back( { MERGE_ANSWER::KEPT::THEIRS, conflict.hasTheirs ? conflict.theirsFile : wxString() } );

            if( conflict.hasOurs && !conflict.oursFile.IsEmpty() )
                aLosingCopies.push_back( { conflict.oursFile, conflict.path + wxS( ".mine" ) } );
        }

        ++ii;
    }

    return MERGE_PROMPT_RESULT::RESOLVED;
}


bool SaveLosingCopies( const wxString& aBackupRoot, const std::vector<MERGE_LOSING_COPY>& aCopies, wxString& aDir,
                       wxString& aFailed )
{
    const wxString sep = wxFileName::GetPathSeparator();

    // Each merge gets its own folder, so copies from different conflicts never meet
    aDir.clear();

    if( !aCopies.empty() )
        aDir = aBackupRoot + sep + wxS( "merge-" ) + wxDateTime::UNow().Format( wxS( "%Y-%m-%dT%H-%M-%S-%l" ) );

    for( const MERGE_LOSING_COPY& copy : aCopies )
    {
        wxFileName dest( aDir + sep + copy.relativeDest );

        if( !dest.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) || !wxCopyFile( copy.source, dest.GetFullPath(), false ) )
        {
            wxFileName::Rmdir( aDir, wxPATH_RMDIR_RECURSIVE );
            aFailed = dest.GetFullPath();
            return false;
        }
    }

    return true;
}
