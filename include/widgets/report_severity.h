/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _REPORT_SEVERITY_H_
#define _REPORT_SEVERITY_H_

// Note: On windows, SEVERITY_ERROR collides with a system declaration,
// so we used RPT_SEVERITY _xxx instead of SEVERITY _xxx
enum SEVERITY {
    RPT_SEVERITY_UNDEFINED = 0x00,
    RPT_SEVERITY_INFO      = 0x01,
    RPT_SEVERITY_EXCLUSION = 0x02,
    RPT_SEVERITY_ACTION    = 0x04,
    RPT_SEVERITY_WARNING   = 0x08,
    RPT_SEVERITY_ERROR     = 0x10,
    RPT_SEVERITY_IGNORE    = 0x20
};

#endif // _REPORT_SEVERITY_H_
