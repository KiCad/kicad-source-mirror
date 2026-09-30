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

#include <vector>

#include <git/git_editor_snapshot.h>
#include <widgets/file_checklist_ctrl.h>

class EDA_BASE_FRAME;
class KIWAY;
class PROJECT;


/**
 * Build the file checklist entries for the project's changed files, transient files filtered
 * out and each path stored relative to the working directory. Shared by the docked commit
 * panel and the advanced window so both show the same list.
 */
std::vector<FILE_CHECKLIST_ENTRY> BuildGitChangeEntries( PROJECT& aProject );

/**
 * What the caller is about to do, which decides the wording and whether the project file is
 * written.
 */
enum class GIT_EDITOR_PREP
{
    COMMIT,      ///< The working files are about to be committed, so the project file is saved too.
    WORKING_TREE ///< The working files are about to be rewritten, by a pull, branch switch or stash.
};

/**
 * Save any open editors before a git operation, prompting when the schematic or board editor has
 * unsaved changes. Returns false when the user cancelled or a save failed.
 *
 * When the working tree is about to be rewritten the only choices are to save or to cancel,
 * since a stale editor's next save would overwrite whatever arrived. The project file is
 * written only for a commit, because before a merge it would dirty the file and fail the
 * checkout.
 */
bool PrepareEditorsForGitOperation( EDA_BASE_FRAME* aFrame, GIT_EDITOR_PREP aPurpose );

/// True when the schematic or board editor holds unsaved changes.
bool DocumentEditorsDirty( KIWAY& aKiway );

/// Names of the library editors holding unsaved changes. They cannot be saved by mail, so an
/// operation that rewrites files refuses until the user saves or discards them.
std::vector<wxString> LibraryEditorsWithUnsavedChanges( KIWAY& aKiway );

/// Repository relative paths of tracked files changed outside the project directory, which
/// the checklist does not show. The caller holds the git action lock.
std::vector<wxString> ChangedTrackedFilesOutsideProject( PROJECT& aProject );

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
 * Settings changes reach editors whose document did not reload. An empty aBefore reloads all.
 */
void ReloadOpenEditors( KIWAY& aKiway, const EDITOR_SNAPSHOT& aBefore = {} );

#endif // GIT_STATUS_ENTRIES_H
