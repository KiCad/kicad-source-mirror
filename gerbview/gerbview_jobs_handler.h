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

#ifndef GERBVIEW_JOBS_HANDLER_H
#define GERBVIEW_JOBS_HANDLER_H

#include <jobs/job_dispatcher.h>
#include <wx/arrstr.h>

class KIWAY;


class GERBVIEW_JOBS_HANDLER : public JOB_DISPATCHER
{
public:
    GERBVIEW_JOBS_HANDLER( KIWAY* aKiway );

private:
    int JobGerberInfo( JOB* aJob );
    int JobGerberExportPng( JOB* aJob );
    int JobGerberDiff( JOB* aJob );

    bool checkStrictMode( const wxArrayString& aMessages, bool aStrict );
};


#endif // GERBVIEW_JOBS_HANDLER_H
