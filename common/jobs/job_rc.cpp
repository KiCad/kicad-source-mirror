/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_rc.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_RC::UNITS, {
                                                     { JOB_RC::UNITS::INCH, "in" },
                                                     { JOB_RC::UNITS::MM,   "mm" },
                                                     { JOB_RC::UNITS::MILS, "mils" },
                                                  } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_RC::OUTPUT_FORMAT,
                              {
                                      { JOB_RC::OUTPUT_FORMAT::REPORT, "report" },
                                      { JOB_RC::OUTPUT_FORMAT::JSON,   "json" },
                              } )

JOB_RC::JOB_RC( const std::string& aType ) :
        JOB( aType, false ),
        m_filename(),
        m_units( JOB_RC::UNITS::MM ),
        m_severity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ),
        m_format( OUTPUT_FORMAT::REPORT ),
        m_exitCodeViolations( true ) // this ensures GUI jobs default to treat violations as job failure, cli will default false elsewhere
{
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<int>( "severity", &m_severity, m_severity ) );
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "fail_on_error", &m_exitCodeViolations, m_exitCodeViolations ) );
}