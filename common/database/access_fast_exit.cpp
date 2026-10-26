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


#include <database/access_fast_exit.h>

#include <atomic>
#include <cstdio>

#include <kiplatform/app.h>
#include <trace_helpers.h>
#include <wx/log.h>

#ifdef _WIN32
#include <windows.h>
#include <ole2.h>
#endif


namespace
{
// The flag lives in kicommon so the kiface that sets it and the app that reads it share one copy
std::atomic<bool> g_accessWorkerAbandoned{ false };
} // namespace


void MarkAccessWorkerAbandoned()
{
    g_accessWorkerAbandoned.store( true );
    wxLogTrace( traceDatabase, wxT( "An Access engine worker was abandoned" ) );
}


void ExitFastIfAccessWorkerAbandoned( int aExitCode )
{
    wxLogTrace( traceDatabase, wxT( "Exit check, Access worker abandoned %d" ),
                g_accessWorkerAbandoned.load() ? 1 : 0 );

    if( !g_accessWorkerAbandoned.load() )
        return;

#ifdef _WIN32
    // Hands clipboard data to the system now, since nothing runs OLE teardown afterwards
    OleFlushClipboard();
#endif

    std::fflush( nullptr );

    // Send pending thread records to stderr, as wxWidgets does during normal shutdown
    delete wxLog::SetActiveTarget( new wxLogStderr );
    wxLog::FlushActive();

    KIPLATFORM::APP::TerminateWithCode( aExitCode );
}
