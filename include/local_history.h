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

#pragma once

#include <kicommon.h>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <wx/string.h>
#include <wx/window.h>

/**
 * Simple local history manager built on libgit2.  Stores history for project files in
 * a hidden .history git repository within the project directory.
 */
class KICOMMON_API LOCAL_HISTORY
{
public:
    LOCAL_HISTORY();
    ~LOCAL_HISTORY();

    /** Initialize the local history repository for the given project path. */
    bool Init( const wxString& aProjectPath );

    /** Commit the given files to the local history repository. */
    bool CommitSnapshot( const std::vector<wxString>& aFiles, const wxString& aTitle );

    /**
     * Commit a snapshot of the entire project directory (excluding the .history directory and
     * ignored transient files) to the local history repository.  This does not modify any
     * document dirty flags; it purely mirrors on-disk state for history purposes.
     */
    bool CommitFullProjectSnapshot( const wxString& aProjectPath, const wxString& aTitle );

    /** Register a saver callback invoked during autosave history commits.
     *  The callback receives the project path and should append absolute file paths
     *  (within that project) to aFiles for inclusion.
     *  @param aSaverObject Unique object pointer identifier for this saver (to prevent duplicate registration)
     *  @param aSaver The saver callback function */
    void RegisterSaver( const void* aSaverObject,
                       const std::function<void( const wxString&, std::vector<wxString>& )>& aSaver );

    /** Unregister a previously registered saver callback.
     *  @param aSaverObject The object pointer that was used to register the saver */
    void UnregisterSaver( const void* aSaverObject );

    /** Clear all registered savers. */
    void ClearAllSavers();

    /** Run all registered savers and, if any staged changes differ from HEAD, create a commit. */
    bool RunRegisteredSaversAndCommit( const wxString& aProjectPath, const wxString& aTitle );

    /** Record that a file has been modified and should be included in the next snapshot. */
    void NoteFileChange( const wxString& aFile );

    /** Commit any pending modified files to the history repository. */
    bool CommitPending();

    /** Return true if history exists for the project. */
    bool HistoryExists( const wxString& aProjectPath );

    /** Tag a manual save in the local history repository. */
    bool TagSave( const wxString& aProjectPath, const wxString& aFileType );

    /** Create a new commit duplicating the tree pointed to by Last_Save_<fileType> and move the
     *  Last_Save_<fileType> tag to the new commit (used when user discards changes). */
    bool CommitDuplicateOfLastSave( const wxString& aProjectPath, const wxString& aFileType,
                                    const wxString& aMessage );

    /** Enforce total size limit by rebuilding trimmed history keeping newest commits whose
     *  cumulative unique blob sizes fit within limit. */
    bool EnforceSizeLimit( const wxString& aProjectPath, size_t aMaxBytes );

    /** Return true if the autosave data is newer than the last manual save. */
    bool HeadNewerThanLastSave( const wxString& aProjectPath );

    /** Return the current head commit hash. */
    wxString GetHeadHash( const wxString& aProjectPath );

    /** Restore the project files to the state recorded by the given commit hash. */
    bool RestoreCommit( const wxString& aProjectPath, const wxString& aHash, wxWindow* aParent = nullptr );

    /** Show a dialog allowing the user to choose a snapshot to restore. */
    void ShowRestoreDialog( const wxString& aProjectPath, wxWindow* aParent );

private:
    std::set<wxString> m_pendingFiles;
    std::map<const void*, std::function<void(const wxString&, std::vector<wxString>&)>> m_savers;
};

