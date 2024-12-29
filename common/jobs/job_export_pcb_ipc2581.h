/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_PCB_IPC2581_H
#define JOB_EXPORT_PCB_IPC2581_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_IPC2581 : public JOB
{
public:
    JOB_EXPORT_PCB_IPC2581();
    wxString GetDefaultDescription() const override;

    void SetDefaultOutputPath( const wxString& aReferenceName );

    enum class IPC2581_UNITS
    {
        INCHES,
        MILLIMETERS
    };

    enum class IPC2581_VERSION
    {
        B,
        C
    };

    wxString m_filename;
    wxString m_drawingSheet;

    IPC2581_UNITS m_units;
    IPC2581_VERSION m_version;
    int m_precision;

    bool m_compress;

    wxString m_colInternalId;
    wxString m_colMfgPn;
    wxString m_colMfg;
    wxString m_colDistPn;
    wxString m_colDist;
};

#endif
