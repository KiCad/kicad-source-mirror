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

#include <jobs/job_registry.h>
#include <jobs/job_gerber_info.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_GERBER_INFO::OUTPUT_FORMAT, { { JOB_GERBER_INFO::OUTPUT_FORMAT::TEXT, "text" },
                                                                { JOB_GERBER_INFO::OUTPUT_FORMAT::JSON, "json" } } )


JOB_GERBER_INFO::JOB_GERBER_INFO() :
        JOB( "gerber_info", false )
{
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "output_format", &m_outputFormat, m_outputFormat ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "calculate_area", &m_calculateArea, m_calculateArea ) );
}


wxString JOB_GERBER_INFO::GetDefaultDescription() const
{
    return _( "Gerber Info" );
}


wxString JOB_GERBER_INFO::GetSettingsDialogTitle() const
{
    return _( "Gerber Info Job Settings" );
}


REGISTER_JOB( gerber_info, _HKI( "Gerber: Info" ), KIWAY::FACE_GERBVIEW, JOB_GERBER_INFO );
