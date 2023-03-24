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

#ifndef JOB_DISPATCHER_H
#define JOB_DISPATCHER_H

#include <functional>
#include <string>
#include <map>
#include <jobs/job.h>


class REPORTER;

class JOB_DISPATCHER
{
public:
    JOB_DISPATCHER();
    void Register( const std::string& aJobTypeName, std::function<int( JOB* job )> aHandler );
    int  RunJob( JOB* job );
    void SetReporter( REPORTER* aReporter );

protected:
    REPORTER* m_reporter; // non-owning

private:
    std::map<std::string, std::function<int( JOB* job )>> m_jobHandlers;

};

#endif