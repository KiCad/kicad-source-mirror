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

#ifndef JOB_PCB_DOWNGRADE_H
#define JOB_PCB_DOWNGRADE_H

#include <kicommon.h>
#include <compatibility_report.h>
#include "job.h"

class KICOMMON_API JOB_PCB_DOWNGRADE : public JOB
{
public:
    JOB_PCB_DOWNGRADE();

    wxString m_filename;   ///< Input board file (may be empty to downgrade only libraries)
    wxString m_outputFile; ///< Where to write the downgraded copy
    wxString m_target;     ///< Target release name, e.g. "KiCad 10.0". Empty means the newest.
    wxString m_libraryDir; ///< If set, downgrade every .kicad_mod footprint under this folder too
    wxString m_variant;    ///< Flatten this variant into the export. Empty means the base design.
    bool     m_force;      ///< Export even when features must be dropped or approximated
    bool     m_dryRun;     ///< Only classify and fill m_report, do not write anything
    bool     m_dropInsteadOfApproximate; ///< Omit features that would otherwise be approximated
    bool     m_inPlace;                  ///< Internal project export operating only on its prepared copy

    COMPATIBILITY_REPORT m_report; ///< Filled by the handler so the caller can show it
};

#endif
