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

#include <jobs/job_special_execute.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_SPECIAL_EXECUTE::JOB_SPECIAL_EXECUTE() :
        JOB( "special_execute", false ),
        m_command(),
        m_ignoreExitcode( false ),
        m_recordOutput( true )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "command", &m_command, m_command ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "ignore_exit_code", &m_ignoreExitcode, m_ignoreExitcode ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "record_output", &m_recordOutput, m_recordOutput ) );
}

wxString JOB_SPECIAL_EXECUTE::GetDefaultDescription() const
{
    return wxString( "Execute command" );
}

REGISTER_JOB( special_execute, _HKI( "Special: Execute Command" ), KIWAY::KIWAY_FACE_COUNT,
              JOB_SPECIAL_EXECUTE );