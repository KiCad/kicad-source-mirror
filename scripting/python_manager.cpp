/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#include <config.h>
#include <gestfich.h>
#include <wx/process.h>

#include <future>
#include <utility>

#include <api/api_utils.h>
#include <paths.h>
#include <pgm_base.h>
#include <python_manager.h>
#include <thread_pool.h>
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
        // Print stdout trace info from the monitor thread
        wxLog::GetActiveTarget()->Flush();

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


long PYTHON_MANAGER::Execute( const std::vector<wxString>& aArgs,
        const std::function<void(int, const wxString&, const wxString&)>& aCallback,
        const wxExecuteEnv* aEnv, bool aSaveOutput )
{
    PYTHON_PROCESS* process = new PYTHON_PROCESS( aCallback );
    process->Redirect();

    auto monitor =
        []( PYTHON_PROCESS* aProcess )
        {
            wxInputStream* processOut = aProcess->GetInputStream();

            while( aProcess->IsInputOpened() )
            {
                if( processOut->CanRead() )
                {
                    char buffer[4096];
                    buffer[processOut->Read( buffer, sizeof( buffer ) - 1 ).LastRead()] = '\0';
                    wxString stdOut( buffer, processOut->LastRead() );
                    stdOut = stdOut.BeforeLast( '\n' );
                    wxLogTrace( traceApi, wxString::Format( "Python: %s", stdOut ) );
                }
            }
        };

    wxString argsStr;
    std::vector<const wchar_t*> args = { m_interpreterPath.wc_str() };

    for( const wxString& arg : aArgs )
    {
        args.emplace_back( arg.wc_str() );
        argsStr << arg << " ";
    }

    args.emplace_back( nullptr );

    wxLogTrace( traceApi, wxString::Format( "Execute: %s %s", m_interpreterPath, argsStr ) );
    long pid = wxExecute( args.data(), wxEXEC_ASYNC, process, aEnv );

    if( pid == 0 )
    {
        delete process;
        aCallback( -1, wxEmptyString, _( "Process could not be created" ) );
    }
    else
    {
        // On Windows, if there is a lot of stdout written by the process, this can
        // hang up the wxProcess such that it will never call OnTerminate.  To work
        // around this, we use this monitor thread to just dump the stdout to the
        // trace log, which prevents the hangup.  This flag is provided to keep the
        // old behavior for commands where we need to read the output directly,
        // which is currently only used for detecting the interpreter version.
        // If we need to use the async monitor thread approach and preserve the stdout
        // contents in the future, a more complicated hack might be necessary.
        if( !aSaveOutput )
        {
            thread_pool& tp = GetKiCadThreadPool();
            auto ret = tp.submit_task( [monitor, process] { monitor( process ); } );
        }
    }

    return pid;
}


wxString PYTHON_MANAGER::FindPythonInterpreter()
{
    // First, attempt to use a Python we distribute with KiCad
#if defined( __WINDOWS__ )
    wxFileName pythonExe = FindKicadFile( "pythonw.exe" );

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
#else
    wxFileName pythonExe;
#endif

    // In case one is forced with cmake
    pythonExe.Assign( wxString::FromUTF8Unchecked( PYTHON_EXECUTABLE ) );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();

    // Fall back on finding any Python in the user's path

#ifdef _WIN32
    wxArrayString output;

    if( 0 == wxExecute( wxS( "where pythonw.exe" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }
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

#ifdef _WIN32
    python.AppendDir( "Scripts" );
    python.SetFullName( "pythonw.exe" );
#else
    python.AppendDir( "bin" );
    python.SetFullName( "python" );
#endif

    if( !python.IsFileExecutable() )
        return std::nullopt;

    return python.GetFullPath();
}
