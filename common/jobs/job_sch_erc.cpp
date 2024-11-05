/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_sch_erc.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_SCH_ERC::UNITS,
                              {
                                      { JOB_SCH_ERC::UNITS::INCHES, "in" },
                                      { JOB_SCH_ERC::UNITS::MILLIMETERS, "mm" },
                                      { JOB_SCH_ERC::UNITS::MILS, "mils" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_SCH_ERC::OUTPUT_FORMAT,
                              {
                                      { JOB_SCH_ERC::OUTPUT_FORMAT::REPORT, "report" },
                                      { JOB_SCH_ERC::OUTPUT_FORMAT::JSON, "json" },
                              } )

JOB_SCH_ERC::JOB_SCH_ERC() :
    JOB( "erc", false ),
    m_filename(),
    m_units( JOB_SCH_ERC::UNITS::MILLIMETERS ),
    m_severity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ),
    m_format( OUTPUT_FORMAT::REPORT ),
    m_exitCodeViolations( false )
{
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<int>( "severity", &m_severity, m_severity ) );
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "format", &m_format, m_format ) );
}


wxString JOB_SCH_ERC::GetDescription()
{
    return wxString::Format( _( "Perform Schematic ERC" ) );
}


REGISTER_JOB( sch_erc, _HKI( "Schematic: Perform ERC" ), KIWAY::FACE_SCH, JOB_SCH_ERC );