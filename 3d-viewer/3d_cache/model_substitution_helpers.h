/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <wx/string.h>

class FILENAME_RESOLVER;


/**
 * Shared helpers for matching obsolete `.wrl`/`.wrz` 3D model references against
 * current STEP-family replacements.  Used both by the interactive migration
 * dialog (which rewrites board-state references) and by the 3D cache (which
 * substitutes on-the-fly in CLI/scripting contexts).  Keeping the matcher in
 * one place guarantees both code paths agree on "same name".
 */
namespace MODEL_SUBSTITUTION
{

/// True iff @p aFilename ends in `.wrl` or `.wrz` (case-insensitive).
bool IsWrlExtension( const wxString& aFilename );


/**
 * An index of STEP-family model files keyed by normalised filename stem.  The
 * catalog is built by walking the 3D search paths once and is then consulted
 * many times to resolve missing WRL references without re-walking the
 * filesystem per lookup.
 */
class STEP_CATALOG
{
public:
    /**
     * Walk the resolver's search paths, the project's `3dshapes/` subdirectory,
     * and the user's `COMMON_SETTINGS::m_Extra3DSearchDirs` collecting every
     * STEP-family file (`.step`, `.stp`, `.stpz`, `.step.gz`, `.stp.gz`,
     * `.iges`, `.igs`) indexed by normalised stem.  Safe to call multiple
     * times; each call rebuilds the index.
     *
     * @param aProjectPath is the project directory used to locate a local
     *                     `3dshapes/` tree; may be empty.
     * @param aResolver provides the list of standard search paths.  May be
     *                  null, in which case only the project path and common
     *                  settings extra directories are scanned.
     */
    void Build( const wxString& aProjectPath, const FILENAME_RESOLVER* aResolver );

    /**
     * Look up the best STEP-family replacement for a missing WRL reference.
     * When multiple catalog entries share the same stem, prefer one whose
     * parent-directory basename matches the parent directory of
     * @p aMissingWrl (e.g. both sit in `Resistor_SMD.3dshapes`).
     *
     * @param aMissingWrl is the original (unresolved) `.wrl` / `.wrz`
     *                    filename, possibly with `${VAR}` aliases intact.
     * @return absolute path of the best match, or empty if none found or the
     *         input is not a WRL reference.
     */
    wxString FindMatchFor( const wxString& aMissingWrl ) const;

    bool Empty() const { return m_byStem.empty(); }

private:
    /// Normalised stem → list of absolute STEP-family paths sharing that stem.
    std::unordered_map<wxString, std::vector<wxString>> m_byStem;
};

}  // namespace MODEL_SUBSTITUTION
