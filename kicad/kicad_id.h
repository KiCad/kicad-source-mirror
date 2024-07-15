/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file kicad/kicad_id.h
 * @brief IDs used in KiCad main frame foe menuitems and tools.
 */

#ifndef KICAD_ID_H
#define KICAD_ID_H

#include <id.h>
#include <eda_base_frame.h>

/**
 * Command IDs for KiCad.
 *
 * Please add IDs that are unique to Kicad here and not in the global id.h file.
 * This will prevent the entire project from being rebuilt when adding
 * new commands to KiCad.
 *
 * However, now the Kicad manager and other sub applications are running inside
 * the same application, these IDs are kept unique inside the whole Kicad code
 * See the global id.h which reserves room for the Kicad manager IDs
 * and expand this room if needed
 *
 * We have experienced issues with duplicate menus IDs between frames
 * because wxUpdateUIEvent events are sent to parent frames, when a wxUpdateUIEvent
 * event function does not exists for some menuitems ID, and therefore
 * with duplicate menuitems IDs in different frames, the wrong menuitem can be used
 * by a function called by the wxUpdateUIEvent event loop.
 *
 * The number of items in this list should be less than ROOM_FOR_KICADMANAGER (see id.h)
 */

enum id_kicad_frm {
    ID_LEFT_FRAME = ID_KICAD_MANAGER_START,
    ID_PROJECT_TREE,
    ID_PROJECT_TXTEDIT,
    ID_PROJECT_SWITCH_TO_OTHER,
    ID_PROJECT_NEWDIR,
    ID_PROJECT_OPEN_DIR,
    ID_PROJECT_DELETE,
    ID_PROJECT_RENAME,

    ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR,
    ID_BROWSE_IN_FILE_EXPLORER,
    ID_SAVE_AND_ZIP_FILES,
    ID_READ_ZIP_ARCHIVE,
    ID_INIT_WATCHED_PATHS,
    ID_IMPORT_CADSTAR_ARCHIVE_PROJECT,
    ID_IMPORT_EAGLE_PROJECT,
    ID_IMPORT_EASYEDA_PROJECT,
    ID_IMPORT_EASYEDAPRO_PROJECT,

    ID_GIT_INITIALIZE_PROJECT,  // Initialize a new git repository in an existing project
    ID_GIT_CLONE_PROJECT,       // Clone a project from a remote repository
    ID_GIT_COMMIT_PROJECT,      // Commit all files in the project
    ID_GIT_COMMIT_FILE,         // Commit a single file
    ID_GIT_SYNC_PROJECT,        // Sync the project with the remote repository (pull and push -- same as Update)
    ID_GIT_FETCH,               // Fetch the remote repository (without merging -- this is the same as Refresh)
    ID_GIT_PUSH,                // Push the local repository to the remote repository
    ID_GIT_PULL,                // Pull the remote repository to the local repository
    ID_GIT_RESOLVE_CONFLICT,    // Present the user with a resolve conflicts dialog (ours/theirs/merge)
    ID_GIT_REVERT_LOCAL,        // Revert the local repository to the last commit
    ID_GIT_COMPARE,             // Compare the current project to a different branch or commit in the git repository
    ID_GIT_REMOVE_VCS,          // Remove the git repository data from the project directory (rm .git)
    ID_GIT_ADD_TO_INDEX,        // Add a file to the git index
    ID_GIT_REMOVE_FROM_INDEX,   // Remove a file from the git index
    ID_GIT_SWITCH_BRANCH,       // Switch the local repository to a different branch
    ID_GIT_SWITCH_QUICK1,       // Switch the local repository to the first quick branch
    ID_GIT_SWITCH_QUICK2,       // Switch the local repository to the second quick branch
    ID_GIT_SWITCH_QUICK3,       // Switch the local repository to the third quick branch
    ID_GIT_SWITCH_QUICK4,       // Switch the local repository to the fourth quick branch
    ID_GIT_SWITCH_QUICK5,       // Switch the local repository to the fifth quick branch

    ID_JOBS_RUN,

    // Please, verify: the number of items in this list should be
    // less than ROOM_FOR_KICADMANAGER (see id.h)
    ID_KICADMANAGER_END_LIST
};

#endif
