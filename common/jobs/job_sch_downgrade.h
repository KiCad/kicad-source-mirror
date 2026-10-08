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

#ifndef JOB_SCH_DOWNGRADE_H
#define JOB_SCH_DOWNGRADE_H

#include <kicommon.h>
#include <compatibility_report.h>
#include "job.h"

class KICOMMON_API JOB_SCH_DOWNGRADE : public JOB
{
public:
    JOB_SCH_DOWNGRADE();

    wxString m_filename;   ///< Root schematic file (may be empty to downgrade only libraries)
    wxString m_outputDir;  ///< Write downgraded sheets under this fresh folder.
    wxString m_target;     ///< Target release name or id, e.g. "KiCad 10.0" or "10.0". Empty means the newest.
    wxString m_libraryDir; ///< If set, downgrade every .kicad_sym symbol library under this folder too
    wxString m_variant;    ///< Flatten this variant into the export. Empty means the base design.
    bool     m_force;      ///< Export even when features must be dropped or approximated
    bool     m_dryRun;     ///< Only classify and fill m_report, do not write anything
    bool     m_dropInsteadOfApproximate; ///< Omit features that would otherwise be approximated
    bool     m_inPlace;                  ///< Internal only: convert the coordinator's already-copied export files

    COMPATIBILITY_REPORT m_report; ///< Filled by the handler so the caller can show it

    /// Variant names found in the schematic when the target drops variants. Filled by a dry
    /// run, so a caller can offer a picker before the real export.
    std::vector<wxString> m_foundVariants;
};

#endif
