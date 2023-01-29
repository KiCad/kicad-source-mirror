/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/process.h>

#include <utility>

#include "python_manager.h"

class PYTHON_PROCESS : public wxProcess
{
public:
    PYTHON_PROCESS( std::function<void(int, const wxString&)> aCallback ) :
            wxProcess(),
            m_callback( std::move( aCallback ) )
    {}

    void OnTerminate( int aPid, int aStatus ) override
    {
        if( m_callback )
        {
            wxString output;
            wxInputStream* processOut = GetInputStream();
            size_t bytesRead = 0;

            while( processOut->CanRead() && bytesRead < MAX_OUTPUT_LEN )
            {
                char buffer[4096];
                buffer[ processOut->Read( buffer, sizeof( buffer ) - 1 ).LastRead() ] = '\0';
                output.append( buffer, sizeof( buffer ) );
                bytesRead += processOut->LastRead();
            }

            m_callback( aStatus, output );
        }
    }

    static constexpr size_t MAX_OUTPUT_LEN = 1024L * 1024L;

private:
    std::function<void(int, const wxString&)> m_callback;
};


void PYTHON_MANAGER::Execute( const wxString& aArgs,
                              const std::function<void( int, const wxString& )>& aCallback )
{
    PYTHON_PROCESS* process = new PYTHON_PROCESS( aCallback );
    process->Redirect();

    wxString cmd = wxString::Format( wxS( "%s %s" ), m_interpreterPath, aArgs );
    long     pid = wxExecute( cmd, wxEXEC_ASYNC, process );

    if( pid == 0 )
        aCallback( -1, wxEmptyString );
}
