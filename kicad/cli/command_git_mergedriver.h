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

#ifndef COMMAND_GIT_MERGEDRIVER_H
#define COMMAND_GIT_MERGEDRIVER_H

#include "command.h"


namespace CLI
{

/**
 * Batch (non-interactive) git merge-driver hook for KiCad documents/libraries.
 *
 * NOT a user-facing command — it is invoked automatically by `git merge` via
 * the `merge.kicad-*.driver` config that ApplyKicadGitConventions writes, and
 * is hidden from `kicad-cli --help` (set_suppress). It does the structural
 * 3-way merge, writes the merged result, and follows git's driver contract:
 * exit 0 when it merges cleanly, non-zero when conflicts remain (git then
 * leaves the path unmerged for the interactive `git mergetool`, which opens
 * DIALOG_KICAD_MERGE_3WAY). The document type is supplied explicitly via
 * --kind because git hands the driver extension-less temp paths.
 */
class GIT_MERGEDRIVER_COMMAND : public COMMAND
{
public:
    GIT_MERGEDRIVER_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};

} // namespace CLI

#endif // COMMAND_GIT_MERGEDRIVER_H
