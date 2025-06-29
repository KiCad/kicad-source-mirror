/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef JOB_DISPATCHER_H
#define JOB_DISPATCHER_H

#include <kicommon.h>
#include <functional>
#include <string>
#include <map>
#include <jobs/job.h>


class KIWAY;
class REPORTER;
class PROGRESS_REPORTER;
class wxWindow;

class KICOMMON_API JOB_DISPATCHER
{
public:
    JOB_DISPATCHER( KIWAY* aKiway );
    void Register( const std::string& aJobTypeName, std::function<int( JOB* job )> aHandler,
                         std::function<bool( JOB* job, wxWindow* aParent )> aConfigHandler );
    int  RunJob( JOB* aJob, REPORTER* aReporter, PROGRESS_REPORTER* aProgressReporter );
    bool HandleJobConfig( JOB* aJob, wxWindow* aParent );
    void SetReporter( REPORTER* aReporter );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

protected:
    KIWAY*             m_kiway;             // non-owning
    REPORTER*          m_reporter;          // non-owning
    PROGRESS_REPORTER* m_progressReporter;  // non-owning

private:
    std::map<std::string, std::function<int( JOB* job )>> m_jobHandlers;
    std::map<std::string, std::function<bool( JOB* job, wxWindow* aParent )>>
            m_jobConfigHandlers;

};

#endif