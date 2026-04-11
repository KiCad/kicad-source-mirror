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

#ifndef JOB_PCB_IMPORT_H
#define JOB_PCB_IMPORT_H

#include <kicommon.h>
#include <map>
#include "job.h"
#include "job_import_utils.h"

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
        PADS_ASCII,
        ALTIUM,
        EAGLE,
        CADSTAR,
        FABMASTER,
        PCAD,
        SOLIDWORKS
    };

    wxString             m_inputFile;
    FORMAT               m_format = FORMAT::AUTO;
    IMPORT_REPORT_FORMAT m_reportFormat = IMPORT_REPORT_FORMAT::NONE;
    wxString             m_reportFile;

    /// Explicit overrides from source layer name to KiCad layer name (canonical board-file name,
    /// e.g. "F.Cu").  Source layers absent from the map keep the importer's automatic best-guess.
    /// An empty map means fully automatic layer mapping.
    std::map<wxString, wxString> m_layerMap;
};

#endif
