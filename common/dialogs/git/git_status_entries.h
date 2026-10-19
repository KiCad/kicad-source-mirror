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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GIT_STATUS_ENTRIES_H
#define GIT_STATUS_ENTRIES_H

#include <functional>
#include <vector>

#include <gestfich.h>
#include <git/git_editor_snapshot.h>
#include <widgets/file_checklist_ctrl.h>
#include <diff_merge/diff_doc_kind.h>

class EDA_BASE_FRAME;
class KIWAY;
class KIGIT_COMMON;
class KIGIT_QUERY;
class wxWindow;
struct git_repository;


/**
 * A private scratch directory for extracting committed file versions to diff, removed when
 * this object goes out of scope. A schematic checks out the whole tree so its sub-sheet files
 * resolve. Other document kinds are written as a single blob.
 */
class GIT_DIFF_WORKSPACE
{
public:
    GIT_DIFF_WORKSPACE();

    GIT_DIFF_WORKSPACE( const GIT_DIFF_WORKSPACE& ) = delete;
    GIT_DIFF_WORKSPACE& operator=( const GIT_DIFF_WORKSPACE& ) = delete;

    /// True when the scratch directory was created.
    bool IsOk() const { return !m_root.Path().IsEmpty(); }

    /// The scratch directory, for callers that lay out their own files (e.g. footprint libraries).
    const wxString& Root() const { return m_root.Path(); }

    /// Extract aRelPath at aRef into a uniquely named slot and return the path to load, or an
    /// empty string when that version does not exist or could not be written.
    wxString Materialize( git_repository* aRepo, const wxString& aRef, const wxString& aRelPath,
                          KICAD_DIFF::DOC_KIND aKind );

private:
    SCOPED_TEMP_PATH m_root;
    int              m_slot = 0;
};

/**
 * Build the file checklist entries for the project's changed files, transient files filtered
 * out and each path stored relative to the working directory. Shared by the docked commit
 * panel and the advanced window so both show the same list.
 *
 * Uses project values and the theme captured on the UI thread. The caller holds the session's
 * action lock. This function never reads a live PROJECT or UI settings.
 */
std::vector<FILE_CHECKLIST_ENTRY> BuildGitChangeEntries( KIGIT_COMMON& aCommon, const wxString& aProjectPath,
                                                         bool aDarkTheme );

/**
 * What the caller is about to do, which decides the wording and whether the project file is
 * written.
 */
enum class GIT_EDITOR_PREP
{
    COMMIT,      ///< Saving dirty editors for a commit also saves the project file.
    WORKING_TREE ///< The working files are about to be rewritten, by a pull, branch switch or stash.
};

/**
 * Offer to save unsaved schematic and board edits before a git operation. Returns false when
 * the user cancelled or a save failed. Library edits must be saved or discarded in their editor
 * before a working tree rewrite. They do not block a commit or take part in its save prompt.
 *
 * When the working tree is about to be rewritten the only choices are to save or to cancel,
 * since a stale editor's next save would overwrite whatever arrived. The project file is
 * written only for a commit, because before a merge it would dirty the file and fail the
 * checkout. The caller must first check LfsRefusal on a worker before saving editors.
 *
 * @param aSavedEditors When supplied, receives true only after dirty editors were successfully
 *                      saved. False for Commit Anyway or when no editor needed saving, so a
 *                      caller need not rescan for changes produced by an editor save.
 */
bool PrepareEditorsForGitOperation( EDA_BASE_FRAME* aFrame, GIT_EDITOR_PREP aPurpose, bool* aSavedEditors = nullptr );

/// Unsaved schematic or board edits can be saved for a commit, even with no changes on disk.
/// Library edits do not count. Call on the UI thread.
bool DocumentEditorsDirty( KIWAY& aKiway );

/// Unsaved document or library edits block operations that rewrite files. Call on the UI thread.
bool EditorsDirty( KIWAY& aKiway );

/// Names of the library editors holding unsaved changes. They cannot be saved by mail, so an
/// operation that rewrites files refuses until the user saves or discards them.
std::vector<wxString> LibraryEditorsWithUnsavedChanges( KIWAY& aKiway );

/// Repository relative paths of tracked files changed outside the project directory, which
/// the checklist does not show. Uses a captured project path. The caller holds the action lock.
std::vector<wxString> ChangedTrackedFilesOutsideProject( KIGIT_COMMON& aCommon, const wxString& aProjectPath );

/**
 * Repository state used before an operation that requires a clean working tree. Read on a
 * worker and combined with live editor state on the UI thread, so unsaved work is never
 * mistaken for an uncommitted file. Stash creation uses the tracked state after saving editors.
 */
struct GIT_WORKTREE_STATUS
{
    bool                  checked = false;
    bool                  hasTrackedChanges = false;
    wxString              lfsMessage;
    std::vector<wxString> outsideProject;

    /// Empty when rewriting is allowed. Unsaved editors take precedence over disk changes.
    /// Stash creation allows tracked changes because it records them before rewriting files.
    wxString RefusalMessage( const std::vector<wxString>& aUnsavedEditors, bool aAllowTrackedChanges = false ) const;
};

