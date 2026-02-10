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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JOB_PCB_IMPORT_H
#define JOB_PCB_IMPORT_H

#include <kicommon.h>
#include "job.h"

/**
 * Job to import a non-KiCad PCB file to KiCad format.
 *
 * Supports importing from PADS, Altium, Eagle, and other formats supported
 * by the PCB_IO plugins.
 */
class KICOMMON_API JOB_PCB_IMPORT : public JOB
{
public:
    JOB_PCB_IMPORT();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class FORMAT
    {
        AUTO,
        PADS,
        ALTIUM,
        EAGLE,
        CADSTAR,
        FABMASTER,
        PCAD,
        SOLIDWORKS
    };

    enum class REPORT_FORMAT
    {
        NONE,
        JSON,
        TEXT
    };

    wxString      m_inputFile;
    FORMAT        m_format = FORMAT::AUTO;
    wxString      m_layerMapFile;
    bool          m_autoMap = true;
    REPORT_FORMAT m_reportFormat = REPORT_FORMAT::NONE;
    wxString      m_reportFile;
};

#endif
