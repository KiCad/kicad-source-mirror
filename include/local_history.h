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
#include <io/kicad/kicad_io_utils.h>

#include <atomic>
#include <future>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <string>
#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wx/string.h>
#include <wx/window.h>

class PROGRESS_REPORTER;


/**
 * Data produced by a registered saver on the UI thread, consumed by either the background
 * local-history commit thread or the foreground autosave-file writer.  Each entry represents
 * one file to write.  The dispatcher joins @p relativePath to a destination root chosen by
 * the active backup format and location preferences.
 */
struct KICOMMON_API HISTORY_FILE_DATA
{
    wxString    relativePath;   ///< Destination path relative to the project root
    std::string content;        ///< Serialized content (mutually exclusive with sourcePath)
    wxString    sourcePath;     ///< For file-copy savers (small files like .kicad_pro)
    bool        prettify = false;
    KICAD_FORMAT::FORMAT_MODE formatMode = KICAD_FORMAT::FORMAT_MODE::NORMAL;
};

struct KICOMMON_API LOCAL_HISTORY_SNAPSHOT_INFO
{
    wxString      hash;
    wxDateTime    date;
    wxString      summary;
    wxString      message;
    int           filesChanged = 0;
    wxArrayString changedFiles;
};

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
     *  The callback receives the project path and should populate aFileData with
     *  serialized content or source paths for inclusion.
     *  @param aSaverObject Unique object pointer identifier for this saver (to prevent duplicate registration)
     *  @param aSaver The saver callback function */
    void RegisterSaver(
            const void* aSaverObject,
            const std::function<void( const wxString&, std::vector<HISTORY_FILE_DATA>& )>& aSaver );

    /** Unregister a previously registered saver callback.
     *  @param aSaverObject The object pointer that was used to register the saver */
    void UnregisterSaver( const void* aSaverObject );

    /** Clear all registered savers. */
    void ClearAllSavers();

    /** Run all registered savers and, if any staged changes differ from HEAD, create a commit.
     *  When @p aTagFileType is non-empty the new commit is tagged Save_<type>_N (manual save);
     *  empty leaves the commit untagged (autosave). */
    bool RunRegisteredSaversAndCommit( const wxString& aProjectPath, const wxString& aTitle,
                                       const wxString& aTagFileType = wxEmptyString );

    /**
     * Run all registered savers and write their output to autosave files instead of
     * committing to the local history repository.  Used when BACKUP_FORMAT::ZIP is
     * selected so the user still has crash recovery without a git store.
     *
     * Output location follows BACKUP_LOCATION:
     *   PROJECT_DIR  ->  "<projectpath>/<reldir>/_autosave-<basename>"
     *   USER_DIR     ->  "<user_root>/<reldir>/<basename>" (mirroring the project tree)
     *
     * Files are written via KIPLATFORM::IO::AtomicWriteFile so a crash mid-write
     * cannot leave a half-written autosave on disk.
     */
    bool RunRegisteredSaversAsAutosaveFiles( const wxString& aProjectPath );

    /**
     * Enumerate autosave files newer than their corresponding source files for the
     * project at @p aProjectPath, restricted to sources whose extension is in
     * @p aExtensions.  Used by editors at startup to detect crash-recovery
     * candidates for files they own.
     *
     * Each entry in the returned vector has:
     *   first  - autosave file path
     *   second - corresponding source path under the project directory
     */
    std::vector<std::pair<wxString, wxString>> FindStaleAutosaveFiles( const wxString&              aProjectPath,
                                                                       const std::vector<wxString>& aExtensions ) const;

    /**
     * Remove every autosave file under the project at @p aProjectPath regardless of
     * which source it shadowed.  Used by the project manager to sweep autosaves on
     * project close so nothing is left to prompt on next launch.  Manual-save callers
     * should prefer the source-scoped overload below to avoid wiping autosaves for
     * files this save did not actually persist.
     */
    void RemoveAutosaveFiles( const wxString& aProjectPath ) const;

    /**
     * Remove only the autosave files corresponding to @p aSourcePaths.  Called after a
     * successful manual save so we drop autosaves the save just superseded without
     * touching unsaved siblings (e.g. a PCB save must not erase a dirty schematic
     * sheet's autosave when both editors are open).
     */
    void RemoveAutosaveFiles( const wxString& aProjectPath,
                              const std::vector<wxString>& aSourcePaths ) const;

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
    bool EnforceSizeLimit( const wxString& aProjectPath, size_t aMaxBytes, PROGRESS_REPORTER* aReporter = nullptr );

    /** Return true if the autosave data is newer than the last manual save. */
    bool HeadNewerThanLastSave( const wxString& aProjectPath );

    /** Return the current head commit hash. */
    wxString GetHeadHash( const wxString& aProjectPath );

    /** Restore the project files to the state recorded by the given commit hash. */
    bool RestoreCommit( const wxString& aProjectPath, const wxString& aHash, wxWindow* aParent = nullptr );

    /** Show a dialog allowing the user to choose a snapshot to restore. */
    void ShowRestoreDialog( const wxString& aProjectPath, wxWindow* aParent );

    /** Block until any pending background save completes.  Call before reading repo
     *  state (HeadNewerThanLastSave / GetHeadHash) at close time so an in-flight
     *  autosave can't leave the close path with a stale view of HEAD. */
    void WaitForPendingSave();

private:
    std::vector<LOCAL_HISTORY_SNAPSHOT_INFO> LoadSnapshots( const wxString& aProjectPath );

    /** Execute file writes and git commit on a background thread. */
    bool commitInBackground( const wxString& aProjectPath, const wxString& aTitle,
                             const std::vector<HISTORY_FILE_DATA>& aFileData, bool aIsManualSave );

    std::set<wxString> m_pendingFiles;
    std::map<const void*,
             std::function<void( const wxString&, std::vector<HISTORY_FILE_DATA>& )>> m_savers;

    std::atomic<bool> m_saveInProgress{ false };
    std::future<bool> m_pendingFuture;
};

