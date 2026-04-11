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


#ifndef REPORT_SEVERITY_H
#define REPORT_SEVERITY_H

#include <kicommon.h>
#include <wx/string.h>

// Note: On windows, SEVERITY_ERROR collides with a system declaration,
// so we used RPT_SEVERITY_xxx instead of SEVERITY_xxx
enum SEVERITY {
    RPT_SEVERITY_UNDEFINED = 0x01,
    RPT_SEVERITY_INFO      = 0x02,
    RPT_SEVERITY_EXCLUSION = 0x04,
    RPT_SEVERITY_ACTION    = 0x08,
    RPT_SEVERITY_WARNING   = 0x10,
    RPT_SEVERITY_ERROR     = 0x20,
    RPT_SEVERITY_IGNORE    = 0x40,
    RPT_SEVERITY_DEBUG     = 0x80,
};

/**
 * Convert a severity mask to a human-readable comma-separated string.
 * @param aSeverities A bitmask of SEVERITY values
 * @return A translated string like "Errors, Warnings"
 */
KICOMMON_API wxString formatSeverities( int aSeverities );

#endif // REPORT_SEVERITY_H
