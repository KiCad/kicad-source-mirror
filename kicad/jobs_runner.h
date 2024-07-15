/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

class JOBSET;
struct JOBSET_OUTPUT;
class KIWAY;
class REPORTER;

class JOBS_RUNNER
{
public:
    JOBS_RUNNER( KIWAY* aKiway, JOBSET* aJobsFile, REPORTER* aReporter = nullptr );

    bool RunJobsAllOutputs( bool aBail = false );
    bool RunJobsForOutput( JOBSET_OUTPUT* aOutput, bool aBail = false );

private:
    KIWAY*     m_kiway;
    JOBSET*         m_jobsFile;
    REPORTER*          m_reporter;
};