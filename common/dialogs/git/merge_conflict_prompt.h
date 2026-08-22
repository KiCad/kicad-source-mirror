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

#ifndef MERGE_CONFLICT_PROMPT_H
#define MERGE_CONFLICT_PROMPT_H

#include <functional>
#include <vector>

#include <wx/string.h>

class KIWAY;
class wxWindow;


/// One conflicted file with its sides written out. A side with no file cannot be merged or taken
struct MERGE_CONFLICT
{
    wxString path; ///< Its extension picks the merge, and its backups are saved under it
    wxString name; ///< What the prompts call it
    wxString ancestorFile;
    wxString oursFile;
    wxString theirsFile;
    bool     hasAncestor = false;
    bool     hasOurs = false;
    bool     hasTheirs = false;
};


/// The prompt text naming the other side. Each situation formats the name, the position and the count
struct MERGE_CONFLICT_TEXT
{
    wxString onlyTheirs;
    wxString deletedHere;
    wxString onlyOurs;
    wxString deletedThere;
    wxString changedBoth;
    wxString useTheirs;
    wxString deleteAsTheirs;
    wxString leaveOutAsTheirs;
};


struct MERGE_ANSWER
{
    enum class KEPT
    {
        MERGED,
        OURS,
        THEIRS
    };

    KEPT     kept = KEPT::MERGED;
    wxString file; ///< The resolved content, empty when the file is deleted
};


/// A version the user did not keep, saved as relativeDest under the backup folder
struct MERGE_LOSING_COPY
{
    wxString source;
    wxString relativeDest;
};


enum class MERGE_PROMPT_RESULT
{
    RESOLVED,
    CANCELLED, ///< The user stopped
    STOPPED    ///< aCanContinue refused
};


/**
 * Settle each of aConflicts with the user. A symbol library first gets one merge that needs no answers, then each
 * conflict offers the merge tool when all three sides exist, keeping ours, or taking theirs. Merged files are
 * written to aScratchDir. Fills aAnswers in conflict order and aLosingCopies with the versions not kept, and stops
 * before any conflict aCanContinue refuses.
 */
MERGE_PROMPT_RESULT PromptMergeConflicts( wxWindow* aParent, KIWAY& aKiway,
                                          const std::vector<MERGE_CONFLICT>& aConflicts,
                                          const MERGE_CONFLICT_TEXT& aText, const wxString& aScratchDir,
                                          std::vector<MERGE_ANSWER>&      aAnswers,
                                          std::vector<MERGE_LOSING_COPY>& aLosingCopies,
                                          const std::function<bool()>&    aCanContinue = nullptr );

/**
 * Copy aCopies into a new merge-<time> folder under aBackupRoot, named in aDir, which stays empty when aCopies is.
 * A copy that cannot be made removes the folder, names its destination in aFailed and returns false.
 */
bool SaveLosingCopies( const wxString& aBackupRoot, const std::vector<MERGE_LOSING_COPY>& aCopies, wxString& aDir,
                       wxString& aFailed );

#endif // MERGE_CONFLICT_PROMPT_H
