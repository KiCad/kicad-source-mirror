/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cli/exit_codes.h>
#include <jobs/job_dispatcher.h>
#include <reporter.h>
#include <wx/debug.h>


JOB_DISPATCHER::JOB_DISPATCHER()
{
    m_reporter = &NULL_REPORTER::GetInstance();
}


void JOB_DISPATCHER::Register( const std::string&             aJobTypeName,
                               std::function<int( JOB* job )> aHandler )
{
    m_jobHandlers.emplace( aJobTypeName, aHandler );
}


int JOB_DISPATCHER::RunJob( JOB* job )
{
    if( m_jobHandlers.count( job->GetType() ) )
    {
        return m_jobHandlers[job->GetType()]( job );
    }

    return CLI::EXIT_CODES::ERR_UNKNOWN;
}


void JOB_DISPATCHER::SetReporter( REPORTER* aReporter )
{
    wxCHECK( aReporter != nullptr, /*void*/ );
    m_reporter = aReporter;
}
