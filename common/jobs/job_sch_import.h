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

#ifndef JOB_SCH_IMPORT_H
#define JOB_SCH_IMPORT_H

#include <kicommon.h>
#include "job.h"
#include "job_import_utils.h"
#include <core/utf8.h>
#include <map>
#include <string>
#include <vector>

/**
 * Job to import a non-KiCad schematic file to KiCad format.
 *
 * Supports importing from Altium, Eagle, CADSTAR, EasyEDA and the other formats supported
 * by the SCH_IO plugins.
 */
class KICOMMON_API JOB_SCH_IMPORT : public JOB
{
public:
    JOB_SCH_IMPORT();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class FORMAT
    {
        AUTO,
        ALTIUM,
        EAGLE,
        CADSTAR,
        EASYEDA,
        EASYEDAPRO,
        LTSPICE,
        PADS,
        DIPTRACE,
        PCAD,
        ORCAD,
        EASYPC,
        TARGET3001
    };

    wxString             m_inputFile;
    FORMAT               m_format = FORMAT::AUTO;
    IMPORT_REPORT_FORMAT m_reportFormat = IMPORT_REPORT_FORMAT::NONE;
    wxString             m_reportFile;
    std::map<wxString, wxString> m_netNameMap;

    /// Importer and library coordination properties, as IMPORT_PROJ_HELPER hands them to the editor.
    std::map<std::string, UTF8> m_properties;

    /// Directory of the source project and the symbol libraries it names, registered before the import.
    wxString              m_sourceProjectDir;
    std::vector<wxString> m_localLibraries;
};

#endif
