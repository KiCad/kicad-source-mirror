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

#include <jobs/job_pcb_import.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_IMPORT::FORMAT,
                              {
                                  { JOB_PCB_IMPORT::FORMAT::AUTO, "auto" },
                                  { JOB_PCB_IMPORT::FORMAT::PADS, "pads" },
                                  { JOB_PCB_IMPORT::FORMAT::ALTIUM, "altium" },
                                  { JOB_PCB_IMPORT::FORMAT::EAGLE, "eagle" },
                                  { JOB_PCB_IMPORT::FORMAT::CADSTAR, "cadstar" },
                                  { JOB_PCB_IMPORT::FORMAT::FABMASTER, "fabmaster" },
                                  { JOB_PCB_IMPORT::FORMAT::PCAD, "pcad" },
                                  { JOB_PCB_IMPORT::FORMAT::SOLIDWORKS, "solidworks" }
                              } )


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PCB_IMPORT::REPORT_FORMAT,
                              {
                                  { JOB_PCB_IMPORT::REPORT_FORMAT::NONE, "none" },
                                  { JOB_PCB_IMPORT::REPORT_FORMAT::JSON, "json" },
                                  { JOB_PCB_IMPORT::REPORT_FORMAT::TEXT, "text" }
                              } )


JOB_PCB_IMPORT::JOB_PCB_IMPORT() :
        JOB( "pcb_import", false )
{
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "layer_map_file", &m_layerMapFile, m_layerMapFile ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "auto_map", &m_autoMap, m_autoMap ) );
    m_params.emplace_back( new JOB_PARAM<REPORT_FORMAT>( "report_format", &m_reportFormat, m_reportFormat ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "report_file", &m_reportFile, m_reportFile ) );
}


wxString JOB_PCB_IMPORT::GetDefaultDescription() const
{
    return _( "Import PCB" );
}


wxString JOB_PCB_IMPORT::GetSettingsDialogTitle() const
{
    return _( "PCB Import Job Settings" );
}


REGISTER_JOB( pcb_import, _HKI( "PCB: Import" ), KIWAY::FACE_PCB, JOB_PCB_IMPORT );
