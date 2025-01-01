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

#include <config.h>
#include <gestfich.h>
#include <wx/process.h>

#include <utility>

#include <paths.h>
#include <pgm_base.h>
#include <python_manager.h>
#include <wx_filename.h>


class PYTHON_PROCESS : public wxProcess
{
public:
    PYTHON_PROCESS( std::function<void(int, const wxString&, const wxString&)> aCallback ) :
            wxProcess(),
            m_callback( std::move( aCallback ) )
    {}

    void OnTerminate( int aPid, int aStatus ) override
    {
        if( m_callback )
        {
            wxString output, error;
            wxInputStream* processOut = GetInputStream();
            size_t bytesRead = 0;

            while( processOut->CanRead() && bytesRead < MAX_OUTPUT_LEN )
            {
                char buffer[4096];
                buffer[ processOut->Read( buffer, sizeof( buffer ) - 1 ).LastRead() ] = '\0';
                output.append( buffer, processOut->LastRead() );
                bytesRead += processOut->LastRead();
            }

            processOut = GetErrorStream();
            bytesRead = 0;

            while( processOut->CanRead() && bytesRead < MAX_OUTPUT_LEN )
            {
                char buffer[4096];
                buffer[ processOut->Read( buffer, sizeof( buffer ) - 1 ).LastRead() ] = '\0';
                error.append( buffer, processOut->LastRead() );
                bytesRead += processOut->LastRead();
            }

            m_callback( aStatus, output, error );
        }
    }

    static constexpr size_t MAX_OUTPUT_LEN = 1024L * 1024L;

private:
    std::function<void(int, const wxString&, const wxString&)> m_callback;
};


PYTHON_MANAGER::PYTHON_MANAGER( const wxString& aInterpreterPath )
{
    wxFileName path( aInterpreterPath );
    path.Normalize( FN_NORMALIZE_FLAGS );
    m_interpreterPath = path.GetFullPath();
}


void PYTHON_MANAGER::Execute( const wxString& aArgs,
                              const std::function<void( int, const wxString&,
                                                        const wxString& )>& aCallback,
                              const wxExecuteEnv* aEnv )
{
    PYTHON_PROCESS* process = new PYTHON_PROCESS( aCallback );
    process->Redirect();

    wxString cmd = wxString::Format( wxS( "%s %s" ), m_interpreterPath, aArgs );
    long     pid = wxExecute( cmd, wxEXEC_ASYNC, process, aEnv );

    if( pid == 0 )
        aCallback( -1, wxEmptyString, _( "Process could not be created" ) );
}


wxString PYTHON_MANAGER::FindPythonInterpreter()
{
    // First, attempt to use a Python we distribute with KiCad
#if defined( __WINDOWS__ )
    wxFileName pythonExe = FindKicadFile( "python.exe" );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();
#elif defined( __WXMAC__ )
    wxFileName pythonExe( PATHS::GetOSXKicadDataDir(), wxEmptyString );
    pythonExe.RemoveLastDir();
    pythonExe.AppendDir( wxT( "Frameworks" ) );
    pythonExe.AppendDir( wxT( "Python.framework" ) );
    pythonExe.AppendDir( wxT( "Versions" ) );
    pythonExe.AppendDir( wxT( "Current" ) );
    pythonExe.AppendDir( wxT( "bin" ) );
    pythonExe.SetFullName(wxT( "python3" ) );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();
#endif

    // In case one is forced with cmake
    pythonExe.Assign( wxString::FromUTF8Unchecked( PYTHON_EXECUTABLE ) );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();

    // Fall back on finding any Python in the user's path

#ifdef _WIN32
    // TODO(JE) where
#else
    wxArrayString output;

    if( 0 == wxExecute( wxS( "which -a python3" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }

    if( 0 == wxExecute( wxS( "which -a python" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }
#endif

    return wxEmptyString;
}


std::optional<wxString> PYTHON_MANAGER::GetPythonEnvironment( const wxString& aNamespace )
{
    wxFileName path( PATHS::GetUserCachePath(), wxEmptyString );
    path.AppendDir( wxS( "python-environments" ) );
    path.AppendDir( aNamespace );

    if( !PATHS::EnsurePathExists( path.GetPath() ) )
        return std::nullopt;

    return path.GetPath();
}


std::optional<wxString> PYTHON_MANAGER::GetVirtualPython( const wxString& aNamespace )
{
    std::optional<wxString> envPath = GetPythonEnvironment( aNamespace );

    if( !envPath )
        return std::nullopt;

    wxFileName python( *envPath, wxEmptyString );
    python.AppendDir( "bin" );
    python.SetFullName( "python" );

    if( !python.IsFileExecutable() )
        return std::nullopt;

    return python.GetFullPath();
}