/// Read tracked changes and Git LFS using a captured project path. The caller holds the action
/// lock on a worker. An unavailable repository leaves checked false and cannot authorize a rewrite.
GIT_WORKTREE_STATUS ReadGitWorktreeStatus( KIGIT_COMMON& aCommon, const wxString& aProjectPath );

/// Combine the repository snapshot with live editor state and explain any refusal. Called on
/// the UI thread before rewriting files, and again after a confirmation that could allow edits.
bool ProjectSafeToRewrite( EDA_BASE_FRAME* aFrame, wxWindow* aParent, const GIT_WORKTREE_STATUS& aStatus,
                           bool aAllowTrackedChanges = false );

/**
 * Record the on-disk identity of the open documents, before an operation that may rewrite
 * them. Takes the KIWAY rather than a frame, because the completion that consumes the
 * snapshot must outlive the window that took it.
 */
EDITOR_SNAPSHOT CaptureOpenEditorDocs( KIWAY& aKiway );

/**
 * Tell the open board and schematic editors to reload from disk. Only a document whose files
 * changed since aBefore is reloaded, because reloading discards undo history and the view
 * state. An editor holding unsaved changes to one asks whether to reload or keep the edits.
 * Settings changes reach editors whose document did not reload.
 *
 * aBefore must come from CaptureOpenEditorDocs() taken before the operation.
 */
void ReloadOpenEditors( KIWAY& aKiway, const EDITOR_SNAPSHOT& aBefore );

/**
 * Check the working tree and Git LFS on a worker, then run aReady on the UI thread when the
 * operation can proceed. Otherwise explain why it is refused. Rechecks unsaved editor changes
 * and the current session after the read. aReady receives the snapshot to recheck editors after
 * any confirmation. aQuery belongs to the window starting the action.
 */
void ProjectClearOfUncommittedWork( EDA_BASE_FRAME* aFrame, wxWindow* aParent, KIGIT_QUERY& aQuery,
                                    std::function<void( const GIT_WORKTREE_STATUS& )> aReady );

/**
 * Check the working tree on a worker, then confirm switching to aBranch and recheck unsaved
 * editors. On success the editors reload and every Git surface refreshes. aQuery belongs to
 * the window starting the action and cancels the continuation when that window closes.
 * aFinished runs after success or refusal, while the owner still exists.
 */
void SwitchProjectToBranch( EDA_BASE_FRAME* aFrame, wxWindow* aParent, const wxString& aBranch, KIGIT_QUERY& aQuery,
                            std::function<void()> aFinished = {} );

/**
 * Open the diff for a changed file, comparing the committed version at HEAD with the working
 * copy. Design files open the visual diff, other text files a line diff, and a new file is
 * compared against an empty committed side. Binary content gets a message instead.
 *
 * @param aFrame Frame to parent the diff to, and whose project supplies the repository.
 * @param aRelativePath Path of the file relative to the repository working directory.
 */
void ShowGitFileDiff( EDA_BASE_FRAME* aFrame, const wxString& aRelativePath );

/**
 * Open the diff for what a commit changed, comparing it against its parent. When the commit
 * changed more than one file the user picks which one to compare. Design files open the
 * visual diff and other text files a line diff.
 *
 * @param aFrame Frame whose project supplies the repository.
 * @param aCommitHash Hash of the commit to inspect.
 * @param aParentWindow Window to parent the diff to, or null to use aFrame. Pass the calling
 *                      dialog so it stays open behind the diff instead of being dismissed.
 */
void ShowGitCommitDiff( EDA_BASE_FRAME* aFrame, const wxString& aCommitHash, wxWindow* aParentWindow = nullptr,
                        const wxString& aRelativePath = wxEmptyString );

/// The two sides of one file, written out for a comparison.
struct GIT_FILE_SIDES
{
    wxString             before; ///< Empty when the file did not exist on that side.
    wxString             after;
    KICAD_DIFF::DOC_KIND kind = KICAD_DIFF::DOC_KIND::UNKNOWN;
    wxString             error;  ///< Why the sides could not be written, when they could not.
};

/**
 * Write both sides of one file a commit changed into aWorkspace, the parent's version and the
 * commit's. False, with the error set, when the commit did not change the file or a side could
 * not be read. The caller holds the session's action lock and keeps the workspace alive until
 * the UI has finished using the materialized documents.
 */
bool MaterializeCommitFile( KIGIT_COMMON& aCommon, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aCommitHash,
                            const wxString& aRelativePath, GIT_FILE_SIDES& aSides );

/**
 * Write the committed version of a file into aWorkspace and name its working copy beside it.
 * False, with the error set, when the committed side could not be read. The caller holds the
 * session's action lock and keeps the workspace alive until the UI has loaded its documents.
 */
bool MaterializeWorkingFile( KIGIT_COMMON& aCommon, GIT_DIFF_WORKSPACE& aWorkspace, const wxString& aRelativePath,
                             GIT_FILE_SIDES& aSides );

#endif // GIT_STATUS_ENTRIES_H
