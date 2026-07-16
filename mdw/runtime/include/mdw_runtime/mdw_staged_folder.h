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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_staged_folder.h
 * @brief A new folder that appears whole or not at all.
 *
 * A first copy may fail or be cancelled halfway. Written in place, it would leave a partial folder the client
 * cannot tell from the user's own files, so every retry would be refused.
 */

#ifndef MDW_STAGED_FOLDER_H
#define MDW_STAGED_FOLDER_H

#include <api/workspace/workspace_types.pb.h>

#include <filesystem>
#include <functional>

/// Writes the new folder's content into aStaging, which exists and is empty
using MDW_FOLDER_FILL = std::function<kiapi::workspace::WorkspaceError( const std::filesystem::path& aStaging )>;

/**
 * Create aRoot through aFill, which writes into a new empty folder inside a sibling only this call owns. That
 * folder is renamed to aRoot once aFill succeeds, provided the request was not cancelled and aRoot is still
 * absent or empty. Otherwise the sibling is removed. ".." is not resolved, since a link may come before it. An
 * existing aRoot passes its permissions on to the copy. Answers aFill's error, WE_CANCELLED, WE_VERSION_CONFLICT
 * when something took aRoot meanwhile, or WE_INVALID_PARAMETER for a filesystem root, a name ending in "..",
 * .kicad, or a name a staging sibling could have.
 *
 * aFill runs on a thread of its own that sees the request's cancellation. A cancelled request answers at once,
 * and aFill then finishes and removes its sibling alone, so it must own everything it uses but the backend.
 *
 * One fill of aRoot runs at a time across processes, each holding <name>.kicad-mdw.lock in the .kicad folder beside
 * aRoot, which is left in place. The holder first removes siblings a dead process left behind, known by their
 * marker files.
 */
kiapi::workspace::WorkspaceError MdwFillStagedFolder( const std::filesystem::path& aRoot, MDW_FOLDER_FILL aFill );

/**
 * Remove the staging siblings in aFolder that fills of dead processes left behind, as the next fill of each
 * root would. A live fill keeps its siblings, so this is safe at any time.
 */
void MdwSweepDeadStaging( const std::filesystem::path& aFolder );

#endif // MDW_STAGED_FOLDER_H
