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

#include <jobs/job_sch_import.h>
#include <wx/intl.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_SCH_IMPORT::FORMAT,
                              {
                                  { JOB_SCH_IMPORT::FORMAT::AUTO, "auto" },
                                  { JOB_SCH_IMPORT::FORMAT::ALTIUM, "altium" },
                                  { JOB_SCH_IMPORT::FORMAT::EAGLE, "eagle" },
                                  { JOB_SCH_IMPORT::FORMAT::CADSTAR, "cadstar" },
                                  { JOB_SCH_IMPORT::FORMAT::EASYEDA, "easyeda" },
                                  { JOB_SCH_IMPORT::FORMAT::EASYEDAPRO, "easyedapro" },
                                  { JOB_SCH_IMPORT::FORMAT::LTSPICE, "ltspice" },
                                  { JOB_SCH_IMPORT::FORMAT::PADS, "pads" },
                                  { JOB_SCH_IMPORT::FORMAT::DIPTRACE, "diptrace" },
                                  { JOB_SCH_IMPORT::FORMAT::PCAD, "pcad" }
                              } )


JOB_SCH_IMPORT::JOB_SCH_IMPORT() :
        JOB( "sch_import", false )
{
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<IMPORT_REPORT_FORMAT>( "report_format", &m_reportFormat, m_reportFormat ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "report_file", &m_reportFile, m_reportFile ) );
}


wxString JOB_SCH_IMPORT::GetDefaultDescription() const
{
    return _( "Import schematic" );
}


wxString JOB_SCH_IMPORT::GetSettingsDialogTitle() const
{
    return _( "Schematic Import Job Settings" );
}
