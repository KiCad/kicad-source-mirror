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

#ifndef JOB_SYM_DOWNGRADE_H
#define JOB_SYM_DOWNGRADE_H

#include <kicommon.h>
#include <compatibility_report.h>
#include "job.h"

class KICOMMON_API JOB_SYM_DOWNGRADE : public JOB
{
public:
    JOB_SYM_DOWNGRADE();

    wxString m_libraryPath;              ///< The .kicad_sym library to convert
    wxString m_outputLibraryPath;        ///< Where to write the converted copy
    wxString m_target;                   ///< Target release name or id, e.g. "KiCad 9.0". Empty means the newest.
    bool     m_force;                    ///< Convert even when features must be dropped or approximated
    bool     m_dryRun;                   ///< Only classify and fill m_report, do not write anything
    bool     m_dropInsteadOfApproximate; ///< Omit features that would otherwise be approximated

    COMPATIBILITY_REPORT m_report; ///< Filled by the handler so the caller can show it
};

#endif
