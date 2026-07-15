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

#include <jobs/job_unknown.h>

JOB_UNKNOWN::JOB_UNKNOWN( const std::string& aType, const nlohmann::json& aSettings ) :
        JOB( aType, false ),
        m_settings( aSettings )
{
}


void JOB_UNKNOWN::FromJson( const nlohmann::json& j )
{
    m_settings = j;
}


void JOB_UNKNOWN::ToJson( nlohmann::json& j ) const
{
    j = m_settings;
}


wxString JOB_UNKNOWN::GetDefaultDescription() const
{
    return wxString::Format( wxT( "Unsupported job type '%s'" ), GetType() );
}
