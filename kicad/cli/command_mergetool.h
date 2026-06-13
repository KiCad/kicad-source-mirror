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

#ifndef KICAD_COMMAND_MERGETOOL_H
#define KICAD_COMMAND_MERGETOOL_H

#include <cli/command.h>


namespace CLI
{

/**
 * `kicad-cli mergetool ANCESTOR OURS THEIRS -o OUTPUT` — uniform entry point
 * for `git mergetool`. It re-execs `kicad --mergetool` (the GUI host) so the
 * user gets the interactive resolution dialog whenever automatic merging
 * fails. Merge itself is an interactive operation; there is no headless
 * `kicad-cli ... merge` subcommand.
 *
 * Exit codes follow the Phase 8 contract:
 *   0   merged file written
 *   1   user cancelled
 *   2   unresolved conflicts remain
 *   3   I/O or parse error
 *   4   initialization failure
 */
class MERGETOOL_COMMAND : public COMMAND
{
public:
    MERGETOOL_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};

} // namespace CLI

#endif // KICAD_COMMAND_MERGETOOL_H
